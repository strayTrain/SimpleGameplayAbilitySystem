#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "InstancedStruct.h"
#include "SimpleAbilityTypes.generated.h"

class USimpleAbilityBase;
class USimpleGameplayAbility;
class USimpleAttributeModifier;
class UAbilityStateResolver;

/* Enums */

/**	
 * The different ways an ability can be activated.
 */
UENUM(BlueprintType)
enum class EAbilityActivationPolicy :uint8
{
	/**
	 * The ability can be activated on the server or the client but won't be replicated. Use this for single player
	 * games or for cosmetic abilities in listen server scenarios.
	 */
	LocalOnly,
	/**
	 * The ability can only be activated on clients and will not replicate.
	 * Use this for cosmetic effects in dedicated server scenarios (listen servers will be ignored as clients in this policy)
	 */
	ClientOnly,
	/**
	 * The ability can only be activated on the server/listen server and will not replicate to clients.
	 */
	ServerOnly,
	/**
	 * The ability can immediately be activated on a client and then the server will be notified of the activation.
	 * If the server fails to activate the same ability, the client will cancel the ability on next replication from the server.
	 * This is the only activation policy that supports StateSnapshots.
	 */
	ClientPredicted,
	/**
	 * The ability can be requested to be activated by a client, but it will always run on the server first.
	 * i.e. Request activate on client -> reliable RPC sent to server -> Activates on server -> replicate AbilityState to client -> Activate on client
	 * If called from the server (e.g. a listen server), it behaves the same as ServerAuthority
	 */
	ServerInitiatedFromClient,
	/**
	 * The ability can only be activated on the server/listen server but will replicate to clients.
	 */
	ServerAuthority,
};

UENUM(BlueprintType)
enum class EAbilityInstancingPolicy : uint8
{
	/**
	 * Only one instance of this ability is created.
	 * If we try to activate it again, and the ability is already active, the previous instance will call
	 * USimpleGameplayAbility::CanCancel() and if it returns true it will be cancelled.
	 * If CanCancel() returns false, the new instance will fail to activate to the ability.
	 */
	SingleInstance,
	/**
	 * Multiple instances of this ability can be active at the same time. Activating the ability again will
	 * create a new instance and they will both run in parallel */
	MultipleInstances
};

UENUM(BlueprintType)
enum class EAbilityStatus :uint8
{
	/* The ability is created but not activated yet */
	PreActivation,
	/* The ability passed all activation requirements and is running */
	ActivationSuccess,
	/* The ability failed to activate because of missing requirements (tags, USimpleGameplayAbility::CanActivate etc.) */
	EndedActivationFailed,
	/* The ability ran to completion and ended successfully */
	EndedSuccessfully,
	/* The ability was cancelled */
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
	TSubclassOf<USimpleGameplayAbility> AbilityClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	EAbilityActivationPolicy ActivationPolicy = EAbilityActivationPolicy::LocalOnly;
	
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

	// Used to keep track of the order of snapshots so we can select the correct one when resolving mispredictions
	UPROPERTY()
	int32 SequenceNumber = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGuid AbilityID;
	
	UPROPERTY(BlueprintReadWrite)
	FGameplayTag SnapshotTag;

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
	EAbilityActivationPolicy ActivationPolicy;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	double ActivationTimeStamp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FInstancedStruct ActivationContext;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FInstancedStruct EndingContext;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EAbilityStatus AbilityStatus = EAbilityStatus::PreActivation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FSimpleAbilitySnapshot> SnapshotHistory;

	bool operator==(const FAbilityState& Other) const
	{
		return AbilityID == Other.AbilityID;
	}
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

USTRUCT(BlueprintType)
struct FSimpleAbilityEndedEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FGuid AbilityID;
	
	UPROPERTY(BlueprintReadWrite)
	FGameplayTag EndStatus;

	UPROPERTY(BlueprintReadWrite)
	FInstancedStruct EndingContext;
};

/* Delegates */

DECLARE_DYNAMIC_DELEGATE_FourParams(
	FResolveStateMispredictionDelegate,
	FInstancedStruct, AuthorityStateData,
	double, AuthorityTimeStamp,
	FInstancedStruct, PredictedStateData,
	double, PredictedTimeStamp);

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
	FOnSnapshotResolved,
	FGameplayTag, StateTag,
	FSimpleAbilitySnapshot, AuthoritySnapshot,
	FSimpleAbilitySnapshot, ClientSnapshot);