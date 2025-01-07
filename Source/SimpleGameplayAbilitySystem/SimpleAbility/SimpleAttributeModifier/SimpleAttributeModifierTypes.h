#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "InstancedStruct.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleAbilityTypes.h"
#include "SimpleGameplayAbilitySystem/SimpleGameplayAbilityComponent/SimpleAbilityComponentTypes.h"
#include "SimpleAttributeModifierTypes.generated.h"

/* Enums */

class USimpleGameplayAbility;
class USimpleGameplayAbilityComponent;
class USimpleAttributeModifier;

UENUM(BlueprintType)
enum class EAttributeModifierType : uint8
{
	/**
	 * Instant modifiers are applied immediately and then removed.
	 */
	Instant,
	/**
	 * Duration modifiers are applied for a set duration (duration can be infinity) and then removed.
	 */
	Duration,
};

UENUM(BlueprintType)
enum class EAttributeModifierApplicationPolicy : uint8
{
	/**
	 * This modifier will be applied immediately on the client and then on the server. When the server applies
	 * the modifier it will be replicated to the client and corrections happen as needed
	 */
	ApplyPredicted,
	/**
	 * This modifier can only be applied on the server and then replicated to the client.
	 * Applying this modifier on the client will have no effect.
	 */
	ApplyServerOnly,
};

UENUM(BlueprintType)
enum class EAttributeModifierSideEffectTrigger : uint8
{
	OnInstantModifierAppliedSuccess,
	OnInstantModifierEndedCancel,
	OnDurationModifierInitiallyAppliedSuccess,
	OnDurationModifierTickSuccess,
	OnDurationModifierEndedSuccess,
	OnDurationModifierEndedCancel,
	OnDurationModifierTickCancel,
};

UENUM(BlueprintType)
enum class EAttributeModifierSideEffectTarget : uint8
{
	Target,
	Instigator,
};

UENUM(BlueprintType)
enum class EDurationTickTagRequirementBehaviour : uint8
{
	/**
	 * The timer will continue to tick but the modifier won't be applied.
	 * e.g. A duration modifier with a duration of 10 seconds and a tick interval of 1 second will tick 10 times.
	 * If the modifier is blocked from ticking for 5 seconds it will tick 5 times but the modifier will still last 10 seconds.
	 */
	SkipOnTagRequirementFailed,
	/**
	 * The modifier timer will freeze if required/blocking tags prevent it from ticking and won't apply. The modifier resumes
	 * when it passes the tag check again.
	 */
	PauseOnTagRequirementFailed,
	/**
	 * The modifier will be cancelled if tags prevent it from ticking.
	 */
	CancelOnTagRequirementFailed,
};

/* Structs */

USTRUCT(BlueprintType)
struct FAttributeModifier
{
	GENERATED_BODY()

	/**
	 * The name of the modifier. This is cosmetic only and used to change the title of the modifier in the editor UI.
	 * Can be left blank if desired. 
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ModifierName = "Modifier";

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EAttributeType AttributeType;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag ModifiedAttribute;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "AttributeType == EAttributeType::FloatAttribute", EditConditionHides))
	EAttributeValueType ModifiedAttributeValueType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool CancelIfAttributeNotFound = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "AttributeType == EAttributeType::FloatAttribute", EditConditionHides))
	EFloatAttributeModificationOperation ModificationOperation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "AttributeType == EAttributeType::FloatAttribute", EditConditionHides))
	EAttributeModificationValueSource ModificationInputValueSource;

	/**
	 * If true, the modifier set the overflow to 0 after using it even if there is overflow left. Use this when you want to reset the overflow between modifiers.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "AttributeType == EAttributeType::FloatAttribute && ModificationInputValueSource == EAttributeModificationValueSource::FromOverflow", EditConditionHides))
	bool ConsumeOverflow = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "ModificationInputValueSource == EAttributeModificationValueSource::Manual && AttributeType == EAttributeType::FloatAttribute", EditConditionHides))
	float ManualInputValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "AttributeType == EAttributeType::FloatAttribute && (ModificationInputValueSource == EAttributeModificationValueSource::FromInstigatorAttribute || ModificationInputValueSource == EAttributeModificationValueSource::FromTargetAttribute)", EditConditionHides))
	FGameplayTag SourceAttribute;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "AttributeType == EAttributeType::FloatAttribute && (ModificationInputValueSource == EAttributeModificationValueSource::FromInstigatorAttribute || ModificationInputValueSource == EAttributeModificationValueSource::FromTargetAttribute)", EditConditionHides))
	EAttributeValueType SourceAttributeValueType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "AttributeType == EAttributeType::StructAttribute", EditConditionHides))
	FGameplayTag StructModifierTag;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "ModificationInputValueSource == EAttributeModificationValueSource::FromMetaAttribute && AttributeType == EAttributeType::FloatAttribute", EditConditionHides))
	FGameplayTag MetaAttributeTag;
};

USTRUCT(BlueprintType)
struct FPendingAttributeModifier
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USimpleGameplayAbilityComponent* Instigator;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<USimpleAttributeModifier> AttributeModifierClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FInstancedStruct AttributeModifierContext;
};

USTRUCT(BlueprintType)
struct FAbilitySideEffect
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<EAttributeModifierSideEffectTrigger> ApplicationTriggers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EAttributeModifierSideEffectTarget SideEffectTarget;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<USimpleGameplayAbility> AbilityClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EAbilityActivationPolicy ActivationPolicy;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool EndIfDurationModifierEnds = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool CancelIfDurationModifierCancels = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag AbilityContextTag;
};

USTRUCT(BlueprintType)
struct FEventSideEffect
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<EAttributeModifierSideEffectTrigger> ApplicationTriggers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EAttributeModifierSideEffectTarget SideEffectTarget;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag EventTag;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag EventDomain;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ESimpleEventReplicationPolicy EventReplicationPolicy;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag EventContextTag;
};

USTRUCT(BlueprintType)
struct FAttributeModifierSideEffect
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<EAttributeModifierSideEffectTrigger> ApplicationTriggers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EAttributeModifierSideEffectTarget SideEffectTarget;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<USimpleAttributeModifier> AttributeModifierClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag ModifierContextTag;
};

USTRUCT(BlueprintType)
struct FAttributeModifierConfiguration
{
	GENERATED_BODY()

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

	/**
	 * How often the modifier ticks. If 0 the modifier will only tick once when applied.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute Modifier|Config|Duration Config", meta = (EditCondition = "ModifierType == EAttributeModifierType::Duration"))
	float TickInterval = 1;
	
	/**
	 * If true, the modifier will undo any float attribute modifications it made when it is removed.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute Modifier|Config|Duration Config", meta = (EditCondition = "ModifierType == EAttributeModifierType::Duration"))
	bool UndoFloatAttributeModificationsOnRemoval = false;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute Modifier|Modifiers", meta = (TitleProperty = "ModifierName"))
	TArray<FAttributeModifier> AttributeModifications;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute Modifier|Side Effects")
	TArray<FAbilitySideEffect> AbilitySideEffects;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute Modifier|Side Effects")
	TArray<FEventSideEffect> EventSideEffects;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute Modifier|Side Effects")
	TArray<FAttributeModifierSideEffect> AttributeModifierSideEffects;
};
