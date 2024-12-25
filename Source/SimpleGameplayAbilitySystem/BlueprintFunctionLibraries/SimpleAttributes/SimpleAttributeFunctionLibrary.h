#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "InstancedStruct.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SimpleAttributeFunctionLibrary.generated.h"

class USimpleAbilityComponent;
struct FGameplayTag;
struct FFloatAttribute;
struct FStructAttribute;

UCLASS()
class SIMPLEGAMEPLAYABILITYSYSTEM_API USimpleAttributeFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/*UFUNCTION(BlueprintCallable, BlueprintPure)
	static bool HasFloatAttribute(const USimpleAbilityComponent* AbilityComponent, const FGameplayTag AttributeTag);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static bool HasStructAttribute(const USimpleAbilityComponent* AbilityComponent, const FGameplayTag AttributeTag);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static FFloatAttribute GetFloatAttributeCopy(const USimpleAbilityComponent* AbilityComponent, const FGameplayTag AttributeTag, bool& WasFound);
	
	UFUNCTION(BlueprintCallable, BlueprintPure, meta = (DeterminesOutputType = "AvatarClass", HideSelfPin))
	static float GetFloatAttributeValue(const USimpleAbilityComponent* AbilityComponent, EAttributeValueType ValueType, FGameplayTag AttributeTag, bool& WasFound);

	UFUNCTION(BlueprintCallable)
	static bool SetFloatAttributeValue(USimpleAbilityComponent* AbilityComponent, EAttributeValueType ValueType, FGameplayTag AttributeTag, float NewValue, float& Overflow);

	UFUNCTION(BlueprintCallable)
	static bool OverrideFloatAttribute(USimpleAbilityComponent* AbilityComponent, FGameplayTag AttributeTag, FFloatAttribute NewAttribute);
	
	UFUNCTION(BlueprintCallable)
	static FStructAttribute GetStructAttributeCopy(const USimpleAbilityComponent* AbilityComponent, FGameplayTag AttributeTag, bool& WasFound);
	
	UFUNCTION(BlueprintCallable)
	static FInstancedStruct GetStructAttributeValue(const USimpleAbilityComponent* AbilityComponent, FGameplayTag AttributeTag, bool& WasFound);

	UFUNCTION(BlueprintCallable)
	static bool SetStructAttributeValue(USimpleAbilityComponent* AbilityComponent, FGameplayTag AttributeTag, FInstancedStruct NewValue);

	UFUNCTION(BlueprintCallable)
	static bool HasModifierWithTags(const USimpleAbilityComponent* AbilityComponent, const FGameplayTagContainer& Tags);

	UFUNCTION(BlueprintCallable)
	static float ClampFloatAttributeValue(const FFloatAttribute& Attribute, EAttributeValueType ValueType, float NewValue, float& Overflow);

	UFUNCTION(BlueprintCallable)
	static int32 GetFloatAttributeIndex(const USimpleAbilityComponent* AbilityComponent, FGameplayTag AttributeTag);

	UFUNCTION(BlueprintCallable)
	static int32 GetStructAttributeIndex(const USimpleAbilityComponent* AbilityComponent, FGameplayTag AttributeTag);

	UFUNCTION(BlueprintCallable)
	static void CompareFloatAttributesAndSendEvents(const USimpleAbilityComponent* AbilityComponent, const FFloatAttribute& OldAttribute, const FFloatAttribute& NewAttribute);

	UFUNCTION()
	static void SendFloatAttributeChangedEvent(const USimpleAbilityComponent* AbilityComponent, FGameplayTag EventTag, FGameplayTag AttributeTag, EAttributeValueType ValueType, float NewValue);*/
};
