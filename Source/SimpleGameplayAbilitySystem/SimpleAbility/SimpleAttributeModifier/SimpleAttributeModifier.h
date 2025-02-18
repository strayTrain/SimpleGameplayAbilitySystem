#pragma once

#include "CoreMinimal.h"
#include "SimpleAttributeModifierTypes.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleAbilityBase/SimpleAbilityBase.h"
#include "SimpleAttributeModifier.generated.h"

class USimpleGameplayAbility;

UCLASS(Blueprintable)
class SIMPLEGAMEPLAYABILITYSYSTEM_API USimpleAttributeModifier : public USimpleAbilityBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute Modifier|Config")
	EAttributeModifierType ModifierType = EAttributeModifierType::Instant;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute Modifier|Config")
	EAttributeModifierApplicationPolicy ModifierApplicationPolicy;
	
	/**
	 * Tags that can be used to classify this modifier. e.g. "DamageOverTime", "StatusEffect" etc.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute Modifier|Config")
	FGameplayTagContainer ModifierTags;

	/**
	 * If true, the modifier will not be removed until explicitly removed.
	 * If false, the modifier has a duration and will be removed after the duration has elapsed.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute Modifier|Config|Duration Config", meta = (EditCondition = "ModifierType == EAttributeModifierType::Duration"))
	bool HasInfiniteDuration = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute Modifier|Config|Duration Config", meta = (EditCondition = "ModifierType == EAttributeModifierType::Duration && !HasInfiniteDuration"))
	float Duration = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute Modifier|Config|Duration Config", meta = (EditCondition = "ModifierType == EAttributeModifierType::Duration"))
	bool TickOnApply = true;
	
	/**
	 * How often the modifier ticks. If 0 the modifier will only tick once when applied.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute Modifier|Config|Duration Config", meta = (EditCondition = "ModifierType == EAttributeModifierType::Duration"))
	float TickInterval = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute Modifier|Config|Duration Config", meta = (EditCondition = "ModifierType == EAttributeModifierType::Duration"))
	EDurationTickTagRequirementBehaviour TickTagRequirementBehaviour;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute Modifier|Config|Duration Config|Stacking Config", meta = (EditCondition = "ModifierType == EAttributeModifierType::Duration"))
	bool CanStack;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute Modifier|Config|Duration Config|Stacking Config", meta = (EditCondition = "ModifierType == EAttributeModifierType::Duration && CanStack"))
	int32 Stacks = 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute Modifier|Config|Duration Config|Stacking Config", meta = (EditCondition = "ModifierType == EAttributeModifierType::Duration && CanStack"))
	bool HasMaxStacks;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute Modifier|Config|Duration Config|Stacking Config", meta = (EditCondition = "ModifierType == EAttributeModifierType::Duration && CanStack && HasMaxStacks"))
	int32 MaxStacks;

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
	 * Cancel abilities of these classes when this modifier is applied.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute Modifier|Application")
	TArray<TSubclassOf<USimpleGameplayAbility>> CancelAbilities;
	
	/**
	 * Cancel abilities with these AbilityTags in their tag config when this modifier is applied.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute Modifier|Application")
	FGameplayTagContainer CancelAbilitiesWithAbilityTags;

	/**
	 * Cancel other duration modifiers with these tags when this modifier is applied.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute Modifier|Application")
	FGameplayTagContainer CancelModifiersWithTag;
	
	/**
	 * These tags are applied to the target ability component when this modifier is applied and removed when it ends.
	 * Only applies to duration type modifiers.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute Modifier|Application", meta = (EditCondition = "ModifierType == EAttributeModifierType::Duration"))
	FGameplayTagContainer TemporarilyAppliedTags;

	/**
	 * These tags are applied to the target ability component when this modifier is applied and must be removed manually.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute Modifier|Application")
	FGameplayTagContainer PermanentlyAppliedTags;
	
	/**
	 * These tags are removed from the target ability component when this modifier is applied.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute Modifier|Application")
	FGameplayTagContainer RemoveGameplayTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute Modifier|Modifiers", meta = (TitleProperty = "ModifierDescription"))
	TArray<FAttributeModifier> AttributeModifications;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute Modifier|Side Effects")
	TArray<FAbilitySideEffect> AbilitySideEffects;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute Modifier|Side Effects")
	TArray<FEventSideEffect> EventSideEffects;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute Modifier|Side Effects")
	TArray<FAttributeModifierSideEffect> AttributeModifierSideEffects;

	/* Modifier Lifecycle Functions */
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Attribute Modifier|Application")
	bool CanApplyModifier(FInstancedStruct ModifierContext) const;
	bool CanApplyModifierInternal(FInstancedStruct ModifierContext) const;
	
	UFUNCTION(BlueprintCallable, Category = "Attribute Modifier|Application")
	bool ApplyModifier(USimpleGameplayAbilityComponent* Instigator, USimpleGameplayAbilityComponent* Target, FInstancedStruct ModifierContext);

	UFUNCTION(BlueprintCallable, Category = "Attribute Modifier|Application")
	void ApplySideEffects(USimpleGameplayAbilityComponent* Instigator, USimpleGameplayAbilityComponent* Target, EAttributeModifierSideEffectTrigger EffectPhase);

	UFUNCTION(BlueprintCallable, Category = "Attribute Modifier|Lifecycle")
	void EndModifier(FGameplayTag EndingStatus, FInstancedStruct EndingContext);

	UFUNCTION(BlueprintCallable, Category = "Attribute Modifier|Lifecycle")
	void AddModifierStack(int32 StackCount);
	
	/* Blueprint Implementable Events */
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Attribute Modifier|Lifecycle")
	void OnPreApplyModifier();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Attribute Modifier|Lifecycle")
	void OnPostApplyModifier();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Attribute Modifier|Lifecycle")
	void OnModifierEnded(FGameplayTag EndingStatus, FInstancedStruct EndingContext);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Attribute Modifier|Lifecycle")
	void OnStacksAdded(int32 AddedStacks, int32 CurrentStacks);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Attribute Modifier|Lifecycle")
	void OnMaxStacksReached();

	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Attribute Modifier|Meta Values")
	void GetFloatMetaAttributeValue(FGameplayTag MetaAttributeTag, float& OutValue, bool& WasHandled) const;

	UFUNCTION(BlueprintNativeEvent, Category = "Attribute Modifier|Meta Values")
	FInstancedStruct GetModifiedStructAttributeValue(FGameplayTag OperationTag, FInstancedStruct StructToModify, bool& WasHandled) const;
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Attribute Modifier|Meta Values")
	FInstancedStruct GetAbilitySideEffectContext(FGameplayTag MetaTag, bool& WasHandled) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Attribute Modifier|Meta Values")
	FInstancedStruct GetEventSideEffectContext(FGameplayTag MetaTag, bool& WasHandled) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Attribute Modifier|Meta Values")
	FInstancedStruct GetAttributeModifierSideEffectContext(FGameplayTag MetaTag, bool& WasHandled) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Attribute Modifier|Meta Values")
	void GetAttributeModifierSideEffectTargets(FGameplayTag TargetsTag, USimpleGameplayAbilityComponent*& OutInstigator, USimpleGameplayAbilityComponent*& OutTarget) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Attribute Modifier|Utility")
	bool IsModifierActive() const { return bIsModifierActive; }

	virtual void ClientFastForwardState(FGameplayTag StateTag, FSimpleAbilitySnapshot LatestAuthorityState) override;
	virtual void ClientResolvePastState(FGameplayTag StateTag, FSimpleAbilitySnapshot AuthorityState, FSimpleAbilitySnapshot PredictedState) override;
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attribute Modifier|State")
	USimpleGameplayAbilityComponent* InstigatorAbilityComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attribute Modifier|State")
	USimpleGameplayAbilityComponent* TargetAbilityComponent;

	UFUNCTION()
	void OnTagsChanged(FGameplayTag EventTag, FGameplayTag Domain, FInstancedStruct Payload, AActor* Sender = nullptr);
	
	bool ApplyFloatAttributeModifier(const FAttributeModifier& Modifier, TArray<FFloatAttribute>& TempFloatAttributes, float& CurrentOverflow) const;
	bool ApplyStructAttributeModifier(const FAttributeModifier& Modifier, TArray<FStructAttribute>& TempStructAttributes) const;
	bool ApplyModifiersInternal(const EAttributeModifierSideEffectTrigger TriggerPhase);

private:
	bool bIsModifierActive = false;
	FInstancedStruct InitialModifierContext;
	FFloatAttribute* GetTempFloatAttribute(const FGameplayTag AttributeTag, TArray<FFloatAttribute>& TempFloatAttributes) const;
	FStructAttribute* GetTempStructAttribute(const FGameplayTag AttributeTag, TArray<FStructAttribute>& TempStructAttributes) const;

	FTimerHandle DurationTimerHandle;
	FTimerHandle TickTimerHandle;
};
