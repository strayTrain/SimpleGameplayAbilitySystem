#include "WaitForClientSubAbility.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleGameplayAbility/SimpleGameplayAbility.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleSubAbility/SimpleSubAbility.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleGameplayAbilityComponent/SimpleGameplayAbilityComponent.h"
#include "SimpleGameplayAbilitySystem/SimpleEventSubsystem/SimpleEventSubsystem.h"
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
		if (UGameInstance* GameInstance = AbilityComponent->GetWorld()->GetGameInstance())
		{
			if (USimpleEventSubsystem* EventSubsystem = GameInstance->GetSubsystem<USimpleEventSubsystem>())
			{
				FGameplayTagContainer EventFilter;
				EventFilter.AddTag(FDefaultTags::SubAbilityEnded());
				EventFilter.AddTag(FDefaultTags::SubAbilityCancelled());

				FGameplayTagContainer DomainFilter;
				DomainFilter.AddTag(FDefaultTags::DomainAbility());

				FSimpleEventDelegate EventDelegate;
				EventDelegate.BindDynamic(this, &UWaitForClientSubAbility::OnEventReceived);

				EventSubscriptionID = EventSubsystem->ListenForEvent(
					this,
					false, // OnlyTriggerOnce
					EventFilter,
					DomainFilter,
					EventDelegate,
					TArray<UScriptStruct*>(),
					TArray<UObject*>(),
					true, // OnlyMatchExactEvent
					true  // OnlyMatchExactDomain
				);
			}
		}
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

void UWaitForClientSubAbility::OnEventReceived(FGameplayTag EventTag, FGameplayTag Domain, FInstancedStruct Payload, UObject* Sender)
{
	// The event is filtered by tag already, so we just need to handle it
	if (EventTag == FDefaultTags::SubAbilityEnded())
	{
		OnEnded.Broadcast(FDefaultTags::SubAbilityEnded(), Payload);
		CleanupAndFinish();
		return;
	}

	OnCancelled.Broadcast(EventTag, Payload);
	CleanupAndFinish();
}

void UWaitForClientSubAbility::OnSubAbilityEnded(USimpleAbilityBase* AbilityInstance, FGameplayTag StopStatus, FInstancedStruct StopContext)
{
	// Send event to all clients (server + other clients) so everyone receives the result
	if (ParentAbilityInstance.IsValid())
	{
		if (USimpleGameplayAbilityComponent* AbilityComponent = ParentAbilityInstance->GetAbilityComponent())
		{
			AbilityComponent->SendEventToAllClients(
				FDefaultTags::SubAbilityEnded(),
				FDefaultTags::DomainAbility(),
				StopContext,
				nullptr, // Don't replicate the async action node
				TArray<UObject*>());
		}
	}

	// Unbind delegates before broadcasting to prevent re-entrancy issues
	if (AbilityInstance)
	{
		AbilityInstance->OnAbilityEnded.RemoveDynamic(this, &UWaitForClientSubAbility::OnSubAbilityEnded);
		AbilityInstance->OnAbilityCancelled.RemoveDynamic(this, &UWaitForClientSubAbility::OnSubAbilityCancelled);
	}

	OnEnded.Broadcast(StopStatus, StopContext);
	CleanupAndFinish();
}

void UWaitForClientSubAbility::OnSubAbilityCancelled(USimpleAbilityBase* AbilityInstance, FGameplayTag StopStatus, FInstancedStruct StopContext)
{
	// Send event to all clients (server + other clients) so everyone receives the result
	if (ParentAbilityInstance.IsValid())
	{
		if (USimpleGameplayAbilityComponent* AbilityComponent = ParentAbilityInstance->GetAbilityComponent())
		{
			AbilityComponent->SendEventToAllClients(
				FDefaultTags::SubAbilityCancelled(),
				FDefaultTags::DomainAbility(),
				StopContext,
				nullptr, // Don't replicate the async action node
				TArray<UObject*>());
		}
	}

	// Unbind delegates before broadcasting to prevent re-entrancy issues
	if (AbilityInstance)
	{
		AbilityInstance->OnAbilityEnded.RemoveDynamic(this, &UWaitForClientSubAbility::OnSubAbilityEnded);
		AbilityInstance->OnAbilityCancelled.RemoveDynamic(this, &UWaitForClientSubAbility::OnSubAbilityCancelled);
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
	// Prevent re-entrant cleanup
	if (bIsCleaningUp)
	{
		return;
	}
	bIsCleaningUp = true;

	// Note: We don't try to unbind sub-ability delegates here because:
	// 1. They're already unbound in OnSubAbilityEnded/OnSubAbilityCancelled before calling this
	// 2. Getting the sub-ability instance here could be unsafe if parent is being destroyed
	// 3. RemoveDynamic is safe to call multiple times on the same delegate

	// Clear the timeout timer
	if (ParentAbilityInstance.IsValid())
	{
		if (USimpleGameplayAbilityComponent* AbilityComponent = ParentAbilityInstance->GetAbilityComponent())
		{
			if (UWorld* World = AbilityComponent->GetWorld())
			{
				World->GetTimerManager().ClearTimer(TimeoutTimerHandle);
			}

			// Unsubscribe from event subsystem
			if (EventSubscriptionID.IsValid())
			{
				if (UGameInstance* GameInstance = AbilityComponent->GetWorld()->GetGameInstance())
				{
					if (USimpleEventSubsystem* EventSubsystem = GameInstance->GetSubsystem<USimpleEventSubsystem>())
					{
						EventSubsystem->StopListeningForEventSubscriptionByID(EventSubscriptionID);
					}
				}
			}
		}
	}

	SetReadyToDestroy();
}

void UWaitForClientSubAbility::SetReadyToDestroy()
{
	Super::SetReadyToDestroy();
}