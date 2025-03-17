#include "SimpleEventSubSystem.h"
#include "SimpleGameplayAbilitySystem/Module/SimpleGameplayAbilitySystem.h"

void USimpleEventSubsystem::SendEvent(FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload, UObject* Sender, TArray<UObject*> ListenerFilter)
{
	// If we come across an invalid listener on a subscription (e.g. the listener was garbage collected and forgot to unsubscribe)
	// we'll remove that subscription. We store the indexes of the invalid listeners here.
	TArray<FEventSubscription> InvalidSubscriptions = TArray<FEventSubscription>();

	// We check subscriptions from the most recently added to the oldest
	for (int32 i = EventSubscriptions.Num() - 1; i >= 0; --i)
	{
		if (!EventSubscriptions.IsValidIndex(i))
		{
			continue;
		}
		
		FEventSubscription Subscription = EventSubscriptions[i];

		const UObject* Listener = Subscription.ListenerObject.Get();
		
		if (!Listener)
		{
			InvalidSubscriptions.Add(Subscription);
			continue;
		}

		if (ListenerFilter.Num() > 0 && !ListenerFilter.Contains(Listener))
		{
			continue;
		}

		if (!Subscription.EventFilter.IsEmpty())
		{
			if (Subscription.OnlyMatchExactEvent)
			{
				if (!Subscription.EventFilter.HasTagExact(EventTag))
				{
					continue;
				}
			}
			else
			{
				if (!Subscription.EventFilter.HasTag(EventTag))
				{
					continue;
				}
			}	
		}

		if (!Subscription.DomainFilter.IsEmpty())
		{
			if (Subscription.OnlyMatchExactDomain)
			{
				if (!Subscription.DomainFilter.HasTagExact(DomainTag))
				{
					continue;
				}
			}
			else
			{
				if (!Subscription.DomainFilter.HasTag(DomainTag))
				{
					continue;
				}
			}	
		}
		
		if (Subscription.PayloadFilter.Num() > 0)
		{
			if (!Payload.IsValid())
			{
				UE_LOG(
					LogSimpleGAS, Warning,
					TEXT("No payload passed for Listener %s but the listener has a payload filter"), 
					*Listener->GetName());
				continue;	
			}

			if (!Subscription.PayloadFilter.Contains(Payload.GetScriptStruct()))
			{
				UE_LOG(
					LogSimpleGAS, Warning,
					TEXT("Payload type %s does not pass the Listener payload filter"),
					*Payload.GetScriptStruct()->GetName());
				continue;
			}
		}

		if (Subscription.SenderFilter.Num() > 0)
		{
			if (!Sender)
			{
				continue;
			}

			if (!Subscription.SenderFilter.Contains(Sender))
			{
				continue;
			}
		}
		
		bool WasCalled = Subscription.CallbackDelegate.ExecuteIfBound(EventTag, DomainTag, Payload, Sender);

		if (!WasCalled)
		{
			UE_LOG(LogSimpleGAS, Warning, TEXT("Event %s passed filters but failed to call the delegate for Listener %s"), *EventTag.GetTagName().ToString(), *Listener->GetName());
			continue;
		}

		if (Subscription.OnlyTriggerOnce)
		{
			InvalidSubscriptions.Add(Subscription);
		}
	}

	EventSubscriptions.RemoveAll([&InvalidSubscriptions](const FEventSubscription& Subscription) {
		return InvalidSubscriptions.Contains(Subscription);
	});
}

FGuid USimpleEventSubsystem::ListenForEvent(UObject* Listener, bool OnlyTriggerOnce, FGameplayTagContainer EventFilter,
                                            FGameplayTagContainer DomainFilter, const FSimpleEventDelegate& EventReceivedDelegate,
                                            TArray<UScriptStruct*> PayloadFilter, TArray<UObject*> SenderFilter, bool OnlyMatchExactEvent,
                                            bool OnlyMatchExactDomain)
{
	FEventSubscription Subscription;

	if (!Listener)
	{
		UE_LOG(LogSimpleGAS, Warning, TEXT("Null Listener passed to ListenForEvent. Can't listen for event."));
		return Subscription.EventSubscriptionID;
	}

	if (!EventReceivedDelegate.IsBound())
	{
		UE_LOG(LogSimpleGAS, Warning, TEXT("No delegate bound to ListenForEvent. Can't listen for event."));
		return Subscription.EventSubscriptionID;
	}

	Subscription.EventSubscriptionID = FGuid::NewGuid();
	Subscription.ListenerObject = Listener;
	Subscription.CallbackDelegate = EventReceivedDelegate;
	Subscription.EventFilter.AppendTags(EventFilter);
	Subscription.DomainFilter.AppendTags(DomainFilter);
	Subscription.PayloadFilter = PayloadFilter;
	Subscription.SenderFilter.Append(SenderFilter);
	Subscription.OnlyTriggerOnce = OnlyTriggerOnce;
	Subscription.OnlyMatchExactEvent = OnlyMatchExactEvent;
	Subscription.OnlyMatchExactDomain = OnlyMatchExactDomain;

	EventSubscriptions.Add(Subscription);
	return Subscription.EventSubscriptionID;
}

void USimpleEventSubsystem::StopListeningForEventSubscriptionByID(FGuid EventSubscriptionID)
{
	EventSubscriptions.RemoveAll([this, EventSubscriptionID](const FEventSubscription& Subscription)
	{
		if (Subscription.EventSubscriptionID == EventSubscriptionID)
		{
			OnEventSubscriptionRemoved.Broadcast(Subscription.EventSubscriptionID);
			return true;
		}
		return false;
	});
}

void USimpleEventSubsystem::StopListeningForEventsByFilter(UObject* Listener, FGameplayTagContainer EventTagFilter, FGameplayTagContainer DomainTagFilter)
{
	EventSubscriptions.RemoveAll([this, Listener, EventTagFilter, DomainTagFilter](const FEventSubscription& Subscription)
	{
		if (Subscription.ListenerObject == Listener &&
			(!EventTagFilter.Num() || EventTagFilter.HasAny(Subscription.EventFilter)) &&
			(!DomainTagFilter.Num() || DomainTagFilter.HasAny(Subscription.DomainFilter)))
			{
				OnEventSubscriptionRemoved.Broadcast(Subscription.EventSubscriptionID);
				return true;
			}
		return false;
	});
}

void USimpleEventSubsystem::StopListeningForAllEvents(UObject* Listener)
{
	EventSubscriptions.RemoveAll([this, Listener](const FEventSubscription& Subscription)
	{
		if (Subscription.ListenerObject == Listener)
		{
			OnEventSubscriptionRemoved.Broadcast(Subscription.EventSubscriptionID);
			return true;
		}
		return false;
	});
}
