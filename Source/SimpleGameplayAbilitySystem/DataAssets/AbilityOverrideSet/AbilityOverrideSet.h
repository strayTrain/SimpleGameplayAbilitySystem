#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleGameplayAbility/SimpleGameplayAbility.h"
#include "AbilityOverrideSet.generated.h"

class USimpleGameplayAbility;

USTRUCT(BlueprintType)
struct FAbilityOverride
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<USimpleGameplayAbility> OriginalAbility;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<USimpleGameplayAbility> OverrideAbility;

	bool operator==(const FAbilityOverride& Other) const
	{
		return OriginalAbility == Other.OriginalAbility;
	}
};

UCLASS()
class SIMPLEGAMEPLAYABILITYSYSTEM_API UAbilityOverrideSet : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FAbilityOverride> AbilityOverrides;
};
