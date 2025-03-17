#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "InstancedStruct.h"
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
