#include "WaitForStructAttributeChange.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeComponent.h"

UWaitForStructAttributeChange* UWaitForStructAttributeChange::WaitForStructAttributeChange(
	USimpleAttributeComponent* AttributeComponent,
	FGameplayTag AttributeTag,
	FGameplayTagContainer ModificationEventFilter,
	bool OnlyTriggerOnce)
{
	UWaitForStructAttributeChange* AsyncAction = NewObject<UWaitForStructAttributeChange>();
	AsyncAction->AttributeComponentPtr = AttributeComponent;
	AsyncAction->TargetAttributeTag = AttributeTag;
	AsyncAction->ModificationEventFilter = ModificationEventFilter;
	AsyncAction->bOnlyTriggerOnce = OnlyTriggerOnce;
	return AsyncAction;
}

void UWaitForStructAttributeChange::Activate()
{
	if (!AttributeComponentPtr.IsValid())
	{
		SetReadyToDestroy();
		return;
	}

	AttributeComponentPtr->OnStructAttributeChanged.AddDynamic(this, &UWaitForStructAttributeChange::OnStructAttributeChangedEvent);
}

void UWaitForStructAttributeChange::OnStructAttributeChangedEvent(FGameplayTag AttributeTag, FInstancedStruct OldValue, FInstancedStruct NewValue, FGameplayTagContainer ModificationTags)
{
	if (!AttributeTag.MatchesTagExact(TargetAttributeTag))
	{
		return;
	}

	// If modification event filter is specified and not empty, check if any of the modification tags match
	if (!ModificationEventFilter.IsEmpty() && !ModificationTags.HasAny(ModificationEventFilter))
	{
		return;
	}

	OnStructAttributeChanged.Broadcast(ModificationTags, NewValue, OldValue);

	if (bOnlyTriggerOnce)
	{
		SetReadyToDestroy();
	}
}

void UWaitForStructAttributeChange::SetReadyToDestroy()
{
	if (AttributeComponentPtr.IsValid())
	{
		AttributeComponentPtr->OnStructAttributeChanged.RemoveDynamic(this, &UWaitForStructAttributeChange::OnStructAttributeChangedEvent);
	}

	Super::SetReadyToDestroy();
}
