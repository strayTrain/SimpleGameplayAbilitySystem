// Fill out your copyright notice in the Description page of Project Settings.


#include "SimpleAttributeModifier.h"

#include "SimpleGameplayAbilitySystem/SimpleAbilityComponent/SimpleAbilityComponent.h"

void USimpleAttributeModifier::InitializeModifier(const FGuid NewModifierID, USimpleAbilityComponent* InstigatingAbilityComponent, USimpleAbilityComponent* TargetedAbilityComponent)
{
	if (!InstigatingAbilityComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("USimpleAttributeModifier::ApplyModifier: TargetAttributes is nullptr."));
		return;
	}

	ModifierID = NewModifierID;
	InstigatorAbilityComponent = InstigatingAbilityComponent;
	TargetAbilityComponent = TargetedAbilityComponent;
	
	bIsInitialized = true;
}

bool USimpleAttributeModifier::CanApplyModifierInternal() const
{
	if (!bIsInitialized)
	{
		UE_LOG(LogTemp, Warning, TEXT("USimpleAttributeModifier::ApplyModifier: Effect not initialized."));
		return false;
	}

	// We require at least a target ability component to apply a modifier
	if (!TargetAbilityComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("USimpleAttributeModifier::ApplyModifier: target ability component is nullptr."));
		return false;
	}
	
	// Check if the modifier can be applied based on the tag settings
	if (!TargetAbilityComponent->GameplayTags.HasAllExact(TagConfig.ApplicationRequiredTags))
	{
		UE_LOG(LogTemp, Warning, TEXT("USimpleAttributeModifier::ApplyModifier: Target ability component doesn't have all required tags."));
		return false;
	}

	if (TargetAbilityComponent->GameplayTags.HasAnyExact(TagConfig.ApplicationBlockingTags))
	{
		UE_LOG(LogTemp, Warning, TEXT("USimpleAttributeModifier::ApplyModifier: Target ability component has application blocking tags."));
		return false;
	}

	if (TargetAbilityComponent->HasModifierWithTags(TagConfig.ApplicationBlockingModifierTags))
	{
		UE_LOG(LogTemp, Warning, TEXT("USimpleAttributeModifier::ApplyModifier: Target ability component has blocking modifiers."));
		return false;
	}
	
	return true;
}

bool USimpleAttributeModifier::CanApplyModifier_Implementation() const
{
	return true;
}

void USimpleAttributeModifier::PreApplyModifierStack_Implementation(TArray<FFloatAttribute>& TempFloatAttributes, TArray<FStructAttribute>& TempStructAttributes)
{
}

bool USimpleAttributeModifier::ApplyModifier(FInstancedStruct ModifierContext)
{
	CurrentModifierContext = ModifierContext;
	
	if (!CanApplyModifierInternal() || !CanApplyModifier())
	{
		OnModifierApplyFailed();
		return false;
	}

	// We process the modifier stack as a transaction to avoid partial application of the stack
	float CurrentFloatModifierOverflow = 0;

	TArray<FFloatAttribute> TempFloatAttributes = TargetAbilityComponent->FloatAttributes;
	TArray<FStructAttribute> TempStructAttributes = TargetAbilityComponent->StructAttributes;
	
	TArray<FGameplayTag> ModifiedFloatAttributes;
	TArray<FGameplayTag> ModifiedStructAttributes;
	
	PreApplyModifierStack(TempFloatAttributes, TempStructAttributes);

	for (const FAttributeModifier& Modifier : ModifierConfig.ApplicationModifications)
	{
		bool WasModifierApplied = false;
		
		switch (Modifier.AttributeType)
		{
			case FloatAttribute:
				WasModifierApplied = ApplyFloatAttributeModifier(Modifier, TempFloatAttributes, CurrentFloatModifierOverflow);
				if (WasModifierApplied)
				{
					ModifiedFloatAttributes.Add(Modifier.ModifiedAttribute);
				}
				break;
			case StructAttribute:
				WasModifierApplied = ApplyStructAttributeModifier(Modifier, TempStructAttributes);
				if (WasModifierApplied)
				{
					ModifiedStructAttributes.Add(Modifier.ModifiedAttribute);
				}
				break;
			default:
				break;
		}
		
		if (!WasModifierApplied && Modifier.CancelIfAttributeNotFound)
		{
			OnModifierApplyFailed();
			return false;
		}
	}

	// If all the modifiers were applied successfully, we update the target ability component's attributes
	for (FFloatAttribute Attribute : TempFloatAttributes)
	{
		if (ModifiedFloatAttributes.Contains(Attribute.AttributeTag))
		{
			TargetAbilityComponent->OverrideFloatAttribute(Attribute.AttributeTag, Attribute);
		}
	}

	for (FStructAttribute Attribute : TempStructAttributes)
	{
		if (ModifiedStructAttributes.Contains(Attribute.AttributeTag))
		{
			TargetAbilityComponent->AddStructAttribute(Attribute);
		}
	}
	
	return true;
}		

void USimpleAttributeModifier::GetFloatMetaAttributeValue_Implementation(FGameplayTag MetaAttributeTag, float& OutValue, bool& WasHandled) const
{
	OutValue = 0;
	WasHandled = false;
}

void USimpleAttributeModifier::ModifyStructAttribute_Implementation(FGameplayTag StructModifierTag, FGameplayTag MetaAttributeTag, FInstancedStruct& OutValue, bool& WasHandled) const
{
	OutValue = FInstancedStruct();
	WasHandled = false;
}

bool USimpleAttributeModifier::ApplyFloatAttributeModifier(const FAttributeModifier& Modifier, TArray<FFloatAttribute>& TempFloatAttributes, float& CurrentOverflow)
{
	FFloatAttribute* AttributeToModify = GetTempFloatAttribute(Modifier.ModifiedAttribute, TempFloatAttributes);
	
	if (!AttributeToModify)
	{
		UE_LOG(LogTemp, Warning, TEXT("USimpleAttributeModifier::ApplyFloatAttributeModifier: Attribute %s not found on target ability component."), *Modifier.ModifiedAttribute.ToString());
		return false;
	}

	// Get the input value for the modification
	float ModificationInputValue = 0;
	bool WasInstigatorAttributeFound = false;
	bool WasTargetAttributeFound = false;
	bool WasMetaAttributeHandled = false;
	switch (Modifier.ModificationInputValueSource)
	{
		case Manual:
			ModificationInputValue = Modifier.ManualInputValue;
			break;
		case FromOverflow:
			ModificationInputValue = CurrentOverflow;

			if (Modifier.ConsumeOverflow)
			{
				CurrentOverflow = 0;
			}
		
			break;
		case FromInstigatorAttribute:
			if (!InstigatorAbilityComponent)
			{
				UE_LOG(LogTemp, Warning, TEXT("USimpleAttributeModifier::ApplyFloatAttributeModifier: Instigator ability component is nullptr."));
				return false;
			}

			
			ModificationInputValue = InstigatorAbilityComponent->GetFloatAttributeValue(Modifier.SourceAttributeValueType, Modifier.SourceAttribute, WasInstigatorAttributeFound);

			if (!WasInstigatorAttributeFound)
			{
				UE_LOG(LogTemp, Warning, TEXT("USimpleAttributeModifier::ApplyFloatAttributeModifier: Source attribute %s not found on instigator ability component."), *Modifier.SourceAttribute.ToString());
				return false;
			}

			break;
		case FromTargetAttribute:
			if (!TargetAbilityComponent)
			{
				UE_LOG(LogTemp, Warning, TEXT("USimpleAttributeModifier::ApplyFloatAttributeModifier: Target ability component is nullptr."));
				return false;
			}

			
			ModificationInputValue = TargetAbilityComponent->GetFloatAttributeValue(Modifier.SourceAttributeValueType, Modifier.SourceAttribute, WasTargetAttributeFound);

			if (!WasTargetAttributeFound)
			{
				UE_LOG(LogTemp, Warning, TEXT("USimpleAttributeModifier::ApplyFloatAttributeModifier: Source attribute %s not found on target ability component."), *Modifier.SourceAttribute.ToString());
				return false;
			}
			
			break;
		case FromMetaAttribute:
			GetFloatMetaAttributeValue(Modifier.MetaAttributeTag, ModificationInputValue, WasMetaAttributeHandled);

			if (!WasMetaAttributeHandled)
			{
				UE_LOG(LogTemp, Warning, TEXT("USimpleAttributeModifier::ApplyFloatAttributeModifier: Meta attribute %s not handled."), *Modifier.MetaAttributeTag.ToString());
				return false;
			}
		
			break;
		default:
			break;
	}

	// Finally, modify AttributeToModify based on the input value and the modifier's operation
	switch (Modifier.ModifiedAttributeValueType)
	{
		case BaseValue:
			switch (Modifier.ModificationOperation)
			{
				case Add:
					AttributeToModify->BaseValue = USimpleAbilityComponent::ClampFloatAttributeValue(*AttributeToModify, BaseValue, AttributeToModify->BaseValue + ModificationInputValue, CurrentOverflow);
					break;
				case Multiply:
					AttributeToModify->BaseValue = USimpleAbilityComponent::ClampFloatAttributeValue(*AttributeToModify, BaseValue, AttributeToModify->BaseValue * ModificationInputValue, CurrentOverflow);
					break;
				case Override:
					AttributeToModify->BaseValue = USimpleAbilityComponent::ClampFloatAttributeValue(*AttributeToModify, BaseValue, ModificationInputValue, CurrentOverflow);
					break;
				default:
					break;
			}
			break;
		
		case CurrentValue:
			switch (Modifier.ModificationOperation)
			{
				case Add:
					AttributeToModify->CurrentValue = USimpleAbilityComponent::ClampFloatAttributeValue(*AttributeToModify, CurrentValue, AttributeToModify->CurrentValue + ModificationInputValue, CurrentOverflow);
						break;
				case Multiply:
					AttributeToModify->CurrentValue = USimpleAbilityComponent::ClampFloatAttributeValue(*AttributeToModify, CurrentValue, AttributeToModify->CurrentValue * ModificationInputValue, CurrentOverflow);
						break;
				case Override:
					AttributeToModify->CurrentValue = USimpleAbilityComponent::ClampFloatAttributeValue(*AttributeToModify, CurrentValue, ModificationInputValue, CurrentOverflow);
						break;
				default:
					break;
			}
			break;
		
		case MaxBaseValue:
			AttributeToModify->ValueLimits.MaxBaseValue = ModificationInputValue;
			break;
		
		case MinBaseValue:
			AttributeToModify->ValueLimits.MinBaseValue = ModificationInputValue;
			break;
		
		case MaxCurrentValue:
			AttributeToModify->ValueLimits.MaxCurrentValue = ModificationInputValue;
			break;
		
		case MinCurrentValue:
			AttributeToModify->ValueLimits.MinCurrentValue = ModificationInputValue;
			break;
		
		default:
			break;
	}
	
	return true;
}

bool USimpleAttributeModifier::ApplyStructAttributeModifier(const FAttributeModifier& Modifier, TArray<FStructAttribute>& TempStructAttributes)
{
	FStructAttribute* AttributeToModify = GetTempStructAttribute(Modifier.ModifiedAttribute, TempStructAttributes);
	
	if (!AttributeToModify)
	{
		UE_LOG(LogTemp, Warning, TEXT("USimpleAttributeModifier::ApplyStructAttributeModifier: Attribute %s not found on target ability component."), *Modifier.ModifiedAttribute.ToString());
		return false;
	}

	FInstancedStruct NewValue = AttributeToModify->AttributeValue;
	bool WasHandled = false;
	
	ModifyStructAttribute(Modifier.StructModifierTag, Modifier.MetaAttributeTag, NewValue, WasHandled);

	if (!WasHandled)
	{
		UE_LOG(LogTemp, Warning, TEXT("USimpleAttributeModifier::ApplyStructAttributeModifier: Struct modifier %s not handled."), *Modifier.StructModifierTag.ToString());
		return false;
	}

	AttributeToModify->AttributeValue = NewValue;
	return true;
}

FFloatAttribute* USimpleAttributeModifier::GetTempFloatAttribute(const FGameplayTag AttributeTag, TArray<FFloatAttribute>& TempFloatAttributes) const
{
	for (int i = 0; i < TempFloatAttributes.Num(); i++)
	{
		if (TempFloatAttributes[i].AttributeTag == AttributeTag)
		{
			return &TempFloatAttributes[i];
		}
	}

	return nullptr;
}

FStructAttribute* USimpleAttributeModifier::GetTempStructAttribute(const FGameplayTag AttributeTag, TArray<FStructAttribute>& TempStructAttributes) const
{
	for (int i = 0; i < TempStructAttributes.Num(); i++)
	{
		if (TempStructAttributes[i].AttributeTag == AttributeTag)
		{
			return &TempStructAttributes[i];
		}
	}

	return nullptr;
}

