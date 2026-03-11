#pragma once

#include "CoreMinimal.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeModifier/ModifierActions/Base/ModifierAction.h"
#include "EndModifierAction.generated.h"

UCLASS(BlueprintType, EditInlineNew)
class SIMPLEGAMEPLAYABILITYSYSTEM_API UEndModifierAction : public UModifierAction
{
	GENERATED_BODY()

public:
	/**
	 * Optional function to determine whether the modifier should be ended when this action triggers.
	 * If no function is bound, the modifier will always be ended.
	 * The function should match the signature: void FunctionName(UModifierAction* OwningAction, bool& ShouldRespond)
	 */
	UPROPERTY(EditAnywhere, meta=(
		FunctionReference,
		AllowFunctionLibraries,
		PrototypeFunction="/Script/SimpleGameplayAbilitySystem.FunctionSelectors.Prototype_ShouldApplyRuntimeAction",
		DefaultBindingName="ShouldEndModifier"))
	FMemberReference ShouldEndModifier;

	virtual void ApplyAction_Implementation() override;
};
