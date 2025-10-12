#include "WaitForSimpleEvent.h"

#include "SimpleGameplayAbilitySystem/SimpleEventSubsystem/SimpleEventSubsystem.h"

UWaitForSimpleEvent* UWaitForSimpleEvent::WaitForSimpleEvent(
	UObject* WorldContextObject,
	UObject* Listener,
	bool OnlyTriggerOnce,
	FGameplayTagContainer EventFilter,
	FGameplayTagContainer DomainFilter,
	TArray<UScriptStruct*> PayloadFilter,
	TArray<UObject*> SenderFilter,
	bool OnlyMatchExactEvent,
	bool OnlyMatchExactDomain)
{
	//Create the task instance via NewObject
	UWaitForSimpleEvent* Task = NewObject<UWaitForSimpleEvent>();

	Task->Listener = Listener;
	Task->OnlyTriggerOnce = OnlyTriggerOnce;
	Task->EventFilter = EventFilter;
	Task->DomainFilter = DomainFilter;
	Task->PayloadFilter = PayloadFilter,
	Task->SenderFilter = SenderFilter;
	Task->OnlyMatchExactEvent = OnlyMatchExactEvent;
	Task->OnlyMatchExactDomain = OnlyMatchExactDomain;
	Task->EventCallbackDelegate = FSimpleEventDelegate();

	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		Task->WorldContext = World;
		Task->RegisterWithGameInstance(World);
	}
	
	return Task;
}

void UWaitForSimpleEvent::Activate()
{
	USimpleEventSubsystem* EventSubsystem = WorldContext->GetGameInstance()->GetSubsystem<USimpleEventSubsystem>();

	if (!EventSubsystem)
	{
		SetReadyToDestroy();
		return;
	}

	EventCallbackDelegate.BindDynamic(this, &UWaitForSimpleEvent::OnSimpleEventReceived);
	EventID = EventSubsystem->ListenForEvent(Listener, OnlyTriggerOnce, EventFilter, DomainFilter, EventCallbackDelegate, PayloadFilter, SenderFilter);
	EventSubsystem->OnEventSubscriptionRemoved.AddDynamic(this, &UWaitForSimpleEvent::OnEventSubscriptionRemoved);
}

void UWaitForSimpleEvent::OnSimpleEventReceived(FGameplayTag AbilityTag, FGameplayTag DomainTag, FInstancedStruct Payload, UObject* Sender)
{
	OnEventReceived.Broadcast(AbilityTag, DomainTag, Payload, Sender, EventID);

	if (OnlyTriggerOnce)
	{
		SetReadyToDestroy();
	}
}

void UWaitForSimpleEvent::OnEventSubscriptionRemoved(FGuid SubscriptionID)
{
	if (EventID == SubscriptionID)
	{
		SetReadyToDestroy();
	}
}
