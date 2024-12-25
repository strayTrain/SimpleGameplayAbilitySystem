#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "InstancedStruct.h"
#include "SimpleAbilityTypes.generated.h"

/* Delegates */

class USimpleAbilityComponent;
class USimpleAttributeModifier;
class UAbilityStateResolver;
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
	/* Same as ServerInitiated except can only be called from the server and the multicast is unreliable. Use this for non gameplay critical abilities. */
	ServerInitiatedNonReliable,
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
	Running,
	Ended,
	WaitingForServerResolve
};

/* Structs */

USTRUCT(BlueprintType)
struct FAbilityTagConfig
{
	GENERATED_BODY()

	/* Tags that can be used to classify this ability. e.g. "Melee", "Ranged", "AOE", etc. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer AbilityTags;
	
	/** These tags must be present for this ability to activate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer ActivationRequiredTags;

	/**
	 * These tags must NOT be present for this ability to activate.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer ActivationBlockingTags;

	/**
	 * Cancel abilities with these AbilityNames when this ability activates.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer CancelAbilities;
	
	/**
	 * Cancel abilities with these AbilityTags in their tag config when this ability activates.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer CancelAbilitiesWithAbilityTags;
	
	/**
	 * These tags are applied when this ability is activated and removed when it ends.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer TemporaryAppliedTags;

	/**
	 * These tags are applied when this ability is activated and not automatically removed when it ends.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer PermanentAppliedTags;
};

USTRUCT(BlueprintType)
struct FAbilityEventActivationConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool CanActivateFromEvent = false;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "CanActivateFromEvent"))
	FGameplayTag EventTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "CanActivateFromEvent"))
	FGameplayTag DomainTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "CanActivateFromEvent"))
	bool RequireContext = false;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "CanActivateFromEvent && RequireContext"))
	UScriptStruct* RequiredContextType;
};

USTRUCT(BlueprintType)
struct FSimpleGameplayAbilityConfig
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag AbilityName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EAbilityActivationPolicy ActivationPolicy = EAbilityActivationPolicy::LocalPredicted;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EAbilityInstancingPolicy InstancingPolicy = EAbilityInstancingPolicy::SingleInstanceCancellable;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FAbilityTagConfig TagConfig;

	/**
	 * If this ability can be activated by an event, configure the triggering event tags and domain tags here.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FAbilityEventActivationConfig AutoActivationEventConfig;

	/* If set to false the OnTick function of the gameplay ability won't be called for better performance */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool CanAbilityTick = false;

	/**
	Every time we change state in an ability we add it to a history list.
	This is the maximum number of states we keep in the history, the oldest history is discarded first
	The reason we limit history size is to manage bandwidth usage when replicating.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxAbilityStateHistorySize = 10;
};

USTRUCT(BlueprintType)
struct FSimpleAbilityState
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite)
	FGameplayTag StateTag;

	UPROPERTY(BlueprintReadWrite)
	double TimeStamp;

	UPROPERTY(BlueprintReadWrite)
	FInstancedStruct StateData;
	
	UPROPERTY(BlueprintReadWrite)
	bool IsClientStateResolved = false;
};

USTRUCT(BlueprintType)
struct FAbilityActivationStateChangedData
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag AbilityName;

	UPROPERTY()
	FGuid AbilityInstanceID;

	UPROPERTY()
	double TimeStamp;
};

USTRUCT(BlueprintType)
struct FActiveAbility
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGuid AbilityInstanceID;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FSimpleAbilityState> AbilityStateHistory;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EAbilityStatus AbilityStatus = EAbilityStatus::Running;
};
