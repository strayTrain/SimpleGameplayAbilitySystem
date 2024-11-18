#include "SimpleGameplayAbility.h"

#include "GameplayAbilityStateResolver/GameplayAbilityStateResolver.h"
#include "Net/UnrealNetwork.h"
#include "SimpleGameplayAbilitySystem/SimpleEventSubsystem/SimpleEventSubSystem.h"
#include "SimpleGameplayAbilitySystem/SimpleGameplayAbilityComponent/SimpleGameplayAbilityComponent.h"
#include "SimpleGameplayAbilitySystem/SimpleGameplayAttributes/SimpleGameplayAttributes.h"
#include "SimpleGameplayAbilitySystem/SimpleGASTypes/DefaultTags/DefaultTags.h"

void USimpleGameplayAbility::InitializeAbility(USimpleGameplayAbilityComponent* InOwningAbilityComponent, FGuid InAbilityInstanceID, double InActivationTime)
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
			StateSnapshotTakenDelegate.BindDynamic(this, &USimpleGameplayAbility::OnAuthorityStateSnapshotEventReceived);
			EventSubsystem->ListenForEvent(this, false, EventTags, DomainTags, StateSnapshotTakenDelegate, TArray<UScriptStruct*>(), TArray<AActor*>());
		}
	}
}

bool USimpleGameplayAbility::CanActivate_Implementation(FInstancedStruct AbilityContext)
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

bool USimpleGameplayAbility::Activate(FInstancedStruct AbilityContext)
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

void USimpleGameplayAbility::EndAbility(FGameplayTag EndStatus = FDefaultTags::AbilityEndedSuccessfully)
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

void USimpleGameplayAbility::TakeStateSnapshot(FGameplayTag SnapshotTag, FInstancedStruct SnapshotData,
	FResolveStateMispredictionDelegate OnResolveState, bool UseCustomStateResolver,
	TSubclassOf<UGameplayAbilityStateResolver> CustomStateResolverClass)
{
	if (AbilityConfig.ActivationPolicy != EAbilityActivationPolicy::LocalPredicted)
	{
		UE_LOG(LogTemp, Warning, TEXT("Ability %s is not set to LocalPredicted, can't take state snapshot"), *GetAbilityName());
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

void USimpleGameplayAbility::OnAuthorityStateSnapshotEventReceived(FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload)
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
			UGameplayAbilityStateResolver* CustomStateResolver = NewObject<UGameplayAbilityStateResolver>(this, PredictedStateData.CustomStateResolverClass);
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

FString USimpleGameplayAbility::GetAbilityName() const
{
	if (AbilityConfig.AbilityName.IsValid())
	{
		return AbilityConfig.AbilityName.GetTagName().ToString();
	}

	return GetName();
}

void USimpleGameplayAbility::SendEvent(FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload, ESimpleEventReplicationPolicy ReplicationPolicy) const
{
	OwningAbilityComponent->SendEvent(EventTag, DomainTag, Payload, ReplicationPolicy);
}

AActor* USimpleGameplayAbility::GetAvatarActorAs(TSubclassOf<AActor> AvatarClass) const
{
	if (AActor* AvatarActor = OwningAbilityComponent->GetAvatarActor())
	{
		return AvatarActor;
	}

	return nullptr;
}

USimpleGameplayAttributes* USimpleGameplayAbility::GetOwningAbilityComponentAttributes() const
{
	if (!OwningAbilityComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("Ability %s has no owning ability component"), *GetAbilityName());
		return nullptr;
	}
	
	return OwningAbilityComponent->Attributes;
}

bool USimpleGameplayAbility::HasAuthority()
{
	if (!OwningAbilityComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("Ability %s has no owning ability component"), *GetAbilityName());
		return false;
	}
	
	return OwningAbilityComponent->GetOwner()->HasAuthority();
}

UWorld* USimpleGameplayAbility::GetWorld() const
{
	if (OwningAbilityComponent)
	{
		return OwningAbilityComponent->GetWorld();
	}

	return nullptr;
}

void USimpleGameplayAbility::SendActivationStateChangeEvent(FGameplayTag EventTag, FGameplayTag DomainTag) const
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

int32 USimpleGameplayAbility::FindStateIndexByTag(FGameplayTag StateTag) const
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

int32 USimpleGameplayAbility::FindPredictedStateIndexByTag(FGameplayTag StateTag) const
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

void USimpleGameplayAbility::Tick(float DeltaTime)
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

ETickableTickType USimpleGameplayAbility::GetTickableTickType() const
{
	if (AbilityConfig.CanAbilityTick)
	{
		return ETickableTickType::Always;
	}

	return ETickableTickType::Never;
}

/* Replication */

void USimpleGameplayAbility::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	UObject::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USimpleGameplayAbility, AbilityInstanceID);
	DOREPLIFETIME(USimpleGameplayAbility, AuthorityStateHistory);
}