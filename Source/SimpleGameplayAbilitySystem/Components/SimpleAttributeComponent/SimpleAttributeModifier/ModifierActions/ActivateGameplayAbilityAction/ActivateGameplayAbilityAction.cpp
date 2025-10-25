#include "ActivateGameplayAbilityAction.h"

#include "SimpleGameplayAbilitySystem/Components/Interfaces/SimpleAbilitySystemInterfaces.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeComponent.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeModifier/SimpleAttributeModifier.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleGameplayAbilityComponent/SimpleGameplayAbilityComponent.h"
#include "SimpleGameplayAbilitySystem/Module/SimpleGameplayAbilitySystem.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleGameplayAbility/SimpleGameplayAbility.h"

void UActivateGameplayAbilityAction::ActivateAbilityOnComponent(USimpleAttributeComponent* AttributeComponent)
{
	if (!AttributeComponent)
	{
		return;
	}

	AActor* Owner = AttributeComponent->GetOwner();
	if (!Owner)
	{
		return;
	}

	// Check if the owner implements IAbilityComponentInterface
	if (!Owner->Implements<UAbilityComponentInterface>())
	{
		SIMPLE_LOG(AttributeComponent, TEXT("[UActivateGameplayAbilityAction::ActivateAbilityOnComponent]: Owner does not implement IAbilityComponentInterface."));
		return;
	}

	USimpleGameplayAbilityComponent* AbilityComponent = IAbilityComponentInterface::Execute_GetSimpleAbilityComponent(Owner);
	if (!AbilityComponent)
	{
		SIMPLE_LOG(AttributeComponent, TEXT("[UActivateGameplayAbilityAction::ActivateAbilityOnComponent]: GetSimpleAbilityComponent returned null."));
		return;
	}

	// Activate the ability using server-initiated approach
	FGuid AbilityID;
	AbilityComponent->ActivateAbilityServerInitiatedWithContext(AbilityClass, OwningModifier->ModifierContext, AbilityID);
}

void UActivateGameplayAbilityAction::ApplyAction_Implementation()
{
	if (!OwningModifier)
	{
		return;
	}

	if (!AbilityClass)
	{
		return;
	}

	switch (ComponentTarget)
	{
		case EModifierActionComponentTarget::Target:
			ActivateAbilityOnComponent(OwningModifier->TargetAttributeComponent);
			break;

		case EModifierActionComponentTarget::Instigator:
			ActivateAbilityOnComponent(OwningModifier->InstigatorAttributeComponent);
			break;

		case EModifierActionComponentTarget::Both:
			ActivateAbilityOnComponent(OwningModifier->TargetAttributeComponent);
			ActivateAbilityOnComponent(OwningModifier->InstigatorAttributeComponent);
			break;
	}
}
