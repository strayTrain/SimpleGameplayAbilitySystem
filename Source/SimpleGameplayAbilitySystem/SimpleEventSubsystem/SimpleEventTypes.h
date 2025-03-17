#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "InstancedStruct.h"
#include "SimpleEventTypes.generated.h"

DECLARE_DYNAMIC_DELEGATE_FourParams(
	FSimpleEventDelegate,
	FGameplayTag, EventTag,
	FGameplayTag, Domain,
	FInstancedStruct, Payload,
	UObject*, Sender);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEventSubscriptionRemoved, FGuid, EventSubscriptionID);

USTRUCT(BlueprintType)
struct FEventSubscription
{
	GENERATED_BODY()

	/** 
	 * The ID of the event subscription. This is used to identify the subscription when removing it.
	 */
	UPROPERTY()
	FGuid EventSubscriptionID;
	
	/**
	 * The delegate to call when an event is received that passes all the filters
	 */
	UPROPERTY()
	FSimpleEventDelegate CallbackDelegate;
	
	/**
	 * The object to call the delegate on
	 */
	UPROPERTY()
	TWeakObjectPtr<UObject> ListenerObject;
	
	/**
	 * Only listen for events with these event tags. If empty, all event tags are accepted.
	 * Event tags examples: "UI.ButtonClicked" or "Game.PlayerDied"
	 */
	UPROPERTY()
	FGameplayTagContainer EventFilter;

	/**
	 * Only listen for events with these domain tags. If empty, all domain tags are accepted.
	 * Event tags examples: "Domains.UI" or "Domains.Game"
	 */
	UPROPERTY()
	FGameplayTagContainer DomainFilter;

	/**
	 * Only listen for events with these payload types. If empty, all payload types are accepted.
	 */
	UPROPERTY()
	TArray<UScriptStruct*> PayloadFilter;

	/**
	 * Only listen for events from these sender actors. If empty, all sender actors are accepted.
	 * Note: Sender actors are an optional parameter when sending events which means that some events might not have a sender actor.
	 */
	UPROPERTY()
	TArray<TWeakObjectPtr<UObject>> SenderFilter;
	
	/**
	 * Only trigger the delegate once. After the first event is received, the listener is removed.
	 */
	UPROPERTY()
	bool OnlyTriggerOnce = false;

	/**
	 * Only trigger the delegate if the event tags match exactly. i.e "A.B" will only match "A.B" and not "A.B.C".
	 */
	UPROPERTY()
	bool OnlyMatchExactEvent = true;

	/**
	 * Only trigger the delegate if the domain tags match exactly. i.e "A.B" will only match "A.B" and not "A.B.C".
	 */
	UPROPERTY()
	bool OnlyMatchExactDomain = true;

	bool operator==(const FEventSubscription& Other) const
	{
		return EventSubscriptionID == Other.EventSubscriptionID;
	}
};