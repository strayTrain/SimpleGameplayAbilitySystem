#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SimpleGameplayAbilitySystem/SimpleGASTypes/SimpleGasTypes.h"
#include "SimpleGameplayAttributes.generated.h"

class USimpleGameplayAbilityComponent;
class UAttributeSet;

UCLASS(EditInlineNew, BlueprintType, ClassGroup=(AbilitySystemComponent), meta=(BlueprintSpawnableComponent))
class SIMPLEGAMEPLAYABILITYSYSTEM_API USimpleGameplayAttributes : public UObject
{
	GENERATED_BODY()

public:
	USimpleGameplayAttributes();

	UPROPERTY(BlueprintReadOnly)
	USimpleGameplayAbilityComponent* OwningAbilityComponent;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing=OnRep_FloatAttributes)
	TArray<FFloatAttribute> FloatAttributes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing=OnRep_StructAttributes)
	TArray<FStructAttribute> StructAttributes;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TArray<UAttributeSet*> AttributeSets;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void InitializeAttributeSets(USimpleGameplayAbilityComponent* NewOwningAbilityComponent);
	
	// Float attribute functions
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void AddFloatAttribute(FFloatAttribute Attribute);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void RemoveFloatAttributes(FGameplayTagContainer AttributeTags);

	UFUNCTION(BlueprintCallable, meta = (DeterminesOutputType = "AvatarClass", HideSelfPin))
	FFloatAttribute GetFloatAttribute(FGameplayTag AttributeTag, bool& WasFound) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, meta = (DeterminesOutputType = "AvatarClass", HideSelfPin))
	float GetFloatAttributeValue(EAttributeValueType ValueType, FGameplayTag AttributeTag, bool& WasFound) const;

	UFUNCTION(BlueprintCallable)
	bool SetFloatAttributeValue(EAttributeValueType ValueType, FGameplayTag AttributeTag, float NewValue, float& Overflow);

	UFUNCTION(BlueprintCallable)
	bool SetFloatAttributeMaxValue(EAttributeValueType ValueType, FGameplayTag AttributeTag, float NewMaxValue, float& Overflow);

	UFUNCTION(BlueprintCallable)
	bool SetFloatAttributeMinValue(EAttributeValueType ValueType, FGameplayTag AttributeTag, float NewMinValue, float& Overflow);

	UFUNCTION(BlueprintCallable)
	bool SetFloatAttributeUseMinValue(EAttributeValueType ValueType, FGameplayTag AttributeTag, bool UseMinValue);

	UFUNCTION(BlueprintCallable)
	bool SetFloatAttributeUseMaxValue(EAttributeValueType ValueType, FGameplayTag AttributeTag, bool UseMaxValue);
	
	UFUNCTION(BlueprintCallable)
	bool AddToFloatAttributeValue(EAttributeValueType ValueType, FGameplayTag AttributeTag, float Amount, float& Overflow);
	
	UFUNCTION(BlueprintCallable)
	bool MultiplyFloatAttributeValue(EAttributeValueType ValueType, FGameplayTag AttributeTag, float Multiplier, float& Overflow);

	// Struct attribute functions
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void AddStructAttribute(FStructAttribute Attribute);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void RemoveStructAttributes(FGameplayTagContainer AttributeTags);
	
	UFUNCTION(BlueprintCallable)
	FStructAttribute GetStructAttribute(FGameplayTag AttributeTag, bool& WasFound) const;

	UFUNCTION(BlueprintCallable)
	FInstancedStruct GetStructAttributeValue(FGameplayTag AttributeTag, bool& WasFound) const;

	UFUNCTION(BlueprintCallable)
	bool SetStructAttributeValue(FGameplayTag AttributeTag, FInstancedStruct NewValue);

protected:
	UFUNCTION()
	void OnRep_FloatAttributes(TArray<FFloatAttribute> OldAttributes);
	UFUNCTION()
	void OnRep_StructAttributes(TArray<FStructAttribute> OldAttributes);
	
	virtual UWorld* GetWorld() const override;
	virtual bool IsSupportedForNetworking() const override { return true; }
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

private:
	int32 GetFloatAttributeIndex(FGameplayTag AttributeTag) const;
	float ClampValue(const FFloatAttribute& Attribute, EAttributeValueType ValueType, float NewValue, float& Overflow);
	void SendFloatAttributeChangedEvent(FGameplayTag EventTag, FGameplayTag AttributeTag, EAttributeValueType ValueType, float NewValue);
};
