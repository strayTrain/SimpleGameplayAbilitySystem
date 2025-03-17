#include "WaitForGameplayTag.h"

#include "SimpleGameplayAbilitySystem/DefaultTags/DefaultTags.h"
#include "SimpleGameplayAbilitySystem/SimpleEventSubsystem/WaitForSimpleEvent/WaitForSimpleEvent.h"
#include "SimpleGameplayAbilitySystem/SimpleGameplayAbilityComponent/SimpleGameplayAbilityComponent.h"

UWaitForGameplayTag* UWaitForGameplayTag::WaitForGameplayTag(
	UObject* WorldContextObject,
	USimpleGameplayAbilityComponent* TagOwner,
	const FGameplayTag GameplayTag,
	const bool OnlyTriggerOnce)
{
	UWaitForGameplayTag* Node = NewObject<UWaitForGameplayTag>();
	Node->WorldContext = WorldContextObject->GetWorld();
	Node->TaskOwner = TagOwner;
	Node->GameplayTag = GameplayTag;
	Node->OnlyTriggerOnce = OnlyTriggerOnce;
	
	return Node;
}

void UWaitForGameplayTag::Activate()
{
	if (!TaskOwner.IsValid() || !WorldContext.IsValid())
	{
		SetReadyToDestroy();
		return;
	}

	FGameplayTagContainer TagEvents;
	TagEvents.AddTag(FDefaultTags::GameplayTagAdded());
	TagEvents.AddTag(FDefaultTags::GameplayTagRemoved());
	
	// Listen for WaitForAbility node to end
	SimpleEventTask = UWaitForSimpleEvent::WaitForSimpleEvent(
		WorldContext.Get(),
		this,
		OnlyTriggerOnce,
		TagEvents,
		FGameplayTagContainer(),
		{  },
		{ TaskOwner.Get() },
		true,
		false);
	
	if (SimpleEventTask)
	{
		SimpleEventTask->OnEventReceived.AddDynamic(this, &UWaitForGameplayTag::OnSimpleEventReceived);
		SimpleEventTask->Activate();
	}
	
	Super::Activate();
}

void UWaitForGameplayTag::OnSimpleEventReceived(FGameplayTag EventTag, FGameplayTag DomainTag,
	FInstancedStruct Payload, UObject* Sender, FGuid EventSubscriptionID)
{
	if (!DomainTag.MatchesTagExact(GameplayTag))
	{
		return;
	}

	if (EventTag.MatchesTagExact(FDefaultTags::GameplayTagAdded()))
	{
		TagAdded.Broadcast();
	}

	if (EventTag.MatchesTagExact(FDefaultTags::GameplayTagRemoved()))
	{
		TagRemoved.Broadcast();
	}
	
	if (OnlyTriggerOnce)
	{
		SetReadyToDestroy();
	}
}

void UWaitForGameplayTag::SetReadyToDestroy()
{
	// Clean up event tasks
	if (SimpleEventTask)
	{
		SimpleEventTask->OnEventReceived.RemoveDynamic(this, &UWaitForGameplayTag::OnSimpleEventReceived);
	}
	
	Super::SetReadyToDestroy();
}


