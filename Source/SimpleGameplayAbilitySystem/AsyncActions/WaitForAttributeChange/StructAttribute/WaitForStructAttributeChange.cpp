#include "WaitForStructAttributeChange.h"

#include "SimpleGameplayAbilitySystem/DefaultTags/DefaultTags.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleAttributeModifier/ModifierActions/ChangeFloatAttributeAction/FloatAttributeActionTypes.h"
#include "SimpleGameplayAbilitySystem/SimpleEventSubsystem/WaitForSimpleEvent/WaitForSimpleEvent.h"
#include "SimpleGameplayAbilitySystem/SimpleGameplayAbilityComponent/SimpleGameplayAbilityComponent.h"

UWaitForStructAttributeChange* UWaitForStructAttributeChange::WaitForStructAttributeChange(
	UObject* WorldContextObject,
	USimpleGameplayAbilityComponent* AttributeOwner,
	const FGameplayTag AttributeTag,
	const bool OnlyTriggerOnce)
{
	UWaitForStructAttributeChange* Node = NewObject<UWaitForStructAttributeChange>();
	Node->WorldContext = WorldContextObject->GetWorld();
	Node->TaskOwner = AttributeOwner;
	Node->AttributeTag = AttributeTag;
	Node->OnlyTriggerOnce = OnlyTriggerOnce;

	return Node;
}

void UWaitForStructAttributeChange::Activate()
{
	if (!TaskOwner.IsValid() || !WorldContext.IsValid())
	{
		SetReadyToDestroy();
		return;
	}
	
	SimpleEventTask = UWaitForSimpleEvent::WaitForSimpleEvent(
		WorldContext.Get(),
		this,
		OnlyTriggerOnce,
		FGameplayTagContainer(FDefaultTags::StructAttributeValueChanged()),
		FGameplayTagContainer(),
		{ FStructAttributeModification::StaticStruct() },
		{ TaskOwner.Get() },
		true,
		false);
	
	if (SimpleEventTask)
	{
		SimpleEventTask->OnEventReceived.AddDynamic(this, &UWaitForStructAttributeChange::OnSimpleEventReceived);
		SimpleEventTask->Activate();
	}
	
	Super::Activate();
}

void UWaitForStructAttributeChange::OnSimpleEventReceived(
	FGameplayTag EventTag,
	const FGameplayTag DomainTag,
	FInstancedStruct Payload,
	UObject* Sender,
	FGuid EventSubscriptionID)
{
	if (!DomainTag.MatchesTagExact(AttributeTag))
	{
		return;
	}

	const FStructAttributeModification Modification = Payload.Get<FStructAttributeModification>();
	OnStructAttributeChanged.Broadcast(Modification.ModificationTags, Modification.NewValue, Modification.OldValue);
	
	if (OnlyTriggerOnce)
	{
		SetReadyToDestroy();
	}
}

void UWaitForStructAttributeChange::SetReadyToDestroy()
{
	if (SimpleEventTask)
	{
		SimpleEventTask->OnEventReceived.RemoveDynamic(this, &UWaitForStructAttributeChange::OnSimpleEventReceived);
	}
	
	Super::SetReadyToDestroy();
}


