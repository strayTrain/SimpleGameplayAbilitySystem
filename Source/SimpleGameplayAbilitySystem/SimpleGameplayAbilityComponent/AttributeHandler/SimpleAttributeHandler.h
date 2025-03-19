#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5)
	#include "StructUtils/InstancedStruct.h"
#else
	#include "InstancedStruct.h"
#endif

#include "UObject/Object.h"
#include "SimpleAttributeHandler.generated.h"

UCLASS(Blueprintable)
class SIMPLEGAMEPLAYABILITYSYSTEM_API USimpleAttributeHandler : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "SimpleGAS|AttributeHandler")
	FGameplayTagContainer GetModificationEvents(const FInstancedStruct& OldValue, const FInstancedStruct& NewValue);
};
