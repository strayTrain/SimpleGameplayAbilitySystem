#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "GameplayTagContainer.h"
#include "WaitForGameplayTag.generated.h"

class USimpleAttributeComponent;

/**
 * Delegate for gameplay tag added events.
 * @param Tag - The gameplay tag that was added
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGameplayTagAddedDelegate, FGameplayTag, Tag);

/**
 * Delegate for gameplay tag removed events.
 * @param Tag - The gameplay tag that was removed
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGameplayTagRemovedDelegate, FGameplayTag, Tag);

/**
 * Async action that waits for a gameplay tag to be added.
 * Monitors the OnGameplayTagAdded event and triggers when the specified tag is added to the attribute component.
 */
UCLASS()
class SIMPLEGAMEPLAYABILITYSYSTEM_API UWaitForGameplayTagAdded : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	/**
	 * Event fired when the gameplay tag is added.
	 * @param Tag - The tag that was added
	 */
	UPROPERTY(BlueprintAssignable)
	FGameplayTagAddedDelegate OnTagAdded;

	/**
	 * Creates an async action that waits for a gameplay tag to be added.
	 * @param AttributeComponent - The attribute component to monitor
	 * @param GameplayTag - The gameplay tag to watch for
	 * @param OnlyTriggerOnce - If true, the action will automatically destroy itself after the first trigger. If false, it will continue listening until manually destroyed
	 * @return The async action instance
	 */
	UFUNCTION(BlueprintCallable, Category = "SimpleGAS|Async Functions", meta=(BlueprintInternalUseOnly=true))
	static UWaitForGameplayTagAdded* WaitForGameplayTagAdded(
		USimpleAttributeComponent* AttributeComponent,
		FGameplayTag GameplayTag,
		bool OnlyTriggerOnce);

	virtual void Activate() override;
	virtual void SetReadyToDestroy() override;

protected:
	/** Internal callback when any gameplay tag is added */
	UFUNCTION()
	void OnGameplayTagAddedEvent(FGameplayTag Tag);

	/** Weak pointer to the attribute component being monitored */
	TWeakObjectPtr<USimpleAttributeComponent> AttributeComponentPtr;

	/** The specific gameplay tag to watch for */
	FGameplayTag TargetTag;

	/** Whether this action should destroy itself after triggering once */
	bool bOnlyTriggerOnce;
};

/**
 * Async action that waits for a gameplay tag to be removed.
 * Monitors the OnGameplayTagRemoved event and triggers when the specified tag is removed from the attribute component.
 */
UCLASS()
class SIMPLEGAMEPLAYABILITYSYSTEM_API UWaitForGameplayTagRemoved : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	/**
	 * Event fired when the gameplay tag is removed.
	 * @param Tag - The tag that was removed
	 */
	UPROPERTY(BlueprintAssignable)
	FGameplayTagRemovedDelegate OnTagRemoved;

	/**
	 * Creates an async action that waits for a gameplay tag to be removed.
	 * @param AttributeComponent - The attribute component to monitor
	 * @param GameplayTag - The gameplay tag to watch for
	 * @param OnlyTriggerOnce - If true, the action will automatically destroy itself after the first trigger. If false, it will continue listening until manually destroyed
	 * @return The async action instance
	 */
	UFUNCTION(BlueprintCallable, Category = "SimpleGAS|Async Functions", meta=(BlueprintInternalUseOnly=true))
	static UWaitForGameplayTagRemoved* WaitForGameplayTagRemoved(
		USimpleAttributeComponent* AttributeComponent,
		FGameplayTag GameplayTag,
		bool OnlyTriggerOnce);

	virtual void Activate() override;
	virtual void SetReadyToDestroy() override;

protected:
	/** Internal callback when any gameplay tag is removed */
	UFUNCTION()
	void OnGameplayTagRemovedEvent(FGameplayTag Tag);

	/** Weak pointer to the attribute component being monitored */
	TWeakObjectPtr<USimpleAttributeComponent> AttributeComponentPtr;

	/** The specific gameplay tag to watch for */
	FGameplayTag TargetTag;

	/** Whether this action should destroy itself after triggering once */
	bool bOnlyTriggerOnce;
};
