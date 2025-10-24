#pragma once

#include "CoreMinimal.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeModifier/ModifierActions/Base/ModifierAction.h"
#include "RuntimeAction.generated.h"

UCLASS(BlueprintType, EditInlineNew)
class SIMPLEGAMEPLAYABILITYSYSTEM_API URuntimeAction : public UModifierAction
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dynamic Action Functions", meta = (TitleProperty = "Description"))
	TArray<FRuntimeActionBranch> ActionBranches;

	virtual void ApplyAction_Implementation() override;
	virtual void OnClientPredictedCorrection_Implementation(
		FAttributeModifierActionScratchPad ServerInputScratchPad,
		FAttributeModifierActionScratchPad ServerOutputScratchPad,
		FAttributeModifierActionScratchPad ClientInputScratchPad,
		FAttributeModifierActionScratchPad ClientOutputScratchPad) override;
};
