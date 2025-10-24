#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeModifier/ModifierActions/Base/ModifierAction.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleAbilityTypes.h"
#include "FunctionSelectors.generated.h"

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

	UFUNCTION(BlueprintInternalUseOnly, meta = (ReturnDisplayName = "ShouldRespond"))
	void Prototype_ShouldRespondToEvent(
		FGameplayTag EventTag,
		FGameplayTag DomainTag,
		FInstancedStruct Payload,
		UObject* Sender,
		bool& ShouldRespond) { ShouldRespond = false; }

	UFUNCTION(BlueprintInternalUseOnly, meta = (ReturnDisplayName = "ShouldRespond"))
	void Prototype_ShouldApplyRuntimeAction(UModifierAction* OwningAction, bool& ShouldRespond) { ShouldRespond = false; }

	UFUNCTION(BlueprintInternalUseOnly)
	void Prototype_ApplyRuntimeAction(UModifierAction* OwningAction) { }

	UFUNCTION(BlueprintInternalUseOnly)
	void Prototype_RuntimeActionPredictionCorrection(
		UModifierAction* OwningAction,
		FAttributeModifierActionScratchPad ServerInputScratchPad,
		FAttributeModifierActionScratchPad ServerOutputScratchPad,
		FAttributeModifierActionScratchPad ClientInputScratchPad,
		FAttributeModifierActionScratchPad ClientOutputScratchPad) {}
#endif

	static bool GetCustomFloatInputValue(
		USimpleAttributeModifier* OwningModifier,
		const FMemberReference& DynamicFunction,
		FGameplayTag AttributeTag,
		float& CustomInputValue);
	
	static bool ApplyFloatAttributeOperation(
		USimpleAttributeModifier* OwningModifier,
		const FMemberReference& DynamicFunction,
		FGameplayTag AttributeTag,
		float CurrentAttributeValue,
		float OperationInputValue,
		float CurrentOverflow,
		FGameplayTag& EventTagOverride,
		float& NewAttributeValue,
		float& NewOverflow);

	static bool ModifyStructAttributeValue(
		USimpleAttributeModifier* OwningModifier,
		const FMemberReference& DynamicFunction,
		FGameplayTag AttributeTag,
		const FInstancedStruct& InStruct,
		FInstancedStruct& OutStruct);
	
	static bool GetStructContext(
		USimpleAttributeModifier* OwningModifier,
		const FMemberReference& DynamicFunction,
		FInstancedStruct& Context);

	static bool GetAttributeModifierSideEffectTargets(
		USimpleAttributeModifier* OwningModifier,
		const FMemberReference& DynamicFunction,
		USimpleGameplayAbilityComponent*& OutInstigator,
		USimpleGameplayAbilityComponent*& OutTarget);

	static void ShouldRespondToEvent(
		USimpleAttributeModifier* OwningModifier,
		const FMemberReference& DynamicFunction,
		FGameplayTag EventTag,
		FGameplayTag DomainTag,
		const FInstancedStruct& Payload,
		UObject* Sender,
		bool& ShouldRespond);

	static void ShouldApplyRuntimeAction(UModifierAction* OwningAction, const FMemberReference& DynamicFunction, bool& ShouldRespond);
	static void ApplyRuntimeAction(const UModifierAction* OwningAction, const FMemberReference& DynamicFunction);
	static void RuntimeActionPredictionCorrection(
		const UModifierAction* OwningAction,
		const FMemberReference& DynamicFunction,
		const FAttributeModifierActionScratchPad& ServerInputScratchPad,
		const FAttributeModifierActionScratchPad& ServerOutputScratchPad,
		const FAttributeModifierActionScratchPad& ClientInputScratchPad,
		const FAttributeModifierActionScratchPad& ClientOutputScratchPad);
};
