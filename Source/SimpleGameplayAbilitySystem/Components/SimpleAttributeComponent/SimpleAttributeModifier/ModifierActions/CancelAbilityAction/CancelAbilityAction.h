#pragma once

#include "CoreMinimal.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeModifier/ModifierActions/Base/ModifierAction.h"

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
};
