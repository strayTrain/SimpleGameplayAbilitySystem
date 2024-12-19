#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "InstancedStruct.h"
#include "SimpleAbilityComponentTypes.generated.h"

/* Enums */

UENUM(BlueprintType)
enum class ESimpleEventReplicationPolicy : uint8
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
