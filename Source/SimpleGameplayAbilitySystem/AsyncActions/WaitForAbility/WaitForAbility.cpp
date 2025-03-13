#include "WaitForAbility.h"

#include "SimpleGameplayAbilitySystem/DefaultTags/DefaultTags.h"
#include "SimpleGameplayAbilitySystem/Module/SimpleGameplayAbilitySystem.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleGameplayAbility/SimpleGameplayAbility.h"
#include "SimpleGameplayAbilitySystem/SimpleGameplayAbilityComponent/SimpleGameplayAbilityComponent.h"
#include "SimpleGameplayAbilitySystem/SimpleEventSubsystem/SimpleEventSubsystem.h"

using enum ESimpleEventReplicationPolicy;
using enum EEventInitiator;

UWaitForAbility* UWaitForAbility::WaitForClientSubAbilityEnd(UObject* WorldContextObject,
	USimpleGameplayAbility* ActivatingAbility, const TSubclassOf<USimpleGameplayAbility> AbilityToActivate,
	const FInstancedStruct Payload)
{
	UWaitForAbility* Node = NewObject<UWaitForAbility>();
	Node->ActivatorAbility = ActivatingAbility;
	Node->WorldContext = WorldContextObject->GetWorld();
	Node->AbilityPayload = Payload;
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

	USimpleEventSubsystem* EventSubsystem = WorldContext->GetGameInstance()->GetSubsystem<USimpleEventSubsystem>();

	if (!EventSubsystem)
	{
		SIMPLE_LOG(ActivatorAbility->OwningAbilityComponent, FString::Printf(TEXT("[UWaitForAbility::Activate]: EventSubsystem not found")));
		SetReadyToDestroy();
		return;
	}
	
	// The Activator is the one who should activate the ability and then tell the other side about the result
	const bool IsDedicatedServer = ActivatorAbility->OwningAbilityComponent->GetNetMode() == NM_DedicatedServer;
	const bool IsListenServer = ActivatorAbility->OwningAbilityComponent->GetNetMode() == NM_ListenServer;
	const bool IsClient = ActivatorAbility->OwningAbilityComponent->GetNetMode() == NM_Client;
	
	// Listen for WaitForAbility node to end
	WaitForAbilityEndedDelegate.BindDynamic(this, &UWaitForAbility::OnWaitAbilityEndedEventReceived);
	EventSubsystem->ListenForEvent(
		this,true,
		FGameplayTagContainer(FDefaultTags::WaitForAbilityEnded()),
		FGameplayTagContainer(),
		WaitForAbilityEndedDelegate,
		TArray<UScriptStruct*>({FSimpleAbilityEndedEvent::StaticStruct()}),
		TArray<AActor*>({ActivatorAbility->OwningAbilityComponent->GetAvatarActor()}));
	
	if (Activator == Client && (IsDedicatedServer || IsListenServer))
	{
		return;
	}

	if (Activator == Server && IsClient)
	{
		return;
	}
	
	// Activate the sub ability and listen for it's ending
	AbilityEndedDelegate.BindDynamic(this, &UWaitForAbility::OnAbilityEndedEventReceived);
	EventSubsystem->ListenForEvent(
		this,true,
		FGameplayTagContainer(FDefaultTags::AbilityEnded()),
		FGameplayTagContainer(),
		AbilityEndedDelegate,
		TArray<UScriptStruct*>({FSimpleAbilityEndedEvent::StaticStruct()}),
		TArray<AActor*>({ActivatorAbility->OwningAbilityComponent->GetAvatarActor()}));

	ActivatedAbilityID = FGuid::NewGuid();
	ActivatorAbility->OwningAbilityComponent->ActivateAbilityWithID(
		ActivatedAbilityID,
		AbilityClass,
		AbilityPayload,
		true,
		EAbilityActivationPolicy::LocalOnly);
}

void UWaitForAbility::OnAbilityEndedEventReceived(FGameplayTag AbilityTag, FGameplayTag DomainTag, FInstancedStruct Payload, AActor* Sender)
{
	FSimpleAbilityEndedEvent EndedEvent = Payload.Get<FSimpleAbilityEndedEvent>();
	
	if (EndedEvent.AbilityID != ActivatedAbilityID)
	{
		return;	
	}

	// The activated ability ID exists only for the one who activated the ability. So, to make sure both client and
	// server know the ability has ended, we use the activating ability's ID as it is the same on both sides
	EndedEvent.AbilityID = ActivatorAbility->AbilityInstanceID;

	const ESimpleEventReplicationPolicy ReplicationPolicy = Activator == LocalOnly ? NoReplication : AllConnectedClientsPredicted;
	
	USimpleGameplayAbilityComponent* OwningAbilityComponent = ActivatorAbility->OwningAbilityComponent;
	OwningAbilityComponent->SendEvent(
		FDefaultTags::WaitForAbilityEnded(),
		EndedEvent.EndStatusTag,
		FInstancedStruct::Make(EndedEvent),
		OwningAbilityComponent->GetAvatarActor(),
		{}, ReplicationPolicy);
}

void UWaitForAbility::OnWaitAbilityEndedEventReceived(FGameplayTag AbilityTag, FGameplayTag DomainTag, FInstancedStruct Payload, AActor* Sender)
{
	const FSimpleAbilityEndedEvent* EndedEvent = Payload.GetPtr<FSimpleAbilityEndedEvent>();

	if (!EndedEvent || EndedEvent->AbilityID != ActivatorAbility->AbilityInstanceID)
	{
		return;
	}
	
	OnAbilityEnded.Broadcast(DomainTag, Payload);
	SetReadyToDestroy();
}
