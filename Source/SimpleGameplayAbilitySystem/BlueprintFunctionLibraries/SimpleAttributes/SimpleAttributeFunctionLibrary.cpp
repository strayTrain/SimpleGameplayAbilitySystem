#include "SimpleAttributeFunctionLibrary.h"

#include "GameplayTagContainer.h"
#include "SimpleGameplayAbilitySystem/DefaultTags/DefaultTags.h"
#include "SimpleGameplayAbilitySystem/Module/SimpleGameplayAbilitySystem.h"
#include "SimpleGameplayAbilitySystem/SimpleEventSubsystem/SimpleEventSubSystem.h"
#include "SimpleGameplayAbilitySystem/SimpleGameplayAbilityComponent/SimpleGameplayAbilityComponent.h"

int32 USimpleAttributeFunctionLibrary::GetFloatAttributeIndex(USimpleGameplayAbilityComponent* AbilityComponent, FGameplayTag AttributeTag)
{
	if (!AbilityComponent)
	{
		UE_LOG(LogSimpleGAS, Warning, TEXT("[USimpleAttributeFunctionLibrary::GetFloatAttributeIndex]: AbilityComponent is null."));
		return -1;
	}
	
	for (int i = 0; i < AbilityComponent->AuthorityFloatAttributes.Attributes.Num(); i++)
	{
		if (AbilityComponent->AuthorityFloatAttributes.Attributes[i].AttributeTag.MatchesTagExact(AttributeTag))
		{
			return i;
		}
	}

	return -1;
}

int32 USimpleAttributeFunctionLibrary::GetStructAttributeIndex(USimpleGameplayAbilityComponent* AbilityComponent, FGameplayTag AttributeTag)
{
	if (!AbilityComponent)
	{
		UE_LOG(LogSimpleGAS, Warning, TEXT("[USimpleAttributeFunctionLibrary::GetStructAttributeIndex]: AbilityComponent is null."));
		return -1;
	}
	
	for (int i = 0; i < AbilityComponent->AuthorityStructAttributes.Attributes.Num(); i++)
	{
		if (AbilityComponent->AuthorityStructAttributes.Attributes[i].AttributeTag.MatchesTagExact(AttributeTag))
		{
			return i;
		}
	}

	return -1;
}

bool USimpleAttributeFunctionLibrary::HasFloatAttribute(USimpleGameplayAbilityComponent* AbilityComponent, const FGameplayTag AttributeTag)
{
	return GetFloatAttributeIndex(AbilityComponent, AttributeTag) >= 0;
}

bool USimpleAttributeFunctionLibrary::HasStructAttribute(USimpleGameplayAbilityComponent* AbilityComponent, const FGameplayTag AttributeTag)
{
	return GetStructAttributeIndex(AbilityComponent, AttributeTag) >= 0;
}

FFloatAttribute USimpleAttributeFunctionLibrary::GetFloatAttributeCopy(USimpleGameplayAbilityComponent* AbilityComponent, const FGameplayTag AttributeTag, bool& WasFound)
{
	int32 AttributeIndex = GetFloatAttributeIndex(AbilityComponent, AttributeTag);

	if (AttributeIndex >= 0)
	{
		WasFound = true;
		return AbilityComponent->AuthorityFloatAttributes.Attributes[AttributeIndex];
	}

	WasFound = false;
	return FFloatAttribute();
}

float USimpleAttributeFunctionLibrary::GetFloatAttributeValue(USimpleGameplayAbilityComponent* AbilityComponent, EAttributeValueType ValueType, FGameplayTag AttributeTag, bool& WasFound)
{
	int32 AttributeIndex = GetFloatAttributeIndex(AbilityComponent, AttributeTag);
	WasFound = AttributeIndex >= 0;
	
	if (!WasFound)
	{
		UE_LOG(LogSimpleGAS, Warning, TEXT("Attribute %s not found."), *AttributeTag.ToString());
		return 0.0f;
	}

	const FFloatAttribute& Attribute = AbilityComponent->AuthorityFloatAttributes.Attributes[AttributeIndex];
	
	switch (ValueType)
	{
		case EAttributeValueType::BaseValue:
			return Attribute.BaseValue;
		case EAttributeValueType::CurrentValue:
			return Attribute.CurrentValue;
		case EAttributeValueType::MaxCurrentValue:
			return Attribute.ValueLimits.MaxCurrentValue;
		case EAttributeValueType::MinCurrentValue:
			return Attribute.ValueLimits.MinCurrentValue;
		case EAttributeValueType::MaxBaseValue:
			return Attribute.ValueLimits.MaxBaseValue;
		case EAttributeValueType::MinBaseValue:
			return Attribute.ValueLimits.MinBaseValue;
			
		default:
			WasFound = false;
			return 0.0f;
	}
}

bool USimpleAttributeFunctionLibrary::SetFloatAttributeValue(USimpleGameplayAbilityComponent* AbilityComponent, EAttributeValueType ValueType, FGameplayTag AttributeTag, float NewValue, float& Overflow)
{
	const int32 AttributeIndex = GetFloatAttributeIndex(AbilityComponent, AttributeTag);

	if (AttributeIndex < 0)
	{
		UE_LOG(LogSimpleGAS, Warning, TEXT("Attribute %s not found."), *AttributeTag.ToString());
		return false;
	}

	TArray<FFloatAttribute>& FloatAttributes = AbilityComponent->AuthorityFloatAttributes.Attributes;
	
	switch (ValueType)
	{
		case EAttributeValueType::BaseValue:
			FloatAttributes[AttributeIndex].BaseValue = ClampFloatAttributeValue(FloatAttributes[AttributeIndex], EAttributeValueType::BaseValue, NewValue, Overflow);
			SendFloatAttributeChangedEvent(AbilityComponent, FDefaultTags::FloatAttributeBaseValueChanged, AttributeTag, EAttributeValueType::BaseValue, FloatAttributes[AttributeIndex].BaseValue);
			break;
		
		case EAttributeValueType::CurrentValue:
			FloatAttributes[AttributeIndex].CurrentValue = ClampFloatAttributeValue(FloatAttributes[AttributeIndex], EAttributeValueType::CurrentValue, NewValue, Overflow);
			SendFloatAttributeChangedEvent(AbilityComponent, FDefaultTags::FloatAttributeCurrentValueChanged, AttributeTag, EAttributeValueType::CurrentValue, FloatAttributes[AttributeIndex].CurrentValue);
			break;
		
		case EAttributeValueType::MaxBaseValue:
			FloatAttributes[AttributeIndex].ValueLimits.MaxBaseValue = NewValue;
			SendFloatAttributeChangedEvent(AbilityComponent, FDefaultTags::FloatAttributeMaxBaseValueChanged, AttributeTag, EAttributeValueType::MaxBaseValue, NewValue);
			break;
		
		case EAttributeValueType::MinBaseValue:
			FloatAttributes[AttributeIndex].ValueLimits.MinBaseValue = NewValue;
			SendFloatAttributeChangedEvent(AbilityComponent, FDefaultTags::FloatAttributeMinBaseValueChanged, AttributeTag, EAttributeValueType::MinBaseValue, NewValue);
			break;
		
		case EAttributeValueType::MaxCurrentValue:
			FloatAttributes[AttributeIndex].ValueLimits.MaxCurrentValue = NewValue;
			SendFloatAttributeChangedEvent(AbilityComponent, FDefaultTags::FloatAttributeMaxCurrentValueChanged, AttributeTag, EAttributeValueType::MaxCurrentValue, NewValue);
			break;
		
		case EAttributeValueType::MinCurrentValue:
			FloatAttributes[AttributeIndex].ValueLimits.MinCurrentValue = NewValue;
			SendFloatAttributeChangedEvent(AbilityComponent, FDefaultTags::FloatAttributeMinCurrentValueChanged, AttributeTag, EAttributeValueType::MinCurrentValue, NewValue);
			break;
		
		default:
			return false;
	}
	
	AbilityComponent->AuthorityFloatAttributes.MarkItemDirty(FloatAttributes[AttributeIndex]);

	return true;
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

FStructAttribute USimpleAttributeFunctionLibrary::GetStructAttributeCopy(USimpleGameplayAbilityComponent* AbilityComponent, FGameplayTag AttributeTag, bool& WasFound)
{
	int32 AttributeIndex = GetStructAttributeIndex(AbilityComponent, AttributeTag);

	if (AttributeIndex >= 0)
	{
		WasFound = true;
		return AbilityComponent->AuthorityStructAttributes.Attributes[AttributeIndex];
	}

	WasFound = false;
	return FStructAttribute();
}

FInstancedStruct USimpleAttributeFunctionLibrary::GetStructAttributeValue(USimpleGameplayAbilityComponent* AbilityComponent, FGameplayTag AttributeTag, bool& WasFound)
{
	int32 AttributeIndex = GetStructAttributeIndex(AbilityComponent, AttributeTag);

	if (AttributeIndex >= 0)
	{
		WasFound = true;
		return AbilityComponent->AuthorityStructAttributes.Attributes[AttributeIndex].AttributeValue;
	}

	WasFound = false;
	return FInstancedStruct();
}

bool USimpleAttributeFunctionLibrary::SetStructAttributeValue(USimpleGameplayAbilityComponent* AbilityComponent, FGameplayTag AttributeTag, FInstancedStruct NewValue)
{
	int32 AttributeIndex = GetStructAttributeIndex(AbilityComponent, AttributeTag);

	if (AttributeIndex < 0)
	{
		UE_LOG(LogSimpleGAS, Warning, TEXT("Attribute %s not found."), *AttributeTag.ToString());
		return false;
	}

	AbilityComponent->AuthorityStructAttributes.Attributes[AttributeIndex].AttributeValue = NewValue;

	if (AbilityComponent->HasAuthority())
	{
		AbilityComponent->AuthorityStructAttributes.MarkItemDirty(AbilityComponent->AuthorityStructAttributes.Attributes[AttributeIndex]);
	}
	
	return true;
}

bool USimpleAttributeFunctionLibrary::HasModifierWithTags(const USimpleGameplayAbilityComponent* AbilityComponent, const FGameplayTagContainer& Tags)
{
	return false;
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
