// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SimpleGameplayAbilitySystem/SimpleAbilityComponent/SimpleAbilityComponentTypes.h"
#include "SimpleAttributeModifierTypes.generated.h"

/* Enums */

class USimpleAttributeModifier;
class USimpleAbilityComponent;

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
struct FAttributeModifierTagConfig
{
	GENERATED_BODY()

	/**
	 * Tags that can be used to classify this modifier. e.g. "DamageOverTime", "StatusEffect" etc.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer ModifierTags;
	
	/**
	 * These tags must be present on the target ability component for this modifier to apply.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer ApplicationRequiredTags;

	/**
	 * These tags must NOT be present on the target ability component for this modifier to apply.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer ApplicationBlockingTags;

	/**
	 * If another duration type modifier with these tags is already applied, this modifier will not be applied.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer ApplicationBlockingModifierTags;

	/**
	 * Cancel abilities with these AbilityNames when this modifier is applied.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer CancelAbilities;
	
	/**
	 * Cancel abilities with these AbilityTags in their tag config when this modifier is applied.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer CancelAbilitiesWithAbilityTags;

	/**
	 * Cancel other duration modifiers with these tags when this modifier is applied.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer CancelModifiersWithTag;
	
	/**
	 * These tags are applied to the target ability component when this modifier is applied and removed when it ends.
	 * Only applies to duration type modifiers.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer TemporaryAppliedTags;

	/**
	 * These tags are applied to the target ability component when this modifier is applied and must be removed manually.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer PermanentAppliedTags;
};

USTRUCT(BlueprintType)
struct FStackSettings
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool CanStack;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "CanStack"))
	bool HasMaxStacks;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "CanStack && HasMaxStacks"))
	int32 MaxStacks;	
};

USTRUCT(BlueprintType)
struct FAttributeModifierDurationSettings
{
	GENERATED_BODY()
	
	/**
	 * If true, the modifier will not be removed until explicitly removed.
	 * If false, the modifier has a duration and will be removed after the duration has elapsed.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool HasInfiniteDuration = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "!HasInfiniteDuration"))
	float Duration;

	/**
	 * How often the modifier ticks. If 0 the modifier will only tick once when applied.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float TickInterval;
	
	/**
	 * If true, the modifier will undo any float attribute modifications it made when it is removed.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool UndoFloatAttributeModificationsOnRemoval = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EDurationTickTagRequirementBehaviour TickTagRequirementBehaviour;
};

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
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "ModificationInputValueSource == EAttributeModificationValueSource::FromMetaAttribute || AttributeType == EAttributeType::StructAttribute", EditConditionHides))
	FGameplayTag MetaAttributeTag;
};

USTRUCT(BlueprintType)
struct FAttributeModifierConfig
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EAttributeModifierType ModifierType = EAttributeModifierType::Instant;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EAttributeModifierApplicationPolicy ModifierApplicationPolicy;

	/**
	 * If the ModifierType is Duration, these settings control how long the modifier lasts and how often it ticks.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "ModifierType == EAttributeModifierType::Duration", EditConditionHides))
	FAttributeModifierDurationSettings DurationConfig;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "ModifierType == EAttributeModifierType::Duration", EditConditionHides))
	int32 Stacks = 1;
	
	/**
	 * Settings controlling how/if the modifier stacks (only applies to duration type modifiers).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "ModifierType == EAttributeModifierType::Duration", EditConditionHides))
	FStackSettings StackingConfig;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (TitleProperty = "ModifierName"))
	TArray<FAttributeModifier> ApplicationModifications;
};

USTRUCT(BlueprintType)
struct FPendingAttributeModifier
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USimpleAbilityComponent* Instigator;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<USimpleAttributeModifier> AttributeModifierClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FInstancedStruct AttributeModifierContext;
};
