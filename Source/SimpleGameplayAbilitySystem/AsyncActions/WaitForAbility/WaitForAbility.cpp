#include "WaitForAbility.h"

#include "SimpleGameplayAbilitySystem/DefaultTags/DefaultTags.h"
#include "SimpleGameplayAbilitySystem/Module/SimpleGameplayAbilitySystem.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleGameplayAbility/SimpleGameplayAbility.h"
#include "SimpleGameplayAbilitySystem/SimpleGameplayAbilityComponent/SimpleGameplayAbilityComponent.h"
#include "SimpleGameplayAbilitySystem/SimpleEventSubsystem/SimpleEventSubsystem.h"

UWaitForAbility* UWaitForAbility::WaitForClientAbility(
	UObject* WorldContextObject, USimpleGameplayAbility* ActivatingAbility,
	const TSubclassOf<USimpleGameplayAbility> AbilityToActivate, const FInstancedStruct Payload)
{
	UWaitForAbility* Node = NewObject<UWaitForAbility>();
	Node->ActivatorAbility = ActivatingAbility;
	Node->WorldContext = WorldContextObject->GetWorld();
	Node->AbilityPayload = Payload;
	Node->AbilityClass = AbilityToActivate;
	Node->Activator = EEventInitiator::Client;
	return Node;
}

UWaitForAbility* UWaitForAbility::WaitForServerAbility(
	UObject* WorldContextObject, USimpleGameplayAbility* ActivatingAbility,
	const TSubclassOf<USimpleGameplayAbility> AbilityToActivate, const FInstancedStruct Payload)
{
	UWaitForAbility* Node = NewObject<UWaitForAbility>();
	Node->ActivatorAbility = ActivatingAbility;
	Node->WorldContext = WorldContextObject->GetWorld();
	Node->AbilityPayload = Payload;
	Node->AbilityClass = AbilityToActivate;
	Node->Activator = EEventInitiator::Server;
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
	bool IsActivator = (Activator == EEventInitiator::Server && ActivatorAbility->WasActivatedOnServer()) || (Activator == EEventInitiator::Client && ActivatorAbility->WasActivatedOnClient());
	
	if (IsActivator)
	{
		// Listen for the activated ability ending
		AbilityEndedDelegate.BindDynamic(this, &UWaitForAbility::OnAbilityEndedEventReceived);
		EventSubsystem->ListenForEvent(
			this,true,
			FGameplayTagContainer(FDefaultTags::AbilityEnded),
			FGameplayTagContainer(),
			AbilityEndedDelegate,
			TArray<UScriptStruct*>({FSimpleAbilityEndedEvent::StaticStruct()}),
			TArray<AActor*>({ActivatorAbility->OwningAbilityComponent->GetAvatarActor()}));
		
		ActivatedAbilityID = ActivatorAbility->OwningAbilityComponent->ActivateAbility(AbilityClass, AbilityPayload, false, EAbilityActivationPolicy::LocalOnly);
	}

	// Listen for WaitForAbility to end
	WaitForAbilityEndedDelegate.BindDynamic(this, &UWaitForAbility::OnWaitAbilityEndedEventReceived);
	EventSubsystem->ListenForEvent(
		this,true,
		FGameplayTagContainer(FDefaultTags::WaitForAbilityEnded),
		FGameplayTagContainer(),
		WaitForAbilityEndedDelegate,
		TArray<UScriptStruct*>({FSimpleAbilityEndedEvent::StaticStruct()}),
		TArray<AActor*>({ActivatorAbility->OwningAbilityComponent->GetAvatarActor()}));
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
	
	USimpleGameplayAbilityComponent* OwningAbilityComponent = ActivatorAbility->OwningAbilityComponent;
	OwningAbilityComponent->SendEvent(
		FDefaultTags::WaitForAbilityEnded,
		EndedEvent.EndStatus,
		FInstancedStruct::Make(EndedEvent),
		OwningAbilityComponent->GetAvatarActor(),
		ESimpleEventReplicationPolicy::AllConnectedClientsPredicted);
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
