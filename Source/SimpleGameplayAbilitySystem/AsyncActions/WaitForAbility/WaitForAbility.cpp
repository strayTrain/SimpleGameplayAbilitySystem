#include "WaitForAbility.h"

#include "SimpleGameplayAbilitySystem/DefaultTags/DefaultTags.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleGameplayAbility/SimpleGameplayAbility.h"
#include "SimpleGameplayAbilitySystem/SimpleGameplayAbilityComponent/SimpleGameplayAbilityComponent.h"

using enum ESimpleEventReplicationPolicy;
using enum EEventInitiator;

// Factory methods remain the same
UWaitForAbility* UWaitForAbility::WaitForClientSubAbilityEnd(UObject* WorldContextObject,
	USimpleGameplayAbility* ActivatingAbility, const TSubclassOf<USimpleGameplayAbility> AbilityToActivate,
	const FInstancedStruct Context)
{
	UWaitForAbility* Node = NewObject<UWaitForAbility>();
	Node->ActivatorAbility = ActivatingAbility;
	Node->WorldContext = WorldContextObject->GetWorld();
	Node->AbilityPayload = Context;
	Node->AbilityClass = AbilityToActivate;
	Node->Activator = Client;
	return Node;
}

UWaitForAbility* UWaitForAbility::WaitForServerSubAbilityEnd(UObject* WorldContextObject,
	USimpleGameplayAbility* ActivatingAbility, const TSubclassOf<USimpleGameplayAbility> AbilityToActivate,
	const FInstancedStruct Payload)
{
	UWaitForAbility* Node = NewObject<UWaitForAbility>();
	Node->ActivatorAbility = ActivatingAbility;
	Node->WorldContext = WorldContextObject->GetWorld();
	Node->AbilityPayload = Payload;
	Node->AbilityClass = AbilityToActivate;
	Node->Activator = Server;
	return Node;
}

UWaitForAbility* UWaitForAbility::WaitForLocalSubAbilityEnd(UObject* WorldContextObject,
	USimpleGameplayAbility* ActivatingAbility, const TSubclassOf<USimpleGameplayAbility> AbilityToActivate,
	const FInstancedStruct Payload)
{
	UWaitForAbility* Node = NewObject<UWaitForAbility>();
	Node->ActivatorAbility = ActivatingAbility;
	Node->WorldContext = WorldContextObject->GetWorld();
	Node->AbilityPayload = Payload;
	Node->AbilityClass = AbilityToActivate;
	Node->Activator = LocalOnly;
	return Node;
}

void UWaitForAbility::Activate()
{
	if (!ActivatorAbility.IsValid() || !AbilityClass)
	{
		SetReadyToDestroy();
		return;
	}
	
	// Listen for WaitForAbility node to end
	// Use WaitForSimpleEvent instead of direct event system calls
	WaitAbilityEndedEventTask = UWaitForSimpleEvent::WaitForSimpleEvent(
		WorldContext.Get(),
		this,
		true,
		FGameplayTagContainer(FDefaultTags::WaitForAbilityEnded()),
		FGameplayTagContainer(),
		TArray<UScriptStruct*>({FSimpleAbilityEndedEvent::StaticStruct()}),
		TArray<UObject*>({ ActivatorAbility->OwningAbilityComponent->GetAvatarActor() }),
		false,
		false);
	
	if (WaitAbilityEndedEventTask)
	{
		WaitAbilityEndedEventTask->OnEventReceived.AddDynamic(this, &UWaitForAbility::OnWaitAbilityEndedEventReceived);
		WaitAbilityEndedEventTask->Activate();
	}
	
	const bool IsDedicatedServer = ActivatorAbility->OwningAbilityComponent->GetNetMode() == NM_DedicatedServer;
	const bool IsListenServer = ActivatorAbility->OwningAbilityComponent->GetNetMode() == NM_ListenServer;
	const bool IsClient = ActivatorAbility->OwningAbilityComponent->GetNetMode() == NM_Client;
	
	if (Activator == Client && (IsDedicatedServer || IsListenServer))
	{
		return;
	}

	if (Activator == Server && IsClient)
	{
		return;
	}
	
	// Listen for the ability's end event
	AbilityEndedEventTask = UWaitForSimpleEvent::WaitForSimpleEvent(
		WorldContext.Get(),
		this,
		true,
		FGameplayTagContainer(FDefaultTags::AbilityEnded()),
		FGameplayTagContainer(),
		TArray<UScriptStruct*>({FSimpleAbilityEndedEvent::StaticStruct()}),
		TArray<UObject*>({ActivatorAbility->OwningAbilityComponent->GetAvatarActor()}),
		false,
		false);
	
	if (AbilityEndedEventTask)
	{
		AbilityEndedEventTask->OnEventReceived.AddDynamic(this, &UWaitForAbility::OnAbilityEndedEventReceived);
		AbilityEndedEventTask->Activate();
	}

	// Activate the sub ability
	ActivatedAbilityID = FGuid::NewGuid();
	ActivatorAbility->OwningAbilityComponent->ActivateAbilityWithID(
		ActivatedAbilityID,
		AbilityClass,
		AbilityPayload,
		true,
		EAbilityActivationPolicy::LocalOnly);
}

void UWaitForAbility::OnAbilityEndedEventReceived(FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload, UObject* Sender, FGuid EventSubscriptionID)
{
	FSimpleAbilityEndedEvent EndedEvent = Payload.Get<FSimpleAbilityEndedEvent>();
	
	if (EndedEvent.AbilityID != ActivatedAbilityID)
	{
		return;	
	}

	// The activated ability ID exists only for the one who activated the ability
	// Use the activating ability's ID as it is the same on both sides
	EndedEvent.AbilityID = ActivatorAbility->AbilityInstanceID;

	const ESimpleEventReplicationPolicy ReplicationPolicy = 
		Activator == LocalOnly ? NoReplication : AllConnectedClientsPredicted;
	
	USimpleGameplayAbilityComponent* OwningAbilityComponent = ActivatorAbility->OwningAbilityComponent;
	OwningAbilityComponent->SendEvent(
		FDefaultTags::WaitForAbilityEnded(),
		EndedEvent.EndStatusTag,
		FInstancedStruct::Make(EndedEvent),
		OwningAbilityComponent->GetAvatarActor(),
		{}, ReplicationPolicy);
}

void UWaitForAbility::OnWaitAbilityEndedEventReceived(FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload, UObject* Sender, FGuid EventSubscriptionID)
{
	const FSimpleAbilityEndedEvent* EndedEvent = Payload.GetPtr<FSimpleAbilityEndedEvent>();

	if (!EndedEvent || EndedEvent->AbilityID != ActivatorAbility->AbilityInstanceID)
	{
		return;
	}
	
	OnAbilityEnded.Broadcast(DomainTag, Payload, EndedEvent->WasCancelled);
	SetReadyToDestroy();
}

void UWaitForAbility::SetReadyToDestroy()
{
	// Clean up event tasks
	if (AbilityEndedEventTask)
	{
		AbilityEndedEventTask->OnEventReceived.RemoveDynamic(this, &UWaitForAbility::OnAbilityEndedEventReceived);
	}
	
	if (WaitAbilityEndedEventTask)
	{
		WaitAbilityEndedEventTask->OnEventReceived.RemoveDynamic(this, &UWaitForAbility::OnWaitAbilityEndedEventReceived);
	}
	
	Super::SetReadyToDestroy();
}