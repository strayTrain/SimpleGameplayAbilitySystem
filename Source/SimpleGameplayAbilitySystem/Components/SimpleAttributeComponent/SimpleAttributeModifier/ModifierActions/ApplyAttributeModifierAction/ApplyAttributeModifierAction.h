#pragma once

#include "CoreMinimal.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeModifier/ModifierActions/Base/ModifierAction.h"
#include "ApplyAttributeModifierAction.generated.h"

UCLASS()
class SIMPLEGAMEPLAYABILITYSYSTEM_API UApplyAttributeModifierAction : public UModifierAction
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<USimpleAttributeModifier> ModifierClass;

	UPROPERTY(EditAnywhere, meta=(
		FunctionReference,
		AllowFunctionLibraries,
		PrototypeFunction="/Script/SimpleGameplayAbilitySystem.FunctionSelectors.Prototype_GetStructContext",
		DefaultBindingName="GetModifierActionContext"))
	FMemberReference ContextFunction;

	/* ModifierAction overrides */
	virtual void ApplyAction_Implementation() override;
};
