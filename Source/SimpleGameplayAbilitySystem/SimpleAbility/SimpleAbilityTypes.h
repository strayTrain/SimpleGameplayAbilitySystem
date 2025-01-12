#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "InstancedStruct.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "SimpleAbilityTypes.generated.h"

class USimpleAbilityBase;
class USimpleAbilityComponent;
class USimpleAttributeModifier;
class UAbilityStateResolver;

/* Delegates */

DECLARE_DYNAMIC_DELEGATE_FourParams(
	FResolveStateMispredictionDelegate,
	FInstancedStruct, AuthorityStateData,
	double, AuthorityTimeStamp,
	FInstancedStruct, PredictedStateData,
	double, PredictedTimeStamp);

/* Enums */

/**
 * The different ways an ability can be activated.
 */
UENUM(BlueprintType)
enum class EAbilityActivationPolicy :uint8
{
	/* The ability is activated only on the local client. Use this for single player games or non gameplay critical/cosmetic abilities in multiplayer games. */
	LocalOnly,
	/* The ability is activated on the client immediately and then activated on the server through a reliable RPC.
	If called from the server e.g. listen server scenario, it behaves the same as ServerInitiated. */
	LocalPredicted,
	/* The ability is activated on the server first and then activated on all connected clients through a reliable multicast RPC.
	If called from the client this will send a reliable RPC to the server which then reliably multicasts activation to all connected clients. */
	ServerInitiated,
	/* The ability is only activated on the server. If called from the client the ability will fail to activate. */
	ServerOnly
};

UENUM(BlueprintType)
enum class EAbilityInstancingPolicy : uint8
{
	/* Only one instance of this ability can be active at a time.
	If we activate it again, the previous instance will be cancelled */
	SingleInstanceCancellable,
	
	/* Same as SingleInstanceCancellable except instead of cancelling the currently running instance,
	the ability will fail to activate again until the instance ability has ended */
	SingleInstanceNonCancellable,
	
	/* Multiple instances of this ability can be active at the same time.
	Activating the ability again will create a new instance and they both run in parallel */
	MultipleInstances
};

UENUM(BlueprintType)
enum class EAbilityStatus :uint8
{
	/* The ability is created but not activated yet */
	PreActivation,
	/* The ability passed all activation requirements and is running */
	ActivationSuccess,
	/* The ability failed to activate because of missing requirements (tags, etc.) */
	EndedActivationFailed,
	/* The ability ran to completion and ended successfully */
	EndedSuccessfully,
	/* The ability was cancelled for some reason */
	EndedCancelled,
	/* The ability was ended with a custom status */
	EndedCustomStatus,
};

/* Structs */

USTRUCT(BlueprintType)
struct FAbilityEventActivationConfig
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag EventTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag DomainTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool RequireContext = false;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "RequireContext"))
	UScriptStruct* RequiredContextType;
};

USTRUCT(BlueprintType)
struct FSimpleAbilitySnapshot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGuid AbilityID;
	
	UPROPERTY(BlueprintReadWrite)
	FGameplayTag StateTag;

	UPROPERTY(BlueprintReadWrite)
	double TimeStamp;

	UPROPERTY(BlueprintReadWrite)
	FInstancedStruct StateData;

	UPROPERTY()
	bool WasClientSnapshotResolved = false;
};

USTRUCT(BlueprintType)
struct FAbilityState : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGuid AbilityID;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TSubclassOf<USimpleAbilityBase> AbilityClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FGameplayTagContainer AbilityTags;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	double ActivationTimeStamp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FInstancedStruct ActivationContext;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EAbilityStatus AbilityStatus = EAbilityStatus::PreActivation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FSimpleAbilitySnapshot> SnapshotHistory;
};

DECLARE_DELEGATE_OneParam(FOnAbilityStateAdded, const FAbilityState&);
DECLARE_DELEGATE_OneParam(FOnAbilityStateChanged, const FAbilityState&);
DECLARE_DELEGATE_OneParam(FOnAbilityStateRemoved, const FAbilityState&);

USTRUCT()
struct FAbilityStateContainer : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	TArray<FAbilityState> AbilityStates;

	FOnAbilityStateAdded   OnAbilityStateAdded;
	FOnAbilityStateChanged OnAbilityStateChanged;
	FOnAbilityStateRemoved OnAbilityStateRemoved;
	
	void PostReplicatedAdd(const TArrayView< int32 >& AddedIndices, int32 FinalSize)
	{
		if (OnAbilityStateAdded.IsBound())
		{
			for (const int32 AddedIndex : AddedIndices)
			{
				OnAbilityStateAdded.Execute(AbilityStates[AddedIndex]);
			}
		}
	}
	
	void PostReplicatedChange(const TArrayView< int32 >& ChangedIndices, int32 FinalSize)
	{
		if (OnAbilityStateChanged.IsBound())
		{
			for (const int32 ChangedIndex : ChangedIndices)
			{
				OnAbilityStateChanged.Execute(AbilityStates[ChangedIndex]);
			}
		}
	}

	void PreReplicatedRemove (const TArrayView< int32 >& RemovedIndices, int32 FinalSize)
	{
		if (OnAbilityStateRemoved.IsBound())
		{
			for (const int32 RemovedIndex : RemovedIndices)
			{
				OnAbilityStateRemoved.Execute(AbilityStates[RemovedIndex]);
			}
		}
	}

	bool NetDeltaSerialize(FNetDeltaSerializeInfo & DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FAbilityState, FAbilityStateContainer>(AbilityStates, DeltaParms, *this);
	}
};

template<>
struct TStructOpsTypeTraits<FAbilityStateContainer> : public TStructOpsTypeTraitsBase2<FAbilityStateContainer>
{
	enum 
	{
		WithNetDeltaSerializer = true,
	};
};
