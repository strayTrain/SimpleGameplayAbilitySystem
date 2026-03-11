#include "CancelSelfModifierAction.h"

#include "SimpleGameplayAbilitySystem/BlueprintFunctionLibraries/FunctionSelectors/FunctionSelectors.h"
#include "SimpleGameplayAbilitySystem/DefaultTags/DefaultTags.h"

void UCancelSelfModifierAction::ApplyAction_Implementation()
{
	bool bShouldCancel = true;
	UFunctionSelectors::ShouldApplyRuntimeAction(this, ShouldCancelModifier, bShouldCancel);

	if (bShouldCancel)
	{
		OwningModifier->CancelModifier(FDefaultTags::AttributeModifierCancelled(), FInstancedStruct());
	}
}
