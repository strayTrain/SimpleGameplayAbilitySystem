#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "InstancedStruct.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SimpleAttributeFunctionLibrary.generated.h"

struct FAbilitySideEffect;
enum class EAttributeValueType : uint8;
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
	
	UFUNCTION(BlueprintCallable, BlueprintPure, meta = (DeterminesOutputType = "AvatarClass", HideSelfPin))
	static float GetFloatAttributeValue(USimpleGameplayAbilityComponent* AbilityComponent, EAttributeValueType ValueType, FGameplayTag AttributeTag, bool& WasFound);

	UFUNCTION(BlueprintCallable)
	static bool SetFloatAttributeValue(USimpleGameplayAbilityComponent* AbilityComponent, EAttributeValueType ValueType, FGameplayTag AttributeTag, float NewValue, float& Overflow);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	static bool OverrideFloatAttribute(USimpleGameplayAbilityComponent* AbilityComponent, FGameplayTag AttributeTag, FFloatAttribute NewAttribute);
	
	UFUNCTION(BlueprintCallable, BlueprintPure)
	static FInstancedStruct GetStructAttributeValue(USimpleGameplayAbilityComponent* AbilityComponent, FGameplayTag AttributeTag, bool& WasFound);
	
	UFUNCTION(BlueprintCallable)
	static bool SetStructAttributeValue(USimpleGameplayAbilityComponent* AbilityComponent, FGameplayTag AttributeTag, FInstancedStruct NewValue);

	UFUNCTION()
	static float ClampFloatAttributeValue(const FFloatAttribute& Attribute, EAttributeValueType ValueType, float NewValue, float& Overflow);

	UFUNCTION()
	static void CompareFloatAttributesAndSendEvents(const USimpleGameplayAbilityComponent* AbilityComponent, const FFloatAttribute& OldAttribute, const FFloatAttribute& NewAttribute);

	UFUNCTION()
	static void SendFloatAttributeChangedEvent(const USimpleGameplayAbilityComponent* AbilityComponent, FGameplayTag EventTag, FGameplayTag AttributeTag, EAttributeValueType ValueType, float NewValue);

	UFUNCTION()
	static void ApplyAbilitySideEffects(USimpleGameplayAbilityComponent* Instigator, const TArray<FAbilitySideEffect>& AbilitySideEffects);

	static FFloatAttribute* GetFloatAttribute(USimpleGameplayAbilityComponent* AbilityComponent, FGameplayTag AttributeTag);
	static FStructAttribute* GetStructAttribute(USimpleGameplayAbilityComponent* AbilityComponent, FGameplayTag AttributeTag);
};
