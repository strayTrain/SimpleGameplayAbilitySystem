#pragma once

#include "CoreMinimal.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleAttributeModifier/SimpleAttributeModifierTypes.h"
#include "UObject/Object.h"
#include "ModifierAction.generated.h"

UCLASS(Abstract, BlueprintType, EditInlineNew, DefaultToInstanced, CollapseCategories)
class SIMPLEGAMEPLAYABILITYSYSTEM_API UModifierAction : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Config", meta = (DisplayPriority = 0))
	FString Description = "New Modifier Action";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Config", meta = (DisplayPriority = 1))
	EAttributeModifierApplicationPolicy ApplicationPolicy = EAttributeModifierApplicationPolicy::ApplyClientPredicted;
	
	UPROPERTY(EditDefaultsOnly, Category="Config", meta = (DisplayPriority = 2, Bitmask, BitmaskEnum = "/Script/SimpleGameplayAbilitySystem.EAttributeModifierPhase"))
	int32 ApplicationPhases = 1 << 1;
	
	UFUNCTION(BlueprintNativeEvent, Category="Modifier")
	bool ShouldApply(EAttributeModifierPhase Phase, USimpleAttributeModifier* OwningModifier) const;
	virtual bool ShouldApply_Implementation(EAttributeModifierPhase Phase, USimpleAttributeModifier* OwningModifier) const { return true; }

	UFUNCTION(BlueprintNativeEvent, Category="Modifier")
	bool ApplyAction(USimpleAttributeModifier* OwningModifier);
	virtual bool ApplyAction_Implementation(USimpleAttributeModifier* OwningModifier) { return true; }
};
