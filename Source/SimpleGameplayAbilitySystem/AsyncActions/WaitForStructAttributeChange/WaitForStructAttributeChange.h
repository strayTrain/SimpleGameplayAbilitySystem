// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "SimpleGameplayAbilitySystem/SimpleEventSubsystem/SimpleEventTypes.h"
#include "WaitForStructAttributeChange.generated.h"

class USimpleGameplayAbilityComponent;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FSimpleStructAttributeChangedDelegate,
	FGameplayTag, StructModificationTag,
	FInstancedStruct, Payload
);

UCLASS()
class SIMPLEGAMEPLAYABILITYSYSTEM_API UWaitForStructAttributeChange : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, meta=(WorldContext = "WorldContextObject", BlueprintInternalUseOnly=true, AdvancedDisplay=4))
	static UWaitForStructAttributeChange* WaitForStructAttributeChange(
		UObject* WorldContextObject,
		USimpleGameplayAbilityComponent* AttributeOwner,
		FGameplayTag AttributeTag,
		bool OnlyTriggerOnce = false);

	UPROPERTY(BlueprintAssignable)
	FSimpleStructAttributeChangedDelegate OnStructAttributeChanged;
	
	virtual void Activate() override;

protected:
	UFUNCTION()
	void OnSimpleEventReceived(FGameplayTag AbilityTag, FGameplayTag DomainTag, FInstancedStruct Payload);

	UFUNCTION()
	void OnEventSubscriptionRemoved(FGuid SubscriptionID);

	TWeakObjectPtr<UWorld> WorldContext;
	TWeakObjectPtr<USimpleGameplayAbilityComponent> AttributeOwner;

	FGameplayTag Attribute;
	bool ShouldOnlyTriggerOnce;
	
	FGuid EventID;
	FSimpleEventDelegate EventCallbackDelegate;
};
