#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SimpleAbilitySet.generated.h"

class USimpleGameplayAbility;
UCLASS()
class SIMPLEGAMEPLAYABILITYSYSTEM_API USimpleAbilitySet : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<TSubclassOf<USimpleGameplayAbility>> AbilitiesToGrant;
};
