#pragma once

#if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5)
	#include "StructUtils/InstancedStruct.h"
#else
	#include "InstancedStruct.h"
#endif

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "FloatAttributeActionTypes.generated.h"

class USimpleGameplayAbilityComponent;

UENUM(BlueprintType)
enum class EAttributeModificationValueSource : uint8
{
	// Directly set the value of the float action input
	Manual,
	// Use the magnitude value passed to the owning modifier as the float action input
	FromMagnitude,
	// Use the overflow value from previous float actions as the float action input
	FromOverflow,
	// Use the value of an attribute on the instigator ability component as the float action input
	FromInstigatorAttribute,
	// Use the value of an attribute on the target ability component as the float action input
	FromTargetAttribute,
	// Use a custom function to get the float action input
	CustomInputValue
};

UENUM(BlueprintType)
enum class EFloatAttributeValueType : uint8
{
	CurrentValue,
	BaseValue,
	MaxCurrentValue,
	MinCurrentValue,
	MaxBaseValue,
	MinBaseValue
};

UENUM(BlueprintType)
enum class EFloatAttributeModificationOperation : uint8
{
	/**
	 * Output = A + B
	 */
	Add,
	/**
	 * Output = A - B
	 */
	Subtract,
	/**
	 * Output = A * B
	 */
	Multiply,
	/**
	 * Output = A / B
	 */
	Divide,
	/**
	 * Output = A ** B
	 */
	Power,
	/**
	 * Output = B
	 */
	Override,
	/**
	 * Output = [function call]
	 */
	Custom
};

USTRUCT(BlueprintType)
struct FFloatAttributeModification
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USimpleGameplayAbilityComponent* AttributeOwner;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag AttributeTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EFloatAttributeValueType ValueType;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float NewValue;
};

USTRUCT(BlueprintType)
struct FStructAttributeModification
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USimpleGameplayAbilityComponent* AttributeOwner;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag AttributeTag;

	// Use these tags to indicate which members of the struct have changed
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer ModificationTags;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FInstancedStruct NewValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FInstancedStruct OldValue;
};