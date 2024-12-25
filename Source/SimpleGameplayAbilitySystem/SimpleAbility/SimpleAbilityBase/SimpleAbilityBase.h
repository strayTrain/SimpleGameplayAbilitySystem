#pragma once

#include "CoreMinimal.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleAbilityTypes.h"
#include "UObject/Object.h"
#include "SimpleAbilityBase.generated.h"

class USimpleGameplayAbilityComponent;
struct FAbilityState;
class USimpleAbilityComponent;

UCLASS()
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
	void PushState(FSimpleAbilityState State);

	UFUNCTION(BlueprintImplementableEvent)
	void ClientResolveState(FGameplayTag StateTag, FSimpleAbilityState AuthorityState, FSimpleAbilityState PredictedState);
};
