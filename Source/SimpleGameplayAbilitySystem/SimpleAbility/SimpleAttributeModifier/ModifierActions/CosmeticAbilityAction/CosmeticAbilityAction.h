#pragma once

#include "CoreMinimal.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleAttributeModifier/ModifierActions/ModifierActionTypes.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleAttributeModifier/ModifierActions/Base/ModifierAction.h"
#include "CosmeticAbilityAction.generated.h"
UCLASS()
class SIMPLEGAMEPLAYABILITYSYSTEM_API UCosmeticAbilityAction : public UModifierAction
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attributes")
	TSubclassOf<USimpleGameplayAbility> AbilityClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attributes")
	EContextSource ContextSource;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attributes", meta=(
		EditConditionHides,
		EditCondition = "ContextSource == EContextSource::FromContextCollection"))
	FGameplayTag ContextCollectionTag;

	UPROPERTY(EditAnywhere, meta=(
		EditConditionHides,
		EditCondition = "ContextSource == EContextSource::FromFunction",
		FunctionReference,
		AllowFunctionLibraries,
		PrototypeFunction="/Script/SimpleGameplayAbilitySystem.FunctionSelectors.Prototype_GetContextCollection",
		DefaultBindingName="GetCosmeticAbilityActionContext"))
	FMemberReference ContextFunction;
};
