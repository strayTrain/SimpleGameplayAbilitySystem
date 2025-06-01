#pragma once

#include "CoreMinimal.h"
#include "SimpleAttributeModifierTypes.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleAbilityBase/SimpleAbilityBase.h"
#include "ModifierActions/Base/ModifierAction.h"
#include "SimpleAttributeModifier.generated.h"

class USimpleGameplayAbility;

UCLASS(Blueprintable)
class SIMPLEGAMEPLAYABILITYSYSTEM_API USimpleAttributeModifier : public USimpleAbilityBase
{
	GENERATED_BODY()

public:
	/* Properties */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute Modifier|Config")
	EAttributeModifierType DurationType = EAttributeModifierType::Instant;

	
	/**
	 * How long this modifier lasts. Only applies to SetDuration type modifiers. If the duration is 0, the modifier
	 * will behave the same as an Instant type modifier.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute Modifier|Config", meta = (
		EditConditionHides,
		EditCondition = "DurationType == EAttributeModifierType::SetDuration"))
	float Duration = 1;
	
	/**
	 * How often the modifier applies its Actions stack. If set to 0 the action stack will only be applied once with
	 * the Phase set to OnApplied. If set to a value greater than 0, the action stack will be applied every TickInterval seconds
	 * with a Phase of Default.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute Modifier|Config", meta = (
		EditConditionHides,
		EditCondition = "DurationType != EAttributeModifierType::Instant"))
	float TickInterval = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute Modifier|Config", meta = (
		EditConditionHides,
		EditCondition = "DurationType != EAttributeModifierType::Instant"))
	EDurationTickTagRequirementBehaviour TickTagRequirementBehaviour;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute Modifier|Config", meta = (
		EditConditionHides,
		EditCondition = "DurationType != EAttributeModifierType::Instant"))
	EDurationModifierReApplicationConfig OnReapplication;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute Modifier|Config", meta = (
		EditConditionHides,
		EditCondition = "DurationType != EAttributeModifierType::Instant && OnReapplication == EDurationModifierReApplicationConfig::AddStack"))
	bool HasMaxStacks;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute Modifier|Config", meta = (
		EditConditionHides,
		EditCondition = "HasMaxStacks && DurationType != EAttributeModifierType::Instant && OnReapplication == EDurationModifierReApplicationConfig::AddStack"))
	int32 MaxStacks;

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
		EditCondition = "DurationType != EAttributeModifierType::Instant"))
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
	
	UPROPERTY(EditAnywhere, Instanced, Category = "Attribute Modifier|Actions", meta = (TitleProperty = "Description"))
	TArray<TObjectPtr<UModifierAction>> ModifierActions;

	UPROPERTY(BlueprintReadWrite, Category = "Attribute Modifier|State")
	float Magnitude = 0;
	
	UPROPERTY(BlueprintReadWrite, Category = "Attribute Modifier|State")
	int32 Stacks = 1;
	
	/* SimpleAbilityBase overrides */
	void InitializeModifier(USimpleGameplayAbilityComponent* Target, const float ModifierMagnitude);
	virtual bool CanActivate(USimpleGameplayAbilityComponent* ActivatingAbilityComponent, const FAbilityContextCollection ActivationContext) override;
	virtual bool Activate(USimpleGameplayAbilityComponent* ActivatingAbilityComponent, const FGuid NewAbilityID, const FAbilityContextCollection ActivationContext) override;
	virtual void Cancel(FGameplayTag CancelStatus, FInstancedStruct CancelContext) override;
	virtual void End(FGameplayTag EndStatus, FInstancedStruct EndContext) override;
	virtual void TakeSnapshotInternal(const FInstancedStruct SnapshotData, const FOnSnapshotResolved& OnResolved) override;
	
	/* Modifier Lifecycle Functions */
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Attribute Modifier|Application")
	bool CanApplyModifier() const;
	virtual bool CanApplyModifier_Implementation() const { return true; }
	bool CanApplyModifierInternal() const;
	
	UFUNCTION(BlueprintCallable, Category = "Attribute Modifier|Lifecycle")
	void AddModifierStack(int32 StackCount);
	
	/* Blueprint Implementable Events */
	
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
	void OnMaxStacksReached();
	void OnMaxStacksReached_Implementation() {}
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attribute Modifier|State")
	USimpleGameplayAbilityComponent* InstigatorAbilityComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attribute Modifier|State")
	USimpleGameplayAbilityComponent* TargetAbilityComponent;

	UPROPERTY(BlueprintReadWrite, Category = "Attribute Modifier|State")
	FAttributeModifierActionScratchPad ModifierActionScratchPad;

	UFUNCTION(BlueprintCallable, Category = "Attribute Modifier|Application")
	bool ApplyActionStacks(const EAttributeModifierPhase& Phase, USimpleAttributeModifier* OwningModifier);
	
	UFUNCTION()
	void OnTagsChanged(FGameplayTag EventTag, FGameplayTag Domain, FInstancedStruct Payload, UObject* Sender = nullptr);
	
	// No tick function calls needed for attribute modifiers as they're based on timers
	virtual bool IsTickable() const override { return false; }

private:
	FTimerHandle DurationTimerHandle;
	FTimerHandle TickTimerHandle;

	void OnDurationTimerExpired();
	void OnTickTimerTriggered();
};
