#include "EndModifierAction.h"

#include "SimpleGameplayAbilitySystem/BlueprintFunctionLibraries/FunctionSelectors/FunctionSelectors.h"
#include "SimpleGameplayAbilitySystem/DefaultTags/DefaultTags.h"

void UEndModifierAction::ApplyAction_Implementation()
{
	bool bShouldEnd = true;
	UFunctionSelectors::ShouldApplyRuntimeAction(this, ShouldEndModifier, bShouldEnd);

	if (bShouldEnd)
	{
		OwningModifier->EndModifier(FDefaultTags::AttributeModifierEnded(), FInstancedStruct());
	}
}
