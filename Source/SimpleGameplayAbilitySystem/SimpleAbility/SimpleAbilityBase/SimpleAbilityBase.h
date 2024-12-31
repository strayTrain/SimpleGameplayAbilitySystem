#pragma once

#include "CoreMinimal.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleAbilityTypes.h"
#include "UObject/Object.h"
#include "SimpleAbilityBase.generated.h"

class USimpleGameplayAbilityComponent;
class USimpleAbilityComponent;

UCLASS(NotBlueprintable)
class SIMPLEGAMEPLAYABILITYSYSTEM_API USimpleAbilityBase : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
	FGuid AbilityInstanceID;
	
	UPROPERTY(BlueprintReadOnly)
	USimpleGameplayAbilityComponent* OwningAbilityComponent;

	UFUNCTION(BlueprintCallable)
	void InitializeAbility(USimpleGameplayAbilityComponent* InOwningAbilityComponent, FGuid InAbilityInstanceID);

	UFUNCTION(BlueprintCallable)
	void TakeStateSnapshot(FSimpleAbilitySnapshot State);

	// Called when the snapshot history locally is ahead of the server (usually in the case of a local predicted ability)
	UFUNCTION(BlueprintImplementableEvent)
	void ClientResolvePastState(FGameplayTag StateTag, FSimpleAbilitySnapshot AuthorityState, FSimpleAbilitySnapshot PredictedState);

	// Called when the snapshot history locally is behind the server (usually in the case of a server initiated ability)
	UFUNCTION(BlueprintImplementableEvent)
	void ClientFastForwardState(FGameplayTag StateTag, FSimpleAbilitySnapshot AuthorityState, FSimpleAbilitySnapshot PredictedState);
};
