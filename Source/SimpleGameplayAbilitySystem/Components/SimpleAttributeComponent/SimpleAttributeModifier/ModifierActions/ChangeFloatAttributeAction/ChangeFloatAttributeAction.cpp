#include "ChangeFloatAttributeAction.h"

#include "SimpleGameplayAbilitySystem/BlueprintFunctionLibraries/FunctionSelectors/FunctionSelectors.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeComponent.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeComponentTypes.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeModifier/SimpleAttributeModifier.h"
#include "SimpleGameplayAbilitySystem/DefaultTags/DefaultTags.h"
#include "SimpleGameplayAbilitySystem/Module/SimpleGameplayAbilitySystem.h"

bool UChangeFloatAttributeAction::CanApply_Implementation() const
{
	return OwningModifier->TargetAttributeComponent->HasFloatAttribute(AttributeToModify);
}

void UChangeFloatAttributeAction::ApplyAction_Implementation()
{
	if (!OwningModifier->TargetAttributeComponent)
	{
		SIMPLE_LOG(OwningModifier->TargetAttributeComponent, TEXT("[USimpleAttributeModifier::ApplyAction]: Owning ability component is null."));
		return;
	}

	const FFloatAttribute* FloatAttribute = OwningModifier->TargetAttributeComponent->GetFloatAttribute(AttributeToModify);
	
	if (!FloatAttribute)
	{
		SIMPLE_LOG(OwningModifier->TargetAttributeComponent, FString::Printf(TEXT("[USimpleAttributeModifier::ApplyAction]: Attribute %s not found."), *AttributeToModify.ToString()));
		return;
	}

	// Cache the current value for rollback
	bool bFound = false;
	CachedPreviousValue = OwningModifier->TargetAttributeComponent->GetFloatAttributeValue(ModifiedAttributeValueType, AttributeToModify, bFound);
	
	/**
	 * The formula is NewAttributeValue = CurrentAttributeValue [operation] ModificationInputValue
	 * Where [operation] is one of the following: add, multiply, override (i.e. replace with) or custom (call a function)
	 */

	// To Start we get the input value for the modification
	float ModificationInputValue = 0;
	bool WasTargetAttributeFound = false;
	bool WasInstigatorAttributeFound = false;
	bool WasOverflowFound = false;
	bool WasScratchPadValueFound = false;

	if (!HasScratchPadValue(FDefaultTags::ScratchPadFloatOverflow()))
	{
		SetScratchPadValue(FDefaultTags::ScratchPadFloatOverflow(), 0.0f);
	}

	float Overflow = GetScratchPadValue(FDefaultTags::ScratchPadFloatOverflow(), WasOverflowFound);
	
	switch (ModificationInputValueSource)
	{
		case EAttributeModificationValueSource::Manual:
			ModificationInputValue = ManualInputValue;
			break;

		case EAttributeModificationValueSource::FromMagnitude:
			ModificationInputValue = OwningModifier->ModifierMagnitude;
			break;

		case EAttributeModificationValueSource::FromScaledMagnitude:
			ModificationInputValue = GetScaledMagnitude();
			break;

		case EAttributeModificationValueSource::FromStackCount:
			ModificationInputValue = static_cast<float>(GetStackCount());
			break;

		case EAttributeModificationValueSource::FromOverflow:
			ModificationInputValue = Overflow;

			if (ConsumeOverflow)
			{
				Overflow = 0;
				SetScratchPadValue(FDefaultTags::ScratchPadFloatOverflow(), 0);
			}
		
			break;

		case EAttributeModificationValueSource::FromScratchPadValue:
			ModificationInputValue = GetScratchPadValue(ScratchPadValueTag, WasScratchPadValueFound);
			break;
		
		case EAttributeModificationValueSource::FromInstigatorAttribute:
			if (!OwningModifier->InstigatorAttributeComponent)
			{
				UE_LOG(LogSimpleGAS, Warning, TEXT("USimpleAttributeModifier::ApplyFloatAttributeModifier: Instigator ability component is nullptr."));
				return;
			}
		
			ModificationInputValue = OwningModifier->InstigatorAttributeComponent->GetFloatAttributeValue(SourceAttributeValueType, SourceAttribute, WasInstigatorAttributeFound);

			if (!WasInstigatorAttributeFound)
			{
				UE_LOG(LogSimpleGAS, Warning, TEXT("USimpleAttributeModifier::ApplyFloatAttributeModifier: Source attribute %s not found on instigator ability component."), *SourceAttribute.ToString());
				return;
			}

			break;
		
		case EAttributeModificationValueSource::FromTargetAttribute:
			if (!OwningModifier->TargetAttributeComponent)
			{
				UE_LOG(LogSimpleGAS, Warning, TEXT("USimpleAttributeModifier::ApplyFloatAttributeModifier: Target ability component is nullptr."));
				return;
			}
		
			ModificationInputValue = OwningModifier->TargetAttributeComponent->GetFloatAttributeValue(SourceAttributeValueType, SourceAttribute, WasTargetAttributeFound);

			if (!WasTargetAttributeFound)
			{
				UE_LOG(LogSimpleGAS, Warning, TEXT("USimpleAttributeModifier::ApplyFloatAttributeModifier: Source attribute %s not found on target ability component."), *SourceAttribute.ToString());
				return;
			}
			
			break;
		
	case EAttributeModificationValueSource::CustomInputValue:
			if (!UFunctionSelectors::GetCustomFloatInputValue(
				OwningModifier,
				CustomInputFunction,
				AttributeToModify,
				ModificationInputValue))
			{
				SIMPLE_LOG(OwningModifier->TargetAttributeComponent, FString::Printf(TEXT("[USimpleAttributeModifier::ApplyFloatAttributeModifier]: Custom input function failed to activate.")));
				return;
			}
		
	}

	// Next up we get the current value of the attribute
	float CurrentAttributeValue = 0;
	switch (ModifiedAttributeValueType)
	{
		case EFloatAttributeValueType::BaseValue:
			CurrentAttributeValue = FloatAttribute->BaseValue;
			break;
		case EFloatAttributeValueType::MinBaseValue:
			CurrentAttributeValue = FloatAttribute->ValueLimits.MinBaseValue;
			break;
		case EFloatAttributeValueType::MaxBaseValue:
			CurrentAttributeValue = FloatAttribute->ValueLimits.MaxBaseValue;
			break;
		case EFloatAttributeValueType::CurrentValue:
			CurrentAttributeValue = FloatAttribute->CurrentValue;
			break;
		case EFloatAttributeValueType::MinCurrentValue:
			CurrentAttributeValue = FloatAttribute->ValueLimits.MinCurrentValue;
			break;
		case EFloatAttributeValueType::MaxCurrentValue:
			CurrentAttributeValue = FloatAttribute->ValueLimits.MaxCurrentValue;
			break;
	}
	
	// Next, modify FloatAttribute based on the input value and the modifier's operation
	float NewAttributeValue = 0;
	FGameplayTag FloatChangedDomainTag = FloatAttribute->AttributeTag;
	switch (ModificationOperation)
	{
		case EFloatAttributeModificationOperation::Add:
			NewAttributeValue = CurrentAttributeValue + ModificationInputValue;
			break;

		case EFloatAttributeModificationOperation::Subtract:
			NewAttributeValue = CurrentAttributeValue - ModificationInputValue;
			break;
					
		case EFloatAttributeModificationOperation::Multiply:
			NewAttributeValue = CurrentAttributeValue * ModificationInputValue;
			break;

		case EFloatAttributeModificationOperation::Divide:
			if (FMath::IsNearlyZero(ModificationInputValue))
			{
				SIMPLE_LOG(OwningModifier->TargetAttributeComponent, TEXT("[USimpleAttributeModifier::ApplyFloatAttributeModifier]: Division by zero."));
				return;
			}
			NewAttributeValue = CurrentAttributeValue / ModificationInputValue;
			break;

		case EFloatAttributeModificationOperation::Power:
			NewAttributeValue = FMath::Pow(CurrentAttributeValue, ModificationInputValue);
			break;
		
		case EFloatAttributeModificationOperation::Override:
			NewAttributeValue =  ModificationInputValue;
			break;
		
		case EFloatAttributeModificationOperation::Custom:
			if (!UFunctionSelectors::ApplyFloatAttributeOperation(
				OwningModifier,
				FloatOperationFunction,
				FloatAttribute->AttributeTag,
				CurrentAttributeValue,
				ModificationInputValue,
				Overflow,
				FloatChangedDomainTag,
				NewAttributeValue,
				Overflow))
			{
				SIMPLE_LOG(OwningModifier->TargetAttributeComponent, FString::Printf(TEXT("[USimpleAttributeModifier::ApplyFloatAttributeModifier]: Custom operation function %s failed to activate."), *CustomInputFunction.GetMemberName().ToString()));
				return;
			}

			break;
	}

	OwningModifier->TargetAttributeComponent->SetFloatAttributeValue(ModifiedAttributeValueType, FloatAttribute->AttributeTag, NewAttributeValue, Overflow);
	SetScratchPadValue(FDefaultTags::ScratchPadFloatOverflow(), Overflow);
}

void UChangeFloatAttributeAction::OnCancelAction_Implementation()
{
	if (OwningModifier && OwningModifier->TargetAttributeComponent)
	{
		float Overflow = 0.0f;
		OwningModifier->TargetAttributeComponent->SetFloatAttributeValue(ModifiedAttributeValueType, AttributeToModify, CachedPreviousValue, Overflow);
	}
}
