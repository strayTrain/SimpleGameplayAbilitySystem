#include "SimpleGameplayAbility.h"

#include "SimpleGameplayAbilitySystem/DefaultTags/DefaultTags.h"
#include "SimpleGameplayAbilitySystem/Module/SimpleGameplayAbilitySystem.h"
#include "SimpleGameplayAbilitySystem/SimpleEventSubsystem/SimpleEventSubSystem.h"
#include "SimpleGameplayAbilitySystem/SimpleGameplayAbilityComponent/SimpleGameplayAbilityComponent.h"

bool USimpleGameplayAbility::CanActivate_Implementation(FInstancedStruct ActivationContext)
{
	return true;
}

void USimpleGameplayAbility::PreActivate_Implementation(FInstancedStruct ActivationContext)
{
}

bool USimpleGameplayAbility::CanCancel_Implementation()
{
	return true;
}

bool USimpleGameplayAbility::ActivateAbility(const FGuid AbilityID, FInstancedStruct ActivationContext)
{
	AbilityInstanceID = AbilityID;
	
	if (!MeetsActivationRequirements(ActivationContext))
	{
		FSimpleAbilityEndedEvent AbilityActivationResult;
		AbilityActivationResult.AbilityID = AbilityInstanceID;
		AbilityActivationResult.EndStatusTag = FDefaultTags::AbilityEnded();
		AbilityActivationResult.EndingContext = ActivationContext;
		AbilityActivationResult.NewAbilityStatus = EAbilityStatus::EndedActivationFailed;
		AbilityActivationResult.WasCancelled = true;

		const FGameplayTag DomainTag = OwningAbilityComponent->HasAuthority() ? FDefaultTags::AuthorityAbilityDomain() : FDefaultTags::LocalAbilityDomain();
		
		SendEvent(FDefaultTags::AbilityEnded(), DomainTag, FInstancedStruct::Make(AbilityActivationResult), ESimpleEventReplicationPolicy::NoReplication);
		
		return false;
	}

	for (const FGameplayTag& TempTag : TemporarilyAppliedTags)
	{
		OwningAbilityComponent->AddGameplayTag(TempTag, ActivationContext);
	}

	for (const FGameplayTag& PermTag : PermanentlyAppliedTags)
	{
		OwningAbilityComponent->AddGameplayTag(PermTag, ActivationContext);
	}

	OwningAbilityComponent->SetAbilityStatus(AbilityInstanceID, EAbilityStatus::ActivationSuccess);
	CachedActivationContext = ActivationContext;
	bIsAbilityActive = true;
	
	PreActivate(ActivationContext);
	OnActivate(ActivationContext);

	return true;
}

FGuid USimpleGameplayAbility::ActivateSubAbility(
	const TSubclassOf<USimpleGameplayAbility> AbilityClass,
	const FInstancedStruct ActivationContext,
	const bool CancelIfParentEnds,
	const bool CancelIfParentCancels,
	ESubAbilityActivationPolicy SubAbilityActivationPolicy)
{
	const bool HasAuthority = OwningAbilityComponent->HasAuthority();
	const bool IsListenServer = OwningAbilityComponent->GetNetMode() == NM_ListenServer;
	EAbilityActivationPolicy FinalActivationPolicy = EAbilityActivationPolicy::LocalOnly;

	switch (SubAbilityActivationPolicy)
	{
	case ESubAbilityActivationPolicy::NoReplication:
		FinalActivationPolicy = EAbilityActivationPolicy::LocalOnly;
		break;

	case ESubAbilityActivationPolicy::ClientOnly:
		if (HasAuthority && !IsListenServer)
		{
			SIMPLE_LOG(OwningAbilityComponent,
			           FString::Printf(TEXT("ClientOnly sub ability %s cannot be activated on server"), *GetName()));
			return FGuid();
		}
		FinalActivationPolicy = EAbilityActivationPolicy::ClientOnly;
		break;

	case ESubAbilityActivationPolicy::InitiateFromClient:
		if (HasAuthority && !IsListenServer)
		{
			SIMPLE_LOG(OwningAbilityComponent,
			           FString::Printf(TEXT("ClientOnly sub ability %s cannot be activated on server"), *GetName()));
			return FGuid();
		}
		FinalActivationPolicy = EAbilityActivationPolicy::ClientPredicted;
		break;

	case ESubAbilityActivationPolicy::ServerOnly:
		if (!HasAuthority)
		{
			SIMPLE_LOG(OwningAbilityComponent,
			           FString::Printf(TEXT("ServerOnly sub ability %s cannot be activated on client"), *GetName()));
			return FGuid();
		}
		FinalActivationPolicy = EAbilityActivationPolicy::ServerOnly;
		break;

	case ESubAbilityActivationPolicy::InitiateFromServer:
		if (!HasAuthority)
		{
			SIMPLE_LOG(OwningAbilityComponent,
			           FString::Printf(TEXT("ServerOnly sub ability %s cannot be activated on client"), *GetName()));
			return FGuid();
		}
		FinalActivationPolicy = EAbilityActivationPolicy::ServerAuthority;
		break;
	}

	const FGuid SubAbilityID = FGuid::NewGuid();

	if (CancelIfParentEnds)
	{
		EndOnEndedSubAbilities.Add(SubAbilityID);
	}

	if (CancelIfParentCancels)
	{
		EndOnCancelledSubAbilities.Add(SubAbilityID);
	}

	OwningAbilityComponent->ActivateAbilityWithID(SubAbilityID, AbilityClass, ActivationContext, true,
	                                              FinalActivationPolicy);

	return SubAbilityID;
}

void USimpleGameplayAbility::OnTick_Implementation(float DeltaTime)
{
}

void USimpleGameplayAbility::EndAbility(const FGameplayTag EndStatus, const FInstancedStruct EndingContext)
{
	if (!bIsAbilityActive)
	{
		return;
	}

	OnEnd(EndStatus, EndingContext, false);
	EndAbilityInternal(EndStatus, EndingContext, false);
}

void USimpleGameplayAbility::CancelAbility(const FGameplayTag CancelStatus, const FInstancedStruct CancelContext, const bool ForceCancel)
{
	if (!bIsAbilityActive || (!CanCancel() && !ForceCancel))
	{
		return;
	}

	OnEnd(CancelStatus, CancelContext, true);
	EndAbilityInternal(CancelStatus, CancelContext, true);
}

void USimpleGameplayAbility::CleanUpAbility_Implementation()
{
	if (IsAbilityActive())
	{
		EndAbility(FDefaultTags::AbilityCancelled(), FInstancedStruct());
	}
	
	if (USimpleEventSubsystem* EventSubsystem = GetWorld() ? GetWorld()->GetGameInstance()->GetSubsystem<USimpleEventSubsystem>() : nullptr)
	{
		EventSubsystem->StopListeningForAllEvents(this);
	}
	
	Super::CleanUpAbility_Implementation();
}

void USimpleGameplayAbility::OnGranted_Implementation(USimpleGameplayAbilityComponent* GrantedAbilityComponent)
{ }

void USimpleGameplayAbility::OnGrantedStatic(TSubclassOf<USimpleGameplayAbility> AbilityClass, USimpleGameplayAbilityComponent* GrantedAbilityComponent)
{
	// CDO = Class Default Object
	USimpleGameplayAbility* CDO = Cast<USimpleGameplayAbility>(AbilityClass->GetDefaultObject());
    
	if (CDO)
	{
		CDO->OnGranted(GrantedAbilityComponent);
	}
}

void USimpleGameplayAbility::EndAbilityInternal(FGameplayTag Status, FInstancedStruct Context, bool WasCancelled)
{
	for (const FGameplayTag& TempTag : TemporarilyAppliedTags)
	{
		OwningAbilityComponent->RemoveGameplayTag(TempTag, Context);
	}

	const TArray<FGuid>& AbilitiesToCancel = WasCancelled ? EndOnCancelledSubAbilities : EndOnEndedSubAbilities;

	for (const FGuid& SubAbilityID : AbilitiesToCancel)
	{
		OwningAbilityComponent->CancelAbility(SubAbilityID, Context, true);
	}
	
	bIsAbilityActive = false;

	if (InstancingPolicy == EAbilityInstancingPolicy::MultipleInstances)
	{
		OwningAbilityComponent->RemoveInstancedAbility(this);
	}
	
	FSimpleAbilityEndedEvent EndEvent;
	EndEvent.AbilityID = AbilityInstanceID;
	EndEvent.EndStatusTag = Status;
	EndEvent.EndingContext = Context;
	EndEvent.WasCancelled = WasCancelled;
	
	OwningAbilityComponent->SendEvent(FDefaultTags::AbilityEnded(), Status, FInstancedStruct::Make(EndEvent), GetAvatarActor(), { }, ESimpleEventReplicationPolicy::NoReplication);
}

AActor* USimpleGameplayAbility::GetAvatarActor() const
{
	if (OwningAbilityComponent)
	{
		if (!OwningAbilityComponent->GetAvatarActor())
		{
			SIMPLE_LOG(OwningAbilityComponent,
			           FString::Printf(
				           TEXT("Owning ability component has no avatar actor for ability %s"), *GetName()));
			return nullptr;
		}

		return OwningAbilityComponent->GetAvatarActor();
	}

	SIMPLE_LOG(OwningAbilityComponent,
	           FString::Printf(TEXT("Owning ability component is null for ability %s"), *GetName()));
	return nullptr;
}

AActor* USimpleGameplayAbility::GetAvatarActorAs(TSubclassOf<AActor> AvatarClass, bool& IsValid) const
{
	AActor* AvatarActor = GetAvatarActor();

	if (!AvatarActor)
	{
		IsValid = false;
		return nullptr;
	}

	if (AvatarActor->IsA(AvatarClass))
	{
		IsValid = true;
		return AvatarActor;
	}

	SIMPLE_LOG(OwningAbilityComponent,
	           FString::Printf(TEXT("Avatar actor %s is not of type %s"), *AvatarActor->GetName(),
	                           *AvatarClass->GetName()));
	IsValid = false;
	return AvatarActor;
}

void USimpleGameplayAbility::ApplyAttributeModifierToTarget(
	USimpleGameplayAbilityComponent* TargetComponent,
	TSubclassOf<USimpleAttributeModifier> ModifierClass,
	FInstancedStruct Context,
	FGuid& ModifierID)
{
	OwningAbilityComponent->ApplyAttributeModifierToTarget(TargetComponent, ModifierClass, Context, ModifierID);
}

bool USimpleGameplayAbility::IsAbilityActive() const
{
	return bIsAbilityActive;
}

double USimpleGameplayAbility::GetActivationTime() const
{
	const FAbilityState* AbilityState = OwningAbilityComponent->GetAbilityState(AbilityInstanceID, OwningAbilityComponent->HasAuthority());
	
	if (!AbilityState)
	{
		SIMPLE_LOG(OwningAbilityComponent, FString::Printf(TEXT("[USimpleGameplayAbility::GetActivationTime]: ability state for %s not found. Unknown activation time"), *GetName()));
		return 0;
	}

	return AbilityState->ActivationTimeStamp;
}

double USimpleGameplayAbility::GetActivationDelay() const
{
	return OwningAbilityComponent->GetServerTime() - GetActivationTime();
}

FInstancedStruct USimpleGameplayAbility::GetActivationContext() const
{
	return CachedActivationContext;
}

bool USimpleGameplayAbility::WasActivatedOnServer() const
{
	return OwningAbilityComponent->HasAuthority() && !IsProxyAbility;
}

bool USimpleGameplayAbility::WasActivatedOnClient() const
{
	return (!OwningAbilityComponent->HasAuthority() || OwningAbilityComponent->GetNetMode() == NM_ListenServer) && !
		IsProxyAbility;
}

EAbilityServerRole USimpleGameplayAbility::GetServerRole(bool& IsListenServer) const
{
	IsListenServer = OwningAbilityComponent->GetNetMode() == NM_ListenServer;
	
	// Check if we're on a server
	if (OwningAbilityComponent->GetNetMode() < NM_Client)
	{
		return EAbilityServerRole::Server;
	}

	return EAbilityServerRole::Client;
}

UWorld* USimpleGameplayAbility::GetWorld() const
{
	if (OwningAbilityComponent)
	{
		return OwningAbilityComponent->GetWorld();
	}

	return nullptr;
}

void USimpleGameplayAbility::Tick(float DeltaTime)
{
	OnTick(DeltaTime);
}

bool USimpleGameplayAbility::IsTickable() const
{
	UWorld* World = GetWorld();
	return CanTick && IsAbilityActive() && World != nullptr;
}

TStatId USimpleGameplayAbility::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(USimpleGameplayAbility, STATGROUP_Tickables);
}

ETickableTickType USimpleGameplayAbility::GetTickableTickType() const
{
	// Only tick during gameplay, not in editor
	return ETickableTickType::Conditional;
}

bool USimpleGameplayAbility::MeetsActivationRequirements(FInstancedStruct& ActivationContext)
{
	if (ActivationBlockingTags.Num() > 0)
	{
		for (const FGameplayTag& BlockingTag : ActivationBlockingTags)
		{
			if (OwningAbilityComponent->HasGameplayTag(BlockingTag))
			{
				UE_LOG(LogSimpleGAS, Warning, TEXT("Ability %s blocked by tag %s"), *GetName(),
				       *BlockingTag.ToString());
				return false;
			}
		}
	}

	if (ActivationRequiredTags.Num() > 0)
	{
		for (const FGameplayTag& RequiredTag : ActivationRequiredTags)
		{
			if (!OwningAbilityComponent->HasGameplayTag(RequiredTag))
			{
				UE_LOG(LogSimpleGAS, Warning, TEXT("Ability %s requires tag %s"), *GetName(), *RequiredTag.ToString());
				return false;
			}
		}
	}

	if (RequiredContextType)
	{
		if (!ActivationContext.IsValid())
		{
			SIMPLE_LOG(OwningAbilityComponent,
			           FString::Printf(
				           TEXT(
					           "Ability %s requires ActivationContext that contains struct type %s. No ActivationContext was provided."),
				           *GetName(), *RequiredContextType->GetName()));
			return false;
		}

		if (ActivationContext.IsValid() && RequiredContextType != ActivationContext.GetScriptStruct())
		{
			SIMPLE_LOG(OwningAbilityComponent, FString::Printf(
				           TEXT(
					           "Ability %s requires ActivationContext that contains struct type %s. The struct that was passed in is type %s"),
				           *GetName(), *RequiredContextType->GetName(),
				           *ActivationContext.GetScriptStruct()->GetName()));
			return false;
		}
	}

	if (AvatarTypeFilter.Num() > 0)
	{
		const AActor* AvatarActor = OwningAbilityComponent->GetAvatarActor();

		if (!AvatarActor)
		{
			SIMPLE_LOG(OwningAbilityComponent,
			           FString::Printf(TEXT("Ability %s requires an avatar actor to activate"), *GetName()));
			return false;
		}

		if (!AvatarTypeFilter.Contains(AvatarActor->GetClass()))
		{
			SIMPLE_LOG(OwningAbilityComponent,
			           FString::Printf(
				           TEXT("Ability %s requires an avatar actor of type %s"), *GetName(),
				           *AvatarTypeFilter[0]->GetName()));
			return false;
		}
	}

	if (!CanActivate(ActivationContext))
	{
		SIMPLE_LOG(OwningAbilityComponent, FString::Printf(TEXT("Ability %s failed CanActivate check"), *GetName()));
		return false;
	}

	return true;
}

void USimpleGameplayAbility::OnEnd_Implementation(FGameplayTag EndingStatus, FInstancedStruct EndingContext, bool WasCancelled) { }

void USimpleGameplayAbility::ClientResolvePastState(FGameplayTag StateTag, FSimpleAbilitySnapshot AuthorityState, FSimpleAbilitySnapshot PredictedState)
{
	// First we do a deep comparison of the underlying FInstancedStructs representing the snapshot data. If they're the same, we don't need to do anything.
	if (AuthorityState.StateData == PredictedState.StateData)
	{
		SIMPLE_LOG(this, FString::Printf(
			           TEXT(
				           "[USimpleGameplayAbility::ClientResolvePastState]: No diverging state detected for ability snapshot %s. No action taken."),
			           *StateTag.ToString()));
		return;
	}

	// Invoke any registered callbacks for this state
	if (SnapshotResolveCallbacks.Contains(PredictedState.SequenceNumber))
	{
		FOnSnapshotResolved& Callback = SnapshotResolveCallbacks[PredictedState.SequenceNumber];
		if (Callback.IsBound())
		{
			Callback.Execute(StateTag, AuthorityState, PredictedState);
			SnapshotResolveCallbacks.Remove(PredictedState.SequenceNumber);
		}
	}
}

void USimpleGameplayAbility::SendEvent(
	const FGameplayTag EventTag,
	const FGameplayTag DomainTag,
	const FInstancedStruct EventContext,
	const ESimpleEventReplicationPolicy ReplicationPolicy)
{
	OwningAbilityComponent->SendEvent(EventTag, DomainTag, EventContext, this, {}, ReplicationPolicy);
}
