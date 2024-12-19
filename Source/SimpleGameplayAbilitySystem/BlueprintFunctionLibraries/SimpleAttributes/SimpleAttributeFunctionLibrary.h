#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SimpleAttributeFunctionLibrary.generated.h"


struct FFloatAttribute;
class USimpleAbilityComponent;
struct FGameplayTag;

UCLASS()
class SIMPLEGAMEPLAYABILITYSYSTEM_API USimpleAttributeFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool HasFloatAttribute(const FGameplayTag AttributeTag, const USimpleAbilityComponent* AbilityComponent);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool HasStructAttribute(const FGameplayTag AttributeTag, const USimpleAbilityComponent* AbilityComponent);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FFloatAttribute GetFloatAttributeCopy(const FGameplayTag AttributeTag, const USimpleAbilityComponent* AbilityComponent, bool& WasFound);
};
