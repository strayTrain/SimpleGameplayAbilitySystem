#pragma once

#include "CoreMinimal.h"
#include "ModifierActionTypes.generated.h"

UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EModifierActionActivationPolicy: uint8
{
	None	    = 0 UMETA(Hidden),
	RunOnServer	= 1 << 0,
	RunOnClient	= 1 << 1,
};
ENUM_CLASS_FLAGS(EModifierActionActivationPolicy);

UENUM(BlueprintType)
enum class EContextSource : uint8
{
	NoContext,
	FromContextCollection,
	FromFunction
};