#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "SimpleGameplayAbilitySystem/AsyncActions/AsyncActionTypes.h"
#include "SimpleGameplayAbilitySystem/SimpleEventSubsystem/SimpleEventTypes.h"
#include "WaitForAbility.generated.h"

class USimpleGameplayAbilityComponent;
class USimpleGameplayAbility;

UCLASS()
class SIMPLEGAMEPLAYABILITYSYSTEM_API UWaitForAbility : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FEventAbilitySenderDelegate OnAbilityEnded;

	UFUNCTION(BlueprintCallable,  meta=(WorldContext = "WorldContextObject", BlueprintInternalUseOnly=true))
	static UWaitForAbility* WaitForClientAbility(
		UObject* WorldContextObject,
		USimpleGameplayAbility* ActivatingAbility,
		TSubclassOf<USimpleGameplayAbility> AbilityToActivate, FInstancedStruct Payload);

	UFUNCTION(BlueprintCallable,  meta=(WorldContext = "WorldContextObject", BlueprintInternalUseOnly=true))
	static UWaitForAbility* WaitForServerAbility(
		UObject* WorldContextObject,
		USimpleGameplayAbility* ActivatingAbility,
		TSubclassOf<USimpleGameplayAbility> AbilityToActivate, FInstancedStruct Payload);

	virtual void Activate() override;

protected:
	UFUNCTION()
	void OnAbilityEndedEventReceived(FGameplayTag AbilityTag, FGameplayTag DomainTag, FInstancedStruct Payload, AActor* Sender = nullptr);

	UFUNCTION()
	void OnWaitAbilityEndedEventReceived(FGameplayTag AbilityTag, FGameplayTag DomainTag, FInstancedStruct Payload, AActor* Sender = nullptr);
	
private:
	TWeakObjectPtr<USimpleGameplayAbility> ActivatorAbility;
	TSubclassOf<USimpleGameplayAbility> AbilityClass;
	FGuid ActivatedAbilityID;
	FInstancedStruct AbilityPayload;
	EEventInitiator Activator;
	TWeakObjectPtr<UWorld> WorldContext;
	FSimpleEventDelegate AbilityEndedDelegate;
	FSimpleEventDelegate WaitForAbilityEndedDelegate;
};
