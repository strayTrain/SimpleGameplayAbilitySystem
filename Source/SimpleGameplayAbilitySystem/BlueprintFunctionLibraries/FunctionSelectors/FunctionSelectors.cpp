#include "FunctionSelectors.h"

#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeModifier/SimpleAttributeModifier.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleGameplayAbilityComponent/SimpleGameplayAbilityComponent.h"


bool UFunctionSelectors::GetCustomFloatInputValue(
	USimpleAttributeModifier* OwningModifier,
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

		OwningModifier->ProcessEvent(Function, &Params);
		CustomInputValue = Params.CustomInputValue;
		return true;
	}

	return false;
}

bool UFunctionSelectors::ApplyFloatAttributeOperation(
	USimpleAttributeModifier* OwningModifier, const FMemberReference& DynamicFunction, const FGameplayTag AttributeTag,
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

		// Call ProcessEvent on the OwningModifier to set the correct 'this' context
		OwningModifier->ProcessEvent(Function, &Params);
		EventTagOverride = Params.EventTagOverride;
		NewAttributeValue = Params.NewAttributeValue;
		NewOverflow = Params.NewOverflow;
		return true;
	}

	return false;
}

bool UFunctionSelectors::ModifyStructAttributeValue(
	USimpleAttributeModifier* OwningModifier,
	const FMemberReference& DynamicFunction,
	const FGameplayTag AttributeTag,
	const FInstancedStruct& InStruct,
	FInstancedStruct& OutStruct)
{
	if (UFunction* Function = DynamicFunction.ResolveMember<UFunction>(OwningModifier->GetClass()))
	{
		struct {
			FGameplayTag AttributeTag;
			FInstancedStruct InStruct;
			FInstancedStruct OutStruct;
		} Params = { AttributeTag, InStruct, OutStruct };

		OwningModifier->ProcessEvent(Function, &Params);
		OutStruct = Params.OutStruct;
		return true;
	}

	return false;
}

bool UFunctionSelectors::GetStructContext(
	USimpleAttributeModifier* OwningModifier,
	const FMemberReference& DynamicFunction,
	FInstancedStruct& Context)
{
	if (UFunction* Function = DynamicFunction.ResolveMember<UFunction>(OwningModifier->GetClass()))
	{
		struct {
			FInstancedStruct Context;
		} Params = { Context };

		OwningModifier->ProcessEvent(Function, &Params);
		Context = Params.Context;
		return true;
	}

	return false;
}

bool UFunctionSelectors::GetAttributeModifierSideEffectTargets(
	USimpleAttributeModifier* OwningModifier,
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

		OwningModifier->ProcessEvent(Function, &Params);
		OutInstigator = Params.Instigator;
		OutTarget = Params.Target;
		return true;
	}

	return false;
}

void UFunctionSelectors::ShouldRespondToEvent(
	USimpleAttributeModifier* OwningModifier,
	const FMemberReference& DynamicFunction,
	const FGameplayTag EventTag,
	const FGameplayTag DomainTag,
	const FInstancedStruct& Payload,
	UObject* Sender,
	bool& ShouldRespond)
{
	if (!OwningModifier)
	{
		ShouldRespond = false;
		return;
	}

	if (UFunction* Function = DynamicFunction.ResolveMember<UFunction>(OwningModifier->GetClass()))
	{
		struct {
			// Input arguments
			FGameplayTag EventTag;
			FGameplayTag DomainTag;
			FInstancedStruct Payload;
			UObject* Sender;
			// Output argument
			bool ShouldRespond;
		} Params = { EventTag, DomainTag, Payload, Sender, ShouldRespond };

		OwningModifier->ProcessEvent(Function, &Params);
		ShouldRespond = Params.ShouldRespond;
		return;
	}

	ShouldRespond = false;
}

void UFunctionSelectors::ShouldApplyRuntimeAction(UModifierAction* OwningAction, const FMemberReference& DynamicFunction, bool& ShouldRespond)
{
	if (!OwningAction || !OwningAction->GetOwningModifier())
	{
		ShouldRespond = false;
		return;
	}

	if (UFunction* Function = DynamicFunction.ResolveMember<UFunction>(OwningAction->GetOwningModifier()->GetClass()))
	{
		struct {
			// Input arguments
			UModifierAction* OwningAction;
			// Output argument
			bool ShouldRespond;
		} Params = { OwningAction, ShouldRespond };

		OwningAction->GetOwningModifier()->ProcessEvent(Function, &Params);
		ShouldRespond = Params.ShouldRespond;
		return;
	}

	// If no function is specified, assume we always want to run this action
	ShouldRespond = true;
}

void UFunctionSelectors::ApplyRuntimeAction(const UModifierAction* OwningAction, const FMemberReference& DynamicFunction)
{
	if (!OwningAction || !OwningAction->GetOwningModifier())
	{
		return;
	}

	if (UFunction* Function = DynamicFunction.ResolveMember<UFunction>(OwningAction->GetOwningModifier()->GetClass()))
	{
		struct {
			// Input arguments
			UModifierAction* OwningAction;
		} Params = { const_cast<UModifierAction*>(OwningAction) };

		OwningAction->GetOwningModifier()->ProcessEvent(Function, &Params);
	}
}

void UFunctionSelectors::RuntimeActionPredictionCorrection(
	const UModifierAction* OwningAction,
	const FMemberReference& DynamicFunction,
	const FAttributeModifierActionScratchPad& ServerInputScratchPad,
	const FAttributeModifierActionScratchPad& ServerOutputScratchPad,
	const FAttributeModifierActionScratchPad& ClientInputScratchPad,
	const FAttributeModifierActionScratchPad& ClientOutputScratchPad)
{
	if (!OwningAction || !OwningAction->GetOwningModifier())
	{
		return;
	}

	if (UFunction* Function = DynamicFunction.ResolveMember<UFunction>(OwningAction->GetOwningModifier()->GetClass()))
	{
		struct {
			// Input arguments
			UModifierAction* OwningAction;
			FAttributeModifierActionScratchPad ServerInputScratchPad;
			FAttributeModifierActionScratchPad ServerOutputScratchPad;
			FAttributeModifierActionScratchPad ClientInputScratchPad;
			FAttributeModifierActionScratchPad ClientOutputScratchPad;
		} Params = {
			const_cast<UModifierAction*>(OwningAction),
			ServerInputScratchPad,
			ServerOutputScratchPad,
			ClientInputScratchPad,
			ClientOutputScratchPad
		};

		OwningAction->GetOwningModifier()->ProcessEvent(Function, &Params);
	}
}

bool UFunctionSelectors::CalculateStackMagnitude(
	USimpleAttributeModifier* OwningModifier,
	const FMemberReference& DynamicFunction,
	const int32 CurrentStackCount,
	const float BaseMagnitude,
	float& ScaledMagnitude)
{
	if (!OwningModifier)
	{
		ScaledMagnitude = BaseMagnitude;
		return false;
	}

	if (UFunction* Function = DynamicFunction.ResolveMember<UFunction>(OwningModifier->GetClass()))
	{
		struct {
			// Input arguments
			int32 CurrentStackCount;
			float BaseMagnitude;
			// Output argument
			float ScaledMagnitude;
		} Params = { CurrentStackCount, BaseMagnitude, BaseMagnitude };

		OwningModifier->ProcessEvent(Function, &Params);
		ScaledMagnitude = Params.ScaledMagnitude;
		return true;
	}

	// If no function is specified, default to linear scaling
	ScaledMagnitude = BaseMagnitude * CurrentStackCount;
	return false;
}
