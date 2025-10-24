#pragma once

#include "CoreMinimal.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeModifier/ModifierActions/Base/ModifierAction.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeModifier/ModifierActions/ModifierActionTypes.h"
#include "ActivateGameplayAbilityAction.generated.h"

class USimpleGameplayAbility;

UCLASS()
class SIMPLEGAMEPLAYABILITYSYSTEM_API UActivateGameplayAbilityAction : public UModifierAction
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<USimpleGameplayAbility> AbilityClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	EModifierActionComponentTarget ComponentTarget = EModifierActionComponentTarget::Target;

	/* ModifierAction overrides */
	virtual void ApplyAction_Implementation() override;

private:
	void ActivateAbilityOnComponent(class USimpleAttributeComponent* AttributeComponent);
};
