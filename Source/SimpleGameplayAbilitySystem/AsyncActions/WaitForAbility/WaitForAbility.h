#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "SimpleGameplayAbilitySystem/AsyncActions/AsyncActionTypes.h"
#include "SimpleGameplayAbilitySystem/SimpleEventSubsystem/SimpleEventTypes.h"
#include "SimpleGameplayAbilitySystem/SimpleEventSubsystem/WaitForSimpleEvent/WaitForSimpleEvent.h"
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
	
	UFUNCTION(BlueprintCallable, Category = "SimpleGAS|Async Functions", meta=(DefaultToSelf="ActivatingAbility", WorldContext = "WorldContextObject", BlueprintInternalUseOnly=true))
	static UWaitForAbility* WaitForClientSubAbilityEnd(
		UObject* WorldContextObject,
		USimpleGameplayAbility* ActivatingAbility,
		TSubclassOf<USimpleGameplayAbility> AbilityToActivate,
		FInstancedStruct Context);
	
	UFUNCTION(BlueprintCallable, Category = "SimpleGAS|Async Functions", meta=(DefaultToSelf="ActivatingAbility", WorldContext = "WorldContextObject", BlueprintInternalUseOnly=true))
	static UWaitForAbility* WaitForServerSubAbilityEnd(
		UObject* WorldContextObject,
		USimpleGameplayAbility* ActivatingAbility,
		TSubclassOf<USimpleGameplayAbility> AbilityToActivate,
		FInstancedStruct Payload);

	UFUNCTION(BlueprintCallable, Category = "SimpleGAS|Async Functions", meta=(DefaultToSelf="ActivatingAbility", WorldContext = "WorldContextObject", BlueprintInternalUseOnly=true))
	static UWaitForAbility* WaitForLocalSubAbilityEnd(
		UObject* WorldContextObject,
		USimpleGameplayAbility* ActivatingAbility,
		TSubclassOf<USimpleGameplayAbility> AbilityToActivate,
		FInstancedStruct Payload);
	
	virtual void Activate() override;
	virtual void SetReadyToDestroy() override;

protected:
	UFUNCTION()
	void OnAbilityEndedEventReceived(FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload, UObject* Sender, FGuid EventSubscriptionID);

	UFUNCTION()
	void OnWaitAbilityEndedEventReceived(FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload, UObject* Sender, FGuid EventSubscriptionID);
	
private:
	TWeakObjectPtr<USimpleGameplayAbility> ActivatorAbility;
	TSubclassOf<USimpleGameplayAbility> AbilityClass;
	FGuid ActivatedAbilityID;
	FInstancedStruct AbilityPayload;
	EEventInitiator Activator;
	TWeakObjectPtr<UWorld> WorldContext;
	
	UPROPERTY()
	UWaitForSimpleEvent* AbilityEndedEventTask;
	
	UPROPERTY()
	UWaitForSimpleEvent* WaitAbilityEndedEventTask;
};