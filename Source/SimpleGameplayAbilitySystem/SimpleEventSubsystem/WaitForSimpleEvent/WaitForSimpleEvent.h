#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "SimpleGameplayAbilitySystem/SimpleEventSubsystem/SimpleEventTypes.h"
#include "WaitForSimpleEvent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
	FSimpleEventReceivedDelegate,
	FGameplayTag, EventTag,
	FGameplayTag, Domain,
	FInstancedStruct, Payload,
	FGuid, EventSubscriptionID
);

UCLASS()
class SIMPLEGAMEPLAYABILITYSYSTEM_API UWaitForSimpleEvent : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, meta=(WorldContext = "WorldContextObject", BlueprintInternalUseOnly=true, AdvancedDisplay=5, AutoCreateRefTerm = "PayloadFilter,SenderFilter"))
	static UWaitForSimpleEvent* WaitForSimpleEvent(
		UObject* WorldContextObject,
		UObject* Listener,
		bool OnlyTriggerOnce,
		FGameplayTagContainer EventFilter,
		FGameplayTagContainer DomainFilter,
		TArray<UScriptStruct*> PayloadFilter,
		TArray<AActor*> SenderFilter,
		bool OnlyMatchExactEvent = true,
		bool OnlyMatchExactDomain = true);

	UPROPERTY(BlueprintAssignable)
	FSimpleEventReceivedDelegate OnEventReceived;

	virtual void Activate() override;

protected:
	UFUNCTION()
	void OnSimpleEventReceived(FGameplayTag AbilityTag, FGameplayTag DomainTag, FInstancedStruct Payload);

	UFUNCTION()
	void OnEventSubscriptionRemoved(FGuid SubscriptionID);

	UWorld* WorldContext;
	UObject* Listener;
	FGuid EventID;
	FEventSubscription EventSubscription;
	bool OnlyTriggerOnce;
	FGameplayTagContainer EventFilter;
	FGameplayTagContainer DomainFilter;
	TArray<UScriptStruct*> PayloadFilter;
	TArray<AActor*> SenderFilter;
	bool OnlyMatchExactEvent = true;
	bool OnlyMatchExactDomain = true;
	FSimpleEventDelegate EventCallbackDelegate;
};
