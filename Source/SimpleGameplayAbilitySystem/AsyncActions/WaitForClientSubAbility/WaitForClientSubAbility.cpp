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

	// Verify this is a client-predicted or server-initiated ability
	const EAbilityNetworkRole NetworkRole = ParentAbility->GetNetworkRole();
	if (NetworkRole == EAbilityNetworkRole::DedicatedServer)
	{
		// Dedicated server with no client prediction - this async node won't work correctly
		OnCancelled.Broadcast(FDefaultTags::SubAbilityCancelled(), FInstancedStruct());
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

	// Check if this is the owning client (works for both pure clients and listen servers)
	const bool IsOwningClient = AbilityComponent->IsOwnedByLocalPlayer();

	if (!IsOwningClient)
	{
		// Server or other clients: Wait for the owning client to send the result via multicast event
		AbilityComponent->OnEventReceived.AddDynamic(this, &UWaitForClientSubAbility::OnEventReceived);
		return;
	}

	// Owning client: Actually run the sub-ability
	USimpleSubAbility* SubAbilityInstance = ParentAbility->GetSubAbilityInstance(SubAbilityClassToActivate);
	if (!SubAbilityInstance)
	{
		OnCancelled.Broadcast(FDefaultTags::SubAbilityCancelled(),FInstancedStruct());
		CleanupAndFinish();
		return;
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
	// Send event to all clients (server + other clients) so everyone receives the result
	if (USimpleGameplayAbilityComponent* AbilityComponent = ParentAbilityInstance->GetAbilityComponent())
	{
		AbilityComponent->SendEventToAllClients(FDefaultTags::SubAbilityEnded(), ExpectedAbilityID, StopContext);
	}

	OnEnded.Broadcast(StopStatus, StopContext);
	CleanupAndFinish();
}

void UWaitForClientSubAbility::OnSubAbilityCancelled(USimpleAbilityBase* AbilityInstance, FGameplayTag StopStatus, FInstancedStruct StopContext)
{
	// Send event to all clients (server + other clients) so everyone receives the result
	if (USimpleGameplayAbilityComponent* AbilityComponent = ParentAbilityInstance->GetAbilityComponent())
	{
		AbilityComponent->SendEventToAllClients(FDefaultTags::SubAbilityCancelled(), ExpectedAbilityID, StopContext);
	}

	OnCancelled.Broadcast(StopStatus, StopContext);
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