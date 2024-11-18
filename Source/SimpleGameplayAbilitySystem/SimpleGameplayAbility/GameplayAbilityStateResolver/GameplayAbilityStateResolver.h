#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayAbilityStateResolver.generated.h"

struct FAbilityState;
class USimpleGameplayAbilityComponent;
class USimpleGameplayAbility;

UCLASS(Blueprintable, ClassGroup = (SimpleGAS))
class SIMPLEGAMEPLAYABILITYSYSTEM_API UGameplayAbilityStateResolver : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "SimpleGAS|Ability")
	void ResolveState(const FAbilityState& AuthorityState, const FAbilityState& PredictedState);
	virtual void ResolveState_Implementation(const FAbilityState& AuthorityState, const FAbilityState& PredictedState);

protected:
	UPROPERTY(BlueprintReadOnly, Category = "SimpleGAS|Ability")
	USimpleGameplayAbility* OwningAbility;

	/* Utility Functions */
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SimpleGAS|Ability")
	USimpleGameplayAbilityComponent* GetOwningAbilityComponent() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SimpleGAS|Ability")
	AActor* GetAvatarActor() const;
};
