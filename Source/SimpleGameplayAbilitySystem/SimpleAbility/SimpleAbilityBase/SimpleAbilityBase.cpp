#include "SimpleAbilityBase.h"

#include "SimpleGameplayAbilitySystem/Module/SimpleGameplayAbilitySystem.h"
#include "SimpleGameplayAbilitySystem/SimpleGameplayAbilityComponent/SimpleGameplayAbilityComponent.h"

void USimpleAbilityBase::InitializeAbility(USimpleGameplayAbilityComponent* InOwningAbilityComponent, FGuid InAbilityInstanceID, bool IsProxyActivation)
{
	OwningAbilityComponent = InOwningAbilityComponent;
	AbilityInstanceID = InAbilityInstanceID;
	IsProxyAbility = IsProxyActivation;
}

void USimpleAbilityBase::TakeStateSnapshot(FGameplayTag SnapshotTag, FInstancedStruct SnapshotData, const FOnSnapshotResolved& OnResolved)
{
	if (!OwningAbilityComponent)
	{
		UE_LOG(LogSimpleGAS, Warning, TEXT("Ability %s has no owning ability component"), *GetName());
		return;
	}

	FSimpleAbilitySnapshot NewSnapshot;
	NewSnapshot.SequenceNumber = SnapshotSequenceCounter++;
	NewSnapshot.AbilityID = AbilityInstanceID;
	NewSnapshot.SnapshotTag = SnapshotTag;
	NewSnapshot.StateData = SnapshotData;
	NewSnapshot.TimeStamp = OwningAbilityComponent->GetServerTime();

	// Store the delegate in a map
	if (OnResolved.IsBound())
	{
		SnapshotResolveCallbacks.Add(NewSnapshot.SequenceNumber, OnResolved);
	}

	OwningAbilityComponent->AddAbilityStateSnapshot(AbilityInstanceID, NewSnapshot);
}

void USimpleAbilityBase::ClientResolvePastState(FGameplayTag StateTag, FSimpleAbilitySnapshot AuthorityState, FSimpleAbilitySnapshot PredictedState)
{
}

void USimpleAbilityBase::ClientFastForwardState(FGameplayTag StateTag, FSimpleAbilitySnapshot LatestAuthorityState)
{
}
