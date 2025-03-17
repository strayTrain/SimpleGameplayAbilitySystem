#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SimpleGameplayAbilitySystem/SimpleGameplayAbilityComponent/SimpleAbilityComponentTypes.h"

#if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5)
	#include "StructUtils/InstancedStruct.h"
#else
	#include "InstancedStruct.h"
#endif

#include "AsyncActionTypes.generated.h"

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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEventSenderDelegate, FInstancedStruct, Payload);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEventAbilitySenderDelegate, FGameplayTag, EndStatus, FInstancedStruct, EndContext);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSimpleFloatAttributeChangedDelegate, EAttributeValueType, ChangedValueType, float, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FSimpleStructAttributeChangedDelegate, FGameplayTagContainer, ModificationTags, FInstancedStruct, NewValue, FInstancedStruct, OldValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSimpleGameplayTagEventDelegate);

