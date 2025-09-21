#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SimpleAbilitySystemInterfaces.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UAttributeComponentInterface : public UInterface
{
	GENERATED_BODY()
};
 
class IAttributeComponentInterface
{
	GENERATED_BODY()
 
public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "SimpleGAS|AttributeComponent")
	class USimpleAttributeComponent* GetSimpleAttributeComponent() const;
};

UINTERFACE(MinimalAPI, Blueprintable)
class UAbilityComponentInterface : public UInterface
{
	GENERATED_BODY()
};
 
class IAbilityComponentInterface
{
	GENERATED_BODY()
 
public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "SimpleGAS|AttributeComponent")
	class USimpleGameplayAbilityComponent* GetSimpleAbilityComponent() const;
};
