#pragma once

#include "CoreMinimal.h"
#include "SimpleGameplayAbilityComponent.h"
#include "SimpleGameplayAbilitySystem/DefaultTags/DefaultTags.h"
#include "SimpleGameplayAbilitySystem/Module/SimpleGameplayAbilitySystem.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleAttributeModifier/SimpleAttributeModifier.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleAttributeModifier/ModifierActions/ChangeFloatAttributeAction/FloatAttributeActionTypes.h"
#include "SimpleGameplayAbilitySystem/SimpleEventSubsystem/SimpleEventSubSystem.h"

using enum EAbilityStatus;

void USimpleGameplayAbilityComponent::AddFloatAttribute(FFloatAttribute AttributeToAdd, bool OverrideValuesIfExists)
{
	for (FFloatAttribute& AuthorityAttribute : AuthorityFloatAttributes.Attributes)
	{
		if (AuthorityAttribute.AttributeTag.MatchesTagExact(AttributeToAdd.AttributeTag))
		{
			// Attribute exists but we don't want to override it
			if (!OverrideValuesIfExists)
			{
				return;
			}

			// Attribute exists and we want to override it
			AuthorityAttribute = AttributeToAdd;
			AuthorityFloatAttributes.MarkItemDirty(AuthorityAttribute);
			return;
		}
	}
	
	AuthorityFloatAttributes.Attributes.Add(AttributeToAdd);
	AuthorityFloatAttributes.MarkItemDirty(AttributeToAdd);
	SendEvent(FDefaultTags::FloatAttributeAdded(), AttributeToAdd.AttributeTag, FInstancedStruct(), GetOwner(), {}, ESimpleEventReplicationPolicy::NoReplication);
}

void USimpleGameplayAbilityComponent::RemoveFloatAttribute(FGameplayTag AttributeTag)
{
	AuthorityFloatAttributes.Attributes.RemoveAll([AttributeTag](const FFloatAttribute& Attribute) { return Attribute.AttributeTag == AttributeTag; });
	AuthorityFloatAttributes.MarkArrayDirty();
	SendEvent(FDefaultTags::FloatAttributeRemoved(), AttributeTag, FInstancedStruct(), GetOwner(), {}, ESimpleEventReplicationPolicy::NoReplication);
}

void USimpleGameplayAbilityComponent::AddStructAttribute(FStructAttribute AttributeToAdd, bool OverrideValuesIfExists)
{
	if (!AttributeToAdd.StructType)
	{
		SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::AddStructAttribute]: StructType is null for attribute %s! Can't add new attribute"), *AttributeToAdd.AttributeTag.ToString()));
		return;
	}
	
	const int32 AttributeIndex = AuthorityStructAttributes.Attributes.Find(AttributeToAdd);
	
	// This is a new attribute
	if (AttributeIndex == INDEX_NONE)
	{
		// Initialise the data within the struct
		if (AttributeToAdd.StructType)
		{
			AttributeToAdd.AttributeValue.InitializeAs(AttributeToAdd.StructType);
		}
		
		AuthorityStructAttributes.Attributes.AddUnique(AttributeToAdd);
		AuthorityStructAttributes.MarkItemDirty(AttributeToAdd);

		SendEvent(FDefaultTags::StructAttributeAdded(), AttributeToAdd.AttributeTag, AttributeToAdd.AttributeValue, GetOwner(), {}, ESimpleEventReplicationPolicy::NoReplication);
		
		return;
	}

	// Attribute exists but we don't want to override it
	if (!OverrideValuesIfExists)
	{
		return;
	}

	// Attribute exists and we want to override it
	AuthorityStructAttributes.Attributes[AttributeIndex] = AttributeToAdd;
	AuthorityStructAttributes.MarkItemDirty(AuthorityStructAttributes.Attributes[AttributeIndex]);
}

void USimpleGameplayAbilityComponent::RemoveStructAttribute(FGameplayTag AttributeTag)
{
	AuthorityStructAttributes.Attributes.RemoveAll([AttributeTag](const FStructAttribute& Attribute) { return Attribute.AttributeTag == AttributeTag; });
	AuthorityStructAttributes.MarkArrayDirty();
	SendEvent(FDefaultTags::StructAttributeRemoved(), AttributeTag, FInstancedStruct(), GetOwner(), {}, ESimpleEventReplicationPolicy::NoReplication);
}

bool USimpleGameplayAbilityComponent::ApplyAttributeModifierToTarget(
	USimpleGameplayAbilityComponent* ModifierTarget,
	const TSubclassOf<USimpleAttributeModifier> ModifierClass,
	const float Magnitude,
	const FAbilityContextCollection ModifierContexts,
	FGuid& ModifierID)
{
	if (!ModifierClass)
	{
		SIMPLE_LOG(this, TEXT("[USimpleGameplayAbilityComponent::ApplyAttributeModifierToTarget]: ModifierClass is null!"));
		return false;
	}
	
	ModifierID = FGuid::NewGuid();
	USimpleAttributeModifier* ModifierInstance = nullptr;

	for (USimpleAttributeModifier* InstancedModifier : InstancedAttributeModifiers)
	{
		if (InstancedModifier->GetClass() == ModifierClass)
		{
			ModifierInstance = InstancedModifier;
		}
	}

	if (!ModifierInstance)
	{
		ModifierInstance = NewObject<USimpleAttributeModifier>(this, ModifierClass);
		InstancedAttributeModifiers.Add(ModifierInstance);
	}

	ModifierInstance->InitializeModifier(ModifierTarget, Magnitude);
	return ModifierInstance->Activate(this, ModifierID, ModifierContexts);
}

bool USimpleGameplayAbilityComponent::ApplyAttributeModifierToSelf(
	const TSubclassOf<USimpleAttributeModifier> ModifierClass,
	const float Magnitude,
	const FAbilityContextCollection ModifierContexts, FGuid& ModifierID)
{
	return ApplyAttributeModifierToTarget(this, ModifierClass, Magnitude, ModifierContexts, ModifierID);
}

void USimpleGameplayAbilityComponent::CancelAttributeModifier(const FGuid ModifierID)
{
	for (USimpleAttributeModifier* InstancedModifier : InstancedAttributeModifiers)
	{
		if (InstancedModifier->AbilityID == ModifierID && InstancedModifier->IsActive)
		{
			InstancedModifier->Cancel(FDefaultTags::AbilityCancelled(), FInstancedStruct());
			return;
		}
	}
}

void USimpleGameplayAbilityComponent::CancelAttributeModifiersWithTags(const FGameplayTagContainer Tags)
{
	for (USimpleAttributeModifier* InstancedModifier : InstancedAttributeModifiers)
	{
		if (InstancedModifier->IsActive && InstancedModifier->ModifierTags.HasAnyExact(Tags))
		{
			InstancedModifier->Cancel(FDefaultTags::AbilityCancelled(), FInstancedStruct());
		}
	}
}

bool USimpleGameplayAbilityComponent::HasAttributeModifierWithTags(const FGameplayTagContainer Tags) const
{
	for (const USimpleAttributeModifier* InstancedModifier : InstancedAttributeModifiers)
	{
		if (InstancedModifier->IsActive && InstancedModifier->ModifierTags.HasAnyExact(Tags))
		{
			return true;
		}
	}

	return false;
}

bool USimpleGameplayAbilityComponent::HasFloatAttribute(const FGameplayTag AttributeTag)
{
	if (GetFloatAttribute(AttributeTag))
	{
		return true;
	}

	return false;
}

bool USimpleGameplayAbilityComponent::HasStructAttribute(const FGameplayTag AttributeTag)
{
	if (GetStructAttribute(AttributeTag))
	{
		return true;
	}

	return false;
}

float USimpleGameplayAbilityComponent::GetFloatAttributeValue(EFloatAttributeValueType ValueType, FGameplayTag AttributeTag, bool& WasFound)
{
	if (const FFloatAttribute* Attribute = GetFloatAttribute(AttributeTag))
	{
		WasFound = true;

		switch (ValueType)
		{
			case EFloatAttributeValueType::BaseValue:
				return Attribute->BaseValue;
			case EFloatAttributeValueType::CurrentValue:
				return Attribute->CurrentValue;
			case EFloatAttributeValueType::MaxCurrentValue:
				return Attribute->ValueLimits.MaxCurrentValue;
			case EFloatAttributeValueType::MinCurrentValue:
				return Attribute->ValueLimits.MinCurrentValue;
			case EFloatAttributeValueType::MaxBaseValue:
				return Attribute->ValueLimits.MaxBaseValue;
			case EFloatAttributeValueType::MinBaseValue:
				return Attribute->ValueLimits.MinBaseValue;
			default:
				SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleAttributeFunctionLibrary::GetFloatAttributeValue]: ValueType %d not supported."), static_cast<int32>(ValueType)));
				return 0.0f;
		}
	}
	
	WasFound = false;
	return 0.0f;
}

bool USimpleGameplayAbilityComponent::SetFloatAttributeValue(EFloatAttributeValueType ValueType, FGameplayTag AttributeTag, float NewValue, float& Overflow)
{
	FFloatAttribute* Attribute = GetFloatAttribute(AttributeTag);
	
	if (!Attribute)
	{
		SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleAttributeFunctionLibrary::SetFloatAttributeValue]: Attribute %s not found."), *AttributeTag.ToString()));
		return false;
	}
	
	const float ClampedValue = ClampFloatAttributeValue(*Attribute, ValueType, NewValue, Overflow);
				
	switch (ValueType)
	{
		case EFloatAttributeValueType::BaseValue:
			Attribute->BaseValue = ClampedValue;
			SendFloatAttributeChangedEvent(FDefaultTags::FloatAttributeBaseValueChanged(), AttributeTag, ValueType, ClampedValue);
			break;
		case EFloatAttributeValueType::CurrentValue:
			Attribute->CurrentValue = ClampedValue;
			SendFloatAttributeChangedEvent(FDefaultTags::FloatAttributeCurrentValueChanged(), AttributeTag, ValueType, ClampedValue);
			break;
		case EFloatAttributeValueType::MaxCurrentValue:
			Attribute->ValueLimits.MaxCurrentValue = ClampedValue;
			SendFloatAttributeChangedEvent(FDefaultTags::FloatAttributeMaxCurrentValueChanged(), AttributeTag, ValueType, ClampedValue);
			break;
		case EFloatAttributeValueType::MinCurrentValue:
			Attribute->ValueLimits.MinCurrentValue = ClampedValue;
			SendFloatAttributeChangedEvent(FDefaultTags::FloatAttributeMinCurrentValueChanged(), AttributeTag, ValueType, ClampedValue);
			break;
		case EFloatAttributeValueType::MaxBaseValue:
			Attribute->ValueLimits.MaxBaseValue = ClampedValue;
			SendFloatAttributeChangedEvent(FDefaultTags::FloatAttributeMaxBaseValueChanged(), AttributeTag, ValueType, ClampedValue);
			break;
		case EFloatAttributeValueType::MinBaseValue:
			Attribute->ValueLimits.MinBaseValue = ClampedValue;
			SendFloatAttributeChangedEvent(FDefaultTags::FloatAttributeMinBaseValueChanged(), AttributeTag, ValueType, ClampedValue);
			break;
	}

	if (HasAuthority())
	{
		AuthorityFloatAttributes.MarkItemDirty(*Attribute);
	}
	
	return true;
}

bool USimpleGameplayAbilityComponent::IncrementFloatAttributeValue(EFloatAttributeValueType ValueType, FGameplayTag AttributeTag, float Increment, float& Overflow)
{
	bool WasFound = false;
	const float CurrentValue = GetFloatAttributeValue(ValueType, AttributeTag, WasFound);

	if (!WasFound)
	{
		return false;
	}

	return SetFloatAttributeValue(ValueType, AttributeTag, CurrentValue + Increment, Overflow);
}

bool USimpleGameplayAbilityComponent::OverrideFloatAttribute(FGameplayTag AttributeTag, FFloatAttribute NewAttribute)
{
	for (FFloatAttribute& Attribute : AuthorityFloatAttributes.Attributes)
	{
		if (Attribute.AttributeTag.MatchesTagExact(AttributeTag))
		{
			CompareFloatAttributesAndSendEvents(Attribute, NewAttribute);
			
			Attribute.AttributeName = NewAttribute.AttributeName;
			Attribute.AttributeTag = NewAttribute.AttributeTag;
			Attribute.BaseValue = NewAttribute.BaseValue;
			Attribute.CurrentValue = NewAttribute.CurrentValue;
			Attribute.ValueLimits = NewAttribute.ValueLimits;
			
			AuthorityFloatAttributes.MarkItemDirty(Attribute);
			
			return true;
		}
	}

	SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleAttributeFunctionLibrary::OverrideFloatAttribute]: Attribute %s not found."), *AttributeTag.ToString()));
	return false;
}

USimpleAttributeHandler* USimpleGameplayAbilityComponent::GetStructAttributeHandlerInstance(TSubclassOf<USimpleAttributeHandler> HandlerClass)
{
	for (USimpleAttributeHandler* InstancedHandler : InstancedAttributeHandlers)
	{
		if (InstancedHandler->GetClass() == HandlerClass)
		{
			return InstancedHandler;
		}
	}

	USimpleAttributeHandler* NewHandlerInstance = NewObject<USimpleAttributeHandler>(this, HandlerClass);
	NewHandlerInstance->AttributeOwner = this;
	InstancedAttributeHandlers.Add(NewHandlerInstance);
	
	return NewHandlerInstance;
}

FInstancedStruct USimpleGameplayAbilityComponent::GetStructAttributeValue(FGameplayTag AttributeTag, bool& WasFound)
{
	if (FStructAttribute* Attribute = GetStructAttribute(AttributeTag))
	{
		WasFound = true;
		return Attribute->AttributeValue;
	}

	SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleAttributeFunctionLibrary::GetStructAttributeValue]: Attribute %s not found."), *AttributeTag.ToString()));
	WasFound = false;
	return FInstancedStruct();
}

bool USimpleGameplayAbilityComponent::SetStructAttributeValue(const FGameplayTag AttributeTag, const FInstancedStruct NewValue)
{
	FStructAttribute* Attribute = GetStructAttribute(AttributeTag);
	
	if (!Attribute)
	{
		SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleAttributeFunctionLibrary::SetStructAttributeValue]: Attribute %s not found."), *AttributeTag.ToString()));
		return false;
	}

	FStructAttributeModification Payload;
	Payload.AttributeOwner = this;
	Payload.AttributeTag = AttributeTag;
	Payload.OldValue = Attribute->AttributeValue;
	Payload.NewValue = NewValue;

	if (Attribute->StructAttributeHandler)
	{
		Payload.ModificationTags = GetStructAttributeHandlerInstance(Attribute->StructAttributeHandler)->GetModificationEvents(AttributeTag, Payload.OldValue, Payload.NewValue);
	}
	
	Attribute->AttributeValue = NewValue;
	
	if (HasAuthority())
	{
		AuthorityStructAttributes.MarkItemDirty(*Attribute);
	}
	
	SendEvent(FDefaultTags::StructAttributeValueChanged(), AttributeTag, FInstancedStruct::Make(Payload), this, { }, ESimpleEventReplicationPolicy::NoReplication);

	Attribute->OnValueChanged.ExecuteIfBound();
	
	return true;
}

float USimpleGameplayAbilityComponent::ClampFloatAttributeValue(
	const FFloatAttribute& Attribute,
	EFloatAttributeValueType ValueType,
	float NewValue,
	float& Overflow)
{
	switch (ValueType)
	{
		case EFloatAttributeValueType::BaseValue:
			if (Attribute.ValueLimits.UseMaxBaseValue && NewValue > Attribute.ValueLimits.MaxBaseValue)
			{
				Overflow = NewValue - Attribute.ValueLimits.MaxBaseValue;
				return Attribute.ValueLimits.MaxBaseValue;
			}

			if (Attribute.ValueLimits.UseMinBaseValue && NewValue < Attribute.ValueLimits.MinBaseValue)
			{
				Overflow = Attribute.ValueLimits.MinBaseValue + NewValue;
				return Attribute.ValueLimits.MinBaseValue;
			}

			return NewValue;
				
		case EFloatAttributeValueType::CurrentValue:
			if (Attribute.ValueLimits.UseMaxCurrentValue && NewValue > Attribute.ValueLimits.MaxCurrentValue)
			{
				Overflow = NewValue - Attribute.ValueLimits.MaxCurrentValue;
				return Attribute.ValueLimits.MaxCurrentValue;
			}

			if (Attribute.ValueLimits.UseMinCurrentValue && NewValue < Attribute.ValueLimits.MinCurrentValue)
			{
				Overflow = Attribute.ValueLimits.MinCurrentValue + NewValue;
				return Attribute.ValueLimits.MinCurrentValue;
			}

			return NewValue;
				
		default:
			SIMPLE_LOG(this, TEXT("[USimpleGameplayAbilityComponent::ClampFloatAttributeValue]: ValueType not supported."));
			return 0.0f;
	}
}

void USimpleGameplayAbilityComponent::CompareFloatAttributesAndSendEvents(const FFloatAttribute& OldAttribute, const FFloatAttribute& NewAttribute)
{
	if (OldAttribute.BaseValue != NewAttribute.BaseValue)
	{
		SendFloatAttributeChangedEvent(FDefaultTags::FloatAttributeBaseValueChanged(), NewAttribute.AttributeTag, EFloatAttributeValueType::BaseValue, NewAttribute.BaseValue);
	}

	if (NewAttribute.ValueLimits.UseMaxBaseValue && OldAttribute.ValueLimits.MaxBaseValue != NewAttribute.ValueLimits.MaxBaseValue)
	{
		SendFloatAttributeChangedEvent(FDefaultTags::FloatAttributeMaxBaseValueChanged(), NewAttribute.AttributeTag, EFloatAttributeValueType::MaxBaseValue, NewAttribute.ValueLimits.MaxBaseValue);
	}

	if (NewAttribute.ValueLimits.UseMinBaseValue && OldAttribute.ValueLimits.MinBaseValue != NewAttribute.ValueLimits.MinBaseValue)
	{
		SendFloatAttributeChangedEvent(FDefaultTags::FloatAttributeMinBaseValueChanged(), NewAttribute.AttributeTag, EFloatAttributeValueType::MinBaseValue, NewAttribute.ValueLimits.MinBaseValue);
	}
	
	if (OldAttribute.CurrentValue != NewAttribute.CurrentValue)
	{
		SendFloatAttributeChangedEvent(FDefaultTags::FloatAttributeCurrentValueChanged(), NewAttribute.AttributeTag, EFloatAttributeValueType::CurrentValue, NewAttribute.CurrentValue);
	}
	
	if (NewAttribute.ValueLimits.UseMaxCurrentValue && OldAttribute.ValueLimits.MaxCurrentValue != NewAttribute.ValueLimits.MaxCurrentValue)
	{
		SendFloatAttributeChangedEvent(FDefaultTags::FloatAttributeMaxCurrentValueChanged(), NewAttribute.AttributeTag, EFloatAttributeValueType::MaxCurrentValue, NewAttribute.ValueLimits.MaxCurrentValue);
	}

	if (NewAttribute.ValueLimits.UseMinCurrentValue && OldAttribute.ValueLimits.MinCurrentValue != NewAttribute.ValueLimits.MinCurrentValue)
	{
		SendFloatAttributeChangedEvent(FDefaultTags::FloatAttributeMinCurrentValueChanged(), NewAttribute.AttributeTag, EFloatAttributeValueType::MinCurrentValue, NewAttribute.ValueLimits.MinCurrentValue);
	}
}

void USimpleGameplayAbilityComponent::SendFloatAttributeChangedEvent(FGameplayTag EventTag, FGameplayTag AttributeTag, EFloatAttributeValueType ValueType, float NewValue)
{
	if (USimpleEventSubsystem* EventSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<USimpleEventSubsystem>())
	{
		FFloatAttributeModification Payload;
		Payload.AttributeOwner = this;
		Payload.AttributeTag = AttributeTag;
		Payload.ValueType = ValueType;
		Payload.NewValue = NewValue;

		const FInstancedStruct EventPayload = FInstancedStruct::Make(Payload);
		const FGameplayTag DomainTag = HasAuthority() ? FDefaultTags::AuthorityAttributeDomain() : FDefaultTags::LocalAttributeDomain();
		
		EventSubsystem->SendEvent(EventTag, DomainTag, EventPayload, this, {});
	}
	else
	{
		SIMPLE_LOG(this, TEXT("[USimpleGameplayAbilityComponent::SendFloatAttributeChangedEvent]: No SimpleEventSubsystem found."));
	}
}

FFloatAttribute* USimpleGameplayAbilityComponent::GetFloatAttribute(FGameplayTag AttributeTag)
{
	if (HasAuthority())
	{
		for (FFloatAttribute& FloatAttribute : AuthorityFloatAttributes.Attributes)
		{
			if (FloatAttribute.AttributeTag.MatchesTagExact(AttributeTag))
			{
				return &FloatAttribute;
			}
		}
	}
	else
	{
		for (FFloatAttribute& FloatAttribute : LocalFloatAttributes)
		{
			if (FloatAttribute.AttributeTag.MatchesTagExact(AttributeTag))
			{
				return &FloatAttribute;
			}
		}
	}

	return nullptr;
}

FStructAttribute* USimpleGameplayAbilityComponent::GetStructAttribute(FGameplayTag AttributeTag)
{
	if (HasAuthority())
	{
		for (FStructAttribute& StructAttribute : AuthorityStructAttributes.Attributes)
		{
			if (StructAttribute.AttributeTag.MatchesTagExact(AttributeTag))
			{
				return &StructAttribute;
			}
		}
	}
	else
	{
		for (FStructAttribute& StructAttribute : LocalStructAttributes)
		{
			if (StructAttribute.AttributeTag.MatchesTagExact(AttributeTag))
			{
				return &StructAttribute;
			}
		}
	}

	return nullptr;
}

void USimpleGameplayAbilityComponent::OnFloatAttributeAdded(const FFloatAttribute& NewFloatAttribute)
{
	LocalFloatAttributes.AddUnique(NewFloatAttribute);
	SendEvent(FDefaultTags::FloatAttributeAdded(), NewFloatAttribute.AttributeTag, FInstancedStruct(), GetOwner(), {}, ESimpleEventReplicationPolicy::NoReplication);
}

void USimpleGameplayAbilityComponent::OnFloatAttributeChanged(const FFloatAttribute& ChangedFloatAttribute)
{
	for (FFloatAttribute& LocalFloatAttribute : LocalFloatAttributes)
	{
		if (LocalFloatAttribute.AttributeTag.MatchesTagExact(ChangedFloatAttribute.AttributeTag))
		{
			CompareFloatAttributesAndSendEvents(LocalFloatAttribute, ChangedFloatAttribute);
			LocalFloatAttribute = ChangedFloatAttribute;
			return;
		}
	}

	LocalFloatAttributes.AddUnique(ChangedFloatAttribute);
	SendEvent(FDefaultTags::FloatAttributeAdded(), ChangedFloatAttribute.AttributeTag, FInstancedStruct(), GetOwner(), {}, ESimpleEventReplicationPolicy::NoReplication);
}

void USimpleGameplayAbilityComponent::OnFloatAttributeRemoved(const FFloatAttribute& RemovedFloatAttribute)
{
	LocalFloatAttributes.Remove(RemovedFloatAttribute);
	SendEvent(FDefaultTags::FloatAttributeRemoved(), RemovedFloatAttribute.AttributeTag, FInstancedStruct(), GetOwner(), {}, ESimpleEventReplicationPolicy::NoReplication);
}

void USimpleGameplayAbilityComponent::OnStructAttributeAdded(const FStructAttribute& NewStructAttribute)
{
	if (!LocalStructAttributes.Contains(NewStructAttribute))
	{
		LocalStructAttributes.Add(NewStructAttribute);
		SendEvent(FDefaultTags::StructAttributeAdded(), NewStructAttribute.AttributeTag, NewStructAttribute.AttributeValue, GetOwner(), {}, ESimpleEventReplicationPolicy::NoReplication);
	}
}

void USimpleGameplayAbilityComponent::OnStructAttributeChanged(const FStructAttribute& ChangedStructAttribute)
{
	for (FStructAttribute& LocalStructAttribute : LocalStructAttributes)
	{
		if (LocalStructAttribute.AttributeTag.MatchesTagExact(ChangedStructAttribute.AttributeTag))
		{
			FStructAttributeModification Payload;
			Payload.AttributeOwner = this;
			Payload.AttributeTag = ChangedStructAttribute.AttributeTag;
			Payload.OldValue = LocalStructAttribute.AttributeValue;
			Payload.NewValue = ChangedStructAttribute.AttributeValue;

			LocalStructAttribute = ChangedStructAttribute;

			if (LocalStructAttribute.StructAttributeHandler)
			{
				Payload.ModificationTags = GetStructAttributeHandlerInstance(LocalStructAttribute.StructAttributeHandler)->GetModificationEvents(ChangedStructAttribute.AttributeTag, Payload.OldValue, Payload.NewValue);
			}
			
			SendEvent(FDefaultTags::StructAttributeValueChanged(), ChangedStructAttribute.AttributeTag, FInstancedStruct::Make(Payload), this, {}, ESimpleEventReplicationPolicy::NoReplication);
			return;
		}
	}

	LocalStructAttributes.Add(ChangedStructAttribute);
	SendEvent(FDefaultTags::StructAttributeAdded(), ChangedStructAttribute.AttributeTag, ChangedStructAttribute.AttributeValue, GetOwner(), {}, ESimpleEventReplicationPolicy::NoReplication);
}

void USimpleGameplayAbilityComponent::OnStructAttributeRemoved(const FStructAttribute& RemovedStructAttribute)
{
	LocalStructAttributes.Remove(RemovedStructAttribute);
	SendEvent(FDefaultTags::StructAttributeRemoved(), RemovedStructAttribute.AttributeTag, FInstancedStruct(), GetOwner(), {}, ESimpleEventReplicationPolicy::NoReplication);
}