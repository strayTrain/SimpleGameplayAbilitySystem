#pragma once

#include "CoreMinimal.h"
#include "SimpleAttributeModifierTypes.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleGameplayAbilityComponent/SimpleAbility/SimpleAbilityBase/SimpleAbilityBase.h"
#include "SimpleAttributeModifier.generated.h"

class USimpleAttributeComponent;
class UModifierAction;
class USimpleGameplayAbility;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute Modifier|Config", meta = (
		EditConditionHides,
		EditCondition = "DurationType == EAttributeModifierDurationType::SetDuration"))
	EDurationModifierReApplicationConfig OnReapplication;

	/**
	 * If true, the non-instant modifier's scratch pad will be reset each time the modifier ticks.
	 * If false, the scratch pad will persist between ticks and can be used to store state
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute Modifier|Config", meta = (
		EditConditionHides,
		EditCondition = "DurationType != EAttributeModifierDurationType::Instant"))
	bool ResetScratchPadOnTick = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute Modifier|Config|Stacking", meta = (EditCondition = "DurationType != EAttributeModifierDurationType::Instant"))
	bool CanStack = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute Modifier|Config|Stacking", meta = (InlineEditConditionToggle))
	bool HasMaxStacks;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute Modifier|Config|Stacking", meta = (EditCondition = "HasMaxStacks && DurationType != EAttributeModifierDurationType::Instant"))
	int32 MaxStacks = 1;

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

	/**
	 * Optional convenience value that can be used for simpler modifiers. e.g. If you know you want to deal 5 damage when
	 * the modifier is applied, you can pass this to 5 into this value and use it in your modifier actions instead of having to
	 * store the value as a context variable.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "Attribute Modifier|State")
	float ModifierMagnitude = 0;
	
	/**
	 * Keeps track of the number of stacks this modifier has. Only applies to duration type modifiers with a stackable configuration.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "Attribute Modifier|State")
	int32 ModifierStacks = 1;

	/**
	 * Keeps track of the number of ticks that have occurred since the modifier was applied if the modifier is a duration type.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Attribute Modifier|State")
	int32 TickCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Attribute Modifier|State")
	bool IsActive = false;
	
	UPROPERTY(EditAnywhere, Instanced, Category = "Attribute Modifier|Actions", meta = (TitleProperty = "Description"))
	TArray<TObjectPtr<UModifierAction>> ModifierActions;

	UPROPERTY(BlueprintReadOnly, Category = "Attribute Modifier|State")
	USimpleAttributeComponent* InstigatorAttributeComponent;
	UPROPERTY(BlueprintReadOnly, Category = "Attribute Modifier|State")
	USimpleAttributeComponent* TargetAttributeComponent;

	/* Event Dispatchers */
	UPROPERTY(BlueprintAssignable, Category = "Attribute Modifier|Events|Lifecycle")
	FOnActionStackAppliedSignature OnActionStackApplied;
	
	UPROPERTY(BlueprintAssignable, Category = "Attribute Modifier|Events|Lifecycle")
	FOnModifierEndedSignature OnAttributeModifierEnded;

	UPROPERTY(BlueprintAssignable, Category = "Attribute Modifier|Events|Lifecycle")
	FOnModifierEndedSignature OnAttributeModifierCancelled;
	
	/* Callable Functions */
	
	UFUNCTION(BlueprintCallable, Category = "Attribute Modifier|Application", meta = (AdvancedDisplay=3))
	bool ApplyModifier(FGuid NewModifierID, USimpleAttributeComponent* Instigator, USimpleAttributeComponent* Target, float Magnitude, const FInstancedStruct Context);

	UFUNCTION(BlueprintCallable, Category = "Attribute Modifier|Application")
	void EndModifier(FGameplayTag EndingStatus, FInstancedStruct EndingContext);

	UFUNCTION(BlueprintCallable, Category = "Attribute Modifier|Application")
	void CancelModifier(FGameplayTag EndingStatus, FInstancedStruct EndingContext);
	
	UFUNCTION(BlueprintCallable, Category = "Attribute Modifier|Lifecycle")
	void AddModifierStack(int32 StackCount);
	
	/* Blueprint Implementable Events */

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Attribute Modifier|Application")
	bool CanApplyModifier() const;
	virtual bool CanApplyModifier_Implementation() const { return true; }
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Attribute Modifier|Lifecycle")
	void OnPreApplyModifierActions();
	void OnPreApplyModifierActions_Implementation() {}

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
	void OnStacksAdded(int32 AddedStacks, int32 CurrentStacks);
	void OnStacksAdded_Implementation(int32 AddedStacks, int32 CurrentStacks) {}

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Attribute Modifier|Lifecycle")
	void OnStacksRemoved(int32 RemovedStacks, int32 CurrentStacks);
	void OnStacksRemoved_Implementation(int32 RemovedStacks, int32 CurrentStacks) {}

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Attribute Modifier|Lifecycle")
	void OnMaxStacksReached();
	void OnMaxStacksReached_Implementation() {}

	UFUNCTION()
	void OnClientReceivedServerActionsResult(FInstancedStruct ServerSnapshot, FInstancedStruct ClientSnapshot);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FAttributeModifierActionScratchPad& GetModifierActionScratchPad()
	{
		return ModifierActionScratchPad;
	}
	
protected:
	UPROPERTY(BlueprintReadOnly, Category = "Attribute Modifier|State")
	float ActivationTime;
	
	UPROPERTY(BlueprintReadWrite, Category = "Attribute Modifier|State")
	FAttributeModifierActionScratchPad ModifierActionScratchPad;

	UFUNCTION(BlueprintCallable, Category = "Attribute Modifier|Application")
	bool ApplyModifierActions(USimpleAttributeModifier* OwningModifier, FGameplayTagContainer ActionTriggers);

private:
	bool CanApplyModifierInternal();
	
	FTimerHandle DurationTimerHandle;
	FTimerHandle TickTimerHandle;

	void OnDurationTimerExpired();
	void OnTickTimerTriggered();
};
