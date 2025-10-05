#include "CancelAbilityAction.h"

#include "SimpleGameplayAbilitySystem/Components/Interfaces/SimpleAbilitySystemInterfaces.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleGameplayAbilityComponent/SimpleGameplayAbilityComponent.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeComponent.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeModifier/SimpleAttributeModifier.h"
#include "SimpleGameplayAbilitySystem/Module/SimpleGameplayAbilitySystem.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleGameplayAbility/SimpleGameplayAbility.h"

void UCancelAbilityAction::CancelAbilitiesOnComponent(USimpleAttributeComponent* AttributeComponent)
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
		SIMPLE_LOG(AttributeComponent, TEXT("[UCancelAbilityAction::CancelAbilitiesOnComponent]: Owner does not implement IAbilityComponentInterface."));
		return;
	}

	USimpleGameplayAbilityComponent* AbilityComponent = IAbilityComponentInterface::Execute_GetSimpleAbilityComponent(Owner);
	if (!AbilityComponent)
	{
		SIMPLE_LOG(AttributeComponent, TEXT("[UCancelAbilityAction::CancelAbilitiesOnComponent]: GetSimpleAbilityComponent returned null."));
		return;
	}

	FInstancedStruct CancellationContext = FInstancedStruct();

	// Cancel abilities by class
	for (const TSubclassOf<USimpleGameplayAbility>& AbilityClass : CancelAbilitiesWithClass)
	{
		if (!AbilityClass)
		{
			continue;
		}

		AbilityComponent->CancelAbilitiesWithClass(AbilityClass, CancellationContext);
	}

	// Cancel abilities by tags
	if (!CancelAbilitiesWithTags.IsEmpty())
	{
		AbilityComponent->CancelAbilitiesWithTags(CancelAbilitiesWithTags, CancellationContext);
	}
}

FInstancedStruct UCancelAbilityAction::ApplyAction_Implementation()
{
	if (!OwningModifier)
	{
		return FInstancedStruct();
	}

	switch (ComponentTarget)
	{
		case EModifierActionComponentTarget::Target:
			CancelAbilitiesOnComponent(OwningModifier->TargetAttributeComponent);
			break;

		case EModifierActionComponentTarget::Instigator:
			CancelAbilitiesOnComponent(OwningModifier->InstigatorAttributeComponent);
			break;

		case EModifierActionComponentTarget::Both:
			CancelAbilitiesOnComponent(OwningModifier->TargetAttributeComponent);
			CancelAbilitiesOnComponent(OwningModifier->InstigatorAttributeComponent);
			break;
	}

	return FInstancedStruct();
}
