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
	 * This modifier can only be applied on the server. Changed attributes will be replicated to the client.
	 */
	ApplyServerOnly,
	/**
	 * This modifier only runs on the server but any side effects are replicated to the client.
	 */
	ApplyServerOnlyButReplicateSideEffects,
	/**
	 * This modifier will be applied immediately on the client and then on the server. The client won't modify any attributes
	 * but it will simulate their modification and execute side effects based on that simulated modification. Tag adding/removing is also replicated.
	 * The server then runs the side effect and modifies the attributes, then replicates the changed attributes and any side effects to the client, which then corrects it's simulated state.
	 */
	ApplyClientPredicted,
};

UENUM(BlueprintType)
enum class EAttributeModifierSideEffectTrigger : uint8
{
	OnInstantModifierEndedSuccess,
	OnInstantModifierEndedCancel,
	OnDurationModifierInitiallyAppliedSuccess,
	OnDurationModifierEndedSuccess,
	OnDurationModifierEndedCancel,
	OnDurationModifierTickSuccess,
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
	 * The description of what the modifier does. This is cosmetic only and used to change the title of the modifier in the editor UI.
	 * Can be left blank if desired. 
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ModifierDescription = "Modifier";

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EAttributeType AttributeType;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag ModifiedAttribute;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "AttributeType == EAttributeType::FloatAttribute", EditConditionHides))
	EAttributeValueType ModifiedAttributeValueType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool CancelIfAttributeNotFound = true;

	/**
	 * If the attribute modifier is a duration type, this modifier will only trigger during these phases.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<EAttributeModifierSideEffectTrigger> DurationApplicationTriggers;
	
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

	UPROPERTY(BlueprintReadWrite)
	FGuid AbilityInstanceID;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<EAttributeModifierSideEffectTrigger> ApplicationTriggers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EAttributeModifierSideEffectTarget ActivatingAbilityComponent;
	
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

	UPROPERTY(BlueprintInternalUseOnly)
	FInstancedStruct AbilityContext = FInstancedStruct();
};

USTRUCT(BlueprintType)
struct FEventSideEffect
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<EAttributeModifierSideEffectTrigger> ApplicationTriggers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EAttributeModifierSideEffectTarget EventSender;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag EventTag;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag EventDomain;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ESimpleEventReplicationPolicy EventReplicationPolicy;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag EventContextTag;

	UPROPERTY(BlueprintInternalUseOnly)
	FInstancedStruct EventContext;
};

USTRUCT(BlueprintType)
struct FAttributeModifierSideEffect
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGuid AttributeID;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<EAttributeModifierSideEffectTrigger> ApplicationTriggers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EAttributeModifierSideEffectTarget ModifierInstigator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EAttributeModifierSideEffectTarget ModifierTarget;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<USimpleAttributeModifier> AttributeModifierClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag ModifierContextTag;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag ModifierTargetsTag;

	UPROPERTY(BlueprintInternalUseOnly)
	FInstancedStruct ModifierContext;
};

USTRUCT(BlueprintType)
struct FAttributeModifierResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USimpleGameplayAbilityComponent* Instigator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USimpleGameplayAbilityComponent* Target;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FAbilitySideEffect> AppliedAbilitySideEffects;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FEventSideEffect> AppliedEventSideEffects;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FAttributeModifierSideEffect> AppliedAttributeModifierSideEffects;
};