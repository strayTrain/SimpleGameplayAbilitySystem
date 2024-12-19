#include "SimpleAbility.h"

#include "AbilityStateResolver/AbilityStateResolver.h"
#include "Net/UnrealNetwork.h"
#include "SimpleGameplayAbilitySystem/SimpleEventSubsystem/SimpleEventSubSystem.h"
#include "SimpleGameplayAbilitySystem/SimpleAbilityComponent/SimpleAbilityComponent.h"
#include "SimpleGameplayAbilitySystem/DefaultTags/DefaultTags.h"

void USimpleAbility::InitializeAbility(USimpleAbilityComponent* InOwningAbilityComponent, FGuid InAbilityInstanceID, double InActivationTime)
{
	if (!AbilityConfig.AbilityName.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Ability %s has no valid name tag set in its config. Can't initialize."), *GetAbilityName());
		return;
	}
	
	AbilityInstanceID = InAbilityInstanceID;
	OwningAbilityComponent = InOwningAbilityComponent;
	bIsAbilityActive = false;
	bIsInitialized = true;
	ActivationTime = InActivationTime;
	PredictedStateHistory.Empty();
	AuthorityStateHistory.Empty();

	// Listen for state snapshot events on the client
	if (!HasAuthority())
	{
		if (USimpleEventSubsystem* EventSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<USimpleEventSubsystem>())
		{
			EventSubsystem->StopListeningForEventsByFilter(this,
				FGameplayTagContainer(FDefaultTags::AbilityStateSnapshotTaken),
				FGameplayTagContainer(FDefaultTags::AuthorityDomain));
			
			FGameplayTagContainer EventTags;
			FGameplayTagContainer DomainTags;
			
			EventTags.AddTag(FDefaultTags::AbilityStateSnapshotTaken);
			DomainTags.AddTag(FDefaultTags::AuthorityDomain);
			
			FSimpleEventDelegate StateSnapshotTakenDelegate;
			StateSnapshotTakenDelegate.BindDynamic(this, &USimpleAbility::OnAuthorityStateSnapshotEventReceived);
			EventSubsystem->ListenForEvent(this, false, EventTags, DomainTags, StateSnapshotTakenDelegate, TArray<UScriptStruct*>(), TArray<AActor*>());
		}
	}
}

bool USimpleAbility::CanActivate_Implementation(FInstancedStruct AbilityContext)
{
	if (AbilityConfig.TagConfig.ActivationRequiredTags.Num() > 0)
	{
		if (!OwningAbilityComponent->GameplayTags.HasAll(AbilityConfig.TagConfig.ActivationRequiredTags))
		{
			return false;
		}
	}

	if (AbilityConfig.TagConfig.ActivationBlockingTags.Num() > 0)
	{
		if (OwningAbilityComponent->GameplayTags.HasAny(AbilityConfig.TagConfig.ActivationBlockingTags))
		{
			return false;
		}
	}
	
	return true;
}

bool USimpleAbility::Activate(FInstancedStruct AbilityContext)
{
	if (!bIsInitialized)
	{
		UE_LOG(LogTemp, Warning, TEXT("Ability %s was not initialized before calling Activate"), *GetAbilityName());
		return false;
	}
	
	if (!CanActivate(AbilityContext))
	{
		UE_LOG(LogTemp, Warning, TEXT("Ability %s cannot be activated"), *GetAbilityName());
		return false;
	}
	
	OwningAbilityComponent->AddGameplayTags(AbilityConfig.TagConfig.TemporaryAppliedTags);
	OwningAbilityComponent->AddGameplayTags(AbilityConfig.TagConfig.PermanentAppliedTags);
	bIsAbilityActive = true;
	
	OnActivate(AbilityContext);
	SendActivationStateChangeEvent(FDefaultTags::AbilityActivated, FDefaultTags::AbilityDomain);
	
	return true;
}

void USimpleAbility::EndAbility(FGameplayTag EndStatus = FDefaultTags::AbilityEndedSuccessfully)
{
	if (!bIsAbilityActive)
	{
		UE_LOG(LogTemp, Warning, TEXT("Ability %s is not active, can't end"), *GetAbilityName());
		return;
	}
	
	OnEnd(EndStatus);
	
	bIsAbilityActive = false;
	bIsInitialized = false;
	OwningAbilityComponent->RemoveGameplayTags(AbilityConfig.TagConfig.TemporaryAppliedTags);
	
	SendActivationStateChangeEvent(FDefaultTags::AbilityEnded, FDefaultTags::AbilityDomain);
}

/* State snapshotting */

void USimpleAbility::TakeStateSnapshot(FGameplayTag SnapshotTag, FInstancedStruct SnapshotData,
	FResolveStateMispredictionDelegate OnResolveState, bool UseCustomStateResolver,
	TSubclassOf<UAbilityStateResolver> CustomStateResolverClass)
{
	if (AbilityConfig.ActivationPolicy != EAbilityActivationPolicy::LocalPredicted)
	{
		UE_LOG(LogTemp, Warning, TEXT("Ability %s is not set to LocalPredicted, can't take state snapshot"), *GetAbilityName());
		return;
	}

	if (!SnapshotData.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Ability %s called TakeStateSnapshot with invalid snapshot data"), *GetAbilityName());
		return;
	}
	
	FAbilityState NewSnapshot;
	NewSnapshot.StateTag = SnapshotTag;
	NewSnapshot.TimeStamp = OwningAbilityComponent->GetServerTime();
	NewSnapshot.StateData = SnapshotData;
	
	if (HasAuthority())
	{
		if (AuthorityStateHistory.Num() >= AbilityConfig.MaxAbilityStateHistorySize)
		{
			AuthorityStateHistory.RemoveAt(0);
		}

		// Because the server is the source of truth, there is nothing to resolve later.
		NewSnapshot.IsStateResolved = true;
		AuthorityStateHistory.Add(NewSnapshot);
		SendEvent(FDefaultTags::AbilityStateSnapshotTaken, FDefaultTags::AuthorityDomain, FInstancedStruct::Make(NewSnapshot), ESimpleEventReplicationPolicy::ServerToClient);
		return;
	}

	if (PredictedStateHistory.Num() >= AbilityConfig.MaxAbilityStateHistorySize)
	{
		PredictedStateHistory.RemoveAt(0);
	}

	NewSnapshot.IsStateResolved = false;
	NewSnapshot.OnResolveState = OnResolveState;
	if (UseCustomStateResolver)
	{
		NewSnapshot.CustomStateResolverClass = CustomStateResolverClass;
	}
	
	PredictedStateHistory.Add(NewSnapshot);
}

void USimpleAbility::OnAuthorityStateSnapshotEventReceived(FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload)
{
	if (const FAbilityState* AuthorityStateData = Payload.GetPtr<FAbilityState>())
	{
		const int32 SnapshotIndex = FindPredictedStateIndexByTag(AuthorityStateData->StateTag);
		if (SnapshotIndex == -1)
		{
			UE_LOG(LogTemp, Warning, TEXT("ClientResolveStateSnapshot called for snapshot tag %s but no matching predicted state was found"), *AuthorityStateData->StateTag.ToString());
			return;
		}

		FAbilityState& PredictedStateData = PredictedStateHistory[SnapshotIndex];
		if (PredictedStateData.IsStateResolved)
		{
			return;
		}

		if (PredictedStateData.CustomStateResolverClass)
		{
			UAbilityStateResolver* CustomStateResolver = NewObject<UAbilityStateResolver>(this, PredictedStateData.CustomStateResolverClass);
			CustomStateResolver->ResolveState(*AuthorityStateData, PredictedStateData);
			PredictedStateData.IsStateResolved = true;
		}
		else
		{
			if (PredictedStateData.OnResolveState.ExecuteIfBound(
				AuthorityStateData->StateData, AuthorityStateData->TimeStamp,
				PredictedStateData.StateData, PredictedStateData.TimeStamp))
			{
				PredictedStateData.IsStateResolved = true;
			}
		}
	}
	
}

/* Utility functions */

FString USimpleAbility::GetAbilityName() const
{
	if (AbilityConfig.AbilityName.IsValid())
	{
		return AbilityConfig.AbilityName.GetTagName().ToString();
	}

	return GetName();
}

void USimpleAbility::SendEvent(FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload, ESimpleEventReplicationPolicy ReplicationPolicy) const
{
	OwningAbilityComponent->SendEvent(EventTag, DomainTag, Payload, ReplicationPolicy);
}

AActor* USimpleAbility::GetAvatarActorAs(TSubclassOf<AActor> AvatarClass) const
{
	if (AActor* AvatarActor = OwningAbilityComponent->GetAvatarActor())
	{
		return AvatarActor;
	}

	UE_LOG(LogTemp, Warning, TEXT("Ability %s owning component has no avatar actor set. Did you remember to call SetAvatarActor?"), *GetAbilityName());
	return nullptr;
}

bool USimpleAbility::HasAuthority()
{
	if (!OwningAbilityComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("Ability %s has no owning ability component"), *GetAbilityName());
		return false;
	}
	
	return OwningAbilityComponent->GetOwner()->HasAuthority();
}

UWorld* USimpleAbility::GetWorld() const
{
	if (OwningAbilityComponent)
	{
		return OwningAbilityComponent->GetWorld();
	}

	return nullptr;
}

void USimpleAbility::SendActivationStateChangeEvent(FGameplayTag EventTag, FGameplayTag DomainTag) const
{
	if (USimpleEventSubsystem* EventSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<USimpleEventSubsystem>())
	{
		FAbilityActivationStateChangedData ActivationData;
		ActivationData.AbilityName = AbilityConfig.AbilityName;
		ActivationData.AbilityInstanceID = AbilityInstanceID;
		ActivationData.TimeStamp = OwningAbilityComponent->GetServerTime();

		FInstancedStruct EventPayload = FInstancedStruct::Make(ActivationData);
		
		EventSubsystem->SendEvent(EventTag, DomainTag, EventPayload);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No event subsystem found when sending activation state change event for ability %s"), *GetAbilityName());
	}
}

int32 USimpleAbility::FindStateIndexByTag(FGameplayTag StateTag) const
{
	for (int32 i = 0; i < AuthorityStateHistory.Num(); i++)
	{
		if (AuthorityStateHistory[i].StateTag.MatchesTagExact(StateTag))
		{
			return i;
		}
	}

	return -1;
}

int32 USimpleAbility::FindPredictedStateIndexByTag(FGameplayTag StateTag) const
{
	for (int32 i = 0; i < PredictedStateHistory.Num(); i++)
	{
		if (PredictedStateHistory[i].StateTag.MatchesTagExact(StateTag) && !PredictedStateHistory[i].IsStateResolved)
		{
			return i;
		}
	}

	return -1;
}

/* Tick support */

void USimpleAbility::Tick(float DeltaTime)
{
	if (LastTickFrame == GFrameCounter)
	{
		return;
	}

	LastTickFrame = GFrameCounter;

	if (bIsAbilityActive)
	{
		OnTick(DeltaTime);
	}
}

ETickableTickType USimpleAbility::GetTickableTickType() const
{
	if (AbilityConfig.CanAbilityTick)
	{
		return ETickableTickType::Always;
	}

	return ETickableTickType::Never;
}

/* Replication */

void USimpleAbility::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	UObject::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USimpleAbility, AbilityInstanceID);
	DOREPLIFETIME(USimpleAbility, AuthorityStateHistory);
}