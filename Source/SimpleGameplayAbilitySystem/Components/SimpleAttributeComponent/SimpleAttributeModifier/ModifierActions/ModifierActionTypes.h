#pragma once

#include "CoreMinimal.h"
#include "ModifierActionTypes.generated.h"

UENUM(BlueprintType)
enum class EModifierActionComponentTarget : uint8
{
	Target UMETA(DisplayName = "Target"),
	Instigator UMETA(DisplayName = "Instigator"),
	Both UMETA(DisplayName = "Both")
};

UENUM(BlueprintType)
enum class EModifierActionPredictionPolicy: uint8
{
	// If the action supports client prediction, it will be predicted on the client and generate an FAttributeModifierMutation to be sent to the server
	PredictIfPossible,
	// The action will only run on the server, with the result FAttributeModifierMutation replicated to the client
	ServerInitiate,
	// The action will only run on the server and won't be replicated to the client
	ServerOnly,
	// The action only runs on clients (ListenServer counts as a client) and won't replicate
	ClientOnly,
};

UENUM(BlueprintType)
enum class EContextSource : uint8
{
	NoContext,
	FromContextCollection,
	FromFunction
};