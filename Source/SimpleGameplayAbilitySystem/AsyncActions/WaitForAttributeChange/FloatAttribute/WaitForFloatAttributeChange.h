#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "SimpleGameplayAbilitySystem/AsyncActions/AsyncActionTypes.h"
#include "SimpleGameplayAbilitySystem/SimpleEventSubsystem/SimpleEventTypes.h"
#include "WaitForFloatAttributeChange.generated.h"

class UWaitForSimpleEvent;
enum class EAttributeType : uint8;
enum class EFloatAttributeValueType : uint8;
class USimpleGameplayAbilityComponent;

UCLASS()
class SIMPLEGAMEPLAYABILITYSYSTEM_API UWaitForFloatAttributeChange : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FSimpleFloatAttributeChangedDelegate OnFloatAttributeChanged;
	
	UFUNCTION(BlueprintCallable, Category = "SimpleGAS|Async Functions", meta=(WorldContext = "WorldContextObject", BlueprintInternalUseOnly=true))
	static UWaitForFloatAttributeChange* WaitForFloatAttributeChange(
		UObject* WorldContextObject,
		USimpleGameplayAbilityComponent* AttributeOwner,
		FGameplayTag AttributeTag,
		bool OnlyTriggerOnce);
	
	virtual void Activate() override;
	virtual void SetReadyToDestroy() override;

protected:
	UFUNCTION()
	void OnFloatAttributeChangedEvent(FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload, UObject* Sender, FGuid EventSubscriptionID);

	UPROPERTY()
	UWaitForSimpleEvent* FloatAttributeChangedEventTask;
	
	TWeakObjectPtr<UWorld> WorldContext;
	TWeakObjectPtr<USimpleGameplayAbilityComponent> AttributeOwner;
	FGameplayTag AttributeTag;
	bool OnlyTriggerOnce;
};
