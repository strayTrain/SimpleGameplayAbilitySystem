#pragma once

#include "CoreMinimal.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeModifier/ModifierActions/Base/ModifierAction.h"
#include "CancelModifierAction.generated.h"
UCLASS()
class SIMPLEGAMEPLAYABILITYSYSTEM_API UCancelModifierAction : public UModifierAction
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<TSubclassOf<USimpleAttributeModifier>> CancelModifiersWithClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTagContainer CancelModifiersWithTags;

	/* ModifierAction overrides */
	virtual EActionPredictionMode GetPredictionMode_Implementation() const override { return EActionPredictionMode::NoPrediction; }
	virtual void ApplyAction_Implementation() override;
};
