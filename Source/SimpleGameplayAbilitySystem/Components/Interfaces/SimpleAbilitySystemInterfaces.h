#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SimpleAbilitySystemInterfaces.generated.h"

UINTERFACE(Blueprintable)
class SIMPLEGAMEPLAYABILITYSYSTEM_API UAttributeComponentInterface : public UInterface
{
	GENERATED_BODY()
};
 
class SIMPLEGAMEPLAYABILITYSYSTEM_API IAttributeComponentInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "SimpleGAS|AttributeComponent")
	class USimpleAttributeComponent* GetSimpleAttributeComponent();
};

UINTERFACE(Blueprintable)
class SIMPLEGAMEPLAYABILITYSYSTEM_API UAbilityComponentInterface : public UInterface
{
	GENERATED_BODY()
};
 
class SIMPLEGAMEPLAYABILITYSYSTEM_API IAbilityComponentInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "SimpleGAS|AbilityComponent")
	class USimpleGameplayAbilityComponent* GetSimpleAbilityComponent();
};
