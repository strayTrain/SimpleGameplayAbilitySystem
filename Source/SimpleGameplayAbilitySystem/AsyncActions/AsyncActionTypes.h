#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "AsyncActionTypes.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEventSenderDelegate, FInstancedStruct, Payload);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEventAbilitySenderDelegate, FGameplayTag, EndStatus, FInstancedStruct, EndContext);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FSimpleFloatAttributeChangedDelegate,
	EAttributeValueType, ChangedValueType,
	float, NewValue
);

UENUM(BlueprintType)
enum class EEventInitiator : uint8
{
	Client,
	Server,
};