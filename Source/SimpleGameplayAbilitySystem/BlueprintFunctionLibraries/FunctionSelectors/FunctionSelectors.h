#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FunctionSelectors.generated.h"

struct FInstancedStruct;
class USimpleAttributeModifier;
class USimpleGameplayAbilityComponent;

UCLASS(MinimalAPI)
class UFunctionSelectors : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	// Prototype functions are used for function selectors in the editor. SHOULD NOT BE CALLED DIRECTLY.
#if WITH_EDITOR
	UFUNCTION(BlueprintInternalUseOnly, meta = (ReturnDisplayName = "CustomInputValue"))
	bool Prototype_GetCustomFloatInputValue(FGameplayTag AttributeTag, float& InputValue) { return false; }
	
	UFUNCTION(BlueprintInternalUseOnly, meta = (ReturnDisplayName = "NewFloatAttributeValue"))
	bool Prototype_ApplyFloatAttributeOperation(
		FGameplayTag AttributeTag,
		float CurrentAttributeValue,
		float OperationInputValue,
		float CurrentOverflow,
		FGameplayTag& EventTagOverride,
		float& NewAttributeValue,
		float& NewOverflow) { return false; }
	
	UFUNCTION(BlueprintInternalUseOnly, meta = (ReturnDisplayName = "NewStructAttributeValue"))
	bool Prototype_ModifyStructAttributeValue(
		FGameplayTag AttributeTag,
		const FInstancedStruct& InStruct,
		FInstancedStruct& OutStruct) { return false; };
	
	UFUNCTION(BlueprintInternalUseOnly, meta = (ReturnDisplayName = "Context"))
	bool Prototype_GetStructContext(FInstancedStruct& Context) { return false; }

	UFUNCTION(BlueprintInternalUseOnly, meta = (ReturnDisplayName = "Context"))
	bool Prototype_GetAttributeModifierSideEffectTargets(
		USimpleGameplayAbilityComponent*& OutInstigator,
		USimpleGameplayAbilityComponent*& OutTarget) { return false; }
#endif

	static bool GetCustomFloatInputValue(
		const USimpleAttributeModifier* OwningModifier,
		const FMemberReference& DynamicFunction,
		FGameplayTag AttributeTag,
		float& CustomInputValue);
	
	static bool ApplyFloatAttributeOperation(
		const USimpleAttributeModifier* OwningModifier,
		const FMemberReference& DynamicFunction,
		FGameplayTag AttributeTag,
		float CurrentAttributeValue,
		float OperationInputValue,
		float CurrentOverflow,
		FGameplayTag& EventTagOverride,
		float& NewAttributeValue,
		float& NewOverflow);

	static bool ModifyStructAttributeValue(
		const USimpleAttributeModifier* OwningModifier,
		const FMemberReference& DynamicFunction,
		FGameplayTag AttributeTag,
		const FInstancedStruct& InStruct,
		FInstancedStruct& OutStruct);
	
	static bool GetStructContext(const USimpleAttributeModifier* OwningModifier, const FMemberReference& DynamicFunction, FInstancedStruct& Context);

	static bool GetAttributeModifierSideEffectTargets(
		const USimpleAttributeModifier* OwningModifier,
		const FMemberReference& DynamicFunction,
		USimpleGameplayAbilityComponent*& OutInstigator,
		USimpleGameplayAbilityComponent*& OutTarget);
};
