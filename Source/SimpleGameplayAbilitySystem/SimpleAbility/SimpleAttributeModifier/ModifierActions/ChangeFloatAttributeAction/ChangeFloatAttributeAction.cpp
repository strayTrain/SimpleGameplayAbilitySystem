#include "ChangeFloatAttributeAction.h"

#include "SimpleGameplayAbilitySystem/BlueprintFunctionLibraries/FunctionSelectors/FunctionSelectors.h"
#include "SimpleGameplayAbilitySystem/DefaultTags/DefaultTags.h"
#include "SimpleGameplayAbilitySystem/Module/SimpleGameplayAbilitySystem.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleAttributeModifier/SimpleAttributeModifier.h"
#include "SimpleGameplayAbilitySystem/SimpleGameplayAbilityComponent/SimpleGameplayAbilityComponent.h"

bool UChangeFloatAttributeAction::ShouldApply_Implementation(USimpleAttributeModifier* NewOwningModifier) const
{
	return OwningModifier->OwningAbilityComponent->HasFloatAttribute(AttributeToModify);
}

bool UChangeFloatAttributeAction::ApplyAction_Implementation(FInstancedStruct& SnapshotData)
{
	USimpleGameplayAbilityComponent* AbilityComponent = OwningModifier->OwningAbilityComponent;

	if (!AbilityComponent)
	{
		SIMPLE_LOG(AbilityComponent, TEXT("[USimpleAttributeModifier::ApplyAction]: Owning ability component is null."));
		return false;
	}

	const FFloatAttribute* FloatAttribute = AbilityComponent->GetFloatAttribute(AttributeToModify);
	
	if (!FloatAttribute)
	{
		SIMPLE_LOG(AbilityComponent, FString::Printf(TEXT("[USimpleAttributeModifier::ApplyAction]: Attribute %s not found."), *AttributeToModify.ToString()));
		return false;
	}
	
	/**
	 * The formula is NewAttributeValue = CurrentAttributeValue [operation] ModificationInputValue
	 * Where [operation] is one of the following: add, multiply, override (i.e. replace with) or custom (call a function)
	 */

	// To Start we get the input value for the modification
	float ModificationInputValue = 0;
	bool WasTargetAttributeFound = false;
	bool WasInstigatorAttributeFound = false;

	if (!GetScratchPad().ScratchpadValues.Contains(FDefaultTags::ScratchPadFloatOverflow()))
	{
		GetScratchPad().ScratchpadValues.Add(FDefaultTags::ScratchPadFloatOverflow(), 0.0f);
	}
	
	switch (ModificationInputValueSource)
	{
		case EAttributeModificationValueSource::Manual:
			ModificationInputValue = ManualInputValue;
			break;

		case EAttributeModificationValueSource::FromMagnitude:
			ModificationInputValue = OwningModifier->Magnitude;
			break;
		
		case EAttributeModificationValueSource::FromOverflow:
			ModificationInputValue = GetScratchPad().ScratchpadValues[FDefaultTags::ScratchPadFloatOverflow()];

			if (ConsumeOverflow)
			{
				GetScratchPad().ScratchpadValues[FDefaultTags::ScratchPadFloatOverflow()] = 0;
			}
		
			break;
		
		case EAttributeModificationValueSource::FromInstigatorAttribute:
			if (!OwningModifier->InstigatorAbilityComponent)
			{
				UE_LOG(LogSimpleGAS, Warning, TEXT("USimpleAttributeModifier::ApplyFloatAttributeModifier: Instigator ability component is nullptr."));
				return false;
			}
		
			ModificationInputValue = OwningModifier->InstigatorAbilityComponent->GetFloatAttributeValue(SourceAttributeValueType, SourceAttribute, WasInstigatorAttributeFound);

			if (!WasInstigatorAttributeFound)
			{
				UE_LOG(LogSimpleGAS, Warning, TEXT("USimpleAttributeModifier::ApplyFloatAttributeModifier: Source attribute %s not found on instigator ability component."), *SourceAttribute.ToString());
				return false;
			}

			break;
		
		case EAttributeModificationValueSource::FromTargetAttribute:
			if (!OwningModifier->TargetAbilityComponent)
			{
				UE_LOG(LogSimpleGAS, Warning, TEXT("USimpleAttributeModifier::ApplyFloatAttributeModifier: Target ability component is nullptr."));
				return false;
			}
		
			ModificationInputValue = OwningModifier->TargetAbilityComponent->GetFloatAttributeValue(SourceAttributeValueType, SourceAttribute, WasTargetAttributeFound);

			if (!WasTargetAttributeFound)
			{
				UE_LOG(LogSimpleGAS, Warning, TEXT("USimpleAttributeModifier::ApplyFloatAttributeModifier: Source attribute %s not found on target ability component."), *SourceAttribute.ToString());
				return false;
			}
			
			break;
		
	case EAttributeModificationValueSource::CustomInputValue:
			if (!UFunctionSelectors::GetCustomFloatInputValue(
				OwningModifier,
				CustomInputFunction,
				AttributeToModify,
				ModificationInputValue))
			{
				SIMPLE_LOG(AbilityComponent, FString::Printf(TEXT("[USimpleAttributeModifier::ApplyFloatAttributeModifier]: Custom input function failed to activate.")));
				return false;
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
				SIMPLE_LOG(AbilityComponent, TEXT("[USimpleAttributeModifier::ApplyFloatAttributeModifier]: Division by zero."));
				return false;
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
				GetScratchPad().ScratchpadValues[FDefaultTags::ScratchPadFloatOverflow()],
				FloatChangedDomainTag,
				NewAttributeValue,
				GetScratchPad().ScratchpadValues[FDefaultTags::ScratchPadFloatOverflow()]))
			{
				SIMPLE_LOG(AbilityComponent, FString::Printf(TEXT("[USimpleAttributeModifier::ApplyFloatAttributeModifier]: Custom operation function %s failed to activate."), *CustomInputFunction.GetMemberName().ToString()));
				return false;
			}

			break;
	}

	return AbilityComponent->SetFloatAttributeValue(ModifiedAttributeValueType, FloatAttribute->AttributeTag, NewAttributeValue, GetScratchPad().ScratchpadValues[FDefaultTags::ScratchPadFloatOverflow()]);
}

void UChangeFloatAttributeAction::OnClientPredictedCorrection_Implementation(FInstancedStruct ServerSnapshot,
	FInstancedStruct ClientSnapshot)
{
	Super::OnClientPredictedCorrection_Implementation(ServerSnapshot, ClientSnapshot);
}

void UChangeFloatAttributeAction::OnServerInitiatedResultReceived_Implementation(FInstancedStruct ServerSnapshot)
{
	Super::OnServerInitiatedResultReceived_Implementation(ServerSnapshot);
}

void UChangeFloatAttributeAction::OnCancelAction_Implementation()
{
	Super::OnCancelAction_Implementation();
}
