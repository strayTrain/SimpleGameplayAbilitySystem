#pragma once

#include "CoreMinimal.h"

#if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5)
	#include "StructUtils/InstancedStruct.h"
#else
	#include "InstancedStruct.h"
#endif

#include "GameplayTagContainer.h"
#include "SimpleAbilityComponentTypes.generated.h"

class USimpleGameplayAbility;

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

/* Structs */

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



/* Event Dispatchers */


