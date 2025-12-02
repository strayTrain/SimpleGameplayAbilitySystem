#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "SimpleGameplayAbilitySystem/UtilityClasses/FastArraySerializerMacros.h"

#if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5)
	#include "StructUtils/InstancedStruct.h"
#else
	#include "InstancedStruct.h"
#endif

#include "SimpleAttributeModifierTypes.generated.h"

class USimpleAttributeModifier;
class USimpleAttributeComponent;
class UModifierAction;
class UCurveFloat;

/* Enums */

UENUM(BlueprintType)
enum class EAttributeModifierDurationType : uint8
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
enum class EStackReapplicationBehavior : uint8
{
	/**
	 * Replace the oldest instance in the stack group with a new one (fresh duration).
	 */
	ReplaceOldest,
	/**
	 * Extend the oldest instance's duration and deny the new application.
	 */
	ExtendDuration,
};

UENUM(BlueprintType)
enum class EStackGroupOverflowBehavior : uint8
{
	/**
	 * Deny the new application if at max stacks.
	 */
	DenyNew,
	/**
	 * Remove the oldest instance and apply the new one.
	 */
	ReplaceOldest,
	/**
	 * Remove the newest instance and apply the new one.
	 */
	ReplaceNewest,
	/**
	 * Extend the oldest instance's duration and deny the new application.
	 */
	ExtendOldest,
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

UENUM(BlueprintType)
enum class EModifierStatus : uint8
{
	Applied,
	Cancelled,
	Ended,
};

/**
 * Determines how duration is handled when stacks are added/removed.
 */
UENUM(BlueprintType)
enum class EStackDurationMode : uint8
{
	/**
	 * All stacks share the same duration timer. When a new stack is added,
	 * the OnReapplication config determines whether to reset/extend the timer.
	 */
	SharedDuration,
	/**
	 * Each stack adds to the total duration. For example, with DurationPerStack=5s
	 * and 3 stacks, total duration is 15s.
	 */
	AdditiveDuration,
	/**
	 * Each stack has its own independent timer. When a stack's timer expires,
	 * only that stack is removed.
	 */
	IndependentDurations
};

/**
 * Determines how stacks are removed when RemoveStacks is called.
 */
UENUM(BlueprintType)
enum class EStackRemovalPolicy : uint8
{
	/** Remove all stacks at once, ending the modifier */
	RemoveAll,
	/** Remove one stack at a time */
	RemoveOne,
	/** Remove a specified number of stacks */
	RemoveCount
};

/**
 * Determines how magnitude is calculated based on stack count.
 */
UENUM(BlueprintType)
enum class EMagnitudeScalingSource : uint8
{
	/** Linear scaling: ScaledMagnitude = BaseMagnitude * StackCount */
	Linear,
	/** Use a UCurveFloat to map stack count to a multiplier */
	CurveAsset,
	/** Use a custom function callback for maximum flexibility */
	FunctionCallback
};

/* Structs */

USTRUCT(BlueprintType)
struct FAttributeModifierActionScratchPadValue
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag ScratchpadTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ScratchpadValue;
	
	bool operator==(const FAttributeModifierActionScratchPadValue& Other) const
	{
		return ScratchpadTag == Other.ScratchpadTag && ScratchpadValue == Other.ScratchpadValue;
	}
};

USTRUCT(BlueprintType)
struct FAttributeModifierActionScratchPadStruct
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag ScratchpadTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FInstancedStruct ScratchpadStruct;
	
	bool operator==(const FAttributeModifierActionScratchPadStruct& Other) const
	{
		return ScratchpadTag == Other.ScratchpadTag && ScratchpadStruct == Other.ScratchpadStruct;
	}
};

USTRUCT(BlueprintType)
struct FAttributeModifierActionScratchPad
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer ScratchpadTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FAttributeModifierActionScratchPadValue> ScratchpadValues;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FAttributeModifierActionScratchPadStruct> ScratchpadStructs;
	
	bool operator==(const FAttributeModifierActionScratchPad& Other) const
	{
		if (ScratchpadTags != Other.ScratchpadTags) return false;

		if (ScratchpadValues.Num() != Other.ScratchpadValues.Num()) return false;
		for (int32 i = 0; i < ScratchpadValues.Num(); i++)
		{
			if (ScratchpadValues[i] != Other.ScratchpadValues[i]) return false;
		}

		if (ScratchpadStructs.Num() != Other.ScratchpadStructs.Num()) return false;
		for (int32 i = 0; i < ScratchpadStructs.Num(); i++)
		{
			if (ScratchpadStructs[i] != Other.ScratchpadStructs[i]) return false;
		}
		return true;
	}
};

USTRUCT(BlueprintType)
struct FModifierActionResult
{
	GENERATED_BODY()

	UPROPERTY()
	int32 ActionIndex;

	UPROPERTY()
	TSubclassOf<UModifierAction> ActionClass;

	UPROPERTY()
	FAttributeModifierActionScratchPad InputScratchpad;
	
	UPROPERTY()
	FAttributeModifierActionScratchPad OutputScratchpad;
	
	bool operator==(const FModifierActionResult& Other) const
	{
		return ActionIndex == Other.ActionIndex && InputScratchpad == Other.InputScratchpad && OutputScratchpad == Other.OutputScratchpad;
	}
};

USTRUCT(BlueprintType)
struct FModifierActionStackResults
{
	GENERATED_BODY()

	UPROPERTY()
	TSubclassOf<USimpleAttributeModifier> ModifierClass;

	UPROPERTY()
	TArray<FModifierActionResult> ActionsResults;
};

/* Stacking Structs */

/**
 * Tracks individual stack duration timers for IndependentDurations mode.
 */
USTRUCT(BlueprintType)
struct FStackDurationEntry
{
	GENERATED_BODY()

	/** Index of this stack (0-based) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 StackIndex = 0;

	/** Server time when this stack expires */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float ExpirationTime = 0.0f;

	bool operator==(const FStackDurationEntry& Other) const
	{
		return StackIndex == Other.StackIndex;
	}
};

/**
 * Configuration for threshold-based events that trigger at specific stack counts.
 */
USTRUCT(BlueprintType)
struct FStackThresholdTrigger
{
	GENERATED_BODY()

	/** The stack count at which this threshold triggers */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ThresholdCount = 1;

	/** Event tag sent when threshold is crossed upward (stack count increases to or past threshold) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag OnThresholdReachedTag;

	/** Event tag sent when threshold is crossed downward (stack count decreases below threshold) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag OnThresholdLostTag;

	/** Tags granted to the target when at or above this threshold, removed when below */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer ThresholdGrantedTags;
};

/**
 * Main configuration struct for the consolidated stacking system.
 * When enabled, modifiers are grouped by class by default. Use StackGroupTag
 * to group different modifier classes together (e.g., multiple poison effects sharing a stack).
 */
USTRUCT(BlueprintType)
struct FStackingConfig
{
	GENERATED_BODY()

	/** Enable the consolidated stacking system for this modifier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bEnableStacking = false;

	/**
	 * Optional tag to group different modifier classes together.
	 * If empty, modifiers are grouped by their UClass (most common case).
	 * Use this when you want different modifier types to share a stack limit
	 * (e.g., AM_PoisonDamage and AM_PoisonSlow sharing an "Effect.Poison" group).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bEnableStacking", EditConditionHides))
	FGameplayTag StackGroupTag;

	/** Maximum number of stacks allowed (0 = unlimited) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bEnableStacking", EditConditionHides, ClampMin = "0"))
	int32 MaxStacks = 0;

	/** What happens when trying to add a stack beyond MaxStacks */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bEnableStacking && MaxStacks > 0", EditConditionHides))
	EStackGroupOverflowBehavior OverflowBehavior = EStackGroupOverflowBehavior::DenyNew;

	/** How duration is handled when stacks are added/removed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bEnableStacking", EditConditionHides))
	EStackDurationMode DurationMode = EStackDurationMode::SharedDuration;

	/** Duration added per stack (only used with AdditiveDuration mode) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bEnableStacking && DurationMode == EStackDurationMode::AdditiveDuration", EditConditionHides, ClampMin = "0.0"))
	float DurationPerStack = 0.0f;

	/** What happens when applying a modifier that already has active stacks */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bEnableStacking && DurationMode == EStackDurationMode::SharedDuration", EditConditionHides))
	EStackReapplicationBehavior OnReapplication = EStackReapplicationBehavior::ReplaceOldest;

	/** How magnitude is calculated based on stack count */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bEnableStacking", EditConditionHides))
	EMagnitudeScalingSource MagnitudeScalingSource = EMagnitudeScalingSource::Linear;

	/** Curve asset for magnitude scaling (X = stack count, Y = multiplier) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bEnableStacking && MagnitudeScalingSource == EMagnitudeScalingSource::CurveAsset", EditConditionHides))
	UCurveFloat* MagnitudeScalingCurve = nullptr;

	/** Threshold triggers that fire events at specific stack counts */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bEnableStacking", EditConditionHides))
	TArray<FStackThresholdTrigger> ThresholdTriggers;

	/** If true, re-run actions when stack count changes (fires StackChanged event) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bEnableStacking", EditConditionHides))
	bool bRerunActionsOnStackChange = false;
};

/* FFastArraySerializer Structs */

// ModifierState
USTRUCT(BlueprintType)
struct FAttributeModifierState : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGuid ModifierID;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TSubclassOf<USimpleAttributeModifier> ModifierClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FInstancedStruct ModifierContext;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float ModifierMagnitude = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EModifierStatus ModifierStatus;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USimpleAttributeComponent* InstigatorAttributeComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USimpleAttributeComponent* TargetAttributeComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float ApplicationTimestamp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float EndedTimestamp;

	/** Current stack count for consolidated stacking (1 for non-stacking modifiers) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 StackCount = 1;

	/** Individual stack expiration times for IndependentDurations mode */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FStackDurationEntry> StackDurations;

	/** Computed magnitude after scaling based on stack count */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float ScaledMagnitude = 0.0f;

	bool operator==(const FAttributeModifierState& Other) const
	{
		return ModifierID == Other.ModifierID;
	}

	friend uint32 GetTypeHash(const FAttributeModifierState& State)
	{
		return GetTypeHash(State.ModifierID);
	}
};

DECLARE_FAST_ARRAY_SERIALIZER_DELEGATES(FAttributeModifierState, AttributeModifierState)

USTRUCT()
struct FAttributeModifierStateContainer : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	TArray<FAttributeModifierState> ModifierStates;

	FOnAttributeModifierStateAdded   OnStateAdded;
	FOnAttributeModifierStateChanged OnStateChanged;
	FOnAttributeModifierStateRemoved OnStateRemoved;
	
	void PostReplicatedAdd(const TArrayView< int32 >& AddedIndices, int32 FinalSize)
	{
		if (OnStateAdded.IsBound())
		{
			for (const int32 AddedIndex : AddedIndices)
			{
				OnStateAdded.Execute(ModifierStates[AddedIndex]);
			}
		}
	}
	
	void PostReplicatedChange(const TArrayView< int32 >& ChangedIndices, int32 FinalSize)
	{
		if (OnStateChanged.IsBound())
		{
			for (const int32 ChangedIndex : ChangedIndices)
			{
				OnStateChanged.Execute(ModifierStates[ChangedIndex]);
			}
		}
	}

	void PreReplicatedRemove (const TArrayView< int32 >& RemovedIndices, int32 FinalSize)
	{
		if (OnStateRemoved.IsBound())
		{
			for (const int32 RemovedIndex : RemovedIndices)
			{
				OnStateRemoved.Execute(ModifierStates[RemovedIndex]);
			}
		}
	}

	bool NetDeltaSerialize(FNetDeltaSerializeInfo & DeltaParms)
	{
		return FastArrayDeltaSerialize<FAttributeModifierState, FAttributeModifierStateContainer>(ModifierStates, DeltaParms, *this);
	}
};

DECLARE_FAST_ARRAY_SERIALIZER_TRAITS(FAttributeModifierStateContainer)

// ModifierMutation
USTRUCT(BlueprintType)
struct FAttributeModifierMutation : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 MutationCounter = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGuid ModifierID;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TSubclassOf<USimpleAttributeModifier> ModifierClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float MutationTimestamp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FModifierActionStackResults ActionStackResult;

	bool operator==(const FAttributeModifierMutation& Other) const
	{
		// Identity by ModifierID (+ MutationCounter if you want stricter identity)
		return ModifierID == Other.ModifierID && MutationCounter == Other.MutationCounter;
	}

	friend uint32 GetTypeHash(const FAttributeModifierMutation& State)
	{
		return HashCombine(GetTypeHash(State.ModifierID), ::GetTypeHash(State.MutationCounter));
	}
};

DECLARE_FAST_ARRAY_SERIALIZER_DELEGATES(FAttributeModifierMutation, AttributeModifierMutation)

USTRUCT()
struct FAttributeModifierMutationContainer : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	TArray<FAttributeModifierMutation> Mutations;

	FOnAttributeModifierMutationAdded   OnStateAdded;
	FOnAttributeModifierMutationChanged OnStateChanged;
	FOnAttributeModifierMutationRemoved OnStateRemoved;
	
	void PostReplicatedAdd(const TArrayView< int32 >& AddedIndices, int32 FinalSize)
	{
		if (OnStateAdded.IsBound())
		{
			for (const int32 AddedIndex : AddedIndices)
			{
				OnStateAdded.Execute(Mutations[AddedIndex]);
			}
		}
	}
	
	void PostReplicatedChange(const TArrayView< int32 >& ChangedIndices, int32 FinalSize)
	{
		if (OnStateChanged.IsBound())
		{
			for (const int32 ChangedIndex : ChangedIndices)
			{
				OnStateChanged.Execute(Mutations[ChangedIndex]);
			}
		}
	}

	void PreReplicatedRemove (const TArrayView< int32 >& RemovedIndices, int32 FinalSize)
	{
		if (OnStateRemoved.IsBound())
		{
			for (const int32 RemovedIndex : RemovedIndices)
			{
				OnStateRemoved.Execute(Mutations[RemovedIndex]);
			}
		}
	}

	bool NetDeltaSerialize(FNetDeltaSerializeInfo & DeltaParms)
	{
		return FastArrayDeltaSerialize<FAttributeModifierMutation, FAttributeModifierMutationContainer>(Mutations, DeltaParms, *this);
	}
};

DECLARE_FAST_ARRAY_SERIALIZER_TRAITS(FAttributeModifierMutationContainer)

/* Attribute Snapshots for Rollback */

USTRUCT(BlueprintType)
struct FAttributeSnapshot
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<struct FFloatAttribute> FloatAttributes;

	UPROPERTY()
	TArray<struct FStructAttribute> StructAttributes;

	UPROPERTY()
	TArray<struct FGameplayTagCounter> GameplayTags;
};

USTRUCT()
struct FPredictedModifierSnapshot
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid ModifierID;

	UPROPERTY()
	FAttributeSnapshot AttributeSnapshot;

	UPROPERTY()
	double SnapshotTimestamp;

	bool operator==(const FPredictedModifierSnapshot& Other) const
	{
		return ModifierID == Other.ModifierID;
	}
};

/* Event Dispatchers */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnModifierAppliedSignature, USimpleAttributeModifier*, ModifierInstance);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnActionStackAppliedSignature, USimpleAttributeModifier*, ModifierInstance, FModifierActionStackResults, Results);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnModifierEndedSignature, USimpleAttributeModifier*, ModifierInstance, FGameplayTag, EndStatus, FInstancedStruct, EndContext);