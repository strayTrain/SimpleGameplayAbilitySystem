// Copyright 2025 Ahmed Elgoni

#include "WaitForClientSubAbility.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleGameplayAbility/SimpleGameplayAbility.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleSubAbility/SimpleSubAbility.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleGameplayAbilityComponent/SimpleGameplayAbilityComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "SimpleGameplayAbilitySystem/DefaultTags/DefaultTags.h"

UWaitForClientSubAbility* UWaitForClientSubAbility::WaitForClientSubAbility(
	USimpleGameplayAbility* ParentAbility,
	TSubclassOf<USimpleSubAbility> SubAbilityClass,
	FInstancedStruct ActivationContext,
	float TimeoutDuration)
{
	UWaitForClientSubAbility* AsyncAction = NewObject<UWaitForClientSubAbility>();
	AsyncAction->ParentAbilityInstance = ParentAbility;
	AsyncAction->SubAbilityClassToActivate = SubAbilityClass;
	AsyncAction->SubAbilityContext = ActivationContext;
	AsyncAction->Timeout = TimeoutDuration;

	if (ParentAbility)
	{
		AsyncAction->ExpectedAbilityID = ParentAbility->AbilityID;
	}

	return AsyncAction;
}

void UWaitForClientSubAbility::Activate()
{
	USimpleGameplayAbility* ParentAbility = ParentAbilityInstance.Get();
	if (!ParentAbilityInstance.IsValid())
	{
		OnCancelled.Broadcast(FDefaultTags::SubAbilityCancelled(),  FInstancedStruct());
		SetReadyToDestroy();
		return;
	}

	USimpleGameplayAbilityComponent* AbilityComponent = ParentAbility->GetAbilityComponent();
	if (!AbilityComponent)
	{
		OnCancelled.Broadcast(FDefaultTags::SubAbilityCancelled(),FInstancedStruct());
		SetReadyToDestroy();
		return;
	}

	// Set up timeout if specified
	if (Timeout > 0.0f)
	{
		if (UWorld* World = AbilityComponent->GetWorld())
		{
			World->GetTimerManager().SetTimer(TimeoutTimerHandle, this, &UWaitForClientSubAbility::OnTimeoutExpired, Timeout, false);
		}
	}

	// True if this async node is running on the client that owns the ability component
	const bool IsLocallyControlled = AbilityComponent->GetOwnerRole() < ROLE_Authority;

	if (!IsLocallyControlled)
	{
		AbilityComponent->OnEventReceived.AddDynamic(this, &UWaitForClientSubAbility::OnEventReceived);
		return;
	}
	
	USimpleSubAbility* SubAbilityInstance = ParentAbility->GetSubAbilityInstance(SubAbilityClassToActivate);
	if (!SubAbilityInstance)
	{
		OnCancelled.Broadcast(FDefaultTags::SubAbilityCancelled(),FInstancedStruct());
		CleanupAndFinish();
	}

	SubAbilityInstance->OnAbilityEnded.AddDynamic(this, &UWaitForClientSubAbility::OnSubAbilityEnded);
	SubAbilityInstance->OnAbilityCancelled.AddDynamic(this, &UWaitForClientSubAbility::OnSubAbilityCancelled);
	SubAbilityInstance->ActivateAbility(SubAbilityContext);

}

void UWaitForClientSubAbility::OnEventReceived(FGameplayTag EventTag, FGuid AbilityID, FInstancedStruct EventContext)
{
	if (AbilityID != ExpectedAbilityID)
	{
		return;
	}

	if (EventTag == FDefaultTags::SubAbilityEnded())
	{
		OnEnded.Broadcast(FDefaultTags::SubAbilityEnded(),EventContext);
		CleanupAndFinish();
		return;
	}
	
	OnCancelled.Broadcast(EventTag,EventContext);
	CleanupAndFinish();
}

void UWaitForClientSubAbility::OnSubAbilityEnded(USimpleAbilityBase* AbilityInstance, FGameplayTag StopStatus, FInstancedStruct StopContext)
{
	if (ParentAbilityInstance->GetNetworkRole() == EAbilityNetworkRole::Client)
	{
		ParentAbilityInstance->GetAbilityComponent()->SendEventToServer(FDefaultTags::SubAbilityEnded(), ExpectedAbilityID, StopContext);
	}

	OnEnded.Broadcast(StopStatus,StopContext);
	CleanupAndFinish();	
}

void UWaitForClientSubAbility::OnSubAbilityCancelled(USimpleAbilityBase* AbilityInstance, FGameplayTag StopStatus, FInstancedStruct StopContext)
{
	if (ParentAbilityInstance->GetNetworkRole() == EAbilityNetworkRole::Client)
	{
		ParentAbilityInstance->GetAbilityComponent()->SendEventToServer(FDefaultTags::SubAbilityCancelled(), ExpectedAbilityID, StopContext);
	}
	
	OnCancelled.Broadcast(StopStatus,StopContext);
	CleanupAndFinish();
}

void UWaitForClientSubAbility::OnTimeoutExpired()
{
	OnTimeout.Broadcast(FDefaultTags::SubAbilityCancelled(), FInstancedStruct());
	CleanupAndFinish();
}

void UWaitForClientSubAbility::CleanupAndFinish()
{
	// Clear the timeout timer
	if (ParentAbilityInstance.IsValid())
	{
		if (USimpleGameplayAbilityComponent* AbilityComponent = ParentAbilityInstance->GetAbilityComponent())
		{
			if (UWorld* World = AbilityComponent->GetWorld())
			{
				World->GetTimerManager().ClearTimer(TimeoutTimerHandle);
			}

			// Unbind from the event dispatcher
			AbilityComponent->OnEventReceived.RemoveDynamic(this, &UWaitForClientSubAbility::OnEventReceived);
		}
	}

	SetReadyToDestroy();
}

void UWaitForClientSubAbility::SetReadyToDestroy()
{
	Super::SetReadyToDestroy();
}