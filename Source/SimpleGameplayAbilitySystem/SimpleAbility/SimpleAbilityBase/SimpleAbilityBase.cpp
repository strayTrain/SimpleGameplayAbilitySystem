﻿#include "SimpleAbilityBase.h"

#include "SimpleGameplayAbilitySystem/SimpleGameplayAbilityComponent/SimpleGameplayAbilityComponent.h"

void USimpleAbilityBase::InitializeAbility(USimpleGameplayAbilityComponent* InOwningAbilityComponent, FGuid InAbilityInstanceID)
{
	OwningAbilityComponent = InOwningAbilityComponent;
	AbilityInstanceID = InAbilityInstanceID;
}

void USimpleAbilityBase::TakeStateSnapshot(FSimpleAbilitySnapshot State)
{
	if (!OwningAbilityComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("Ability %s has no owning ability component"), *GetName());
		return;
	}

	OwningAbilityComponent->AddAbilityStateSnapshot(AbilityInstanceID, State);
}
