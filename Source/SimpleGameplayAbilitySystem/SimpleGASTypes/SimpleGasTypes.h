#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "InstancedStruct.h"
#include "SimpleGasTypes.generated.h"

/* Delegates */

class USimpleGameplayAbilityComponent;
class USimpleGameplayEffect;
class UGameplayAbilityStateResolver;
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
UENUM()
enum EAbilityActivationPolicy
{
	// The ability is activated only on the local client. Use this for single player games or non gameplay critical/cosmetic abilities in multiplayer games.
	LocalOnly,
	// The ability is activated on the client immediately and then activated on the server through a reliable RPC. If called from the server e.g. listen server, it behaves the same as ServerInitiated.
	LocalPredicted,
	// The ability is activated on the server first and then activated on all connected clients through a reliable multicast RPC. If called from the client this will send a reliable server RPC which then reliably multicasts activation to all connected clients.
	ServerInitiated,
	// Same as ServerInitiated except the multicast is unreliable and can only be called from the server. Use this for non gameplay critical abilities.
	ServerInitiatedNonReliable,
	// The ability is only activated on the server. If called from the client the ability will fail to activate.
	ServerOnly
};

UENUM()
enum EAbilityInstancingPolicy
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

UENUM()
enum ESimpleEventReplicationPolicy
{
	// The same as using the SimpleEventSubsystem directly. i.e. no replication
	NoReplication,
	// The event is sent on the client first and then sent on the server. If called from the server nothing happens.
	ClientToServer,
	// The event is sent on the server first and then sent to the connected client. If called from the client nothing happens.
	ServerToClient,
	// The event is sent on the server first and then to the client. If called from the client a server RPC is sent which then calls the event on the client.
	ServerToClientPredicted,
	// The event is sent on the server first and then to all connected clients. If called from the client nothing happens.
	ServerToAll,
	/* The event is sent on the client first and then to the server and then to all connected clients.
	   Behaves the same as ServerToAll if called from the server */
	ServerToAllPredicted,
};

UENUM()
enum EAttributeValueType
{
	CurrentValue,
	BaseValue,
	MaxValue,
	MinValue
};

UENUM()
enum EFloatAttributeModificationOperation
{
	Add,
	Multiply,
	Override
};

UENUM()
enum EAttributeType
{
	FloatAttribute,
	StructAttribute,
};

UENUM()
enum EAttributeModificationValueSource
{
	Manual,
	FromOverflow,
	FromAttribute,
	FromMetaAttribute,
};

UENUM()
enum EGameplayEffectType
{
	/**
	 * Instant effects are applied immediately and then removed.
	 */
	Instant,
	/**
	 * Duration effects are applied for a set duration (duration can be infinity) and then removed.
	 */
	Duration,
};

/* Structs */

USTRUCT(BlueprintType)
struct FAbilityTagConfig
{
	GENERATED_BODY()

	/**
	 * Tags that can be used to classify this ability. e.g. "Melee", "Ranged", "AOE", etc.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Ability")
	FGameplayTagContainer AbilityTags;
	
	/**
	 * These tags must be present on the owning gameplay ability component for this ability to activate.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Ability")
	FGameplayTagContainer ActivationRequiredTags;

	/**
	 * These tags must NOT be present on the owning gameplay ability component for this ability to activate.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Ability")
	FGameplayTagContainer ActivationBlockingTags;

	/**
	 * When this ability activates successfully any abilities with these tags will be cancelled.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Ability")
	FGameplayTagContainer CancelAbilitiesWithTag;
	
	/**
	 * These tags are applied to the owning gameplay ability component when this ability is activated and removed when it ends.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Ability")
	FGameplayTagContainer TemporaryAppliedTags;

	/**
	 * These tags are applied to the owning gameplay ability component when this ability is activated and not automatically removed when it ends.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Ability")
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
	UScriptStruct* RequiredContextType;
};

USTRUCT(BlueprintType)
struct FSimpleGameplayAbilityConfig
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag AbilityName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<EAbilityActivationPolicy> ActivationPolicy = EAbilityActivationPolicy::LocalPredicted;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<EAbilityInstancingPolicy> InstancingPolicy = EAbilityInstancingPolicy::SingleInstanceCancellable;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FAbilityTagConfig TagConfig;

	/**
	 * If this ability can be activated by an event, configure the triggering event tags and domain tags here.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SimpleGAS")
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
struct FAbilityState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "SimpleGAS|Ability")
	FGameplayTag StateTag;
	
	UPROPERTY(BlueprintReadWrite, Category = "SimpleGAS|Ability")
	float TimeStamp;
	
	UPROPERTY(BlueprintReadWrite, Category = "SimpleGAS|Ability")
	FInstancedStruct StateData;

	UPROPERTY(BlueprintReadWrite, Category = "SimpleGAS|Ability")
	FResolveStateMispredictionDelegate OnResolveState;

	UPROPERTY(BlueprintReadWrite, Category = "SimpleGAS|Ability")
	TSubclassOf<UGameplayAbilityStateResolver> CustomStateResolverClass;
	
	UPROPERTY(BlueprintReadWrite, Category = "SimpleGAS|Ability")
	bool IsStateResolved = false;
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

/* Attribute Structs */

USTRUCT(BlueprintType)
struct FValueLimits
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "UseMinBaseValue"))
	float MinBaseValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "UseMaxBaseValue"))
	float MaxBaseValue;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "UseMinCurrentValue"))
	float MinCurrentValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "UseMaxCurrentValue"))
	float MaxCurrentValue;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Ability")
	bool UseMinBaseValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Ability")
	bool UseMaxBaseValue;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Ability")
	bool UseMinCurrentValue;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Ability")
	bool UseMaxCurrentValue;
};

USTRUCT(BlueprintType)
struct FFloatAttribute
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Ability")
	FGameplayTag AttributeName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Ability")
	float BaseValue;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Ability")
	float CurrentValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Ability")
	FValueLimits ValueLimits;
};

USTRUCT(BlueprintType)
struct FStructAttribute
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Ability")
	FGameplayTag AttributeName;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Ability")
	UScriptStruct* AttributeType;

	UPROPERTY(BlueprintReadWrite, Category = "SimpleGAS|Ability")
	FInstancedStruct AttributeValue;
};

USTRUCT(BlueprintType)
struct FFloatAttributeModification
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Ability")
	FGameplayTag AttributeTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Ability")
	TEnumAsByte<EAttributeValueType> ValueType;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Ability")
	float NewValue;
};

USTRUCT(BlueprintType)
struct FStructAttributeModification
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Ability")
	FGameplayTag AttributeTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Ability")
	FInstancedStruct Value;
};

/* Gameplay Effect Structs */

USTRUCT(BlueprintType)
struct FEffectTagConfig
{
	GENERATED_BODY()

	/**
	 * Tags that can be used to classify this effect. e.g. "DamageOverTime", "StatusEffect" etc.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Ability")
	FGameplayTagContainer EffectTags;
	
	/**
	 * These tags must be present on the owning gameplay ability component for this effect to apply.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Ability")
	FGameplayTagContainer ApplicationRequiredComponentTags;

	/**
	 * These tags must NOT be present on the owning gameplay ability component for this effect to apply.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Ability")
	FGameplayTagContainer ApplicationBlockingComponentTags;

	/**
	 * If another duration type effect with these tags is already applied, this effect will not be applied.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Ability")
	FGameplayTagContainer ApplicationBlockingEffectTags;

	/**
	 * Cancel abilities with these tags when this effect is applied.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Ability")
	FGameplayTagContainer CancelAbilitiesWithTag;

	/**
	 * Cancel effects with these tags when this effect is applied.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Ability")
	FGameplayTagContainer CancelEffectsWithTag;
	
	/**
	 * These tags are applied to the owning gameplay ability component when this ability is activated and removed when it ends.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Ability")
	FGameplayTagContainer TemporaryAppliedComponentTags;

	/**
	 * These tags are applied to the owning gameplay ability component when this ability is activated and not automatically removed when it ends.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Ability")
	FGameplayTagContainer PermanentAppliedComponentTags;
};

USTRUCT(BlueprintType)
struct FStackSettings
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Effect")
	bool CanStack;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Effect", meta = (EditCondition = "CanStack"))
	bool HasMaxStacks;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Effect", meta = (EditCondition = "CanStack && HasMaxStacks"))
	int32 MaxStacks;	
};

USTRUCT(BlueprintType)
struct FGameplayEffectDurationSettings
{
	GENERATED_BODY()
	
	/**
	 * If true, the effect will not be removed until explicitly removed.
	 * If false, the effect has a duration and will be removed after the duration has elapsed.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Effect")
	bool HasInfiniteDuration = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Effect", meta = (EditCondition = "!HasInfiniteDuration"))
	float Duration;

	/**
	 * How often the effect ticks. If 0 the effect will only tick once at the end of the duration.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Effect")
	float TickInterval;
	
	/**
	 * If true, the effect will tick immediately on application.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Effect")
	bool TickOnInitialApplication;
};

USTRUCT(BlueprintType)
struct FGameplayEffectModifier
{
	GENERATED_BODY()

	/**
	 * The name of the modifier. This is cosmetic only and used to change the title of the modifier in the editor UI.
	 * Can be left blank if desired. 
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Effect")
	FString ModifierName = "Modifier";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Effect")
	TEnumAsByte<EAttributeType> AttributeType;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Effect")
	FGameplayTag ModifiedAttribute;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Effect", meta = (EditCondition = "AttributeType == EAttributeType::FloatAttribute", EditConditionHides))
	TEnumAsByte<EAttributeValueType> ModifiedAttributeValueType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Effect")
	bool CancelEffectIfAttributeNotFound = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Effect", meta = (EditCondition = "AttributeType == EAttributeType::FloatAttribute", EditConditionHides))
	TEnumAsByte<EFloatAttributeModificationOperation> ModificationOperation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Effect", meta = (EditCondition = "AttributeType == EAttributeType::FloatAttribute", EditConditionHides))
	TEnumAsByte<EAttributeModificationValueSource> ModificationInputValueSource;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Effect", meta = (EditCondition = "ModificationInputValueSource == EAttributeModificationValueSource::Manual && AttributeType == EAttributeType::FloatAttribute", EditConditionHides))
	float ManualInputValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Effect", meta = (EditCondition = "ModificationInputValueSource == EAttributeModificationValueSource::FromAttribute && AttributeType == EAttributeType::FloatAttribute", EditConditionHides))
	FGameplayTag SourceAttribute;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Effect", meta = (EditCondition = "ModificationInputValueSource == EAttributeModificationValueSource::FromAttribute && AttributeType == EAttributeType::FloatAttribute", EditConditionHides))
	TEnumAsByte<EAttributeValueType> SourceAttributeValueType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Effect", meta = (EditCondition = "AttributeType == EAttributeType::StructAttribute", EditConditionHides))
	FGameplayTag StructModifierTag;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Effect", meta = (EditCondition = "ModificationInputValueSource == EAttributeModificationValueSource::FromMetaAttribute || AttributeType == EAttributeType::StructAttribute", EditConditionHides))
	FGameplayTag MetaAttributeTag;
};

USTRUCT(BlueprintType)
struct FGameplayEffectConfig
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<EGameplayEffectType> EffectType = EGameplayEffectType::Instant;

	/**
	 * If the EffectType is Duration, these settings control how long the effect lasts and how often it ticks.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "EffectType == EGameplayEffectType::Duration", EditConditionHides))
	FGameplayEffectDurationSettings DurationConfig;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "EffectType == EGameplayEffectType::Duration", EditConditionHides))
	int32 Stacks = 1;
	
	/**
	 * Settings controlling how/if the effect stacks (only applies to duration type effects).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "EffectType == EGameplayEffectType::Duration", EditConditionHides))
	FStackSettings StackingConfig;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FEffectTagConfig TagConfig;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (TitleProperty = "ModifierName"))
	TArray<FGameplayEffectModifier> Modifiers;
};

USTRUCT(BlueprintType)
struct FPendingGameplayEffect
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USimpleGameplayAbilityComponent* Instigator;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<USimpleGameplayEffect> EffectClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FInstancedStruct EffectContext;
};
