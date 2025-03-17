#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SimpleEventTypes.h"
#include "SimpleEventSubsystem.generated.h"

UCLASS()
class SIMPLEGAMEPLAYABILITYSYSTEM_API USimpleEventSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * Sends an event to all listeners. The event is not replicated.
	 *
	 * @param EventTag The gameplay tag identifying the event. (mandatory)
	 * @param DomainTag The domain tag categorizing the event. (optional)
	 * @param Payload The payload of the event as an instanced struct (optional).
	 * @param Sender The actor that sent the event. (optional)
	 * @param ListenerFilter Only send the event to listeners in this list. If not set, the event will be sent to all listeners.
	 */
	UFUNCTION(BlueprintCallable, Category = "SimpleEventSubsystem", meta=(AdvancedDisplay=4, AutoCreateRefTerm = "ListenerFilter"))
	void SendEvent(FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload, UObject* Sender, TArray<UObject*> ListenerFilter);

	/**
	 * Register a listener to receive events. The listener will be notified when an event is sent that matches the provided filters.
	 *
	 * @param Listener The object listening for the event (mandatory).
	 * @param OnlyTriggerOnce If true, the listener will only be notified once and then automatically unsubscribed.
	 * @param EventFilter Only listen for events with tags matching this filter. If not set, the listener will accept all events.
	 * @param DomainFilter Only listen for events with domains matching this filter. If not set, the listener will accept events from all domains.
	 * @param EventReceivedDelegate The FGlobalEventDelegate that gets called when the event is received and passes the filters (mandatory).
  	 * @param PayloadFilter Only respond to the event if there is a payload present and it is of this type. If not set, the listener will accept events with any (or no) payload.
	 * @param SenderFilter Only respond to the event if the sender is in this list. If not set, the listener will accept events from any sender.
	 * @param OnlyMatchExactEvent If true, only listen for events that match the filter tags exactly. i.e "A.B" will only match "A.B" and not "A.B.C".
	 * @param OnlyMatchExactDomain If true, only listen for events that match the domain tags exactly. i.e "A.B" will only match "A.B" and not "A.B.C".
	 */
	UFUNCTION(BlueprintCallable, Category = "SimpleEventSubsystem", meta=(AdvancedDisplay=5, AutoCreateRefTerm = "PayloadFilter,SenderFilter"))
	FGuid ListenForEvent(
		UObject* Listener,
		bool OnlyTriggerOnce,
		FGameplayTagContainer EventFilter,
		FGameplayTagContainer DomainFilter,
		const FSimpleEventDelegate& EventReceivedDelegate,
		TArray<UScriptStruct*> PayloadFilter,
		TArray<UObject*> SenderFilter,
		bool OnlyMatchExactEvent = true,
		bool OnlyMatchExactDomain = true);

	/**
	 * Stop listening for an event on a listener.
	 *
	 * @param EventSubscriptionID The ID of the event subscription to stop listening for (mandatory).
	 */
	UFUNCTION(BlueprintCallable, Category = "SimpleEventSubsystem")
	void StopListeningForEventSubscriptionByID(FGuid EventSubscriptionID);

	/**
	 * Stop listening for events on a listener that match the provided filters.
	 *
	 * @param Listener The object listening for the event (mandatory).
	 * @param EventTagFilter Stop listening for these events (mandatory).
	 * @param DomainTagFilter Only stop listening if the domain also matches (optional). If not set, the listener will stop listening for events in EventTagFilter across all domains.
	 */
	UFUNCTION(BlueprintCallable, Category = "SimpleEventSubsystem")
	void StopListeningForEventsByFilter(UObject* Listener, FGameplayTagContainer EventTagFilter, FGameplayTagContainer DomainTagFilter);

	/**
	 * Stop listening all for events on a listener 
	 *
	 * @param Listener The object listening for the event (mandatory).
	 */
	UFUNCTION(BlueprintCallable, Category = "SimpleEventSubsystem")
	void StopListeningForAllEvents(UObject* Listener);

	UPROPERTY(BlueprintAssignable)
	FOnEventSubscriptionRemoved OnEventSubscriptionRemoved;

private:
	TArray<FEventSubscription> EventSubscriptions;
};