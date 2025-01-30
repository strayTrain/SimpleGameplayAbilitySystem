#include "SimpleAbilityBase.h"

#include "SimpleGameplayAbilitySystem/Module/SimpleGameplayAbilitySystem.h"
#include "SimpleGameplayAbilitySystem/SimpleGameplayAbilityComponent/SimpleGameplayAbilityComponent.h"

void USimpleAbilityBase::InitializeAbility(USimpleGameplayAbilityComponent* InOwningAbilityComponent, FGuid InAbilityInstanceID)
{
	OwningAbilityComponent = InOwningAbilityComponent;
	AbilityInstanceID = InAbilityInstanceID;
}

void USimpleAbilityBase::TakeStateSnapshot(FGameplayTag SnapshotTag, FInstancedStruct SnapshotData)
{
	if (!OwningAbilityComponent)
	{
		UE_LOG(LogSimpleGAS, Warning, TEXT("Ability %s has no owning ability component"), *GetName());
		return;
	}

	FSimpleAbilitySnapshot NewSnapshot;
	NewSnapshot.AbilityID = AbilityInstanceID;
	NewSnapshot.StateTag = SnapshotTag;
	NewSnapshot.StateData = SnapshotData;
	NewSnapshot.TimeStamp = OwningAbilityComponent->GetServerTime();

	OwningAbilityComponent->AddAbilityStateSnapshot(AbilityInstanceID, NewSnapshot);
}

void USimpleAbilityBase::ClientResolvePastState(FGameplayTag StateTag, FSimpleAbilitySnapshot AuthorityState, FSimpleAbilitySnapshot PredictedState)
{
}

void USimpleAbilityBase::ClientFastForwardState(FGameplayTag StateTag, FSimpleAbilitySnapshot LatestAuthorityState)
{
}
