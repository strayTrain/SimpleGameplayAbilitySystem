// Copyright 2025 Ahmed Elgoni

#include "WaitForSubAbility.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleGameplayAbility/SimpleGameplayAbility.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleSubAbility/SimpleSubAbility.h"
#include "SimpleGameplayAbilitySystem/DefaultTags/DefaultTags.h"

UWaitForSubAbility* UWaitForSubAbility::WaitForSubAbility(
	USimpleGameplayAbility* ParentAbility,
	TSubclassOf<USimpleSubAbility> SubAbilityClass,
	FInstancedStruct ActivationContext)
{
	UWaitForSubAbility* AsyncAction = NewObject<UWaitForSubAbility>();
	AsyncAction->ParentAbilityInstance = ParentAbility;
	AsyncAction->SubAbilityClassToActivate = SubAbilityClass;
	AsyncAction->SubAbilityContext = ActivationContext;

	return AsyncAction;
}

void UWaitForSubAbility::Activate()
{
	USimpleGameplayAbility* ParentAbility = ParentAbilityInstance.Get();
	if (!ParentAbilityInstance.IsValid())
	{
		OnCancelled.Broadcast(FDefaultTags::SubAbilityCancelled(), FInstancedStruct());
		SetReadyToDestroy();
		return;
	}

	// Use the parent ability's ActivateSubAbility method
	EAbilityActivationResult ActivationResult;
	USimpleSubAbility* SubAbilityInstance = ParentAbility->ActivateSubAbility(
		SubAbilityClassToActivate,
		SubAbilityContext,
		ActivationResult);

	if (!SubAbilityInstance || ActivationResult != EAbilityActivationResult::Activated)
	{
		OnCancelled.Broadcast(FDefaultTags::SubAbilityCancelled(), FInstancedStruct());
		CleanupAndFinish();
		return;
	}

	// Bind to the sub-ability's end/cancel delegates
	SubAbilityInstance->OnAbilityEnded.AddDynamic(this, &UWaitForSubAbility::OnSubAbilityEnded);
	SubAbilityInstance->OnAbilityCancelled.AddDynamic(this, &UWaitForSubAbility::OnSubAbilityCancelled);
}

void UWaitForSubAbility::OnSubAbilityEnded(USimpleAbilityBase* AbilityInstance, FGameplayTag StopStatus, FInstancedStruct StopContext)
{
	OnEnded.Broadcast(StopStatus, StopContext);
	CleanupAndFinish();
}

void UWaitForSubAbility::OnSubAbilityCancelled(USimpleAbilityBase* AbilityInstance, FGameplayTag StopStatus, FInstancedStruct StopContext)
{
	OnCancelled.Broadcast(StopStatus, StopContext);
	CleanupAndFinish();
}

void UWaitForSubAbility::CleanupAndFinish()
{
	SetReadyToDestroy();
}

void UWaitForSubAbility::SetReadyToDestroy()
{
	Super::SetReadyToDestroy();
}
