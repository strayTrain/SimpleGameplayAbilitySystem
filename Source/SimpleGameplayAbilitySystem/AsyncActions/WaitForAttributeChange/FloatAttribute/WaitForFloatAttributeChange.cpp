#include "WaitForFloatAttributeChange.h"

#include "SimpleGameplayAbilitySystem/AsyncActions/WaitForAbility/WaitForAbility.h"
#include "SimpleGameplayAbilitySystem/DefaultTags/DefaultTags.h"
#include "SimpleGameplayAbilitySystem/SimpleGameplayAbilityComponent/SimpleGameplayAbilityComponent.h"

UWaitForFloatAttributeChange* UWaitForFloatAttributeChange::WaitForFloatAttributeChange(
	UObject* WorldContextObject,
	USimpleGameplayAbilityComponent* AttributeOwner,
	const FGameplayTag AttributeTag,
	const bool OnlyTriggerOnce)
{
	UWaitForFloatAttributeChange* Node = NewObject<UWaitForFloatAttributeChange>();
	Node->WorldContext = WorldContextObject->GetWorld();
	Node->AttributeOwner = AttributeOwner;
	Node->AttributeTag = AttributeTag;
	Node->OnlyTriggerOnce = OnlyTriggerOnce;
	
	return Node;
}

void UWaitForFloatAttributeChange::Activate()
{
	if (!AttributeOwner.IsValid() || !WorldContext.IsValid())
	{
		SetReadyToDestroy();
		return;
	}

	FGameplayTagContainer FloatEvents;
	FloatEvents.AddTag(FDefaultTags::FloatAttributeCurrentValueChanged());
	FloatEvents.AddTag(FDefaultTags::FloatAttributeMinCurrentValueChanged());
	FloatEvents.AddTag(FDefaultTags::FloatAttributeMaxCurrentValueChanged());
	FloatEvents.AddTag(FDefaultTags::FloatAttributeBaseValueChanged());
	FloatEvents.AddTag(FDefaultTags::FloatAttributeMinBaseValueChanged());
	FloatEvents.AddTag(FDefaultTags::FloatAttributeMaxBaseValueChanged());
	
	// Listen for WaitForAbility node to end
	// Use WaitForSimpleEvent instead of direct event system calls
	FloatAttributeChangedEventTask = UWaitForSimpleEvent::WaitForSimpleEvent(
		WorldContext.Get(),
		this,
		OnlyTriggerOnce,
		FloatEvents,
		FGameplayTagContainer(),
		{ FFloatAttributeModification::StaticStruct() },
		{ AttributeOwner.Get() },
		true,
		false);
	
	if (FloatAttributeChangedEventTask)
	{
		FloatAttributeChangedEventTask->OnEventReceived.AddDynamic(this, &UWaitForFloatAttributeChange::OnFloatAttributeChangedEvent);
		FloatAttributeChangedEventTask->Activate();
	}
}

void UWaitForFloatAttributeChange::OnFloatAttributeChangedEvent(FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload, UObject* Sender, FGuid EventSubscriptionID)
{
	const FFloatAttributeModification AttributeModification = Payload.Get<FFloatAttributeModification>();

	if (AttributeModification.AttributeTag != AttributeTag)
	{
		return;
	}

	OnFloatAttributeChanged.Broadcast(AttributeModification.ValueType, AttributeModification.NewValue);
	
	if (OnlyTriggerOnce)
	{
		SetReadyToDestroy();
	}
}

void UWaitForFloatAttributeChange::SetReadyToDestroy()
{
	// Clean up event tasks
	if (FloatAttributeChangedEventTask)
	{
		FloatAttributeChangedEventTask->OnEventReceived.RemoveDynamic(this, &UWaitForFloatAttributeChange::OnFloatAttributeChangedEvent);
	}
	
	Super::SetReadyToDestroy();
}