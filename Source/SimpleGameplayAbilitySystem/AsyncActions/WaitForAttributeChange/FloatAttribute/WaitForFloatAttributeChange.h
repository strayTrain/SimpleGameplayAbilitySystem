//  Copyright 2025 Ahmed Elgoni

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "WaitForFloatAttributeChange.generated.h"

class USimpleAttributeComponent;

/**
 * Delegate for float attribute value changed events.
 * @param AttributeTag - The gameplay tag identifying the attribute that changed
 * @param OldValue - The previous value of the attribute
 * @param NewValue - The new value of the attribute
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FFloatAttributeValueChangedDelegate, FGameplayTag, AttributeTag, float, OldValue, float, NewValue);

/**
 * Delegate for float attribute limit reached events.
 * @param AttributeTag - The gameplay tag identifying the attribute that reached its limit
 * @param Overflow - The amount by which the value exceeded the limit
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFloatAttributeLimitReachedDelegate, FGameplayTag, AttributeTag, float, Overflow);

/**
 * Async action that waits for a float attribute's base value to change.
 * Monitors the OnFloatAttributeBaseValueChanged event and triggers when the specified attribute's base value is modified.
 */
UCLASS()
class SIMPLEGAMEPLAYABILITYSYSTEM_API UWaitForFloatAttributeBaseValueChanged : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	/**
	 * Event fired when the attribute's base value changes.
	 * @param AttributeTag - The tag of the attribute that changed
	 * @param OldValue - The previous base value
	 * @param NewValue - The new base value
	 */
	UPROPERTY(BlueprintAssignable)
	FFloatAttributeValueChangedDelegate OnValueChanged;

	/**
	 * Creates an async action that waits for a float attribute's base value to change.
	 * @param AttributeComponent - The attribute component to monitor
	 * @param AttributeTag - The gameplay tag identifying the float attribute to watch
	 * @param OnlyTriggerOnce - If true, the action will automatically destroy itself after the first trigger. If false, it will continue listening until manually destroyed
	 * @return The async action instance
	 */
	UFUNCTION(BlueprintCallable, Category = "SimpleGAS|Async Functions", meta=(BlueprintInternalUseOnly=true))
	static UWaitForFloatAttributeBaseValueChanged* WaitForFloatAttributeBaseValueChanged(
		USimpleAttributeComponent* AttributeComponent,
		FGameplayTag AttributeTag,
		bool OnlyTriggerOnce);

	virtual void Activate() override;
	virtual void SetReadyToDestroy() override;

protected:
	/** Internal callback when any float attribute's base value changes */
	UFUNCTION()
	void OnAttributeValueChanged(FGameplayTag AttributeTag, float OldValue, float NewValue);

	/** Weak pointer to the attribute component being monitored */
	TWeakObjectPtr<USimpleAttributeComponent> AttributeComponentPtr;

	/** The specific attribute tag to watch for changes */
	FGameplayTag TargetAttributeTag;

	/** Whether this action should destroy itself after triggering once */
	bool bOnlyTriggerOnce;
};

/**
 * Async action that waits for a float attribute's current value to change.
 * Monitors the OnFloatAttributeCurrentValueChanged event and triggers when the specified attribute's current value is modified.
 */
UCLASS()
class SIMPLEGAMEPLAYABILITYSYSTEM_API UWaitForFloatAttributeCurrentValueChanged : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	/**
	 * Event fired when the attribute's current value changes.
	 * @param AttributeTag - The tag of the attribute that changed
	 * @param OldValue - The previous current value
	 * @param NewValue - The new current value
	 */
	UPROPERTY(BlueprintAssignable)
	FFloatAttributeValueChangedDelegate OnValueChanged;

	/**
	 * Creates an async action that waits for a float attribute's current value to change.
	 * @param AttributeComponent - The attribute component to monitor
	 * @param AttributeTag - The gameplay tag identifying the float attribute to watch
	 * @param OnlyTriggerOnce - If true, the action will automatically destroy itself after the first trigger. If false, it will continue listening until manually destroyed
	 * @return The async action instance
	 */
	UFUNCTION(BlueprintCallable, Category = "SimpleGAS|Async Functions", meta=(BlueprintInternalUseOnly=true))
	static UWaitForFloatAttributeCurrentValueChanged* WaitForFloatAttributeCurrentValueChanged(
		USimpleAttributeComponent* AttributeComponent,
		FGameplayTag AttributeTag,
		bool OnlyTriggerOnce);

	virtual void Activate() override;
	virtual void SetReadyToDestroy() override;

protected:
	/** Internal callback when any float attribute's current value changes */
	UFUNCTION()
	void OnAttributeValueChanged(FGameplayTag AttributeTag, float OldValue, float NewValue);

	/** Weak pointer to the attribute component being monitored */
	TWeakObjectPtr<USimpleAttributeComponent> AttributeComponentPtr;

	/** The specific attribute tag to watch for changes */
	FGameplayTag TargetAttributeTag;

	/** Whether this action should destroy itself after triggering once */
	bool bOnlyTriggerOnce;
};

/**
 * Async action that waits for a float attribute's min base value to change.
 * Monitors the OnFloatAttributeMinBaseValueChanged event and triggers when the specified attribute's minimum base value limit is modified.
 */
UCLASS()
class SIMPLEGAMEPLAYABILITYSYSTEM_API UWaitForFloatAttributeMinBaseValueChanged : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	/**
	 * Event fired when the attribute's min base value changes.
	 * @param AttributeTag - The tag of the attribute that changed
	 * @param OldValue - The previous min base value
	 * @param NewValue - The new min base value
	 */
	UPROPERTY(BlueprintAssignable)
	FFloatAttributeValueChangedDelegate OnValueChanged;

	/**
	 * Creates an async action that waits for a float attribute's min base value to change.
	 * @param AttributeComponent - The attribute component to monitor
	 * @param AttributeTag - The gameplay tag identifying the float attribute to watch
	 * @param OnlyTriggerOnce - If true, the action will automatically destroy itself after the first trigger. If false, it will continue listening until manually destroyed
	 * @return The async action instance
	 */
	UFUNCTION(BlueprintCallable, Category = "SimpleGAS|Async Functions", meta=(BlueprintInternalUseOnly=true))
	static UWaitForFloatAttributeMinBaseValueChanged* WaitForFloatAttributeMinBaseValueChanged(
		USimpleAttributeComponent* AttributeComponent,
		FGameplayTag AttributeTag,
		bool OnlyTriggerOnce);

	virtual void Activate() override;
	virtual void SetReadyToDestroy() override;

protected:
	/** Internal callback when any float attribute's min base value changes */
	UFUNCTION()
	void OnAttributeValueChanged(FGameplayTag AttributeTag, float OldValue, float NewValue);

	/** Weak pointer to the attribute component being monitored */
	TWeakObjectPtr<USimpleAttributeComponent> AttributeComponentPtr;

	/** The specific attribute tag to watch for changes */
	FGameplayTag TargetAttributeTag;

	/** Whether this action should destroy itself after triggering once */
	bool bOnlyTriggerOnce;
};

/**
 * Async action that waits for a float attribute's min current value to change.
 * Monitors the OnFloatAttributeMinCurrentValueChanged event and triggers when the specified attribute's minimum current value limit is modified.
 */
UCLASS()
class SIMPLEGAMEPLAYABILITYSYSTEM_API UWaitForFloatAttributeMinCurrentValueChanged : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	/**
	 * Event fired when the attribute's min current value changes.
	 * @param AttributeTag - The tag of the attribute that changed
	 * @param OldValue - The previous min current value
	 * @param NewValue - The new min current value
	 */
	UPROPERTY(BlueprintAssignable)
	FFloatAttributeValueChangedDelegate OnValueChanged;

	/**
	 * Creates an async action that waits for a float attribute's min current value to change.
	 * @param AttributeComponent - The attribute component to monitor
	 * @param AttributeTag - The gameplay tag identifying the float attribute to watch
	 * @param OnlyTriggerOnce - If true, the action will automatically destroy itself after the first trigger. If false, it will continue listening until manually destroyed
	 * @return The async action instance
	 */
	UFUNCTION(BlueprintCallable, Category = "SimpleGAS|Async Functions", meta=(BlueprintInternalUseOnly=true))
	static UWaitForFloatAttributeMinCurrentValueChanged* WaitForFloatAttributeMinCurrentValueChanged(
		USimpleAttributeComponent* AttributeComponent,
		FGameplayTag AttributeTag,
		bool OnlyTriggerOnce);

	virtual void Activate() override;
	virtual void SetReadyToDestroy() override;

protected:
	/** Internal callback when any float attribute's min current value changes */
	UFUNCTION()
	void OnAttributeValueChanged(FGameplayTag AttributeTag, float OldValue, float NewValue);

	/** Weak pointer to the attribute component being monitored */
	TWeakObjectPtr<USimpleAttributeComponent> AttributeComponentPtr;

	/** The specific attribute tag to watch for changes */
	FGameplayTag TargetAttributeTag;

	/** Whether this action should destroy itself after triggering once */
	bool bOnlyTriggerOnce;
};

/**
 * Async action that waits for a float attribute's max base value to change.
 * Monitors the OnFloatAttributeMaxBaseValueChanged event and triggers when the specified attribute's maximum base value limit is modified.
 */
UCLASS()
class SIMPLEGAMEPLAYABILITYSYSTEM_API UWaitForFloatAttributeMaxBaseValueChanged : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	/**
	 * Event fired when the attribute's max base value changes.
	 * @param AttributeTag - The tag of the attribute that changed
	 * @param OldValue - The previous max base value
	 * @param NewValue - The new max base value
	 */
	UPROPERTY(BlueprintAssignable)
	FFloatAttributeValueChangedDelegate OnValueChanged;

	/**
	 * Creates an async action that waits for a float attribute's max base value to change.
	 * @param AttributeComponent - The attribute component to monitor
	 * @param AttributeTag - The gameplay tag identifying the float attribute to watch
	 * @param OnlyTriggerOnce - If true, the action will automatically destroy itself after the first trigger. If false, it will continue listening until manually destroyed
	 * @return The async action instance
	 */
	UFUNCTION(BlueprintCallable, Category = "SimpleGAS|Async Functions", meta=(BlueprintInternalUseOnly=true))
	static UWaitForFloatAttributeMaxBaseValueChanged* WaitForFloatAttributeMaxBaseValueChanged(
		USimpleAttributeComponent* AttributeComponent,
		FGameplayTag AttributeTag,
		bool OnlyTriggerOnce);

	virtual void Activate() override;
	virtual void SetReadyToDestroy() override;

protected:
	/** Internal callback when any float attribute's max base value changes */
	UFUNCTION()
	void OnAttributeValueChanged(FGameplayTag AttributeTag, float OldValue, float NewValue);

	/** Weak pointer to the attribute component being monitored */
	TWeakObjectPtr<USimpleAttributeComponent> AttributeComponentPtr;

	/** The specific attribute tag to watch for changes */
	FGameplayTag TargetAttributeTag;

	/** Whether this action should destroy itself after triggering once */
	bool bOnlyTriggerOnce;
};

/**
 * Async action that waits for a float attribute's max current value to change.
 * Monitors the OnFloatAttributeMaxCurrentValueChanged event and triggers when the specified attribute's maximum current value limit is modified.
 */
UCLASS()
class SIMPLEGAMEPLAYABILITYSYSTEM_API UWaitForFloatAttributeMaxCurrentValueChanged : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	/**
	 * Event fired when the attribute's max current value changes.
	 * @param AttributeTag - The tag of the attribute that changed
	 * @param OldValue - The previous max current value
	 * @param NewValue - The new max current value
	 */
	UPROPERTY(BlueprintAssignable)
	FFloatAttributeValueChangedDelegate OnValueChanged;

	/**
	 * Creates an async action that waits for a float attribute's max current value to change.
	 * @param AttributeComponent - The attribute component to monitor
	 * @param AttributeTag - The gameplay tag identifying the float attribute to watch
	 * @param OnlyTriggerOnce - If true, the action will automatically destroy itself after the first trigger. If false, it will continue listening until manually destroyed
	 * @return The async action instance
	 */
	UFUNCTION(BlueprintCallable, Category = "SimpleGAS|Async Functions", meta=(BlueprintInternalUseOnly=true))
	static UWaitForFloatAttributeMaxCurrentValueChanged* WaitForFloatAttributeMaxCurrentValueChanged(
		USimpleAttributeComponent* AttributeComponent,
		FGameplayTag AttributeTag,
		bool OnlyTriggerOnce);

	virtual void Activate() override;
	virtual void SetReadyToDestroy() override;

protected:
	/** Internal callback when any float attribute's max current value changes */
	UFUNCTION()
	void OnAttributeValueChanged(FGameplayTag AttributeTag, float OldValue, float NewValue);

	/** Weak pointer to the attribute component being monitored */
	TWeakObjectPtr<USimpleAttributeComponent> AttributeComponentPtr;

	/** The specific attribute tag to watch for changes */
	FGameplayTag TargetAttributeTag;

	/** Whether this action should destroy itself after triggering once */
	bool bOnlyTriggerOnce;
};

/**
 * Async action that waits for a float attribute's min base value limit to be reached.
 * Monitors the OnMinBaseValueReached event and triggers when the specified attribute's value hits its minimum base limit.
 */
UCLASS()
class SIMPLEGAMEPLAYABILITYSYSTEM_API UWaitForMinBaseValueReached : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	/**
	 * Event fired when the attribute's min base value limit is reached.
	 * @param AttributeTag - The tag of the attribute that reached its limit
	 * @param Overflow - The amount by which the value tried to go below the minimum (negative value)
	 */
	UPROPERTY(BlueprintAssignable)
	FFloatAttributeLimitReachedDelegate OnLimitReached;

	/**
	 * Creates an async action that waits for a float attribute's min base value limit to be reached.
	 * @param AttributeComponent - The attribute component to monitor
	 * @param AttributeTag - The gameplay tag identifying the float attribute to watch
	 * @param OnlyTriggerOnce - If true, the action will automatically destroy itself after the first trigger. If false, it will continue listening until manually destroyed
	 * @return The async action instance
	 */
	UFUNCTION(BlueprintCallable, Category = "SimpleGAS|Async Functions", meta=(BlueprintInternalUseOnly=true))
	static UWaitForMinBaseValueReached* WaitForMinBaseValueReached(
		USimpleAttributeComponent* AttributeComponent,
		FGameplayTag AttributeTag,
		bool OnlyTriggerOnce);

	virtual void Activate() override;
	virtual void SetReadyToDestroy() override;

protected:
	/** Internal callback when any float attribute's min base value limit is reached */
	UFUNCTION()
	void OnLimitReachedEvent(FGameplayTag AttributeTag, float Overflow);

	/** Weak pointer to the attribute component being monitored */
	TWeakObjectPtr<USimpleAttributeComponent> AttributeComponentPtr;

	/** The specific attribute tag to watch for limit events */
	FGameplayTag TargetAttributeTag;

	/** Whether this action should destroy itself after triggering once */
	bool bOnlyTriggerOnce;
};

/**
 * Async action that waits for a float attribute's min current value limit to be reached.
 * Monitors the OnMinCurrentValueReached event and triggers when the specified attribute's value hits its minimum current limit.
 */
UCLASS()
class SIMPLEGAMEPLAYABILITYSYSTEM_API UWaitForMinCurrentValueReached : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	/**
	 * Event fired when the attribute's min current value limit is reached.
	 * @param AttributeTag - The tag of the attribute that reached its limit
	 * @param Overflow - The amount by which the value tried to go below the minimum (negative value)
	 */
	UPROPERTY(BlueprintAssignable)
	FFloatAttributeLimitReachedDelegate OnLimitReached;

	/**
	 * Creates an async action that waits for a float attribute's min current value limit to be reached.
	 * @param AttributeComponent - The attribute component to monitor
	 * @param AttributeTag - The gameplay tag identifying the float attribute to watch
	 * @param OnlyTriggerOnce - If true, the action will automatically destroy itself after the first trigger. If false, it will continue listening until manually destroyed
	 * @return The async action instance
	 */
	UFUNCTION(BlueprintCallable, Category = "SimpleGAS|Async Functions", meta=(BlueprintInternalUseOnly=true))
	static UWaitForMinCurrentValueReached* WaitForMinCurrentValueReached(
		USimpleAttributeComponent* AttributeComponent,
		FGameplayTag AttributeTag,
		bool OnlyTriggerOnce);

	virtual void Activate() override;
	virtual void SetReadyToDestroy() override;

protected:
	/** Internal callback when any float attribute's min current value limit is reached */
	UFUNCTION()
	void OnLimitReachedEvent(FGameplayTag AttributeTag, float Overflow);

	/** Weak pointer to the attribute component being monitored */
	TWeakObjectPtr<USimpleAttributeComponent> AttributeComponentPtr;

	/** The specific attribute tag to watch for limit events */
	FGameplayTag TargetAttributeTag;

	/** Whether this action should destroy itself after triggering once */
	bool bOnlyTriggerOnce;
};

/**
 * Async action that waits for a float attribute's max base value limit to be reached.
 * Monitors the OnMaxBaseValueReached event and triggers when the specified attribute's value hits its maximum base limit.
 */
UCLASS()
class SIMPLEGAMEPLAYABILITYSYSTEM_API UWaitForMaxBaseValueReached : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	/**
	 * Event fired when the attribute's max base value limit is reached.
	 * @param AttributeTag - The tag of the attribute that reached its limit
	 * @param Overflow - The amount by which the value tried to exceed the maximum (positive value)
	 */
	UPROPERTY(BlueprintAssignable)
	FFloatAttributeLimitReachedDelegate OnLimitReached;

	/**
	 * Creates an async action that waits for a float attribute's max base value limit to be reached.
	 * @param AttributeComponent - The attribute component to monitor
	 * @param AttributeTag - The gameplay tag identifying the float attribute to watch
	 * @param OnlyTriggerOnce - If true, the action will automatically destroy itself after the first trigger. If false, it will continue listening until manually destroyed
	 * @return The async action instance
	 */
	UFUNCTION(BlueprintCallable, Category = "SimpleGAS|Async Functions", meta=(BlueprintInternalUseOnly=true))
	static UWaitForMaxBaseValueReached* WaitForMaxBaseValueReached(
		USimpleAttributeComponent* AttributeComponent,
		FGameplayTag AttributeTag,
		bool OnlyTriggerOnce);

	virtual void Activate() override;
	virtual void SetReadyToDestroy() override;

protected:
	/** Internal callback when any float attribute's max base value limit is reached */
	UFUNCTION()
	void OnLimitReachedEvent(FGameplayTag AttributeTag, float Overflow);

	/** Weak pointer to the attribute component being monitored */
	TWeakObjectPtr<USimpleAttributeComponent> AttributeComponentPtr;

	/** The specific attribute tag to watch for limit events */
	FGameplayTag TargetAttributeTag;

	/** Whether this action should destroy itself after triggering once */
	bool bOnlyTriggerOnce;
};

/**
 * Async action that waits for a float attribute's max current value limit to be reached.
 * Monitors the OnMaxCurrentValueReached event and triggers when the specified attribute's value hits its maximum current limit.
 */
UCLASS()
class SIMPLEGAMEPLAYABILITYSYSTEM_API UWaitForMaxCurrentValueReached : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	/**
	 * Event fired when the attribute's max current value limit is reached.
	 * @param AttributeTag - The tag of the attribute that reached its limit
	 * @param Overflow - The amount by which the value tried to exceed the maximum (positive value)
	 */
	UPROPERTY(BlueprintAssignable)
	FFloatAttributeLimitReachedDelegate OnLimitReached;

	/**
	 * Creates an async action that waits for a float attribute's max current value limit to be reached.
	 * @param AttributeComponent - The attribute component to monitor
	 * @param AttributeTag - The gameplay tag identifying the float attribute to watch
	 * @param OnlyTriggerOnce - If true, the action will automatically destroy itself after the first trigger. If false, it will continue listening until manually destroyed
	 * @return The async action instance
	 */
	UFUNCTION(BlueprintCallable, Category = "SimpleGAS|Async Functions", meta=(BlueprintInternalUseOnly=true))
	static UWaitForMaxCurrentValueReached* WaitForMaxCurrentValueReached(
		USimpleAttributeComponent* AttributeComponent,
		FGameplayTag AttributeTag,
		bool OnlyTriggerOnce);

	virtual void Activate() override;
	virtual void SetReadyToDestroy() override;

protected:
	/** Internal callback when any float attribute's max current value limit is reached */
	UFUNCTION()
	void OnLimitReachedEvent(FGameplayTag AttributeTag, float Overflow);

	/** Weak pointer to the attribute component being monitored */
	TWeakObjectPtr<USimpleAttributeComponent> AttributeComponentPtr;

	/** The specific attribute tag to watch for limit events */
	FGameplayTag TargetAttributeTag;

	/** Whether this action should destroy itself after triggering once */
	bool bOnlyTriggerOnce;
};
