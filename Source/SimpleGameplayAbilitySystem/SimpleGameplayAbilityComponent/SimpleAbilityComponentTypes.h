#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "InstancedStruct.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "SimpleAbilityComponentTypes.generated.h"

/* Enums */

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

// Float attribute 

USTRUCT(BlueprintType)
struct FFloatAttribute
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
struct FFloatAttributeItem : public FFastArraySerializerItem
{
	GENERATED_BODY()
	
	UPROPERTY()
	FFloatAttribute FloatAttribute;
};

DECLARE_DELEGATE_OneParam(FOnFloatAttributeAdded, const FFloatAttribute&);
DECLARE_DELEGATE_OneParam(FOnFloatAttributeChanged, const FFloatAttribute&);
DECLARE_DELEGATE_OneParam(FOnFloatAttributeRemoved, const FFloatAttribute&);

USTRUCT()
struct FFloatAttributeContainer : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FFloatAttributeItem> Items;

	FOnFloatAttributeAdded   OnFloatAttributeAdded;
	FOnFloatAttributeChanged OnFloatAttributeChanged;
	FOnFloatAttributeRemoved OnFloatAttributeRemoved;
	
	void PostReplicatedAdd(const TArrayView< int32 >& AddedIndices, int32 FinalSize)
	{
		if (OnFloatAttributeAdded.IsBound())
		{
			for (const int32 AddedIndex : AddedIndices)
			{
				OnFloatAttributeAdded.Execute(Items[AddedIndex].FloatAttribute);
			}
		}
	}
	
	void PostReplicatedChange(const TArrayView< int32 >& ChangedIndices, int32 FinalSize)
	{
		if (OnFloatAttributeChanged.IsBound())
		{
			for (const int32 ChangedIndex : ChangedIndices)
			{
				OnFloatAttributeChanged.Execute(Items[ChangedIndex].FloatAttribute);
			}
		}
	}

	void PreReplicatedRemove (const TArrayView< int32 >& RemovedIndices, int32 FinalSize)
	{
		if (OnFloatAttributeRemoved.IsBound())
		{
			for (const int32 RemovedIndex : RemovedIndices)
			{
				OnFloatAttributeRemoved.Execute(Items[RemovedIndex].FloatAttribute);
			}
		}
	}

	bool NetDeltaSerialize(FNetDeltaSerializeInfo & DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FFloatAttributeItem, FFloatAttributeContainer>(Items, DeltaParms, *this);
	}
};

// Struct attribute

USTRUCT(BlueprintType)
struct FStructAttribute
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName AttributeName;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag AttributeTag;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UScriptStruct* AttributeType;

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

USTRUCT(BlueprintType)
struct FStructAttributeModification
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag AttributeTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FInstancedStruct Value;
};

USTRUCT()
struct FStructAttributeItem : public FFastArraySerializerItem
{
	GENERATED_BODY()
	
	UPROPERTY()
	FStructAttribute StructAttribute;
};

DECLARE_DELEGATE_OneParam(FOnStructAttributeAdded, const FStructAttribute&);
DECLARE_DELEGATE_OneParam(FOnStructAttributeChanged, const FStructAttribute&);
DECLARE_DELEGATE_OneParam(FOnStructAttributeRemoved, const FStructAttribute&);

USTRUCT()
struct FStructAttributeContainer : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FStructAttributeItem> Items;

	FOnStructAttributeAdded   OnStructAttributeAdded;
	FOnStructAttributeChanged OnStructAttributeChanged;
	FOnStructAttributeRemoved OnStructAttributeRemoved;
	
	void PostReplicatedAdd(const TArrayView< int32 >& AddedIndices, int32 FinalSize)
	{
		if (OnStructAttributeAdded.IsBound())
		{
			for (const int32 AddedIndex : AddedIndices)
			{
				OnStructAttributeAdded.Execute(Items[AddedIndex].StructAttribute);
			}
		}
	}
	
	void PostReplicatedChange(const TArrayView< int32 >& ChangedIndices, int32 FinalSize)
	{
		if (OnStructAttributeAdded.IsBound())
		{
			for (const int32 ChangedIndex : ChangedIndices)
			{
				OnStructAttributeAdded.Execute(Items[ChangedIndex].StructAttribute);
			}
		}
	}

	void PreReplicatedRemove (const TArrayView< int32 >& RemovedIndices, int32 FinalSize)
	{
		if (OnStructAttributeAdded.IsBound())
		{
			for (const int32 RemovedIndex : RemovedIndices)
			{
				OnStructAttributeAdded.Execute(Items[RemovedIndex].StructAttribute);
			}
		}
	}

	bool NetDeltaSerialize(FNetDeltaSerializeInfo & DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FStructAttributeItem, FStructAttributeContainer>(Items, DeltaParms, *this);
	}
};
