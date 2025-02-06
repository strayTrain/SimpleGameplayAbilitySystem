#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "InstancedStruct.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "SimpleAbilityComponentTypes.generated.h"

/* Enums */

class USimpleStructAttributeHandler;

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
	Add,
	Multiply,
	Override
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
	FromMetaAttribute,
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
	FGameplayTag AttributeTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FInstancedStruct Value;
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
	
	/**
	 * The struct type that this attribute will hold. If you try to set a value that is not of this type, it will be ignored.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UScriptStruct* AttributeType;

	/**
	 * Optional handler class that can be used to check which individual members of the struct changed when the attribute is updated.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<USimpleStructAttributeHandler> AttributeHandler;

	UPROPERTY(BlueprintReadWrite)
	FInstancedStruct AttributeValue;

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