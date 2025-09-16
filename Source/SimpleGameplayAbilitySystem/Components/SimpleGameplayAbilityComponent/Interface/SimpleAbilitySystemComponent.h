#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SimpleAbilitySystemComponent.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class USimpleAbilitySystemComponent : public UInterface
{
	GENERATED_BODY()
};

class ISimpleAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "AbilityComponent")
	class USimpleGameplayAbilityComponent* GetAbilitySystemComponent() const;
};
