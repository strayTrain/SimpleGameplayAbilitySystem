#pragma once

#include "CoreMinimal.h"
#include "SimpleAttributeModifierTypes.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleAbilityTypes.h"
#include "UObject/Object.h"
#include "SimpleAttributeModifier.generated.h"

class USimpleAbilityComponent;
class USimpleGameplayAttributes;

UCLASS(Blueprintable, ClassGroup = (SimpleGAS))
class SIMPLEGAMEPLAYABILITYSYSTEM_API USimpleAttributeModifier : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
	FGuid ModifierID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FAttributeModifierTagConfig TagConfig;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FAttributeModifierConfig ModifierConfig;

	UFUNCTION(BlueprintCallable)
	void InitializeModifier(const FGuid NewModifierID, USimpleAbilityComponent* InstigatingAbilityComponent, USimpleAbilityComponent* TargetedAbilityComponent);

	UFUNCTION(BlueprintNativeEvent)
	bool CanApplyModifier() const;
	
	UFUNCTION(BlueprintNativeEvent)
	void PreApplyModifierStack(TArray<FFloatAttribute>& TempFloatAttributes, TArray<FStructAttribute>& TempStructAttributes);

	UFUNCTION(BlueprintCallable)
	bool ApplyModifier(FInstancedStruct ModifierContext);
	
	UFUNCTION(BlueprintImplementableEvent)
	void PostApplyModifierStack();

	UFUNCTION(BlueprintImplementableEvent)
	void OnModifierApplySuccess();

	UFUNCTION(BlueprintImplementableEvent)
	void OnModifierApplyFailed();
	
	UFUNCTION(BlueprintNativeEvent, BlueprintPure)
	void GetFloatMetaAttributeValue(FGameplayTag MetaAttributeTag, float& OutValue, bool& WasHandled) const;

	UFUNCTION(BlueprintNativeEvent)
	void ModifyStructAttribute(FGameplayTag StructModifierTag, FGameplayTag MetaAttributeTag, FInstancedStruct& OutValue, bool& WasHandled) const;
	
	/**
	 * This function is called when the modifier stacks reach the max stacks.
	 * @param Overflow The amount of stacks that overflowed the max stacks.
	 */
	UFUNCTION(BlueprintImplementableEvent)
	void OnMaxStacksReached(int32 Overflow);

protected:
	UPROPERTY(BlueprintReadOnly)
	USimpleAbilityComponent* InstigatorAbilityComponent = nullptr;
	UPROPERTY(BlueprintReadOnly)
	USimpleAbilityComponent* TargetAbilityComponent = nullptr;

	UPROPERTY(BlueprintReadOnly)
	FInstancedStruct CurrentModifierContext;

private:
	bool bIsInitialized = false;
	
	// Checks if the ability can be applied based on the modifier's tag settings and if the modifier is initialized.
	bool CanApplyModifierInternal() const;
	FFloatAttribute* GetTempFloatAttribute(const FGameplayTag AttributeTag, TArray<FFloatAttribute>& TempFloatAttributes) const;
	FStructAttribute* GetTempStructAttribute(const FGameplayTag AttributeTag, TArray<FStructAttribute>& TempStructAttributes) const;
	bool ApplyFloatAttributeModifier(const FAttributeModifier& Modifier, TArray<FFloatAttribute>& TempFloatAttributes, float& CurrentOverflow);
	bool ApplyStructAttributeModifier(const FAttributeModifier& Modifier, TArray<FStructAttribute>& TempStructAttributes);
}; 
