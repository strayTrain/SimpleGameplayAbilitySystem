
#pragma once

#include "CoreMinimal.h"
#include "FloatAttributeActionTypes.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleAttributeModifier/ModifierActions/Base/ModifierAction.h"
#include "ChangeFloatAttributeAction.generated.h"

UCLASS(BlueprintType, EditInlineNew)
class SIMPLEGAMEPLAYABILITYSYSTEM_API UChangeFloatAttributeAction : public UModifierAction
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Float Modification Params")
	FGameplayTag AttributeToModify;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Float Modification Params")
	EFloatAttributeValueType ModifiedAttributeValueType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Float Modification Params")
	EAttributeModificationValueSource ModificationInputValueSource;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Float Modification Params", meta = (EditCondition = "ModificationInputValueSource == EAttributeModificationValueSource::Manual", EditConditionHides))
	float ManualInputValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Float Modification Params", meta = (EditCondition = "((ModificationInputValueSource == EAttributeModificationValueSource::FromInstigatorAttribute) || (ModificationInputValueSource == EAttributeModificationValueSource::FromTargetAttribute))", EditConditionHides))
	FGameplayTag SourceAttribute;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Float Modification Params", meta = (EditCondition = "((ModificationInputValueSource == EAttributeModificationValueSource::FromInstigatorAttribute) || (ModificationInputValueSource == EAttributeModificationValueSource::FromTargetAttribute))", EditConditionHides))
	EFloatAttributeValueType SourceAttributeValueType;
	
	UPROPERTY(EditAnywhere, Category="Float Modification Params", meta=(EditCondition = "ModificationInputValueSource == EAttributeModificationValueSource::CustomInputValue", EditConditionHides, FunctionReference, AllowFunctionLibraries, PrototypeFunction="/Script/SimpleGameplayAbilitySystem.FunctionSelectors.Prototype_GetCustomFloatInputValue", DefaultBindingName="GetCustomInputValue"))
	FMemberReference CustomInputFunction;

	/**
	 * If true, the modifier set the overflow to 0 after using it even if there is overflow left. Use this when you want to reset the overflow between modifiers.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Float Modification Params", meta = (EditCondition = "ModificationInputValueSource == EAttributeModificationValueSource::FromOverflow", EditConditionHides))
	bool ConsumeOverflow = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Float Modification Params")
	EFloatAttributeModificationOperation ModificationOperation;

	UPROPERTY(EditAnywhere, Category="Float Modification Params", meta=(EditCondition = "ModificationOperation == EFloatAttributeModificationOperation::Custom", EditConditionHides, FunctionReference, AllowFunctionLibraries, PrototypeFunction="/Script/SimpleGameplayAbilitySystem.FunctionSelectors.Prototype_ApplyFloatAttributeOperation", DefaultBindingName="CustomFloatOperation"))
	FMemberReference FloatOperationFunction;

	/* ModifierAction overrides */
	virtual bool ShouldApply_Implementation(USimpleAttributeModifier* NewOwningModifier) const override;
	virtual bool ApplyAction_Implementation(FInstancedStruct& SnapshotData) override;
	virtual void OnClientPredictedCorrection_Implementation(FInstancedStruct ServerSnapshot, FInstancedStruct ClientSnapshot) override;
	virtual void OnServerInitiatedResultReceived_Implementation(FInstancedStruct ServerSnapshot) override;
	virtual void OnCancelAction_Implementation() override;
};
