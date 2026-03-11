#pragma once

#include "CoreMinimal.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeModifier/ModifierActions/Base/ModifierAction.h"
#include "CancelSelfModifierAction.generated.h"

UCLASS(BlueprintType, EditInlineNew)
class SIMPLEGAMEPLAYABILITYSYSTEM_API UCancelSelfModifierAction : public UModifierAction
{
	GENERATED_BODY()

public:
	/**
	 * Optional function to determine whether the owning modifier should be cancelled when this action triggers.
	 * If no function is bound, the modifier will always be cancelled.
	 * The function should match the signature: void FunctionName(UModifierAction* OwningAction, bool& ShouldRespond)
	 */
	UPROPERTY(EditAnywhere, meta=(
		FunctionReference,
		AllowFunctionLibraries,
		PrototypeFunction="/Script/SimpleGameplayAbilitySystem.FunctionSelectors.Prototype_ShouldApplyRuntimeAction",
		DefaultBindingName="ShouldCancelModifier"))
	FMemberReference ShouldCancelModifier;

	virtual void ApplyAction_Implementation() override;
};
