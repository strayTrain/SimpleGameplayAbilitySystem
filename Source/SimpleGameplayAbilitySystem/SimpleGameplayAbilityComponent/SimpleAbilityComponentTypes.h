#pragma once

#include "CoreMinimal.h"

#if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5)
	#include "StructUtils/InstancedStruct.h"
#else
	#include "InstancedStruct.h"
#endif

#include "GameplayTagContainer.h"
#include "SimpleGameplayAbilitySystem/UtilityClasses/FastArraySerializerMacros.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "SimpleGameplayAbilitySystem/SimpleGameplayAbilityComponent/AttributeHandler/SimpleAttributeHandler.h"
#include "SimpleAbilityComponentTypes.generated.h"

/* Enums */

class USimpleGameplayAbility;
class USimpleGameplayAbilityComponent;
struct FFloatAttribute;

UENUM(BlueprintType)
enum class ESimpleEventReplicationPolicy : uint8
{
	/**
	 * Behaves the same as sending an event normally using the SimpleEventSubsystem.
	 */
	NoReplication,
	/**
	 * This event will be sent to the server and the owning client. Always gets sent on the server first.
	 */
	ServerAndOwningClient,
	/**
	 * This event will be sent to the server and the owning client.
	 * Clients can send the event locally before the server sends the event.
	 */
	ServerAndOwningClientPredicted,
	/**
	 * This event will be sent to all connected clients. Always sent on the server first.
	 */
	AllConnectedClients,
	/**
	 * This event will be sent to all connected clients.
	 * Clients can send the event locally before the server sends the event.
	 */
	AllConnectedClientsPredicted
};

UENUM(BlueprintType)
enum class EAttributeValueType : uint8
{
	CurrentValue,
	BaseValue,
	MaxCurrentValue,
	MinCurrentValue,
	MaxBaseValue,
	MinBaseValue
};

UENUM(BlueprintType)
enum class EFloatAttributeModificationOperation : uint8
{
	/**
	 * Output = A + B
	 */
	Add,
	/**
	 * Output = A - B
	 */
	Subtract,
	/**
	 * Output = A * B
	 */
	Multiply,
	/**
	 * Output = A / B
	 */
	Divide,
	/**
	 * Output = A ** B
	 */
	Power,
	/**
	 * Output = B
	 */
	Override,
	/**
	 * Output = [function call]
	 */
	Custom
};

UENUM(BlueprintType)
enum class EAttributeType : uint8
{
	FloatAttribute,
	StructAttribute,
};

UENUM(BlueprintType)
enum class EAttributeModificationValueSource : uint8
{
	Manual,
	FromOverflow,
	FromInstigatorAttribute,
	FromTargetAttribute,
	CustomInputValue
};

/* Structs */

USTRUCT(BlueprintType)
struct FValueLimits
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, meta=(InlineEditConditionToggle))
	bool UseMinBaseValue;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "UseMinBaseValue"))
	float MinBaseValue;

	UPROPERTY(EditAnywhere, meta=(InlineEditConditionToggle))
	bool UseMaxBaseValue;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "UseMaxBaseValue"))
	float MaxBaseValue;

	UPROPERTY(EditAnywhere, meta=(InlineEditConditionToggle))
	bool UseMinCurrentValue;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "UseMinCurrentValue"))
	float MinCurrentValue;

	UPROPERTY(EditAnywhere, meta=(InlineEditConditionToggle))
	bool UseMaxCurrentValue;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "UseMaxCurrentValue"))
	float MaxCurrentValue;
};

USTRUCT(BlueprintType)
struct FFloatAttributeModification
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USimpleGameplayAbilityComponent* AttributeOwner;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag AttributeTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EAttributeValueType ValueType;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float NewValue;
};

USTRUCT(BlueprintType)
struct FStructAttributeModification
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USimpleGameplayAbilityComponent* AttributeOwner;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag AttributeTag;

	// Use these tags to indicate which members of the struct have changed
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer ModificationTags;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FInstancedStruct NewValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FInstancedStruct OldValue;
};

UENUM(BlueprintType)
enum class EFlowControl : uint8
{
	Found,
	NotFound
};

USTRUCT(BlueprintType)
struct FEventContext
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag ContextTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FInstancedStruct ContextData;
};

USTRUCT(BlueprintType)
struct FEventContextCollection
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FEventContext> EventContexts;
};

USTRUCT(BlueprintType)
struct FAbilityActivationEvent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGuid AbilityID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<USimpleGameplayAbility> AbilityClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FInstancedStruct AbilityContext;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool WasActivatedSuccessfully;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ActivationTimeStamp;
};

/* FFastArraySerializer Structs */

// Float attribute
DECLARE_FAST_ARRAY_SERIALIZER_DELEGATES(FFloatAttribute, FloatAttribute)

USTRUCT(BlueprintType)
struct FFloatAttribute : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName AttributeName;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag AttributeTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BaseValue;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CurrentValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FValueLimits ValueLimits;

	void PreReplicatedRemove(const struct FFloatAttributeContainer& InArraySerializer);
	void PostReplicatedAdd(const struct FFloatAttributeContainer& InArraySerializer);
	void PostReplicatedChange(const struct FFloatAttributeContainer& InArraySerializer);
	
	bool operator==(const FFloatAttribute& Other) const
	{
		return AttributeTag == Other.AttributeTag;
	}

	friend uint32 GetTypeHash(const FFloatAttribute& FloatAttribute)
	{
		return GetTypeHash(FloatAttribute.AttributeTag);
	}
};

USTRUCT()
struct FFloatAttributeContainer : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, meta = (TitleProperty = "AttributeName"))
	TArray<FFloatAttribute> Attributes;

	FOnFloatAttributeAdded   OnFloatAttributeAdded;
	FOnFloatAttributeChanged OnFloatAttributeChanged;
	FOnFloatAttributeRemoved OnFloatAttributeRemoved;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo & DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FFloatAttribute, FFloatAttributeContainer>(Attributes, DeltaParms, *this);
	}
};

DECLARE_FAST_ARRAY_SERIALIZER_TRAITS(FFloatAttributeContainer)

// Struct attribute
DECLARE_DELEGATE(FOnStructAttributeValueChanged);

USTRUCT(BlueprintType)
struct FStructAttribute : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName AttributeName;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag AttributeTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UScriptStruct* StructType;

	UPROPERTY(BlueprintReadWrite)
	FInstancedStruct AttributeValue;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<USimpleAttributeHandler> StructAttributeHandler;

	// Add this delegate to listen for value changes
	FOnStructAttributeValueChanged OnValueChanged;

	bool operator==(const FStructAttribute& Other) const
	{
		return AttributeTag == Other.AttributeTag;
	}

	friend uint32 GetTypeHash(const FStructAttribute& StructAttribute)
	{
		return GetTypeHash(StructAttribute.AttributeTag);
	}
};

DECLARE_FAST_ARRAY_SERIALIZER_DELEGATES(FStructAttribute, StructAttribute)

USTRUCT()
struct FStructAttributeContainer : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, meta = (TitleProperty = "AttributeName"))
	TArray<FStructAttribute> Attributes;

	FOnStructAttributeAdded   OnStructAttributeAdded;
	FOnStructAttributeChanged OnStructAttributeChanged;
	FOnStructAttributeRemoved OnStructAttributeRemoved;
	
	void PostReplicatedAdd(const TArrayView< int32 >& AddedIndices, int32 FinalSize)
	{
		if (OnStructAttributeAdded.IsBound())
		{
			for (const int32 AddedIndex : AddedIndices)
			{
				OnStructAttributeAdded.Execute(Attributes[AddedIndex]);
			}
		}
	}
	
	void PostReplicatedChange(const TArrayView< int32 >& ChangedIndices, int32 FinalSize)
	{
		if (OnStructAttributeChanged.IsBound())
		{
			for (const int32 ChangedIndex : ChangedIndices)
			{
				OnStructAttributeChanged.Execute(Attributes[ChangedIndex]);
			}
		}
	}

	void PreReplicatedRemove (const TArrayView< int32 >& RemovedIndices, int32 FinalSize)
	{
		if (OnStructAttributeRemoved.IsBound())
		{
			for (const int32 RemovedIndex : RemovedIndices)
			{
				OnStructAttributeRemoved.Execute(Attributes[RemovedIndex]);
			}
		}
	}

	bool NetDeltaSerialize(FNetDeltaSerializeInfo & DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FStructAttribute, FStructAttributeContainer>(Attributes, DeltaParms, *this);
	}
};

DECLARE_FAST_ARRAY_SERIALIZER_TRAITS(FStructAttributeContainer)

// GameplayTagCounter
USTRUCT(BlueprintType)
struct FGameplayTagCounter : public FFastArraySerializerItem
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag GameplayTag;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	int32 ReferenceCounter;

	bool operator==(const FGameplayTagCounter& Other) const
	{
		return GameplayTag.MatchesTagExact(Other.GameplayTag);
	}

	friend uint32 GetTypeHash(const FGameplayTagCounter& GameplayTagCounter)
	{
		return GetTypeHash(GameplayTagCounter.GameplayTag);
	}
};

DECLARE_FAST_ARRAY_SERIALIZER_DELEGATES(FGameplayTagCounter, GameplayTagCounter)

USTRUCT()
struct FGameplayTagCounterContainer : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, SaveGame, meta=(TitleProperty="GameplayTag"))
	TArray<FGameplayTagCounter> Tags;

	FOnGameplayTagCounterAdded   OnGameplayTagCounterAdded;
	FOnGameplayTagCounterChanged OnGameplayTagCounterChanged;
	FOnGameplayTagCounterRemoved OnGameplayTagCounterRemoved;

	/** Called after new items are added on clients */
	void PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize)
	{
		if (OnGameplayTagCounterAdded.IsBound())
		{
			for (int32 Index : AddedIndices)
			{
				OnGameplayTagCounterAdded.ExecuteIfBound(Tags[Index]);
			}
		}
	}

	/** Called when existing items change on clients */
	void PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize)
	{
		if (OnGameplayTagCounterChanged.IsBound())
		{
			for (int32 Index : ChangedIndices)
			{
				OnGameplayTagCounterChanged.ExecuteIfBound(Tags[Index]);
			}
		}
	}

	/** Called before items are removed on clients */
	void PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize)
	{
		if (OnGameplayTagCounterRemoved.IsBound())
		{
			for (int32 Index : RemovedIndices)
			{
				OnGameplayTagCounterRemoved.ExecuteIfBound(Tags[Index]);
			}
		}
	}

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FGameplayTagCounter, FGameplayTagCounterContainer>(Tags, DeltaParms, *this);
	}
};

DECLARE_FAST_ARRAY_SERIALIZER_TRAITS(FGameplayTagCounterContainer)