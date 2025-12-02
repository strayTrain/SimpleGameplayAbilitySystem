#pragma once

#include "CoreMinimal.h"
#include "SimpleAttributeModifierTypes.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleAbilityBase/SimpleAbilityBase.h"
#include "Curves/CurveFloat.h"
#include "SimpleAttributeModifier.generated.h"

class USimpleAttributeComponent;
class UModifierAction;
class USimpleGameplayAbility;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStackCountChangedSignature, USimpleAttributeModifier*, ModifierInstance, int32, NewStackCount);



UCLASS(Blueprintable, Abstract)
class SIMPLEGAMEPLAYABILITYSYSTEM_API USimpleAttributeModifier : public UObject
{
	GENERATED_BODY()

public:
	/* Properties */

	UPROPERTY(BlueprintReadOnly, Category = "State")
	FGuid ModifierID;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	FInstancedStruct ModifierContext;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute Modifier|Config")
	EAttributeModifierDurationType DurationType = EAttributeModifierDurationType::Instant;
	
	/**
	 * How long this modifier lasts. Only applies to SetDuration type modifiers. If the duration is 0, the modifier
	 * will behave the same as an Instant type modifier.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute Modifier|Config", meta = (
		EditConditionHides,
		EditCondition = "DurationType == EAttributeModifierDurationType::SetDuration"))
	float Duration = 1;
	
	/**
	 * How often the modifier applies its Actions stack. If set to 0 the action stack will only be applied once with
	 * the Phase set to OnApplied. If set to a value greater than 0, the action stack will be applied every TickInterval seconds
	 * with a Phase of Default.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute Modifier|Config", meta = (
		EditConditionHides,
		EditCondition = "DurationType != EAttributeModifierDurationType::Instant"))
	float TickInterval = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute Modifier|Config", meta = (
		EditConditionHides,
		EditCondition = "DurationType != EAttributeModifierDurationType::Instant"))
	EDurationTickTagRequirementBehaviour TickRequirementsFailedBehaviour;

	/**
	 * If true, the non-instant modifier's scratch pad will be reset each time the modifier ticks.
	 * If false, the scratch pad will persist between ticks and can be used to store state
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute Modifier|Config", meta = (
		EditConditionHides,
		EditCondition = "DurationType != EAttributeModifierDurationType::Instant"))
	bool ResetScratchPadOnTick = true;

	/**
	 * @deprecated Use StackingConfig.bEnableStacking instead
	 * If enabled, this modifier will be part of a stack group identified by StackGroupTag.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "Attribute Modifier|Config|Stacking (DEPRECATED)",
		meta = (DeprecatedProperty, DeprecationMessage = "Use StackingConfig.bEnableStacking instead"))
	bool bUseStackGroup = false;

	/**
	 * @deprecated Use StackingConfig.StackGroupTag instead
	 * Tag that identifies which stack group this modifier belongs to.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "Attribute Modifier|Config|Stacking (DEPRECATED)",
		meta = (DeprecatedProperty, DeprecationMessage = "Use StackingConfig.StackGroupTag instead"))
	FGameplayTag StackGroupTag;

	/**
	 * @deprecated Use StackingConfig.OnReapplication instead
	 */
	UPROPERTY(BlueprintReadWrite, Category = "Attribute Modifier|Config|Stacking (DEPRECATED)",
		meta = (DeprecatedProperty, DeprecationMessage = "Use StackingConfig.OnReapplication instead"))
	EStackReapplicationBehavior OnReapplication;

	/**
	 * @deprecated Use StackingConfig.MaxStacks instead (set to 0 for unlimited)
	 */
	UPROPERTY(BlueprintReadWrite, Category = "Attribute Modifier|Config|Stacking (DEPRECATED)",
		meta = (DeprecatedProperty, DeprecationMessage = "Use StackingConfig.MaxStacks instead"))
	bool bHasMaxStacksInGroup = false;

	/**
	 * @deprecated Use StackingConfig.MaxStacks instead
	 * Maximum number of modifier instances allowed in this stack group.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "Attribute Modifier|Config|Stacking (DEPRECATED)",
		meta = (DeprecatedProperty, DeprecationMessage = "Use StackingConfig.MaxStacks instead"))
	int32 MaxStacksInGroup = 1;

	/**
	 * @deprecated Use StackingConfig.OverflowBehavior instead
	 * Determines what happens when trying to apply a new modifier when the stack group is at max capacity.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "Attribute Modifier|Config|Stacking (DEPRECATED)",
		meta = (DeprecatedProperty, DeprecationMessage = "Use StackingConfig.OverflowBehavior instead"))
	EStackGroupOverflowBehavior OverflowBehavior = EStackGroupOverflowBehavior::DenyNew;

	/**
	 * Main stacking configuration for the consolidated stacking system.
	 * This replaces bUseStackGroup, StackGroupTag, MaxStacksInGroup, etc.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute Modifier|Config|Stacking",
		meta = (EditCondition = "DurationType != EAttributeModifierDurationType::Instant", EditConditionHides))
	FStackingConfig StackingConfig;

	/**
	 * Function callback for custom magnitude scaling based on stack count.
	 * Signature: float FunctionName(int32 StackCount, float BaseMagnitude, USimpleAttributeModifier* Modifier)
	 * Only used when StackingConfig.MagnitudeScalingSource == FunctionCallback
	 */
	UPROPERTY(EditAnywhere, Category = "Attribute Modifier|Config|Stacking",
		meta = (FunctionReference, AllowFunctionLibraries,
		PrototypeFunction = "/Script/SimpleGameplayAbilitySystem.FunctionSelectors.Prototype_CalculateStackMagnitude",
		EditCondition = "StackingConfig.bEnableStacking && StackingConfig.MagnitudeScalingSource == EMagnitudeScalingSource::FunctionCallback && DurationType != EAttributeModifierDurationType::Instant",
		EditConditionHides))
	FMemberReference MagnitudeScalingFunction;

	/**
	 * Tags that can be used to classify this modifier. e.g. "DamageOverTime", "StatusEffect" etc.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute Modifier|Tags")
	FGameplayTagContainer ModifierTags;

	/**
	 * These tags are applied to the target ability component when this modifier is applied and removed when it ends.
	 * Only applies to duration type modifiers.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute Modifier|Tags", meta = (
		EditConditionHides,
		EditCondition = "DurationType != EAttributeModifierDurationType::Instant"))
	FGameplayTagContainer TemporarilyAppliedTags;

	/**
	 * These tags are applied to the target ability component when this modifier is applied and must be removed manually.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute Modifier|Tags")
	FGameplayTagContainer PermanentlyAppliedTags;
	
	/**
	 * These tags must be present on the target ability component for this modifier to apply.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute Modifier|Requirements")
	FGameplayTagContainer TargetRequiredTags;

	/**
	 * These tags must NOT be present on the target ability component for this modifier to apply.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute Modifier|Requirements")
	FGameplayTagContainer TargetBlockingTags;

	/**
	 * If another duration type modifier with these tags is already applied, this modifier will not be applied.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute Modifier|Requirements")
	FGameplayTagContainer TargetBlockingModifierTags;

	/*
	 * If set, this ability will only activate if ActivationContext contains this struct type.
	 * If left null, the ability will activate with any payload.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attribute Modifier|Requirements")
	UScriptStruct* RequiredContextType;

	/**
	 * Optional convenience value that can be used for simpler modifiers. e.g. If you know you want to deal 5 damage when
	 * the modifier is applied, you can pass this to 5 into this value and use it in your modifier actions instead of having to
	 * store the value as a context variable.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "Attribute Modifier|State")
	float ModifierMagnitude = 0;

	/**
	 * Keeps track of the number of ticks that have occurred since the modifier was applied if the modifier is a duration type.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Attribute Modifier|State")
	int32 TickCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Attribute Modifier|State")
	bool IsActive = false;

	/** Current stack count for consolidated stacking (1 for non-stacking modifiers) */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Attribute Modifier|State|Stacking")
	int32 CurrentStackCount = 0;

	/** Computed magnitude after scaling based on stack count */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Attribute Modifier|State|Stacking")
	float ScaledMagnitude = 0.0f;

	/** Individual stack expiration times for IndependentDurations mode */
	UPROPERTY(Transient)
	TArray<FStackDurationEntry> StackDurations;

	UPROPERTY(EditAnywhere, Category = "Attribute Modifier|Actions")
	FAttributeModifierActionScratchPad InitialScratchPadValues;
	
	UPROPERTY(EditAnywhere, Instanced, Category = "Attribute Modifier|Actions", meta = (TitleProperty = "Description"))
	TArray<TObjectPtr<UModifierAction>> ModifierActions;

	UPROPERTY(BlueprintReadOnly, Category = "Attribute Modifier|State")
	USimpleAttributeComponent* InstigatorAttributeComponent;
	UPROPERTY(BlueprintReadOnly, Category = "Attribute Modifier|State")
	USimpleAttributeComponent* TargetAttributeComponent;

	/**
	 * The event subscription GUID for this modifier listening to SimpleEventSubsystem.
	 * Used to unsubscribe when the modifier ends or is cancelled.
	 */
	UPROPERTY()
	FGuid GlobalEventSubscriptionID;

	/* Event Dispatchers */
	
	UPROPERTY(BlueprintReadOnly, Category = "Attribute Modifier|Events|Lifecycle")
	FOnModifierAppliedSignature OnModifierApplied;
	
	UPROPERTY(BlueprintAssignable, Category = "Attribute Modifier|Events|Lifecycle")
	FOnActionStackAppliedSignature OnActionStackApplied;
	
	UPROPERTY(BlueprintAssignable, Category = "Attribute Modifier|Events|Lifecycle")
	FOnModifierEndedSignature OnAttributeModifierEnded;

	UPROPERTY(BlueprintAssignable, Category = "Attribute Modifier|Events|Lifecycle")
	FOnModifierEndedSignature OnAttributeModifierCancelled;

	/** Event fired when the stack count changes (for consolidated stacking) */
	UPROPERTY(BlueprintAssignable, Category = "Attribute Modifier|Events|Stacking")
	FOnStackCountChangedSignature OnStackCountChanged;

	/* Callable Functions */

	void InitializeModifier(FGuid NewModifierID, USimpleAttributeComponent* Instigator, USimpleAttributeComponent* Target, float Magnitude, const FInstancedStruct Context, const bool DoesReplicate);
	
	UFUNCTION(BlueprintCallable, Category = "Attribute Modifier|Application", meta = (AdvancedDisplay=3))
	bool ApplyModifier();

	UFUNCTION(BlueprintCallable, Category = "Attribute Modifier|Application")
	void EndModifier(FGameplayTag EndingStatus, FInstancedStruct EndingContext);

	UFUNCTION(BlueprintCallable, Category = "Attribute Modifier|Application")
	void CancelModifier(FGameplayTag EndingStatus, FInstancedStruct EndingContext);

	/**
	 * Get the remaining duration in seconds (returns 0 if not a SetDuration modifier or if already expired).
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Attribute Modifier|Duration")
	float GetRemainingDuration() const;

	/**
	 * Extend the duration by additional seconds. Only works for SetDuration modifiers.
	 */
	UFUNCTION(BlueprintCallable, Category = "Attribute Modifier|Duration")
	void ExtendDuration(float AdditionalSeconds);

	/**
	 * Set a new remaining duration. Only works for SetDuration modifiers.
	 */
	UFUNCTION(BlueprintCallable, Category = "Attribute Modifier|Duration")
	void SetRemainingDuration(float NewDuration);

	/**
	 * Get the time this modifier was activated (server time).
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Attribute Modifier")
	float GetActivationTime() const { return ActivationTime; }

	/* Stacking Methods */

	/**
	 * Add stacks to this modifier (consolidated stacking only).
	 * @param Count Number of stacks to add (default 1)
	 * @return True if stacks were added successfully
	 */
	UFUNCTION(BlueprintCallable, Category = "Attribute Modifier|Stacking")
	bool AddStacks(int32 Count = 1);

	/**
	 * Remove stacks from this modifier (consolidated stacking only).
	 * If stack count reaches 0, the modifier will end.
	 * @param Count Number of stacks to remove (default 1)
	 * @return True if stacks were removed successfully
	 */
	UFUNCTION(BlueprintCallable, Category = "Attribute Modifier|Stacking")
	bool RemoveStacks(int32 Count = 1);

	/**
	 * Get the current stack count.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Attribute Modifier|Stacking")
	int32 GetStackCount() const { return CurrentStackCount; }

	/**
	 * Get the scaled magnitude (base magnitude * stack scaling).
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Attribute Modifier|Stacking")
	float GetScaledMagnitude() const { return ScaledMagnitude; }

	/**
	 * Force set the stack count (used for prediction reconciliation).
	 * Does not trigger events or recalculate magnitude - use AddStacks/RemoveStacks for normal operation.
	 */
	void ForceSetStackCount(int32 NewStackCount) { CurrentStackCount = NewStackCount; }

	/**
	 * Check if this modifier uses consolidated stacking.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Attribute Modifier|Stacking")
	bool UsesConsolidatedStacking() const { return StackingConfig.bEnableStacking; }

	/**
	 * Check if this modifier uses a custom stack group tag (vs class-based grouping).
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Attribute Modifier|Stacking")
	bool UsesCustomStackGroupTag() const { return StackingConfig.bEnableStacking && StackingConfig.StackGroupTag.IsValid(); }

	/** Recalculate ScaledMagnitude based on current stack count and scaling configuration */
	void RecalculateMagnitude();
	
	/** Inject stack count and scaled magnitude into scratchpad before actions run */
	void InjectStackCountToScratchpad();
	
	/* Blueprint Implementable Events */

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Attribute Modifier|Application")
	bool CanApplyModifier() const;
	virtual bool CanApplyModifier_Implementation() const { return true; }
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Attribute Modifier|Lifecycle")
	void OnPreApplyModifier();
	void OnPreApplyModifier_Implementation() {}

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Attribute Modifier|Lifecycle")
	void OnPostApplyModifierActions();
	void OnPostApplyModifierActions_Implementation() {}

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Attribute Modifier|Lifecycle")
	void OnModifierEnded(FGameplayTag EndingStatus, FInstancedStruct EndingContext);
	void OnModifierEnded_Implementation(FGameplayTag EndingStatus, FInstancedStruct EndingContext) {}

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Attribute Modifier|Lifecycle")
	void OnModifierCancelled(FGameplayTag EndingStatus, FInstancedStruct EndingContext);
	void OnModifierCancelled_Implementation(FGameplayTag EndingStatus, FInstancedStruct EndingContext) {}

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Attribute Modifier|Lifecycle")
	void OnCleanupModifier();
	void OnCleanupModifier_Implementation();

	UFUNCTION()
	void OnClientReceivedServerActionsResult(FModifierActionStackResults ServerMutation, FModifierActionStackResults ClientMutation);

	void TriggerActionsForEvents(FGameplayTagContainer EventTags);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FAttributeModifierActionScratchPad& GetModifierActionScratchPad()
	{
		return ModifierActionScratchPad;
	}

	bool WasModifierInitialized() const { return WasInitialized; }

	// UObject interface
	virtual void PostLoad() override;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Attribute Modifier|State")
	float ActivationTime;

	UPROPERTY(BlueprintReadWrite, Category = "Attribute Modifier|State")
	FAttributeModifierActionScratchPad ModifierActionScratchPad;
	
	/** Check for threshold crossings and fire appropriate events/tags */
	void CheckThresholdCrossings(int32 OldCount, int32 NewCount);

	/** Handle stack duration expiration for IndependentDurations mode */
	void HandleStackDurationExpired(int32 StackIndex);

	/** Update stack duration timers (for IndependentDurations mode) */
	void UpdateStackDurationTimers();

private:
	bool CanApplyModifierInternal();

	/**
	 * Handles stack group reapplication logic and overflow behavior.
	 * Returns false if the application should be denied, true if it should proceed.
	 */
	bool HandleStackGroupReapplication();

	FTimerHandle DurationTimerHandle;
	FTimerHandle TickTimerHandle;

	/** Timer handles for individual stack durations (IndependentDurations mode) */
	TMap<int32, FTimerHandle> StackDurationTimerHandles;

	void OnDurationTimerExpired();
	void OnTickTimerTriggered();
	
	UFUNCTION()
	void OnModifierEventReceived(FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload, UObject* Sender);

	void TriggerActions(TArray<UModifierAction*>& Actions);
	
	bool DoesModifierReplicate = true;
	bool WasInitialized = false;
};
