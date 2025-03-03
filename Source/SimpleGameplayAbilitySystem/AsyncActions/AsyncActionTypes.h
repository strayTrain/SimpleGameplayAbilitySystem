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
	// This ability is created for the local player and does not replicate
	LocalOnly,
	// This ability is created on the client and the end result is sent to the server
	Client,
	// This ability is created on the server and the end result is sent to the client
	Server
};