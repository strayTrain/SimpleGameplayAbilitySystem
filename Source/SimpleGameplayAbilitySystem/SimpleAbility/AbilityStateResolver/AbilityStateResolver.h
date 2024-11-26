#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "AbilityStateResolver.generated.h"

struct FAbilityState;
class USimpleAbilityComponent;
class USimpleAbility;

UCLASS(Blueprintable, ClassGroup = (SimpleGAS))
class SIMPLEGAMEPLAYABILITYSYSTEM_API UAbilityStateResolver : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "SimpleGAS|Ability")
	void ResolveState(const FAbilityState& AuthorityState, const FAbilityState& PredictedState);
	virtual void ResolveState_Implementation(const FAbilityState& AuthorityState, const FAbilityState& PredictedState);

protected:
	UPROPERTY(BlueprintReadOnly, Category = "SimpleGAS|Ability")
	USimpleAbility* OwningAbility;

	/* Utility Functions */
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SimpleGAS|Ability")
	USimpleAbilityComponent* GetOwningAbilityComponent() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SimpleGAS|Ability")
	AActor* GetAvatarActor() const;
};
