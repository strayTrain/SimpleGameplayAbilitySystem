#pragma once

#include "CoreMinimal.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeModifier/ModifierActions/Base/ModifierAction.h"
#include "ChangeStructAttributeAction.generated.h"

UCLASS()
class SIMPLEGAMEPLAYABILITYSYSTEM_API UChangeStructAttributeAction : public UModifierAction
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag AttributeToModify;
	
	UPROPERTY(EditAnywhere, meta=(FunctionReference, AllowFunctionLibraries, PrototypeFunction="/Script/SimpleGameplayAbilitySystem.FunctionSelectors.Prototype_ModifyStructAttributeValue", DefaultBindingName="GetModifiedStructAttribute"))
	FMemberReference StructModificationFunction;

	virtual EActionPredictionMode GetPredictionMode_Implementation() const override { return EActionPredictionMode::PredictInstantOnly; }
	virtual bool CanApply_Implementation() const override;
	virtual void ApplyAction_Implementation() override;
	virtual void OnCancelAction_Implementation() override;

private:
	FInstancedStruct CachedPreviousValue;
};
