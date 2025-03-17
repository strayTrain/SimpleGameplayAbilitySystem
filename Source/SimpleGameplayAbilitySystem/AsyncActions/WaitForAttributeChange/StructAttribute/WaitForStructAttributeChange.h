#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "SimpleGameplayAbilitySystem/AsyncActions/AsyncActionTypes.h"
#include "WaitForStructAttributeChange.generated.h"

class UWaitForSimpleEvent;
class USimpleGameplayAbilityComponent;

UCLASS()
class SIMPLEGAMEPLAYABILITYSYSTEM_API UWaitForStructAttributeChange : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FSimpleStructAttributeChangedDelegate OnStructAttributeChanged;
	
	UFUNCTION(BlueprintCallable, Category = "SimpleGAS|Async Functions", meta=(WorldContext = "WorldContextObject", BlueprintInternalUseOnly=true))
	static UWaitForStructAttributeChange* WaitForStructAttributeChange(
		UObject* WorldContextObject,
		USimpleGameplayAbilityComponent* AttributeOwner,
		FGameplayTag AttributeTag,
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
	FGameplayTag AttributeTag;
	bool OnlyTriggerOnce;
};
