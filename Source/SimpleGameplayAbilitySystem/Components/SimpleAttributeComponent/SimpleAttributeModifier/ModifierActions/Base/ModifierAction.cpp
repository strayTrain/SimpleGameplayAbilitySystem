#include "ModifierAction.h"

#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeModifier/SimpleAttributeModifier.h"

float UModifierAction::GetScratchPadValue(const FGameplayTag ScratchPadTag, bool& WasFound) const
{
	for (const FAttributeModifierActionScratchPadValue& Value : OwningModifier->GetModifierActionScratchPad().ScratchpadValues)
	{
		if (Value.ScratchpadTag == ScratchPadTag)
		{
			WasFound = true;
			return Value.ScratchpadValue;
		}
	}

	WasFound = false;
	return 0;
}

bool UModifierAction::HasScratchPadValue(const FGameplayTag ScratchPadTag) const
{
	for (const FAttributeModifierActionScratchPadValue& Value : OwningModifier->GetModifierActionScratchPad().ScratchpadValues)
	{
		if (Value.ScratchpadTag == ScratchPadTag)
		{
			return true;
		}
	}

	return false;
}

bool UModifierAction::HasScratchPadTag(const FGameplayTag ScratchPadTag) const
{
	return ScratchPad.ScratchpadTags.HasTagExact(ScratchPadTag);
}

void UModifierAction::SetScratchPadValue(FGameplayTag ScratchPadTag, float Value)
{
	for (FAttributeModifierActionScratchPadValue& CurrentValue : ScratchPad.ScratchpadValues)
	{
		if (CurrentValue.ScratchpadTag == ScratchPadTag)
		{
			CurrentValue.ScratchpadValue = Value;
			return;
		}
	}

	ScratchPad.ScratchpadValues.Add(FAttributeModifierActionScratchPadValue{ScratchPadTag, Value});
}

void UModifierAction::IncrementScratchPadValue(const FGameplayTag ScratchPadTag, const float IncrementAmount)
{
	for (FAttributeModifierActionScratchPadValue& CurrentValue : ScratchPad.ScratchpadValues)
	{
		if (CurrentValue.ScratchpadTag == ScratchPadTag)
		{
			CurrentValue.ScratchpadValue += IncrementAmount;
			return;
		}
	}

	ScratchPad.ScratchpadValues.Add(FAttributeModifierActionScratchPadValue{ScratchPadTag, IncrementAmount});
}

void UModifierAction::OnClientPredictedCorrection_Implementation(FAttributeModifierActionScratchPad ServerInputScratchPad, FInstancedStruct ServerResult, FAttributeModifierActionScratchPad
                                                                 ClientInputScratchPad, FInstancedStruct ClientResult)
{
	// By default, we undo the client action and re-apply the action with the server's scratchpad input
	OnCancelAction();
	ScratchPad = ServerInputScratchPad;
	FInstancedStruct ActionResult = ApplyAction();
}

void UModifierAction::AddScratchPadTag(const FGameplayTag ScratchPadTag)
{
	ScratchPad.ScratchpadTags.AddTag(ScratchPadTag);
}

void UModifierAction::RemoveScratchPadTag(const FGameplayTag ScratchPadTag)
{
	ScratchPad.ScratchpadTags.RemoveTag(ScratchPadTag);
}
