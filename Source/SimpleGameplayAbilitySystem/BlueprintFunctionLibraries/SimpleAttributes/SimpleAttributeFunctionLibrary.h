﻿#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "InstancedStruct.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SimpleAttributeFunctionLibrary.generated.h"

class USimpleGameplayAbilityComponent;
struct FGameplayTag;
struct FFloatAttribute;
struct FStructAttribute;

UCLASS()
class SIMPLEGAMEPLAYABILITYSYSTEM_API USimpleAttributeFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintPure)
	static bool HasFloatAttribute(const USimpleGameplayAbilityComponent* AbilityComponent, const FGameplayTag AttributeTag);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static bool HasStructAttribute(const USimpleGameplayAbilityComponent* AbilityComponent, const FGameplayTag AttributeTag);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static FFloatAttribute GetFloatAttributeCopy(const USimpleGameplayAbilityComponent* AbilityComponent, const FGameplayTag AttributeTag, bool& WasFound);
	
	UFUNCTION(BlueprintCallable, BlueprintPure, meta = (DeterminesOutputType = "AvatarClass", HideSelfPin))
	static float GetFloatAttributeValue(const USimpleGameplayAbilityComponent* AbilityComponent, EAttributeValueType ValueType, FGameplayTag AttributeTag, bool& WasFound);

	UFUNCTION(BlueprintCallable)
	static bool SetFloatAttributeValue(USimpleGameplayAbilityComponent* AbilityComponent, EAttributeValueType ValueType, FGameplayTag AttributeTag, float NewValue, float& Overflow);

	UFUNCTION(BlueprintCallable)
	static bool OverrideFloatAttribute(USimpleGameplayAbilityComponent* AbilityComponent, FGameplayTag AttributeTag, FFloatAttribute NewAttribute);
	
	UFUNCTION(BlueprintCallable)
	static FStructAttribute GetStructAttributeCopy(const USimpleGameplayAbilityComponent* AbilityComponent, FGameplayTag AttributeTag, bool& WasFound);
	
	UFUNCTION(BlueprintCallable)
	static FInstancedStruct GetStructAttributeValue(const USimpleGameplayAbilityComponent* AbilityComponent, FGameplayTag AttributeTag, bool& WasFound);

	UFUNCTION(BlueprintCallable)
	static bool SetStructAttributeValue(USimpleGameplayAbilityComponent* AbilityComponent, FGameplayTag AttributeTag, FInstancedStruct NewValue);

	UFUNCTION(BlueprintCallable)
	static bool HasModifierWithTags(const USimpleGameplayAbilityComponent* AbilityComponent, const FGameplayTagContainer& Tags);

	UFUNCTION(BlueprintCallable)
	static float ClampFloatAttributeValue(const FFloatAttribute& Attribute, EAttributeValueType ValueType, float NewValue, float& Overflow);

	UFUNCTION(BlueprintCallable)
	static int32 GetFloatAttributeIndex(const USimpleGameplayAbilityComponent* AbilityComponent, FGameplayTag AttributeTag);

	UFUNCTION(BlueprintCallable)
	static int32 GetStructAttributeIndex(const USimpleGameplayAbilityComponent* AbilityComponent, FGameplayTag AttributeTag);

	UFUNCTION(BlueprintCallable)
	static void CompareFloatAttributesAndSendEvents(const USimpleGameplayAbilityComponent* AbilityComponent, const FFloatAttribute& OldAttribute, const FFloatAttribute& NewAttribute);

	UFUNCTION()
	static void SendFloatAttributeChangedEvent(const USimpleGameplayAbilityComponent* AbilityComponent, FGameplayTag EventTag, FGameplayTag AttributeTag, EAttributeValueType ValueType, float NewValue);
};
