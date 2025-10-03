#include "WaitForGameplayTag.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeComponent.h"

// ============================================================================
// UWaitForGameplayTagAdded
// ============================================================================

UWaitForGameplayTagAdded* UWaitForGameplayTagAdded::WaitForGameplayTagAdded(
	USimpleAttributeComponent* AttributeComponent,
	FGameplayTag GameplayTag,
	bool OnlyTriggerOnce)
{
	UWaitForGameplayTagAdded* AsyncAction = NewObject<UWaitForGameplayTagAdded>();
	AsyncAction->AttributeComponentPtr = AttributeComponent;
	AsyncAction->TargetTag = GameplayTag;
	AsyncAction->bOnlyTriggerOnce = OnlyTriggerOnce;
	return AsyncAction;
}

void UWaitForGameplayTagAdded::Activate()
{
	if (!AttributeComponentPtr.IsValid())
	{
		SetReadyToDestroy();
		return;
	}

	AttributeComponentPtr->OnGameplayTagAdded.AddDynamic(this, &UWaitForGameplayTagAdded::OnGameplayTagAddedEvent);
}

void UWaitForGameplayTagAdded::OnGameplayTagAddedEvent(FGameplayTag Tag)
{
	if (!Tag.MatchesTagExact(TargetTag))
	{
		return;
	}

	OnTagAdded.Broadcast(Tag);

	if (bOnlyTriggerOnce)
	{
		SetReadyToDestroy();
	}
}

void UWaitForGameplayTagAdded::SetReadyToDestroy()
{
	if (AttributeComponentPtr.IsValid())
	{
		AttributeComponentPtr->OnGameplayTagAdded.RemoveDynamic(this, &UWaitForGameplayTagAdded::OnGameplayTagAddedEvent);
	}

	Super::SetReadyToDestroy();
}

// ============================================================================
// UWaitForGameplayTagRemoved
// ============================================================================

UWaitForGameplayTagRemoved* UWaitForGameplayTagRemoved::WaitForGameplayTagRemoved(
	USimpleAttributeComponent* AttributeComponent,
	FGameplayTag GameplayTag,
	bool OnlyTriggerOnce)
{
	UWaitForGameplayTagRemoved* AsyncAction = NewObject<UWaitForGameplayTagRemoved>();
	AsyncAction->AttributeComponentPtr = AttributeComponent;
	AsyncAction->TargetTag = GameplayTag;
	AsyncAction->bOnlyTriggerOnce = OnlyTriggerOnce;
	return AsyncAction;
}

void UWaitForGameplayTagRemoved::Activate()
{
	if (!AttributeComponentPtr.IsValid())
	{
		SetReadyToDestroy();
		return;
	}

	AttributeComponentPtr->OnGameplayTagRemoved.AddDynamic(this, &UWaitForGameplayTagRemoved::OnGameplayTagRemovedEvent);
}

void UWaitForGameplayTagRemoved::OnGameplayTagRemovedEvent(FGameplayTag Tag)
{
	if (!Tag.MatchesTagExact(TargetTag))
	{
		return;
	}

	OnTagRemoved.Broadcast(Tag);

	if (bOnlyTriggerOnce)
	{
		SetReadyToDestroy();
	}
}

void UWaitForGameplayTagRemoved::SetReadyToDestroy()
{
	if (AttributeComponentPtr.IsValid())
	{
		AttributeComponentPtr->OnGameplayTagRemoved.RemoveDynamic(this, &UWaitForGameplayTagRemoved::OnGameplayTagRemovedEvent);
	}

	Super::SetReadyToDestroy();
}


