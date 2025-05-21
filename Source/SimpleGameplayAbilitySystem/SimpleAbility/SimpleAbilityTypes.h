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
	 * If called from the server (e.g. a listen server), it behaves the same as ServerInitiated
	 */
	ServerInitiatedFromClient,
	/**
	 * The ability can only be activated on the server/listen server but will replicate to clients.
	 */
	ServerInitiated,
};

UENUM(BlueprintType)
enum class EAbilityActivationPolicyOverride :uint8
{
	DontOverride,
	ForceLocalOnly,
	ForceClientOnly,
	ForceServerOnly,
	ForceClientPredicted,
	ForceServerInitiatedFromClient,
	ForceServerInitiated,
};

UENUM(BlueprintType)
enum class ESubAbilityCancellationPolicy :uint8
{
	/** If the parent ability stops running for any reason, cancel this sub ability */
	CancelOnParentAbilityEndedOrCancelled,
	/** Cancel only if the parent ability is canceled */
	CancelOnParentAbilityCancelled,
	/** Cancel when the parent ability ends */
	CancelOnParentAbilityEnded,
	/** Won't cancel if the parent ability stops running */
	IgnoreParentAbility,
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
	ActivationFailed,
	/* The ability ran to completion and ended successfully */
	Ended,
	/* The ability was cancelled */
	Cancelled,
};

UENUM(BlueprintType)
enum class EAbilityNetworkRole :uint8
{
	/* This ability is running on either a dedicated server or a listen server or a single player game */
	Server,
	/* This ability is running on a client or listen server (because they're also clients) */
	Client,
};

/* Structs */

USTRUCT(BlueprintType)
struct FAbilityContext
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Ability|Context")
	FGameplayTag ContextTag;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Ability|Context")
	FInstancedStruct ContextData;
};

USTRUCT(BlueprintType)
struct FAbilityContextCollection
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Ability|Context")
	TArray<FAbilityContext> Contexts;
};

USTRUCT()
struct FActivatedSubAbility
{
	GENERATED_BODY()

	FGuid AbilityID;
	ESubAbilityCancellationPolicy CancellationPolicy;
};

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

/* Delegates */

// Called by an ability instance when it is activated or failed to activate
DECLARE_DYNAMIC_DELEGATE_OneParam(
	FAbilityActivationDelegate,
	FGuid, AbilityID);

// Called by an ability instance when the ability ends (either normally or canceled)
DECLARE_DYNAMIC_DELEGATE_ThreeParams(
	FAbilityStoppedDelegate,
	FGuid, AbilityID,
	FGameplayTag, StopStatus,
	FInstancedStruct, StopContext);

DECLARE_DYNAMIC_DELEGATE_FourParams(
	FResolveStateMispredictionDelegate,
	FInstancedStruct, AuthorityStateData,
	double, AuthorityTimeStamp,
	FInstancedStruct, PredictedStateData,
	double, PredictedTimeStamp);

DECLARE_DYNAMIC_DELEGATE_TwoParams(
	FOnSnapshotResolved,
	FInstancedStruct, AuthoritySnapshotData,
	FInstancedStruct, ClientSnapshotData);

/* FFastArraySerializer Structs */

// AbilityState
USTRUCT(BlueprintType)
struct FAbilityState : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TSubclassOf<USimpleAbilityBase> AbilityClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGuid AbilityID;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EAbilityStatus AbilityStatus = EAbilityStatus::PreActivation;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EAbilityActivationPolicy ActivationPolicy;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	double ActivationTimeStamp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FAbilityContextCollection ActivationContexts;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FInstancedStruct EndingContext;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	double EndingTimeStamp;

	bool operator==(const FAbilityState& Other) const
	{
		return AbilityID == Other.AbilityID;
	}

	friend uint32 GetTypeHash(const FAbilityState& State)
	{
		return GetTypeHash(State.AbilityID);
	}
};

DECLARE_FAST_ARRAY_SERIALIZER_DELEGATES(FAbilityState, AbilityState)

USTRUCT()
struct FAbilityStateContainer : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	TArray<FAbilityState> AbilityStates;

	FOnAbilityStateAdded   OnStateAdded;
	FOnAbilityStateChanged OnStateChanged;
	FOnAbilityStateRemoved OnStateRemoved;
	
	void PostReplicatedAdd(const TArrayView< int32 >& AddedIndices, int32 FinalSize)
	{
		if (OnStateAdded.IsBound())
		{
			for (const int32 AddedIndex : AddedIndices)
			{
				OnStateAdded.Execute(AbilityStates[AddedIndex]);
			}
		}
	}
	
	void PostReplicatedChange(const TArrayView< int32 >& ChangedIndices, int32 FinalSize)
	{
		if (OnStateChanged.IsBound())
		{
			for (const int32 ChangedIndex : ChangedIndices)
			{
				OnStateChanged.Execute(AbilityStates[ChangedIndex]);
			}
		}
	}

	void PreReplicatedRemove (const TArrayView< int32 >& RemovedIndices, int32 FinalSize)
	{
		if (OnStateRemoved.IsBound())
		{
			for (const int32 RemovedIndex : RemovedIndices)
			{
				OnStateRemoved.Execute(AbilityStates[RemovedIndex]);
			}
		}
	}

	bool NetDeltaSerialize(FNetDeltaSerializeInfo & DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FAbilityState, FAbilityStateContainer>(AbilityStates, DeltaParms, *this);
	}
};

DECLARE_FAST_ARRAY_SERIALIZER_TRAITS(FAbilityStateContainer)

// AbilitySnapshot
USTRUCT(BlueprintType)
struct FAbilitySnapshot : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY()
	int32 SnapshotCounter = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGuid AbilityID;
	
	UPROPERTY(BlueprintReadWrite)
	double TimeStamp;

	UPROPERTY(BlueprintReadWrite)
	FInstancedStruct SnapshotData;

	bool operator==(const FAbilitySnapshot& Other) const
	{
		return AbilityID == Other.AbilityID && SnapshotCounter == Other.SnapshotCounter;
	}
};

DECLARE_FAST_ARRAY_SERIALIZER_DELEGATES(FAbilitySnapshot, AbilitySnapshot)

USTRUCT()
struct FAbilitySnapshotContainer : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, SaveGame, meta=(TitleProperty="GameplayTag"))
	TArray<FAbilitySnapshot> Snapshots;

	FOnAbilitySnapshotAdded   OnSnapshotAdded;
	FOnAbilitySnapshotChanged OnSnapshotChanged;
	FOnAbilitySnapshotRemoved OnSnapshotRemoved;

	/** Called after new items are added on clients */
	void PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize)
	{
		if (OnSnapshotAdded.IsBound())
		{
			for (int32 Index : AddedIndices)
			{
				OnSnapshotAdded.ExecuteIfBound(Snapshots[Index]);
			}
		}
	}

	/** Called when existing items change on clients */
	void PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize)
	{
		if (OnSnapshotChanged.IsBound())
		{
			for (int32 Index : ChangedIndices)
			{
				OnSnapshotChanged.ExecuteIfBound(Snapshots[Index]);
			}
		}
	}

	/** Called before items are removed on clients */
	void PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize)
	{
		if (OnSnapshotRemoved.IsBound())
		{
			for (int32 Index : RemovedIndices)
			{
				OnSnapshotRemoved.ExecuteIfBound(Snapshots[Index]);
			}
		}
	}

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FAbilitySnapshot, FAbilitySnapshotContainer>(Snapshots, DeltaParms, *this);
	}
};

DECLARE_FAST_ARRAY_SERIALIZER_TRAITS(FAbilitySnapshotContainer)