// Copyright 2025 Ahmed Elgoni

#include "WaitForFloatAttributeChange.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeComponent.h"

// ============================================================================
// UWaitForFloatAttributeBaseValueChanged
// ============================================================================

UWaitForFloatAttributeBaseValueChanged* UWaitForFloatAttributeBaseValueChanged::WaitForFloatAttributeBaseValueChanged(
	USimpleAttributeComponent* AttributeComponent,
	FGameplayTag AttributeTag,
	bool OnlyTriggerOnce)
{
	UWaitForFloatAttributeBaseValueChanged* AsyncAction = NewObject<UWaitForFloatAttributeBaseValueChanged>();
	AsyncAction->AttributeComponentPtr = AttributeComponent;
	AsyncAction->TargetAttributeTag = AttributeTag;
	AsyncAction->bOnlyTriggerOnce = OnlyTriggerOnce;
	return AsyncAction;
}

void UWaitForFloatAttributeBaseValueChanged::Activate()
{
	if (!AttributeComponentPtr.IsValid())
	{
		SetReadyToDestroy();
		return;
	}

	AttributeComponentPtr->OnFloatAttributeBaseValueChanged.AddDynamic(this, &UWaitForFloatAttributeBaseValueChanged::OnAttributeValueChanged);
}

void UWaitForFloatAttributeBaseValueChanged::OnAttributeValueChanged(FGameplayTag AttributeTag, float OldValue, float NewValue)
{
	if (!AttributeTag.MatchesTagExact(TargetAttributeTag))
	{
		return;
	}

	OnValueChanged.Broadcast(AttributeTag, OldValue, NewValue);

	if (bOnlyTriggerOnce)
	{
		SetReadyToDestroy();
	}
}

void UWaitForFloatAttributeBaseValueChanged::SetReadyToDestroy()
{
	if (AttributeComponentPtr.IsValid())
	{
		AttributeComponentPtr->OnFloatAttributeBaseValueChanged.RemoveDynamic(this, &UWaitForFloatAttributeBaseValueChanged::OnAttributeValueChanged);
	}

	Super::SetReadyToDestroy();
}

// ============================================================================
// UWaitForFloatAttributeCurrentValueChanged
// ============================================================================

UWaitForFloatAttributeCurrentValueChanged* UWaitForFloatAttributeCurrentValueChanged::WaitForFloatAttributeCurrentValueChanged(
	USimpleAttributeComponent* AttributeComponent,
	FGameplayTag AttributeTag,
	bool OnlyTriggerOnce)
{
	UWaitForFloatAttributeCurrentValueChanged* AsyncAction = NewObject<UWaitForFloatAttributeCurrentValueChanged>();
	AsyncAction->AttributeComponentPtr = AttributeComponent;
	AsyncAction->TargetAttributeTag = AttributeTag;
	AsyncAction->bOnlyTriggerOnce = OnlyTriggerOnce;
	return AsyncAction;
}

void UWaitForFloatAttributeCurrentValueChanged::Activate()
{
	if (!AttributeComponentPtr.IsValid())
	{
		SetReadyToDestroy();
		return;
	}

	AttributeComponentPtr->OnFloatAttributeCurrentValueChanged.AddDynamic(this, &UWaitForFloatAttributeCurrentValueChanged::OnAttributeValueChanged);
}

void UWaitForFloatAttributeCurrentValueChanged::OnAttributeValueChanged(FGameplayTag AttributeTag, float OldValue, float NewValue)
{
	if (!AttributeTag.MatchesTagExact(TargetAttributeTag))
	{
		return;
	}

	OnValueChanged.Broadcast(AttributeTag, OldValue, NewValue);

	if (bOnlyTriggerOnce)
	{
		SetReadyToDestroy();
	}
}

void UWaitForFloatAttributeCurrentValueChanged::SetReadyToDestroy()
{
	if (AttributeComponentPtr.IsValid())
	{
		AttributeComponentPtr->OnFloatAttributeCurrentValueChanged.RemoveDynamic(this, &UWaitForFloatAttributeCurrentValueChanged::OnAttributeValueChanged);
	}

	Super::SetReadyToDestroy();
}

// ============================================================================
// UWaitForFloatAttributeMinBaseValueChanged
// ============================================================================

UWaitForFloatAttributeMinBaseValueChanged* UWaitForFloatAttributeMinBaseValueChanged::WaitForFloatAttributeMinBaseValueChanged(
	USimpleAttributeComponent* AttributeComponent,
	FGameplayTag AttributeTag,
	bool OnlyTriggerOnce)
{
	UWaitForFloatAttributeMinBaseValueChanged* AsyncAction = NewObject<UWaitForFloatAttributeMinBaseValueChanged>();
	AsyncAction->AttributeComponentPtr = AttributeComponent;
	AsyncAction->TargetAttributeTag = AttributeTag;
	AsyncAction->bOnlyTriggerOnce = OnlyTriggerOnce;
	return AsyncAction;
}

void UWaitForFloatAttributeMinBaseValueChanged::Activate()
{
	if (!AttributeComponentPtr.IsValid())
	{
		SetReadyToDestroy();
		return;
	}

	AttributeComponentPtr->OnFloatAttributeMinBaseValueChanged.AddDynamic(this, &UWaitForFloatAttributeMinBaseValueChanged::OnAttributeValueChanged);
}

void UWaitForFloatAttributeMinBaseValueChanged::OnAttributeValueChanged(FGameplayTag AttributeTag, float OldValue, float NewValue)
{
	if (!AttributeTag.MatchesTagExact(TargetAttributeTag))
	{
		return;
	}

	OnValueChanged.Broadcast(AttributeTag, OldValue, NewValue);

	if (bOnlyTriggerOnce)
	{
		SetReadyToDestroy();
	}
}

void UWaitForFloatAttributeMinBaseValueChanged::SetReadyToDestroy()
{
	if (AttributeComponentPtr.IsValid())
	{
		AttributeComponentPtr->OnFloatAttributeMinBaseValueChanged.RemoveDynamic(this, &UWaitForFloatAttributeMinBaseValueChanged::OnAttributeValueChanged);
	}

	Super::SetReadyToDestroy();
}

// ============================================================================
// UWaitForFloatAttributeMinCurrentValueChanged
// ============================================================================

UWaitForFloatAttributeMinCurrentValueChanged* UWaitForFloatAttributeMinCurrentValueChanged::WaitForFloatAttributeMinCurrentValueChanged(
	USimpleAttributeComponent* AttributeComponent,
	FGameplayTag AttributeTag,
	bool OnlyTriggerOnce)
{
	UWaitForFloatAttributeMinCurrentValueChanged* AsyncAction = NewObject<UWaitForFloatAttributeMinCurrentValueChanged>();
	AsyncAction->AttributeComponentPtr = AttributeComponent;
	AsyncAction->TargetAttributeTag = AttributeTag;
	AsyncAction->bOnlyTriggerOnce = OnlyTriggerOnce;
	return AsyncAction;
}

void UWaitForFloatAttributeMinCurrentValueChanged::Activate()
{
	if (!AttributeComponentPtr.IsValid())
	{
		SetReadyToDestroy();
		return;
	}

	AttributeComponentPtr->OnFloatAttributeMinCurrentValueChanged.AddDynamic(this, &UWaitForFloatAttributeMinCurrentValueChanged::OnAttributeValueChanged);
}

void UWaitForFloatAttributeMinCurrentValueChanged::OnAttributeValueChanged(FGameplayTag AttributeTag, float OldValue, float NewValue)
{
	if (!AttributeTag.MatchesTagExact(TargetAttributeTag))
	{
		return;
	}

	OnValueChanged.Broadcast(AttributeTag, OldValue, NewValue);

	if (bOnlyTriggerOnce)
	{
		SetReadyToDestroy();
	}
}

void UWaitForFloatAttributeMinCurrentValueChanged::SetReadyToDestroy()
{
	if (AttributeComponentPtr.IsValid())
	{
		AttributeComponentPtr->OnFloatAttributeMinCurrentValueChanged.RemoveDynamic(this, &UWaitForFloatAttributeMinCurrentValueChanged::OnAttributeValueChanged);
	}

	Super::SetReadyToDestroy();
}

// ============================================================================
// UWaitForFloatAttributeMaxBaseValueChanged
// ============================================================================

UWaitForFloatAttributeMaxBaseValueChanged* UWaitForFloatAttributeMaxBaseValueChanged::WaitForFloatAttributeMaxBaseValueChanged(
	USimpleAttributeComponent* AttributeComponent,
	FGameplayTag AttributeTag,
	bool OnlyTriggerOnce)
{
	UWaitForFloatAttributeMaxBaseValueChanged* AsyncAction = NewObject<UWaitForFloatAttributeMaxBaseValueChanged>();
	AsyncAction->AttributeComponentPtr = AttributeComponent;
	AsyncAction->TargetAttributeTag = AttributeTag;
	AsyncAction->bOnlyTriggerOnce = OnlyTriggerOnce;
	return AsyncAction;
}

void UWaitForFloatAttributeMaxBaseValueChanged::Activate()
{
	if (!AttributeComponentPtr.IsValid())
	{
		SetReadyToDestroy();
		return;
	}

	AttributeComponentPtr->OnFloatAttributeMaxBaseValueChanged.AddDynamic(this, &UWaitForFloatAttributeMaxBaseValueChanged::OnAttributeValueChanged);
}

void UWaitForFloatAttributeMaxBaseValueChanged::OnAttributeValueChanged(FGameplayTag AttributeTag, float OldValue, float NewValue)
{
	if (!AttributeTag.MatchesTagExact(TargetAttributeTag))
	{
		return;
	}

	OnValueChanged.Broadcast(AttributeTag, OldValue, NewValue);

	if (bOnlyTriggerOnce)
	{
		SetReadyToDestroy();
	}
}

void UWaitForFloatAttributeMaxBaseValueChanged::SetReadyToDestroy()
{
	if (AttributeComponentPtr.IsValid())
	{
		AttributeComponentPtr->OnFloatAttributeMaxBaseValueChanged.RemoveDynamic(this, &UWaitForFloatAttributeMaxBaseValueChanged::OnAttributeValueChanged);
	}

	Super::SetReadyToDestroy();
}

// ============================================================================
// UWaitForFloatAttributeMaxCurrentValueChanged
// ============================================================================

UWaitForFloatAttributeMaxCurrentValueChanged* UWaitForFloatAttributeMaxCurrentValueChanged::WaitForFloatAttributeMaxCurrentValueChanged(
	USimpleAttributeComponent* AttributeComponent,
	FGameplayTag AttributeTag,
	bool OnlyTriggerOnce)
{
	UWaitForFloatAttributeMaxCurrentValueChanged* AsyncAction = NewObject<UWaitForFloatAttributeMaxCurrentValueChanged>();
	AsyncAction->AttributeComponentPtr = AttributeComponent;
	AsyncAction->TargetAttributeTag = AttributeTag;
	AsyncAction->bOnlyTriggerOnce = OnlyTriggerOnce;
	return AsyncAction;
}

void UWaitForFloatAttributeMaxCurrentValueChanged::Activate()
{
	if (!AttributeComponentPtr.IsValid())
	{
		SetReadyToDestroy();
		return;
	}

	AttributeComponentPtr->OnFloatAttributeMaxCurrentValueChanged.AddDynamic(this, &UWaitForFloatAttributeMaxCurrentValueChanged::OnAttributeValueChanged);
}

void UWaitForFloatAttributeMaxCurrentValueChanged::OnAttributeValueChanged(FGameplayTag AttributeTag, float OldValue, float NewValue)
{
	if (!AttributeTag.MatchesTagExact(TargetAttributeTag))
	{
		return;
	}

	OnValueChanged.Broadcast(AttributeTag, OldValue, NewValue);

	if (bOnlyTriggerOnce)
	{
		SetReadyToDestroy();
	}
}

void UWaitForFloatAttributeMaxCurrentValueChanged::SetReadyToDestroy()
{
	if (AttributeComponentPtr.IsValid())
	{
		AttributeComponentPtr->OnFloatAttributeMaxCurrentValueChanged.RemoveDynamic(this, &UWaitForFloatAttributeMaxCurrentValueChanged::OnAttributeValueChanged);
	}

	Super::SetReadyToDestroy();
}

// ============================================================================
// UWaitForMinBaseValueReached
// ============================================================================

UWaitForMinBaseValueReached* UWaitForMinBaseValueReached::WaitForMinBaseValueReached(
	USimpleAttributeComponent* AttributeComponent,
	FGameplayTag AttributeTag,
	bool OnlyTriggerOnce)
{
	UWaitForMinBaseValueReached* AsyncAction = NewObject<UWaitForMinBaseValueReached>();
	AsyncAction->AttributeComponentPtr = AttributeComponent;
	AsyncAction->TargetAttributeTag = AttributeTag;
	AsyncAction->bOnlyTriggerOnce = OnlyTriggerOnce;
	return AsyncAction;
}

void UWaitForMinBaseValueReached::Activate()
{
	if (!AttributeComponentPtr.IsValid())
	{
		SetReadyToDestroy();
		return;
	}

	AttributeComponentPtr->OnMinBaseValueReached.AddDynamic(this, &UWaitForMinBaseValueReached::OnLimitReachedEvent);
}

void UWaitForMinBaseValueReached::OnLimitReachedEvent(FGameplayTag AttributeTag, float Overflow)
{
	if (!AttributeTag.MatchesTagExact(TargetAttributeTag))
	{
		return;
	}

	OnLimitReached.Broadcast(AttributeTag, Overflow);

	if (bOnlyTriggerOnce)
	{
		SetReadyToDestroy();
	}
}

void UWaitForMinBaseValueReached::SetReadyToDestroy()
{
	if (AttributeComponentPtr.IsValid())
	{
		AttributeComponentPtr->OnMinBaseValueReached.RemoveDynamic(this, &UWaitForMinBaseValueReached::OnLimitReachedEvent);
	}

	Super::SetReadyToDestroy();
}

// ============================================================================
// UWaitForMinCurrentValueReached
// ============================================================================

UWaitForMinCurrentValueReached* UWaitForMinCurrentValueReached::WaitForMinCurrentValueReached(
	USimpleAttributeComponent* AttributeComponent,
	FGameplayTag AttributeTag,
	bool OnlyTriggerOnce)
{
	UWaitForMinCurrentValueReached* AsyncAction = NewObject<UWaitForMinCurrentValueReached>();
	AsyncAction->AttributeComponentPtr = AttributeComponent;
	AsyncAction->TargetAttributeTag = AttributeTag;
	AsyncAction->bOnlyTriggerOnce = OnlyTriggerOnce;
	return AsyncAction;
}

void UWaitForMinCurrentValueReached::Activate()
{
	if (!AttributeComponentPtr.IsValid())
	{
		SetReadyToDestroy();
		return;
	}

	AttributeComponentPtr->OnMinCurrentValueReached.AddDynamic(this, &UWaitForMinCurrentValueReached::OnLimitReachedEvent);
}

void UWaitForMinCurrentValueReached::OnLimitReachedEvent(FGameplayTag AttributeTag, float Overflow)
{
	if (!AttributeTag.MatchesTagExact(TargetAttributeTag))
	{
		return;
	}

	OnLimitReached.Broadcast(AttributeTag, Overflow);

	if (bOnlyTriggerOnce)
	{
		SetReadyToDestroy();
	}
}

void UWaitForMinCurrentValueReached::SetReadyToDestroy()
{
	if (AttributeComponentPtr.IsValid())
	{
		AttributeComponentPtr->OnMinCurrentValueReached.RemoveDynamic(this, &UWaitForMinCurrentValueReached::OnLimitReachedEvent);
	}

	Super::SetReadyToDestroy();
}

// ============================================================================
// UWaitForMaxBaseValueReached
// ============================================================================

UWaitForMaxBaseValueReached* UWaitForMaxBaseValueReached::WaitForMaxBaseValueReached(
	USimpleAttributeComponent* AttributeComponent,
	FGameplayTag AttributeTag,
	bool OnlyTriggerOnce)
{
	UWaitForMaxBaseValueReached* AsyncAction = NewObject<UWaitForMaxBaseValueReached>();
	AsyncAction->AttributeComponentPtr = AttributeComponent;
	AsyncAction->TargetAttributeTag = AttributeTag;
	AsyncAction->bOnlyTriggerOnce = OnlyTriggerOnce;
	return AsyncAction;
}

void UWaitForMaxBaseValueReached::Activate()
{
	if (!AttributeComponentPtr.IsValid())
	{
		SetReadyToDestroy();
		return;
	}

	AttributeComponentPtr->OnMaxBaseValueReached.AddDynamic(this, &UWaitForMaxBaseValueReached::OnLimitReachedEvent);
}

void UWaitForMaxBaseValueReached::OnLimitReachedEvent(FGameplayTag AttributeTag, float Overflow)
{
	if (!AttributeTag.MatchesTagExact(TargetAttributeTag))
	{
		return;
	}

	OnLimitReached.Broadcast(AttributeTag, Overflow);

	if (bOnlyTriggerOnce)
	{
		SetReadyToDestroy();
	}
}

void UWaitForMaxBaseValueReached::SetReadyToDestroy()
{
	if (AttributeComponentPtr.IsValid())
	{
		AttributeComponentPtr->OnMaxBaseValueReached.RemoveDynamic(this, &UWaitForMaxBaseValueReached::OnLimitReachedEvent);
	}

	Super::SetReadyToDestroy();
}

// ============================================================================
// UWaitForMaxCurrentValueReached
// ============================================================================

UWaitForMaxCurrentValueReached* UWaitForMaxCurrentValueReached::WaitForMaxCurrentValueReached(
	USimpleAttributeComponent* AttributeComponent,
	FGameplayTag AttributeTag,
	bool OnlyTriggerOnce)
{
	UWaitForMaxCurrentValueReached* AsyncAction = NewObject<UWaitForMaxCurrentValueReached>();
	AsyncAction->AttributeComponentPtr = AttributeComponent;
	AsyncAction->TargetAttributeTag = AttributeTag;
	AsyncAction->bOnlyTriggerOnce = OnlyTriggerOnce;
	return AsyncAction;
}

void UWaitForMaxCurrentValueReached::Activate()
{
	if (!AttributeComponentPtr.IsValid())
	{
		SetReadyToDestroy();
		return;
	}

	AttributeComponentPtr->OnMaxCurrentValueReached.AddDynamic(this, &UWaitForMaxCurrentValueReached::OnLimitReachedEvent);
}

void UWaitForMaxCurrentValueReached::OnLimitReachedEvent(FGameplayTag AttributeTag, float Overflow)
{
	if (!AttributeTag.MatchesTagExact(TargetAttributeTag))
	{
		return;
	}

	OnLimitReached.Broadcast(AttributeTag, Overflow);

	if (bOnlyTriggerOnce)
	{
		SetReadyToDestroy();
	}
}

void UWaitForMaxCurrentValueReached::SetReadyToDestroy()
{
	if (AttributeComponentPtr.IsValid())
	{
		AttributeComponentPtr->OnMaxCurrentValueReached.RemoveDynamic(this, &UWaitForMaxCurrentValueReached::OnLimitReachedEvent);
	}

	Super::SetReadyToDestroy();
}