#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "SimpleGameplayAbilitySystem/AsyncActions/AsyncActionTypes.h"
#include "WaitForGameplayTag.generated.h"

class UWaitForSimpleEvent;
class USimpleGameplayAbilityComponent;

UCLASS()
class SIMPLEGAMEPLAYABILITYSYSTEM_API UWaitForGameplayTag : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FSimpleGameplayTagEventDelegate TagAdded;

	UPROPERTY(BlueprintAssignable)
	FSimpleGameplayTagEventDelegate TagRemoved;
	
	UFUNCTION(BlueprintCallable, Category = "SimpleGAS|Async Functions", meta=(WorldContext = "WorldContextObject", BlueprintInternalUseOnly=true))
	static UWaitForGameplayTag* WaitForGameplayTag(
		UObject* WorldContextObject,
		USimpleGameplayAbilityComponent* TagOwner,
		FGameplayTag GameplayTag,
		bool OnlyTriggerOnce);
	
	virtual void Activate() override;
	virtual void SetReadyToDestroy() override;

protected:
	UPROPERTY()
	UWaitForSimpleEvent* SimpleEventTask;
	
	UFUNCTION()
	void OnSimpleEventReceived(FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload, UObject* Sender, FGuid EventSubscriptionID);
	
	TWeakObjectPtr<UWorld> WorldContext;
	TWeakObjectPtr<USimpleGameplayAbilityComponent> TaskOwner;
	FGameplayTag GameplayTag;
	bool OnlyTriggerOnce;
};
