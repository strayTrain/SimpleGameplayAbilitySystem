#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5)
	#include "StructUtils/InstancedStruct.h"
#else
	#include "InstancedStruct.h"
#endif

#include "Net/Serialization/FastArraySerializer.h"
#include "SimpleGameplayAbilitySystem/SimpleGameplayAbilityComponent/AttributeHandler/SimpleAttributeHandler.h"
#include "SimpleAbilityComponentTypes.generated.h"


/* Enums */

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "UseMinBaseValue"))
	float MinBaseValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "UseMaxBaseValue"))
	float MaxBaseValue;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "UseMinCurrentValue"))
	float MinCurrentValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "UseMaxCurrentValue"))
	float MaxCurrentValue;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool UseMinBaseValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool UseMaxBaseValue;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool UseMinCurrentValue;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool UseMaxCurrentValue;
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

// Float attribute 
DECLARE_DELEGATE_OneParam(FOnFloatAttributeAdded, const FFloatAttribute&);
DECLARE_DELEGATE_OneParam(FOnFloatAttributeChanged, const FFloatAttribute&);
DECLARE_DELEGATE_OneParam(FOnFloatAttributeRemoved, const FFloatAttribute&);

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

template<>
struct TStructOpsTypeTraits<FFloatAttributeContainer> : public TStructOpsTypeTraitsBase2<FFloatAttributeContainer>
{
	enum 
	{
		WithNetDeltaSerializer = true,
	};
};

// Struct attribute

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

	bool operator==(const FStructAttribute& Other) const
	{
		return AttributeTag == Other.AttributeTag;
	}

	friend uint32 GetTypeHash(const FStructAttribute& StructAttribute)
	{
		return GetTypeHash(StructAttribute.AttributeTag);
	}
};

DECLARE_DELEGATE_OneParam(FOnStructAttributeAdded, const FStructAttribute&);
DECLARE_DELEGATE_OneParam(FOnStructAttributeChanged, const FStructAttribute&);
DECLARE_DELEGATE_OneParam(FOnStructAttributeRemoved, const FStructAttribute&);

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
		if (OnStructAttributeAdded.IsBound())
		{
			for (const int32 ChangedIndex : ChangedIndices)
			{
				OnStructAttributeAdded.Execute(Attributes[ChangedIndex]);
			}
		}
	}

	void PreReplicatedRemove (const TArrayView< int32 >& RemovedIndices, int32 FinalSize)
	{
		if (OnStructAttributeAdded.IsBound())
		{
			for (const int32 RemovedIndex : RemovedIndices)
			{
				OnStructAttributeAdded.Execute(Attributes[RemovedIndex]);
			}
		}
	}

	bool NetDeltaSerialize(FNetDeltaSerializeInfo & DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FStructAttribute, FStructAttributeContainer>(Attributes, DeltaParms, *this);
	}
};

template<>
struct TStructOpsTypeTraits<FStructAttributeContainer> : public TStructOpsTypeTraitsBase2<FStructAttributeContainer>
{
	enum 
	{
		WithNetDeltaSerializer = true,
	};
};

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

DECLARE_DELEGATE_OneParam(FOnGameplayTagCounterAdded, const FGameplayTagCounter&);
DECLARE_DELEGATE_OneParam(FOnGameplayTagCounterChanged, const FGameplayTagCounter&);
DECLARE_DELEGATE_OneParam(FOnGameplayTagCounterRemoved, const FGameplayTagCounter&);

USTRUCT()
struct FGameplayTagCounterContainer : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, meta = (TitleProperty = "GameplayTag"))
	TArray<FGameplayTagCounter> Tags;

	FOnGameplayTagCounterAdded   OnGameplayTagCounterAdded;
	FOnGameplayTagCounterChanged OnGameplayTagCounterChanged;
	FOnGameplayTagCounterRemoved OnGameplayTagCounterRemoved;
	
	void PostReplicatedAdd(const TArrayView< int32 >& AddedIndices, int32 FinalSize)
	{
		if (OnGameplayTagCounterAdded.IsBound())
		{
			for (const int32 AddedIndex : AddedIndices)
			{
				OnGameplayTagCounterAdded.Execute(Tags[AddedIndex]);
			}
		}
	}
	
	void PostReplicatedChange(const TArrayView< int32 >& ChangedIndices, int32 FinalSize)
	{
		if (OnGameplayTagCounterChanged.IsBound())
		{
			for (const int32 ChangedIndex : ChangedIndices)
			{
				OnGameplayTagCounterChanged.Execute(Tags[ChangedIndex]);
			}
		}
	}

	void PreReplicatedRemove (const TArrayView< int32 >& RemovedIndices, int32 FinalSize)
	{
		if (OnGameplayTagCounterRemoved.IsBound())
		{
			for (const int32 RemovedIndex : RemovedIndices)
			{
				OnGameplayTagCounterRemoved.Execute(Tags[RemovedIndex]);
			}
		}
	}

	bool NetDeltaSerialize(FNetDeltaSerializeInfo & DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FGameplayTagCounter, FGameplayTagCounterContainer>(Tags, DeltaParms, *this);
	}
};

template<>
struct TStructOpsTypeTraits<FGameplayTagCounterContainer> : public TStructOpsTypeTraitsBase2<FGameplayTagCounterContainer>
{
	enum 
	{
		WithNetDeltaSerializer = true,
	};
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