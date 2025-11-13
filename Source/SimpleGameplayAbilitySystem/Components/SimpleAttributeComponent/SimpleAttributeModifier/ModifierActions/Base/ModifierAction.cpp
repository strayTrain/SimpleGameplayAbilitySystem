#include "ModifierAction.h"

#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeModifier/SimpleAttributeModifier.h"

FInstancedStruct UModifierAction::GetOwningModifierContext() const
{
	return OwningModifier ? OwningModifier->ModifierContext : FInstancedStruct();
}

USimpleAttributeComponent* UModifierAction::GetOwningModifierInstigator()
{
	return OwningModifier ? OwningModifier->InstigatorAttributeComponent : nullptr;
}

USimpleAttributeComponent* UModifierAction::GetOwningModifierTarget()
{
	return OwningModifier ? OwningModifier->TargetAttributeComponent : nullptr;
}

FAttributeModifierActionScratchPad& UModifierAction::GetScratchPad()
{
	if (bOverrideScratchPadSource)
	{
		return OverrideScratchPad;
	}
	return OwningModifier->GetModifierActionScratchPad(); 
}

float UModifierAction::GetScratchPadValue(const FGameplayTag ScratchPadTag, bool& WasFound)
{
	for (const FAttributeModifierActionScratchPadValue& Value : GetScratchPad().ScratchpadValues)
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

bool UModifierAction::HasScratchPadValue(const FGameplayTag ScratchPadTag)
{
	for (const FAttributeModifierActionScratchPadValue& Value : GetScratchPad().ScratchpadValues)
	{
		if (Value.ScratchpadTag == ScratchPadTag)
		{
			return true;
		}
	}

	return false;
}

bool UModifierAction::HasScratchPadTag(const FGameplayTag ScratchPadTag)
{
	return GetScratchPad().ScratchpadTags.HasTagExact(ScratchPadTag);
}

void UModifierAction::SetScratchPadValue(FGameplayTag ScratchPadTag, float Value)
{
	for (FAttributeModifierActionScratchPadValue& CurrentValue : GetScratchPad().ScratchpadValues)
	{
		if (CurrentValue.ScratchpadTag == ScratchPadTag)
		{
			CurrentValue.ScratchpadValue = Value;
			return;
		}
	}

	GetScratchPad().ScratchpadValues.Add(FAttributeModifierActionScratchPadValue{ScratchPadTag, Value});
}

void UModifierAction::IncrementScratchPadValue(const FGameplayTag ScratchPadTag, const float IncrementAmount)
{
	for (FAttributeModifierActionScratchPadValue& CurrentValue : GetScratchPad().ScratchpadValues)
	{
		if (CurrentValue.ScratchpadTag == ScratchPadTag)
		{
			CurrentValue.ScratchpadValue += IncrementAmount;
			return;
		}
	}

	GetScratchPad().ScratchpadValues.Add(FAttributeModifierActionScratchPadValue{ScratchPadTag, IncrementAmount});
}

FInstancedStruct UModifierAction::GetScratchPadStruct(FGameplayTag ScratchPadTag, bool& WasFound)
{
	for (FAttributeModifierActionScratchPadStruct StructEntry : GetScratchPad().ScratchpadStructs)
	{
		if (StructEntry.ScratchpadTag == ScratchPadTag)
		{
			WasFound = true;
			return StructEntry.ScratchpadStruct;
		}
	}
	WasFound = false;
	return FInstancedStruct();
}

void UModifierAction::SetScratchPadStruct(FGameplayTag ScratchPadTag, FInstancedStruct StructValue)
{
	if (!StructValue.IsValid() || !StructValue.GetScriptStruct())
	{
		return;
	}

	// Check for existing entry
	for (FAttributeModifierActionScratchPadStruct& StructEntry : GetScratchPad().ScratchpadStructs)
	{
		if (StructEntry.ScratchpadTag == ScratchPadTag)
		{
			StructEntry.ScratchpadStruct = StructValue;
			return;
		}
	}

	GetScratchPad().ScratchpadStructs.Add(FAttributeModifierActionScratchPadStruct{ScratchPadTag, StructValue});
}

void UModifierAction::AddScratchPadTag(const FGameplayTag ScratchPadTag)
{
	GetScratchPad().ScratchpadTags.AddTag(ScratchPadTag);
}

void UModifierAction::RemoveScratchPadTag(const FGameplayTag ScratchPadTag)
{
	GetScratchPad().ScratchpadTags.RemoveTag(ScratchPadTag);
}

void UModifierAction::OverrideScratchPadSource(const FAttributeModifierActionScratchPad& ScratchPadSource)
{
	bOverrideScratchPadSource = true;
	OverrideScratchPad = ScratchPadSource;
}

void UModifierAction::OnClientPredictedCorrection_Implementation(
	FAttributeModifierActionScratchPad ServerInputScratchPad,
	FAttributeModifierActionScratchPad ServerOutputScratchPad,
	FAttributeModifierActionScratchPad ClientInputScratchPad,
	FAttributeModifierActionScratchPad ClientOutputScratchPad)
{
	// By default, we undo the client action and re-apply the action with the server's scratchpad input
	OnCancelAction();
	
	OverrideScratchPadSource(ServerInputScratchPad);
	ApplyAction();
	ClearScratchPadSourceOverride();
}
