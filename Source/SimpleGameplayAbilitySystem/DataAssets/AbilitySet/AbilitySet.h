#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AbilitySet.generated.h"

class USimpleGameplayAbility;
UCLASS()
class SIMPLEGAMEPLAYABILITYSYSTEM_API UAbilitySet : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<TSubclassOf<USimpleGameplayAbility>> AbilitiesToGrant;
};
