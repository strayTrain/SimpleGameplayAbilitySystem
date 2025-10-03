#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "SimpleGameplayAbilitySystem/AsyncActions/AsyncActionTypes.h"
#include "WaitForStructAttributeChange.generated.h"

class USimpleAttributeComponent;

/**
 * Async action that waits for a struct attribute to change.
 * Binds to the OnStructAttributeChanged event of a USimpleAttributeComponent and triggers when the specified struct attribute changes.
 * Optionally filters events based on modification tags and can be configured to trigger once or continuously.
 */
UCLASS()
class SIMPLEGAMEPLAYABILITYSYSTEM_API UWaitForStructAttributeChange : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	/**
	 * Event fired when the struct attribute changes.
	 * @param ModificationTags - Tags describing how the attribute was modified (e.g., damage types, buff types)
	 * @param NewValue - The new value of the struct attribute
	 * @param OldValue - The previous value of the struct attribute
	 */
	UPROPERTY(BlueprintAssignable)
	FSimpleStructAttributeChangedDelegate OnStructAttributeChanged;

	/**
	 * Creates an async action that waits for a struct attribute to change.
	 * @param AttributeComponent - The attribute component to monitor
	 * @param AttributeTag - The gameplay tag identifying the struct attribute to watch
	 * @param ModificationEventFilter - Optional tag container to filter modification events. If not empty, only triggers when modification tags match any of these tags
	 * @param OnlyTriggerOnce - If true, the action will automatically destroy itself after the first trigger. If false, it will continue listening until manually destroyed
	 * @return The async action instance
	 */
	UFUNCTION(BlueprintCallable, Category = "SimpleGAS|Async Functions", meta=(BlueprintInternalUseOnly=true))
	static UWaitForStructAttributeChange* WaitForStructAttributeChange(
		USimpleAttributeComponent* AttributeComponent,
		FGameplayTag AttributeTag,
		FGameplayTagContainer ModificationEventFilter,
		bool OnlyTriggerOnce);

	virtual void Activate() override;
	virtual void SetReadyToDestroy() override;

protected:
	/**
	 * Internal callback when any struct attribute changes on the attribute component.
	 * Filters the event based on AttributeTag and ModificationEventFilter before broadcasting.
	 */
	UFUNCTION()
	void OnStructAttributeChangedEvent(FGameplayTag AttributeTag, FInstancedStruct OldValue, FInstancedStruct NewValue, FGameplayTagContainer ModificationTags);

	/** Weak pointer to the attribute component being monitored */
	TWeakObjectPtr<USimpleAttributeComponent> AttributeComponentPtr;

	/** The specific attribute tag to watch for changes */
	FGameplayTag TargetAttributeTag;

	/** Optional filter for modification event tags. Only events matching these tags will trigger the output */
	FGameplayTagContainer ModificationEventFilter;

	/** Whether this action should destroy itself after triggering once */
	bool bOnlyTriggerOnce;
};
