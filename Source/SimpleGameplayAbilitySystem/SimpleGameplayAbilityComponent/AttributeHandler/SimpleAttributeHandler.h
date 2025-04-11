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

class USimpleGameplayAbilityComponent;

UCLASS(Blueprintable)
class SIMPLEGAMEPLAYABILITYSYSTEM_API USimpleAttributeHandler : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * The struct type that this handler expects to work with.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UScriptStruct* StructType;
	
	UFUNCTION(BlueprintNativeEvent, Category = "SimpleGAS|AttributeHandler")
	FGameplayTagContainer GetModificationEvents(FGameplayTag AttributeTag, const FInstancedStruct& OldValue, const FInstancedStruct& NewValue);
	virtual FGameplayTagContainer GetModificationEvents_Implementation(FGameplayTag AttributeTag, const FInstancedStruct& OldValue, const FInstancedStruct& NewValue);

	/**
	 * Gets a copy of the struct associated with the given tag on the AttributeOwner.
	 * @param AttributeTag The tag of the struct to get
	 * @param WasFound True if the attribute was found and matches the StructType, false otherwise
	 * @return A copy of the struct associated with the tag, or an empty struct if not found
	 */
	UFUNCTION(BlueprintCallable, Category = "SimpleGAS|AttributeHandler")
	FInstancedStruct GetStruct(FGameplayTag AttributeTag, bool& WasFound) const;

	/**
	 * Sets the struct associated with the given tag on the AttributeOwner.
	 * @param AttributeTag The tag of the struct to set
	 * @param NewValue The instanced struct wrapping the changed struct
	 * @return True if the struct was set successfully, false if the attribute was not found or the struct type does not match
	 */
	UFUNCTION(BlueprintCallable, Category = "SimpleGAS|AttributeHandler")
	bool SetStruct(FGameplayTag AttributeTag, const FInstancedStruct& NewValue);
	
	TSoftObjectPtr<USimpleGameplayAbilityComponent> AttributeOwner;
};
