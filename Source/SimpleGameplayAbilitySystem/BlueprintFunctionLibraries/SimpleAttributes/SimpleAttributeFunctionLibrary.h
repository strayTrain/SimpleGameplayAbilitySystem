#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "InstancedStruct.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SimpleAttributeFunctionLibrary.generated.h"

class USimpleGameplayAbilityComponent;
struct FGameplayTag;
struct FFloatAttribute;
struct FStructAttribute;

UCLASS(Blueprintable)
class SIMPLEGAMEPLAYABILITYSYSTEM_API USimpleAttributeFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintPure)
	static bool HasFloatAttribute(USimpleGameplayAbilityComponent* AbilityComponent, const FGameplayTag AttributeTag);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static bool HasStructAttribute(USimpleGameplayAbilityComponent* AbilityComponent, const FGameplayTag AttributeTag);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static FFloatAttribute GetFloatAttributeCopy(USimpleGameplayAbilityComponent* AbilityComponent, const FGameplayTag AttributeTag, bool& WasFound);
	
	UFUNCTION(BlueprintCallable, BlueprintPure, meta = (DeterminesOutputType = "AvatarClass", HideSelfPin))
	static float GetFloatAttributeValue(USimpleGameplayAbilityComponent* AbilityComponent, EAttributeValueType ValueType, FGameplayTag AttributeTag, bool& WasFound);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	static bool SetFloatAttributeValue(USimpleGameplayAbilityComponent* AbilityComponent, EAttributeValueType ValueType, FGameplayTag AttributeTag, float NewValue, float& Overflow);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	static bool OverrideFloatAttribute(USimpleGameplayAbilityComponent* AbilityComponent, FGameplayTag AttributeTag, FFloatAttribute NewAttribute);
	
	UFUNCTION(BlueprintCallable)
	static FStructAttribute GetStructAttributeCopy(USimpleGameplayAbilityComponent* AbilityComponent, FGameplayTag AttributeTag, bool& WasFound);
	
	UFUNCTION(BlueprintCallable)
	static FInstancedStruct GetStructAttributeValue(USimpleGameplayAbilityComponent* AbilityComponent, FGameplayTag AttributeTag, bool& WasFound);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	static bool SetStructAttributeValue(USimpleGameplayAbilityComponent* AbilityComponent, FGameplayTag AttributeTag, FInstancedStruct NewValue);

	UFUNCTION(BlueprintCallable)
	static bool HasModifierWithTags(const USimpleGameplayAbilityComponent* AbilityComponent, const FGameplayTagContainer& Tags);

	UFUNCTION(BlueprintCallable)
	static float ClampFloatAttributeValue(const FFloatAttribute& Attribute, EAttributeValueType ValueType, float NewValue, float& Overflow);

	UFUNCTION(BlueprintCallable)
	static int32 GetFloatAttributeIndex(USimpleGameplayAbilityComponent* AbilityComponent, FGameplayTag AttributeTag);

	UFUNCTION(BlueprintCallable)
	static int32 GetStructAttributeIndex(USimpleGameplayAbilityComponent* AbilityComponent, FGameplayTag AttributeTag);

	UFUNCTION(BlueprintCallable)
	static void CompareFloatAttributesAndSendEvents(const USimpleGameplayAbilityComponent* AbilityComponent, const FFloatAttribute& OldAttribute, const FFloatAttribute& NewAttribute);

	UFUNCTION()
	static void SendFloatAttributeChangedEvent(const USimpleGameplayAbilityComponent* AbilityComponent, FGameplayTag EventTag, FGameplayTag AttributeTag, EAttributeValueType ValueType, float NewValue);
};
