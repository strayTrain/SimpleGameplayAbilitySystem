#include "ChangeStructAttributeAction.h"

#include "SimpleGameplayAbilitySystem/BlueprintFunctionLibraries/FunctionSelectors/FunctionSelectors.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeComponent.h"

bool UChangeStructAttributeAction::CanApply_Implementation() const
{
	return OwningModifier->TargetAttributeComponent->HasStructAttribute(AttributeToModify);
}

FInstancedStruct UChangeStructAttributeAction::ApplyAction_Implementation()
{
	bool WasFound = false;
	const FInstancedStruct CurrentValue = OwningModifier->TargetAttributeComponent->GetStructAttributeValue(AttributeToModify,WasFound);
	FInstancedStruct NewValue;

	if (UFunctionSelectors::ModifyStructAttributeValue(OwningModifier, StructModificationFunction, AttributeToModify, CurrentValue, NewValue))
	{
		OwningModifier->TargetAttributeComponent->SetStructAttributeValue(AttributeToModify, NewValue);
	}

	return NewValue;
}
