#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5)
	#include "StructUtils/InstancedStruct.h"
#else
	#include "InstancedStruct.h"
#endif

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
	 * SetDuration modifiers are applied for a set duration in seconds and then removed.
	 */
	SetDuration,
	/**
	 * InfiniteDuration modifiers are applied indefinitely and must be removed manually.
	 */
	InfiniteDuration
};

UENUM(BlueprintType)
enum class EDurationModifierReApplicationConfig : uint8
{
	ResetDuration,
	IncreaseDuration,
	AddStack,
};

UENUM(BlueprintType)
enum class EAttributeModiferNotFoundBehaviour : uint8
{
	CancelModifier,
	SkipAttributeModification,
};

UENUM(BlueprintType)
enum class EAttributeModifierApplicationPolicy : uint8
{
	/**
	 * Apples locally only with no replication. This is the default.
	 */
	ApplyLocalOnly,
	/**
	 * Applies on the server only and doesn't replicate to the client.
	 */
	ApplyServerOnly,
	/**
	 * Applies on the server only and replicates to the client.
	 */
	ApplyServerOnlyButReplicateSideEffects,
	/**
	 * Applied immediately on the client and then on the server.
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

UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EAttributeModifierPhase : uint8
{
	None = 0,
	// Triggered every time the action stack is applied.
	Default = 1 << 1,
	// Triggered when the modifier is applied successfully
	OnApplied = 1 << 2,
	// Triggered when the modifier fails to apply
	OnApplicationFailed = 1 << 3,
	// Triggered when a stack is added to a duration modifier
	OnStackAdded = 1 << 4,
	// Triggered when the maximum number of stacks is reached on a duration modifier
	OnMaxStacksReached = 1 << 5,
	// Triggered when the modifier ends successfully
	OnEnded = 1 << 6,
	// Triggered when the modifier is cancelled
	OnCancelled = 1 << 7,
};
ENUM_CLASS_FLAGS(EAttributeModifierPhase)

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
struct FAttributeModifierActionScratchPad
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FGameplayTagContainer ScratchpadTags;

	UPROPERTY(BlueprintReadWrite)
	TMap<FGameplayTag, float> ScratchpadValues;
};