#pragma once

#include "CoreMinimal.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleAttributeModifier/ModifierActions/ModifierActionTypes.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleAttributeModifier/ModifierActions/Base/ModifierAction.h"
#include "ApplyModifierAction.generated.h"

UCLASS()
class SIMPLEGAMEPLAYABILITYSYSTEM_API UApplyModifierAction : public UModifierAction
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<USimpleAttributeModifier> ModifierClass;

	UPROPERTY(EditAnywhere, meta=(
		FunctionReference,
		AllowFunctionLibraries,
		PrototypeFunction="/Script/SimpleGameplayAbilitySystem.FunctionSelectors.Prototype_GetContextCollection",
		DefaultBindingName="GetModifierActionContext"))
	FMemberReference ContextFunction;
};
