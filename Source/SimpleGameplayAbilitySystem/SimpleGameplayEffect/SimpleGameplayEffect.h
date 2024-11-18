#pragma once

#include "CoreMinimal.h"
#include "SimpleGameplayAbilitySystem/SimpleGASTypes/SimpleGasTypes.h"
#include "UObject/Object.h"
#include "SimpleGameplayEffect.generated.h"

class USimpleGameplayAbilityComponent;
class USimpleGameplayAttributes;

UCLASS(Blueprintable, ClassGroup = (SimpleGAS))
class SIMPLEGAMEPLAYABILITYSYSTEM_API USimpleGameplayEffect : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
	FGuid EffectID;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FGameplayEffectConfig EffectConfig;

	UFUNCTION(BlueprintNativeEvent, Category = "SimpleGAS|Effect")
	void InitializeEffect(USimpleGameplayAbilityComponent* SourceAbilityComponent, USimpleGameplayAbilityComponent* TargetAbilityComponent);
	
	UFUNCTION(BlueprintCallable, Category = "SimpleGAS|Effect")
	bool ApplyEffect(FInstancedStruct EffectContext);

	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "SimpleGAS|Effect")
	void GetFloatMetaAttributeValue(FGameplayTag MetaAttributeTag, float& OutValue, bool& WasHandled) const;

	UFUNCTION(BlueprintNativeEvent, Category = "SimpleGAS|Effect")
	void ModifyStructAttribute(FGameplayTag StructModifierTag, FGameplayTag MetaAttributeTag, FInstancedStruct& OutValue, bool& WasHandled) const;

	UFUNCTION(BlueprintNativeEvent, Category = "SimpleGAS|Effect")
	bool CanApplyEffect() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "SimpleGAS|Effect")
	void OnApplicationFailed();
	
	/**
	 * This function is called when the effect stacks reach the max stacks.
	 * @param Overflow The amount of stacks that overflowed the max stacks.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "SimpleGAS|Effect")
	void OnMaxStacksReached(int32 Overflow);
	
	UFUNCTION(BlueprintImplementableEvent, Category = "SimpleGAS|Effect")
	void OnEffectApplySuccess();

	UFUNCTION(BlueprintImplementableEvent, Category = "SimpleGAS|Effect")
	void OnEffectApplyFailed();

	UFUNCTION(BlueprintImplementableEvent, Category = "SimpleGAS|Effect")
	void OnPreDurationEffectTick();
	
	UFUNCTION(BlueprintImplementableEvent, Category = "SimpleGAS|Effect")
	void OnDurationEffectTick();

protected:
	UPROPERTY(BlueprintReadOnly)
	USimpleGameplayAbilityComponent* SourceAbilitySystemComponent = nullptr;
	UPROPERTY(BlueprintReadOnly)
	USimpleGameplayAbilityComponent* TargetAbilitySystemComponent = nullptr;

	UPROPERTY(BlueprintReadOnly)
	FInstancedStruct CurrentEffectContext;

private:
	bool bIsInitialized = false;

	bool GetTempFloatAttribute(const TArray<FFloatAttribute>& FloatAttributes, const FGameplayTag AttributeTag, FFloatAttribute& OutAttribute) const;
	bool GetTempStructAttribute(const TArray<FStructAttribute>& StructAttributes, const FGameplayTag AttributeTag, FStructAttribute& OutAttribute) const;
	
	// Checks if the ability can be applied based on the effect's tag settings and if the effect is initialized.
	bool CanApplyEffectInternal() const;
}; 
