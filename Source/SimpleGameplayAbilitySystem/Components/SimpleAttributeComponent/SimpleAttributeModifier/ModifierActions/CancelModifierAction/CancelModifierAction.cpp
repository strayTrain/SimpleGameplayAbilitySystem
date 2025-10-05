#include "CancelModifierAction.h"

#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeComponent.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeModifier/SimpleAttributeModifier.h"

FInstancedStruct UCancelModifierAction::ApplyAction_Implementation()
{
	if (!OwningModifier->TargetAttributeComponent)
	{
		return FInstancedStruct();
	}

	FInstancedStruct CancellationContext = FInstancedStruct();

	// Cancel modifiers by class
	for (const TSubclassOf<USimpleAttributeModifier>& ModifierClass : CancelModifiersWithClass)
	{
		if (!ModifierClass)
		{
			continue;
		}

		OwningModifier->TargetAttributeComponent->CancelAttributeModifiersWithClass(ModifierClass);
	}

	// Cancel modifiers by tags
	if (!CancelModifiersWithTags.IsEmpty())
	{
		OwningModifier->TargetAttributeComponent->CancelAttributeModifiersWithTags(CancelModifiersWithTags);
	}

	return FInstancedStruct();
}
