#include "ChangeStructAttributeAction.h"

#include "SimpleGameplayAbilitySystem/BlueprintFunctionLibraries/FunctionSelectors/FunctionSelectors.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeComponent.h"

bool UChangeStructAttributeAction::CanApply_Implementation() const
{
	return OwningModifier->TargetAttributeComponent->HasStructAttribute(AttributeToModify);
}

void UChangeStructAttributeAction::ApplyAction_Implementation()
{
	bool WasFound = false;
	const FInstancedStruct CurrentValue = OwningModifier->TargetAttributeComponent->GetStructAttributeValue(AttributeToModify,WasFound);
	
	// Cache for rollback
	CachedPreviousValue = CurrentValue;
	
	FInstancedStruct NewValue;

	if (UFunctionSelectors::ModifyStructAttributeValue(OwningModifier, StructModificationFunction, AttributeToModify, CurrentValue, NewValue))
	{
		OwningModifier->TargetAttributeComponent->SetStructAttributeValue(AttributeToModify, NewValue);
	}
}

void UChangeStructAttributeAction::OnCancelAction_Implementation()
{
	if (OwningModifier && OwningModifier->TargetAttributeComponent)
	{
		OwningModifier->TargetAttributeComponent->SetStructAttributeValue(AttributeToModify, CachedPreviousValue);
	}
}
