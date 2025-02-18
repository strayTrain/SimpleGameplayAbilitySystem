#include "SimpleAttributeFunctionLibrary.h"

#include "GameplayTagContainer.h"
#include "SimpleGameplayAbilitySystem/DefaultTags/DefaultTags.h"
#include "SimpleGameplayAbilitySystem/Module/SimpleGameplayAbilitySystem.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleAttributeModifier/SimpleAttributeModifierTypes.h"
#include "SimpleGameplayAbilitySystem/SimpleEventSubsystem/SimpleEventSubSystem.h"
#include "SimpleGameplayAbilitySystem/SimpleGameplayAbilityComponent/SimpleGameplayAbilityComponent.h"

bool USimpleAttributeFunctionLibrary::HasFloatAttribute(USimpleGameplayAbilityComponent* AbilityComponent, const FGameplayTag AttributeTag)
{
	if (GetFloatAttribute(AbilityComponent, AttributeTag))
	{
		return true;
	}

	return false;
}

bool USimpleAttributeFunctionLibrary::HasStructAttribute(USimpleGameplayAbilityComponent* AbilityComponent, const FGameplayTag AttributeTag)
{
	if (GetStructAttribute(AbilityComponent, AttributeTag))
	{
		return true;
	}

	return false;
}

float USimpleAttributeFunctionLibrary::GetFloatAttributeValue(USimpleGameplayAbilityComponent* AbilityComponent, EAttributeValueType ValueType, FGameplayTag AttributeTag, bool& WasFound)
{
	if (const FFloatAttribute* Attribute = GetFloatAttribute(AbilityComponent, AttributeTag))
	{
		WasFound = true;

		switch (ValueType)
		{
			case EAttributeValueType::BaseValue:
				return Attribute->BaseValue;
			case EAttributeValueType::CurrentValue:
				return Attribute->CurrentValue;
			case EAttributeValueType::MaxCurrentValue:
				return Attribute->ValueLimits.MaxCurrentValue;
			case EAttributeValueType::MinCurrentValue:
				return Attribute->ValueLimits.MinCurrentValue;
			case EAttributeValueType::MaxBaseValue:
				return Attribute->ValueLimits.MaxBaseValue;
			case EAttributeValueType::MinBaseValue:
				return Attribute->ValueLimits.MinBaseValue;
			default:
				SIMPLE_LOG(AbilityComponent, FString::Printf(TEXT("[USimpleAttributeFunctionLibrary::GetFloatAttributeValue]: ValueType %d not supported."), static_cast<int32>(ValueType)));
				return 0.0f;
		}
	}
	
	WasFound = false;
	return 0.0f;
}

bool USimpleAttributeFunctionLibrary::SetFloatAttributeValue(USimpleGameplayAbilityComponent* AbilityComponent, EAttributeValueType ValueType, FGameplayTag AttributeTag, float NewValue, float& Overflow)
{
	FFloatAttribute* Attribute = GetFloatAttribute(AbilityComponent, AttributeTag);
	
	if (!Attribute)
	{
		UE_LOG(LogSimpleGAS, Warning, TEXT("[USimpleAttributeFunctionLibrary::SetFloatAttributeValue]: Attribute %s not found."), *AttributeTag.ToString());
		return false;
	}
	
	const float ClampedValue = ClampFloatAttributeValue(*Attribute, ValueType, NewValue, Overflow);
				
	switch (ValueType)
	{
		case EAttributeValueType::BaseValue:
			Attribute->BaseValue = ClampedValue;
			SendFloatAttributeChangedEvent(AbilityComponent, FDefaultTags::FloatAttributeBaseValueChanged, AttributeTag, ValueType, ClampedValue);
			break;
		case EAttributeValueType::CurrentValue:
			Attribute->CurrentValue = ClampedValue;
			SendFloatAttributeChangedEvent(AbilityComponent, FDefaultTags::FloatAttributeCurrentValueChanged, AttributeTag, ValueType, ClampedValue);
			break;
		case EAttributeValueType::MaxCurrentValue:
			Attribute->ValueLimits.MaxCurrentValue = ClampedValue;
			SendFloatAttributeChangedEvent(AbilityComponent, FDefaultTags::FloatAttributeMaxCurrentValueChanged, AttributeTag, ValueType, ClampedValue);
			break;
		case EAttributeValueType::MinCurrentValue:
			Attribute->ValueLimits.MinCurrentValue = ClampedValue;
			SendFloatAttributeChangedEvent(AbilityComponent, FDefaultTags::FloatAttributeMinCurrentValueChanged, AttributeTag, ValueType, ClampedValue);
			break;
		case EAttributeValueType::MaxBaseValue:
			Attribute->ValueLimits.MaxBaseValue = ClampedValue;
			SendFloatAttributeChangedEvent(AbilityComponent, FDefaultTags::FloatAttributeMaxBaseValueChanged, AttributeTag, ValueType, ClampedValue);
			break;
		case EAttributeValueType::MinBaseValue:
			Attribute->ValueLimits.MinBaseValue = ClampedValue;
			SendFloatAttributeChangedEvent(AbilityComponent, FDefaultTags::FloatAttributeMinBaseValueChanged, AttributeTag, ValueType, ClampedValue);
			break;
	}

	if (AbilityComponent->HasAuthority())
	{
		AbilityComponent->AuthorityFloatAttributes.MarkItemDirty(*Attribute);
	}
	
	return true;
}

bool USimpleAttributeFunctionLibrary::IncrementFloatAttributeValue(USimpleGameplayAbilityComponent* AbilityComponent,
	EAttributeValueType ValueType, FGameplayTag AttributeTag, float Increment, float& Overflow)
{
	bool WasFound = false;
	float CurrentValue = GetFloatAttributeValue(AbilityComponent, ValueType, AttributeTag, WasFound);

	if (!WasFound)
	{
		return false;
	}

	return SetFloatAttributeValue(AbilityComponent, ValueType, AttributeTag, CurrentValue + Increment, Overflow);
}

bool USimpleAttributeFunctionLibrary::OverrideFloatAttribute(USimpleGameplayAbilityComponent* AbilityComponent, FGameplayTag AttributeTag, FFloatAttribute NewAttribute)
{
	for (FFloatAttribute& Attribute : AbilityComponent->AuthorityFloatAttributes.Attributes)
	{
		if (Attribute.AttributeTag.MatchesTagExact(AttributeTag))
		{
			CompareFloatAttributesAndSendEvents(AbilityComponent, Attribute, NewAttribute);
			
			Attribute.AttributeName = NewAttribute.AttributeName;
			Attribute.AttributeTag = NewAttribute.AttributeTag;
			Attribute.BaseValue = NewAttribute.BaseValue;
			Attribute.CurrentValue = NewAttribute.CurrentValue;
			Attribute.ValueLimits = NewAttribute.ValueLimits;
			
			AbilityComponent->AuthorityFloatAttributes.MarkItemDirty(Attribute);
			
			return true;
		}
	}

	SIMPLE_LOG(AbilityComponent, FString::Printf(TEXT("[USimpleAttributeFunctionLibrary::OverrideFloatAttribute]: Attribute %s not found."), *AttributeTag.ToString()));
	return false;
}

FInstancedStruct USimpleAttributeFunctionLibrary::GetStructAttributeValue(USimpleGameplayAbilityComponent* AbilityComponent, FGameplayTag AttributeTag, bool& WasFound)
{
	if (FStructAttribute* Attribute = GetStructAttribute(AbilityComponent, AttributeTag))
	{
		WasFound = true;
		return Attribute->AttributeValue;
	}

	SIMPLE_LOG(AbilityComponent, FString::Printf(TEXT("[USimpleAttributeFunctionLibrary::GetStructAttributeValue]: Attribute %s not found."), *AttributeTag.ToString()));
	WasFound = false;
	return FInstancedStruct();
}

bool USimpleAttributeFunctionLibrary::SetStructAttributeValue(USimpleGameplayAbilityComponent* AbilityComponent, FGameplayTag AttributeTag, FInstancedStruct NewValue)
{
	FStructAttribute* Attribute = GetStructAttribute(AbilityComponent, AttributeTag);
	
	if (!Attribute)
	{
		UE_LOG(LogSimpleGAS, Warning, TEXT("[USimpleAttributeFunctionLibrary::SetStructAttributeValue]: Attribute %s not found."), *AttributeTag.ToString());
		return false;
	}
	
	Attribute->AttributeValue = NewValue;
	
	if (AbilityComponent->HasAuthority())
	{
		AbilityComponent->AuthorityStructAttributes.MarkItemDirty(*Attribute);
	}
	
	return true;
}

float USimpleAttributeFunctionLibrary::ClampFloatAttributeValue(const FFloatAttribute& Attribute, EAttributeValueType ValueType, float NewValue, float& Overflow)
{
	switch (ValueType)
	{
		case EAttributeValueType::BaseValue:
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
				
		case EAttributeValueType::CurrentValue:
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
			UE_LOG(LogSimpleGAS, Warning, TEXT("USimpleGameplayAttributes::ClampValue: Invalid attribute value type."));
			return 0.0f;
	}
}

void USimpleAttributeFunctionLibrary::CompareFloatAttributesAndSendEvents(const USimpleGameplayAbilityComponent* AbilityComponent, const FFloatAttribute& OldAttribute, const FFloatAttribute& NewAttribute)
{
	if (OldAttribute.BaseValue != NewAttribute.BaseValue)
	{
		SendFloatAttributeChangedEvent(AbilityComponent, FDefaultTags::FloatAttributeBaseValueChanged, NewAttribute.AttributeTag, EAttributeValueType::BaseValue, NewAttribute.BaseValue);
	}

	if (NewAttribute.ValueLimits.UseMaxBaseValue && OldAttribute.ValueLimits.MaxBaseValue != NewAttribute.ValueLimits.MaxBaseValue)
	{
		SendFloatAttributeChangedEvent(AbilityComponent, FDefaultTags::FloatAttributeMaxBaseValueChanged, NewAttribute.AttributeTag, EAttributeValueType::MaxBaseValue, NewAttribute.ValueLimits.MaxBaseValue);
	}

	if (NewAttribute.ValueLimits.UseMinBaseValue && OldAttribute.ValueLimits.MinBaseValue != NewAttribute.ValueLimits.MinBaseValue)
	{
		SendFloatAttributeChangedEvent(AbilityComponent, FDefaultTags::FloatAttributeMinBaseValueChanged, NewAttribute.AttributeTag, EAttributeValueType::MinBaseValue, NewAttribute.ValueLimits.MinBaseValue);
	}
	
	if (OldAttribute.CurrentValue != NewAttribute.CurrentValue)
	{
		SendFloatAttributeChangedEvent(AbilityComponent, FDefaultTags::FloatAttributeCurrentValueChanged, NewAttribute.AttributeTag, EAttributeValueType::CurrentValue, NewAttribute.CurrentValue);
	}
	
	if (NewAttribute.ValueLimits.UseMaxCurrentValue && OldAttribute.ValueLimits.MaxCurrentValue != NewAttribute.ValueLimits.MaxCurrentValue)
	{
		SendFloatAttributeChangedEvent(AbilityComponent, FDefaultTags::FloatAttributeMaxCurrentValueChanged, NewAttribute.AttributeTag, EAttributeValueType::MaxCurrentValue, NewAttribute.ValueLimits.MaxCurrentValue);
	}

	if (NewAttribute.ValueLimits.UseMinCurrentValue && OldAttribute.ValueLimits.MinCurrentValue != NewAttribute.ValueLimits.MinCurrentValue)
	{
		SendFloatAttributeChangedEvent(AbilityComponent, FDefaultTags::FloatAttributeMinCurrentValueChanged, NewAttribute.AttributeTag, EAttributeValueType::MinCurrentValue, NewAttribute.ValueLimits.MinCurrentValue);
	}
}

void USimpleAttributeFunctionLibrary::SendFloatAttributeChangedEvent(const USimpleGameplayAbilityComponent* AbilityComponent, FGameplayTag EventTag, FGameplayTag AttributeTag, EAttributeValueType ValueType, float NewValue)
{
	if (USimpleEventSubsystem* EventSubsystem = AbilityComponent->GetWorld()->GetGameInstance()->GetSubsystem<USimpleEventSubsystem>())
	{
		FFloatAttributeModification Payload;
		Payload.AttributeTag = AttributeTag;
		Payload.ValueType = ValueType;
		Payload.NewValue = NewValue;

		const FInstancedStruct EventPayload = FInstancedStruct::Make(Payload);
		const FGameplayTag DomainTag = AbilityComponent->HasAuthority() ? FDefaultTags::AuthorityDomain : FDefaultTags::LocalDomain;
		
		EventSubsystem->SendEvent(EventTag, DomainTag, EventPayload, AbilityComponent->GetOwner());
	}
	else
	{
		UE_LOG(LogSimpleGAS, Warning, TEXT("USimpleGameplayAttributes::SendFloatAttributeChangedEvent: No event subsystem found."));
	}
}

void USimpleAttributeFunctionLibrary::ApplyAbilitySideEffects(USimpleGameplayAbilityComponent* Instigator, const TArray<FAbilitySideEffect>& AbilitySideEffects)
{
	for (const FAbilitySideEffect& SideEffect : AbilitySideEffects)
	{
		Instigator->ActivateAbility(SideEffect.AbilityClass, SideEffect.AbilityContext, true, SideEffect.ActivationPolicy);
	}
}

FFloatAttribute* USimpleAttributeFunctionLibrary::GetFloatAttribute(USimpleGameplayAbilityComponent* AbilityComponent, FGameplayTag AttributeTag)
{
	if (!AbilityComponent)
	{
		UE_LOG(LogSimpleGAS, Warning, TEXT("[USimpleAttributeFunctionLibrary::GetFloatAttribute]: AbilityComponent is null."));
		return nullptr;
	}

	if (AbilityComponent->HasAuthority())
	{
		for (FFloatAttribute& FloatAttribute : AbilityComponent->AuthorityFloatAttributes.Attributes)
		{
			if (FloatAttribute.AttributeTag.MatchesTagExact(AttributeTag))
			{
				return &FloatAttribute;
			}
		}
	}
	else
	{
		for (FFloatAttribute& FloatAttribute : AbilityComponent->LocalFloatAttributes)
		{
			if (FloatAttribute.AttributeTag.MatchesTagExact(AttributeTag))
			{
				return &FloatAttribute;
			}
		}
	}

	return nullptr;
}

FStructAttribute* USimpleAttributeFunctionLibrary::GetStructAttribute(USimpleGameplayAbilityComponent* AbilityComponent, FGameplayTag AttributeTag)
{
	if (!AbilityComponent)
	{
		UE_LOG(LogSimpleGAS, Warning, TEXT("[USimpleAttributeFunctionLibrary::GetStructAttribute]: AbilityComponent is null."));
		return nullptr;
	}

	if (AbilityComponent->HasAuthority())
	{
		for (FStructAttribute& StructAttribute : AbilityComponent->AuthorityStructAttributes.Attributes)
		{
			if (StructAttribute.AttributeTag.MatchesTagExact(AttributeTag))
			{
				return &StructAttribute;
			}
		}
	}
	else
	{
		for (FStructAttribute& StructAttribute : AbilityComponent->LocalStructAttributes)
		{
			if (StructAttribute.AttributeTag.MatchesTagExact(AttributeTag))
			{
				return &StructAttribute;
			}
		}
	}

	return nullptr;
}

