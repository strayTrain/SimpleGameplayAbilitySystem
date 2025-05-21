
#pragma once

#include "CoreMinimal.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleAttributeModifier/ModifierActions/Base/ModifierAction.h"
#include "ChangeFloatAttributeAction.generated.h"

UCLASS(BlueprintType, EditInlineNew)
class SIMPLEGAMEPLAYABILITYSYSTEM_API UChangeFloatAttributeAction : public UModifierAction
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag AttributeToModify;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EAttributeValueType ModifiedAttributeValueType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EAttributeModificationValueSource ModificationInputValueSource;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "ModificationInputValueSource == EAttributeModificationValueSource::Manual", EditConditionHides))
	float ManualInputValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "((ModificationInputValueSource == EAttributeModificationValueSource::FromInstigatorAttribute) || (ModificationInputValueSource == EAttributeModificationValueSource::FromTargetAttribute))", EditConditionHides))
	FGameplayTag SourceAttribute;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "((ModificationInputValueSource == EAttributeModificationValueSource::FromInstigatorAttribute) || (ModificationInputValueSource == EAttributeModificationValueSource::FromTargetAttribute))", EditConditionHides))
	EAttributeValueType SourceAttributeValueType;
	
	UPROPERTY(EditAnywhere, meta=(EditCondition = "ModificationInputValueSource == EAttributeModificationValueSource::CustomInputValue", EditConditionHides, FunctionReference, AllowFunctionLibraries, PrototypeFunction="/Script/SimpleGameplayAbilitySystem.FunctionSelectors.Prototype_GetCustomFloatInputValue", DefaultBindingName="GetCustomInputValue"))
	FMemberReference CustomInputFunction;

	/**
	 * If true, the modifier set the overflow to 0 after using it even if there is overflow left. Use this when you want to reset the overflow between modifiers.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "ModificationInputValueSource == EAttributeModificationValueSource::FromOverflow", EditConditionHides))
	bool ConsumeOverflow = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EFloatAttributeModificationOperation ModificationOperation;

	UPROPERTY(EditAnywhere, meta=(EditCondition = "ModificationOperation == EFloatAttributeModificationOperation::Custom", EditConditionHides, FunctionReference, AllowFunctionLibraries, PrototypeFunction="/Script/SimpleGameplayAbilitySystem.FunctionSelectors.Prototype_ApplyFloatAttributeOperation", DefaultBindingName="CustomFloatOperation"))
	FMemberReference FloatOperationFunction;
};
