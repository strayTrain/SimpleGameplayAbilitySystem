#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "SimpleGameplayAbilitySystem/SimpleGameplayAbilityComponent/SimpleAbilityComponentTypes.h"
#include "SimpleGameplayAbilitySystem/SimpleEventSubsystem/SimpleEventTypes.h"
#include "WaitForFloatAttributeChange.generated.h"

class USimpleGameplayAbilityComponent;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FSimpleFloatAttributeChangedDelegate,
	EAttributeValueType, ChangedValueType,
	float, NewValue
);

UCLASS()
class SIMPLEGAMEPLAYABILITYSYSTEM_API UWaitForFloatAttributeChange : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, meta=(WorldContext = "WorldContextObject", BlueprintInternalUseOnly=true, AdvancedDisplay=4))
	static UWaitForFloatAttributeChange* WaitForFloatAttributeChange(
		UObject* WorldContextObject,
		USimpleGameplayAbilityComponent* AttributeOwner,
		FGameplayTag AttributeTag,
		bool OnlyTriggerOnce = false,
		bool ListenForBaseValueChange = true,
		bool ListenForCurrentValueChange = true,
		bool ListenForMaxBaseValueChange = true,
		bool ListenForMinBaseValueChange = true,
		bool ListenForMaxCurrentValueChange = true,
		bool ListenForMinCurrentValueChange = true);

	UPROPERTY(BlueprintAssignable)
	FSimpleFloatAttributeChangedDelegate OnFloatAttributeChanged;
	
	virtual void Activate() override;

protected:
	UFUNCTION()
	void OnSimpleEventReceived(FGameplayTag AbilityTag, FGameplayTag DomainTag, FInstancedStruct Payload);

	UFUNCTION()
	void OnEventSubscriptionRemoved(FGuid SubscriptionID);

	TWeakObjectPtr<UWorld> WorldContext;
	TWeakObjectPtr<USimpleGameplayAbilityComponent> AttributeOwner;
	
	bool ShouldOnlyTriggerOnce;
	bool ShouldListenForBaseValueChange = true;
	bool ShouldListenForCurrentValueChange = true;
	bool ShouldListenForMaxBaseValueChange = true;
	bool ShouldListenForMinBaseValueChange = true;
	bool ShouldListenForMaxCurrentValueChange = true;
	bool ShouldListenForMinCurrentValueChange = true;
	
	FGameplayTag Attribute;

	FGuid EventID;
	FSimpleEventDelegate EventCallbackDelegate;
};
