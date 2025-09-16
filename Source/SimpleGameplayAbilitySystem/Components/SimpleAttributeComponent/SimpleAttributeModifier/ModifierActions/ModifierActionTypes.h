#pragma once

#include "CoreMinimal.h"

#if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5)
	#include "StructUtils/InstancedStruct.h"
#else
	#include "InstancedStruct.h"
#endif

#include "ModifierActionTypes.generated.h"

class UModifierAction;

UENUM(BlueprintType)
enum class EAttributeModifierActionPolicy : uint8
{
	// This modifier action will run on both client and server without any replication
	ApplyLocally,
	
	// This modifier action will run on both client and server and will take a snapshot of the modifier action results for replication 
	ApplyClientPredicted,
	
	// This modifier action will only run on the client. Listen servers count as clients.
	ApplyClientOnly,
	
	// This modifier action will only run on the server and will NOT replicate to the client
	ApplyServerOnly,
	
	// This modifier action will only run on the server and will replicate a snapshot of the modifier action result to the client
	ApplyServerInitiated,
};

UENUM(BlueprintType)
enum class EContextSource : uint8
{
	NoContext,
	FromContextCollection,
	FromFunction
};

USTRUCT(BlueprintType)
struct FModifierActionResult
{
	GENERATED_BODY()

	UPROPERTY()
	int32 ActionIndex;

	UPROPERTY()
	TSubclassOf<UModifierAction> ActionClass;
	
	UPROPERTY()
	FInstancedStruct ActionResult;
};

USTRUCT(BlueprintType)
struct FModifierActionStackResults
{
	GENERATED_BODY()

	UPROPERTY()
	TSubclassOf<UModifierAction> ActionClass;

	UPROPERTY()
	TArray<FModifierActionResult> ActionsResults;
};
