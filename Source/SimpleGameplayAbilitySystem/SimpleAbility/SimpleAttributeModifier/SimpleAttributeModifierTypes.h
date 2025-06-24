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
	ResetDurationTimer,
	ExtendDurationTimer,
	AddStack,
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