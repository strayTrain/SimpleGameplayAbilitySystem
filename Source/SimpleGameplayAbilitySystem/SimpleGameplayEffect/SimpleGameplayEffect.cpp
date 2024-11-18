// Fill out your copyright notice in the Description page of Project Settings.


#include "SimpleGameplayEffect.h"

#include "SimpleGameplayAbilitySystem/SimpleGameplayAbilityComponent/SimpleGameplayAbilityComponent.h"
#include "SimpleGameplayAbilitySystem/SimpleGameplayAttributes/SimpleGameplayAttributes.h"

void USimpleGameplayEffect::InitializeEffect_Implementation(USimpleGameplayAbilityComponent* SourceAbilityComponent, USimpleGameplayAbilityComponent* TargetAbilityComponent)
{
	if (!SourceAbilityComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("USimpleGameplayEffect::ApplyEffect: TargetAttributes is nullptr."));
		return;
	}
	
	SourceAbilitySystemComponent = SourceAbilityComponent;
	TargetAbilitySystemComponent = TargetAbilityComponent;
	
	bIsInitialized = true;
}

bool USimpleGameplayEffect::CanApplyEffect_Implementation() const
{
	return true;
}

bool USimpleGameplayEffect::ApplyEffect(FInstancedStruct EffectContext)
{
	if (!CanApplyEffectInternal() || !CanApplyEffect())
	{
		OnApplicationFailed();
		return false;
	}
	
	CurrentEffectContext = EffectContext;

	float CurrentOverflow = 0;

	// Start by making a copy of the target attributes component's attributes
	TArray<FFloatAttribute> TempFloatAttributes = TargetAbilitySystemComponent->Attributes->FloatAttributes;
	TArray<FStructAttribute> TempStructAttributes = TargetAbilitySystemComponent->Attributes->StructAttributes;
	// To keep track of which attributes were modified in this application
	TArray<FGameplayTag> ModifiedAttributes;
	
	bool WasModifierSuccessful = false;
	//FFloatAttribute FloatAttribute;
	FStructAttribute StructAttribute;
	float FloatModifierInput = 0;

	/*
	for (const FGameplayEffectModifier& Modifier : Modifiers)
	{
		WasModifierSuccessful = false;
		
		switch (Modifier.AttributeType)
		{
			case EAttributeType::FloatAttribute:
				if (GetTempFloatAttribute(TempFloatAttributes, Modifier.ModifiedAttribute, FloatAttribute))
				{
					// We start by getting the input to the modifier
					switch (Modifier.ModificationInputValueSource)
					{
						case Manual:
							FloatModifierInput = Modifier.ManualInputValue;
							break;
						case FromOverflow:
							FloatModifierInput = CurrentOverflow;
							break;
						case FromMetaAttribute:
							WasModifierSuccessful = GetFloatMetaAttributeValue(Modifier.MetaAttributeTag, FloatModifierInput);
							break;
						case FromAttribute:
							if (GetTempFloatAttribute(TempFloatAttributes, Modifier.SourceAttribute, FloatAttribute))
							{
								switch (Modifier.SourceAttributeValueType)
								{
									case BaseValue:
										FloatModifierInput = FloatAttribute.BaseValue;
										break;
									case CurrentValue:
										FloatModifierInput = FloatAttribute.CurrentValue;
										break;
									case MaxValue:
										FloatModifierInput = FloatAttribute.ValueLimits.MaxCurrentValue;
										break;
									case MinValue:
										FloatModifierInput = FloatAttribute.ValueLimits.MinCurrentValue;
										break;
									default:
										break;
								}
							}
							break;
						default:
							break;
					}
				}

				// Now that we have an input value, we do the modification
				if (WasModifierSuccessful)
				{
					switch (Modifier.ModificationOperation)
					{
						case EFloatAttributeModificationOperation::Add:
							FloatAttribute.CurrentValue += FloatModifierInput;
							break;
						case EFloatAttributeModificationOperation::Multiply:
							FloatAttribute.CurrentValue *= FloatModifierInput;
							break;
						case EFloatAttributeModificationOperation::Override:
							FloatAttribute.CurrentValue = FloatModifierInput;
							break;
						default:
							break;
					}

					// Clamp the new value
					FloatAttribute.CurrentValue = TargetAttributes->ClampValue(FloatAttribute, EAttributeValueType::CurrentValue, FloatAttribute.CurrentValue, CurrentOverflow);
					// Add the attribute to the modified list
					ModifiedAttributes.Add(Modifier.ModifiedAttribute);
					// Update the temporary attributes list
					TempFloatAttributes[TargetAttributes->GetAttributeIndex(Modifier.ModifiedAttribute)] = FloatAttribute;
					WasModifierSuccessful = true;
				}
			
				break;
			case EAttributeType::StructAttribute:
				break;
			default:
				return false;
		}

		if (!WasModifierSuccessful && Modifier.CancelEffectIfAttributeNotFound)
		{
			OnEffectApplyFailed();
			return false;
		}
	} */

	return false;
}

void USimpleGameplayEffect::GetFloatMetaAttributeValue_Implementation(FGameplayTag MetaAttributeTag, float& OutValue, bool& WasHandled) const
{
	OutValue = 0;
	WasHandled = false;
}

void USimpleGameplayEffect::ModifyStructAttribute_Implementation(FGameplayTag StructModifierTag, FGameplayTag MetaAttributeTag, FInstancedStruct& OutValue, bool& WasHandled) const
{
	OutValue = FInstancedStruct();
	WasHandled = false;
}

bool USimpleGameplayEffect::GetTempFloatAttribute(const TArray<FFloatAttribute>& FloatAttributes, const FGameplayTag AttributeTag, FFloatAttribute& OutAttribute) const
{
	for (const FFloatAttribute& Attribute : FloatAttributes)
	{
		if (Attribute.AttributeName == AttributeTag)
		{
			OutAttribute = Attribute;
			return true;
		}
	}

	return false;
}

bool USimpleGameplayEffect::GetTempStructAttribute(const TArray<FStructAttribute>& StructAttributes, const FGameplayTag AttributeTag, FStructAttribute& OutAttribute) const
{
	for (const FStructAttribute& Attribute : StructAttributes)
	{
		if (Attribute.AttributeName == AttributeTag)
		{
			OutAttribute = Attribute;
			return true;
		}
	}

	return false;
}

bool USimpleGameplayEffect::CanApplyEffectInternal() const
{
	if (!bIsInitialized)
	{
		UE_LOG(LogTemp, Warning, TEXT("USimpleGameplayEffect::ApplyEffect: Effect not initialized."));
		return false;
	}
	
	return true;
}
