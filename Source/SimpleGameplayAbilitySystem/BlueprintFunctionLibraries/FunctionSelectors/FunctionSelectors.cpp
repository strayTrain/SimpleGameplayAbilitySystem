#include "FunctionSelectors.h"

#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleAttributeModifier/SimpleAttributeModifier.h"
#include "SimpleGameplayAbilitySystem/SimpleGameplayAbilityComponent/SimpleGameplayAbilityComponent.h"


bool UFunctionSelectors::GetCustomFloatInputValue(
	const USimpleAttributeModifier* OwningModifier,
	const FMemberReference& DynamicFunction,
	const FGameplayTag AttributeTag,
	float& CustomInputValue)
{
	if (UFunction* Function = DynamicFunction.ResolveMember<UFunction>(OwningModifier->GetClass()))
	{
		struct {
			FGameplayTag AttributeTag;
			float CustomInputValue;
		} Params = { AttributeTag, 0.0f };

		OwningModifier->OwningAbilityComponent->ProcessEvent(Function, &Params);
		CustomInputValue = Params.CustomInputValue;
		return true;
	}

	return false;
}

bool UFunctionSelectors::ApplyFloatAttributeOperation(
	const USimpleAttributeModifier* OwningModifier, const FMemberReference& DynamicFunction, const FGameplayTag AttributeTag,
	const float CurrentAttributeValue, const float OperationInputValue, const float CurrentOverflow,
	FGameplayTag& EventTagOverride, float& NewAttributeValue, float& NewOverflow)
{
	if (UFunction* Function = DynamicFunction.ResolveMember<UFunction>(OwningModifier->GetClass()))
	{
		struct {
			// Input arguments
			FGameplayTag AttributeTag;
			float CurrentAttributeValue;
			float OperationInputValue;
			float CurrentOverflow;
			// Output arguments
			FGameplayTag EventTagOverride;
			float NewAttributeValue;
			float NewOverflow;
		} Params = { AttributeTag, CurrentAttributeValue, OperationInputValue, CurrentOverflow, EventTagOverride, NewAttributeValue, NewOverflow };

		OwningModifier->OwningAbilityComponent->ProcessEvent(Function, &Params);
		EventTagOverride = Params.EventTagOverride;
		NewAttributeValue = Params.NewAttributeValue;
		NewOverflow = Params.NewOverflow;
		return true;
	}

	return false;
}

bool UFunctionSelectors::ModifyStructAttributeValue(
	const USimpleAttributeModifier* OwningModifier,
	const FMemberReference& DynamicFunction, const FGameplayTag AttributeTag, const FInstancedStruct& InStruct,
	FGameplayTagContainer& EventTags, FInstancedStruct& OutStruct)
{
	if (UFunction* Function = DynamicFunction.ResolveMember<UFunction>(OwningModifier->GetClass()))
	{
		struct {
			FGameplayTag AttributeTag;
			FInstancedStruct InStruct;
			FGameplayTagContainer EventTags;
			FInstancedStruct OutStruct;
		} Params = { AttributeTag, InStruct, EventTags, OutStruct };

		OwningModifier->OwningAbilityComponent->ProcessEvent(Function, &Params);
		OutStruct = Params.OutStruct;
		return true;
	}

	return false;
}

bool UFunctionSelectors::GetStructContext(
	const USimpleAttributeModifier* OwningModifier,
	const FMemberReference& DynamicFunction,
	FInstancedStruct& Context)
{
	if (UFunction* Function = DynamicFunction.ResolveMember<UFunction>(OwningModifier->GetClass()))
	{
		struct {
			FInstancedStruct Context;
		} Params = { Context };

		OwningModifier->OwningAbilityComponent->ProcessEvent(Function, &Params);
		Context = Params.Context;
		return true;
	}

	return false;
}

bool UFunctionSelectors::GetAttributeModifierSideEffectTargets(
	const USimpleAttributeModifier* OwningModifier,
	const FMemberReference& DynamicFunction,
	USimpleGameplayAbilityComponent*& OutInstigator,
	USimpleGameplayAbilityComponent*& OutTarget)
{
	if (UFunction* Function = DynamicFunction.ResolveMember<UFunction>(OwningModifier->GetClass()))
	{
		struct {
			USimpleGameplayAbilityComponent* Instigator;
			USimpleGameplayAbilityComponent* Target;
		} Params = { OutInstigator, OutTarget };

		OwningModifier->OwningAbilityComponent->ProcessEvent(Function, &Params);
		OutInstigator = Params.Instigator;
		OutTarget = Params.Target;
		return true;
	}

	return false;
}
