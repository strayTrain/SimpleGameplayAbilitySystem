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

class USimpleAttributeComponent;

UCLASS(Blueprintable, Abstract, BlueprintType)
class SIMPLEGAMEPLAYABILITYSYSTEM_API USimpleAttributeHandler : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * The struct type that this handler expects to work with.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UScriptStruct* StructType;

	// Called from C++ only to initialize the handler with its owner and tag
	void InitializeHandler(USimpleAttributeComponent* NewAttributeOwner, FGameplayTag NewAttributeTag);

	UFUNCTION(BlueprintNativeEvent, Category = "SimpleGAS|AttributeHandler")
	FGameplayTagContainer GetModificationEvents(const FInstancedStruct& OldValue, const FInstancedStruct& NewValue);
	virtual FGameplayTagContainer GetModificationEvents_Implementation(const FInstancedStruct& OldValue, const FInstancedStruct& NewValue);

	/**
	 * Gets a copy of the struct associated with the given tag on the AttributeOwner.
	 * @return A copy of the struct associated with the tag, or an empty FInstancedStruct if not found
	 */
	UFUNCTION(BlueprintCallable, Category = "SimpleGAS|AttributeHandler")
	FInstancedStruct GetStructValue() const;

	/**
	 * Sets the struct associated with the AttributeTag on the AttributeOwner.
	 * @return True if the struct was set successfully, false if the attribute was not found or the struct type does not match
	 */
	UFUNCTION(BlueprintCallable, Category = "SimpleGAS|AttributeHandler")
	bool SetStructValue(const FInstancedStruct& NewValue);
	
	virtual UWorld* GetWorld() const override;

protected:
	FGameplayTag AttributeTag;
	TSoftObjectPtr<USimpleAttributeComponent> AttributeOwner;
};
