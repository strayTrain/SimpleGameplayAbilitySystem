#include "AddGameplayTagAction.h"

#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeComponent.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeModifier/SimpleAttributeModifier.h"

void UAddGameplayTagAction::AddTagsToComponent(USimpleAttributeComponent* AttributeComponent)
{
	if (!AttributeComponent)
	{
		return;
	}

	for (const FGameplayTag& Tag : TagsToAdd)
	{
		AttributeComponent->AddGameplayTag(Tag);
	}
}

void UAddGameplayTagAction::ApplyAction_Implementation()
{
	if (!OwningModifier)
	{
		return;
	}

	switch (ComponentTarget)
	{
		case EModifierActionComponentTarget::Target:
			AddTagsToComponent(OwningModifier->TargetAttributeComponent);
			break;

		case EModifierActionComponentTarget::Instigator:
			AddTagsToComponent(OwningModifier->InstigatorAttributeComponent);
			break;

		case EModifierActionComponentTarget::Both:
			AddTagsToComponent(OwningModifier->TargetAttributeComponent);
			AddTagsToComponent(OwningModifier->InstigatorAttributeComponent);
			break;
	}
}
