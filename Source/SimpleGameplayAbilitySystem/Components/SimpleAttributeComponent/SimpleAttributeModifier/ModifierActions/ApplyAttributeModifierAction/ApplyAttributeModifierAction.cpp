#include "ApplyAttributeModifierAction.h"

#include "SimpleGameplayAbilitySystem/BlueprintFunctionLibraries/FunctionSelectors/FunctionSelectors.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeComponent.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeModifier/SimpleAttributeModifier.h"

FInstancedStruct UApplyAttributeModifierAction::ApplyAction_Implementation()
{
	if (!OwningModifier || !OwningModifier->InstigatorAttributeComponent || !OwningModifier->TargetAttributeComponent)
	{
		return FInstancedStruct();
	}

	if (!ModifierClass)
	{
		return FInstancedStruct();
	}

	// Get context from custom function if provided
	FInstancedStruct ModifierContext;
	if (ContextFunction.GetMemberName() != NAME_None)
	{
		UFunctionSelectors::GetStructContext(OwningModifier, ContextFunction, ModifierContext);
	}

	// Apply the modifier using server-initiated approach for safety
	FGuid ModifierID;
	OwningModifier->InstigatorAttributeComponent->ApplyAttributeModifierToTargetServerInitiated(
		ModifierID,
		ModifierClass,
		OwningModifier->TargetAttributeComponent,
		OwningModifier->ModifierMagnitude,
		ModifierContext);

	return FInstancedStruct();
}
