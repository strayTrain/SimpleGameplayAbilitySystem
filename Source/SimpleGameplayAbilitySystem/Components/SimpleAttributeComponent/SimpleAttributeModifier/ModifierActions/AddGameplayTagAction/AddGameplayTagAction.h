#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeModifier/ModifierActions/Base/ModifierAction.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeModifier/ModifierActions/ModifierActionTypes.h"
#include "AddGameplayTagAction.generated.h"

UCLASS(BlueprintType, EditInlineNew)
class SIMPLEGAMEPLAYABILITYSYSTEM_API UAddGameplayTagAction : public UModifierAction
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Config")
	FGameplayTagContainer TagsToAdd;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Config")
	EModifierActionComponentTarget ComponentTarget = EModifierActionComponentTarget::Target;

	virtual void ApplyAction_Implementation() override;

private:
	void AddTagsToComponent(class USimpleAttributeComponent* AttributeComponent);
};
