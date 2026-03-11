#include "RemoveGameplayTagAction.h"

#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeComponent.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeModifier/SimpleAttributeModifier.h"

void URemoveGameplayTagAction::RemoveTagsFromComponent(USimpleAttributeComponent* AttributeComponent)
{
	if (!AttributeComponent)
	{
		return;
	}

	for (const FGameplayTag& Tag : TagsToRemove)
	{
		AttributeComponent->RemoveGameplayTag(Tag);
	}
}

void URemoveGameplayTagAction::ApplyAction_Implementation()
{
	if (!OwningModifier)
	{
		return;
	}

	switch (ComponentTarget)
	{
		case EModifierActionComponentTarget::Target:
			RemoveTagsFromComponent(OwningModifier->TargetAttributeComponent);
			break;

		case EModifierActionComponentTarget::Instigator:
			RemoveTagsFromComponent(OwningModifier->InstigatorAttributeComponent);
			break;

		case EModifierActionComponentTarget::Both:
			RemoveTagsFromComponent(OwningModifier->TargetAttributeComponent);
			RemoveTagsFromComponent(OwningModifier->InstigatorAttributeComponent);
			break;
	}
}
