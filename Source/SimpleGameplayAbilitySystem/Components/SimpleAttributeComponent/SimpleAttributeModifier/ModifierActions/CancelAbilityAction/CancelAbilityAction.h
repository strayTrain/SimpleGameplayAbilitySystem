#pragma once

#include "CoreMinimal.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeModifier/ModifierActions/Base/ModifierAction.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeModifier/ModifierActions/ModifierActionTypes.h"

#include "CancelAbilityAction.generated.h"

class USimpleGameplayAbility;

UCLASS()
class SIMPLEGAMEPLAYABILITYSYSTEM_API UCancelAbilityAction : public UModifierAction
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<TSubclassOf<USimpleGameplayAbility>> CancelAbilitiesWithClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTagContainer CancelAbilitiesWithTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	EModifierActionComponentTarget ComponentTarget = EModifierActionComponentTarget::Target;

	/* ModifierAction overrides */
	virtual bool SupportsClientPrediction_Implementation() const override { return false; }
	virtual void ApplyAction_Implementation() override;

private:
	void CancelAbilitiesOnComponent(class USimpleAttributeComponent* AttributeComponent);
};
