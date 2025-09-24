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

class UModifierAction;

/* Enums */

class USimpleAttributeModifier;

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

UENUM(BlueprintType)
enum class EModifierStatus : uint8
{
	Applied,
	AppliedFailed,
	Cancelled,
	Ended,
};

/* Structs */

USTRUCT(BlueprintType)
struct FAttributeModifierActionScratchPadValue
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FGameplayTag ScratchpadTag;

	UPROPERTY(BlueprintReadWrite)
	float ScratchpadValue;
	
	bool operator==(const FAttributeModifierActionScratchPadValue& Other) const
	{
		return ScratchpadTag == Other.ScratchpadTag && ScratchpadValue == Other.ScratchpadValue;
	}
};

USTRUCT(BlueprintType)
struct FAttributeModifierActionScratchPad
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FGameplayTagContainer ScratchpadTags;

	UPROPERTY(BlueprintReadWrite)
	TArray<FAttributeModifierActionScratchPadValue> ScratchpadValues;
	
	bool operator==(const FAttributeModifierActionScratchPad& Other) const
	{
		if (ScratchpadTags != Other.ScratchpadTags) return false;
		if (ScratchpadValues.Num() != Other.ScratchpadValues.Num()) return false;
		for (int32 i = 0; i < ScratchpadValues.Num(); i++)
		{
			if (ScratchpadValues[i] != Other.ScratchpadValues[i]) return false;
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
	FInstancedStruct ActionResult;
	
	bool operator==(const FModifierActionResult& Other) const
	{
		return ActionIndex == Other.ActionIndex && InputScratchpad == Other.InputScratchpad && ActionResult == Other.ActionResult;
	}
};

USTRUCT(BlueprintType)
struct FModifierActionStackResults
{
	GENERATED_BODY()
	
	UPROPERTY()
	TArray<FModifierActionResult> ActionsResults;
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
	EModifierStatus ModifierStatus;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float ApplicationTimestamp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float EndedTimestamp;
	
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
	
	bool operator==(const FAttributeModifierState& Other) const
	{
		return ModifierID == Other.ModifierID;
	}

	friend uint32 GetTypeHash(const FAttributeModifierMutation& State)
	{
		return GetTypeHash(State.ModifierID);
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

/* Event Dispatchers */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnActionStackAppliedSignature, FGuid, ModifierID, FModifierActionStackResults, Results);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnModifierEndedSignature, FGuid, ModifierID, FGameplayTag, EndStatus, FInstancedStruct, EndContext);