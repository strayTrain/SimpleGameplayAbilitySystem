#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "InstancedStruct.h"
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
	FName AttributeName;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Ability")
	FGameplayTag AttributeTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Ability")
	float BaseValue;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Ability")
	float CurrentValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Ability")
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

USTRUCT(BlueprintType)
struct FStructAttribute
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Ability")
	FName AttributeName;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Ability")
	FGameplayTag AttributeTag;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Ability")
	UScriptStruct* AttributeType;

	UPROPERTY(BlueprintReadWrite, Category = "SimpleGAS|Ability")
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
struct FFloatAttributeModification
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Ability")
	FGameplayTag AttributeTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleGAS|Ability")
	EAttributeValueType ValueType;
	
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
