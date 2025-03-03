#pragma once

#include "CoreMinimal.h"
#include "SimpleGameplayAbilitySystem/SimpleGameplayAbilityComponent/SimpleAbilityComponentTypes.h"
#include "UObject/Object.h"
#include "SimpleStructAttributeHandler.generated.h"

struct FCyberSlotGroup;
struct FStructAttribute;
enum class ESimpleEventReplicationPolicy : uint8;
struct FGameplayTag;
class USimpleGameplayAbilityComponent;
struct FInstancedStruct;

UCLASS(Blueprintable)
class SIMPLEGAMEPLAYABILITYSYSTEM_API USimpleStructAttributeHandler : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SimpleGAS|StructAttributeHandler")
	UScriptStruct* StructType;
	
	void SetOwningAbilityComponent(USimpleGameplayAbilityComponent* InAbilityComponent) { AbilityComponent = InAbilityComponent; }
	USimpleGameplayAbilityComponent* GetOwningAbilityComponent() const { return AbilityComponent; }
	
	UFUNCTION(BlueprintCallable, Category = "SimpleGAS|StructAttributeHandler")
	USimpleGameplayAbilityComponent* GetAbilityComponent() const { return AbilityComponent; }
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SimpleGAS|StructAttributeHandler")
	FInstancedStruct GetStruct(FGameplayTag AttributeTag, bool& WasFound) const;

	UFUNCTION(BlueprintCallable, Category = "SimpleGAS|StructAttributeHandler")
	bool SetStruct(FGameplayTag AttributeTag, FInstancedStruct NewStruct);

	UFUNCTION(BlueprintNativeEvent, Category = "SimpleGAS|StructAttributeHandler")
	void OnStructChanged(FGameplayTag AttributeTag, FInstancedStruct OldStruct, FInstancedStruct NewStruct) const;
	virtual void OnStructChanged_Implementation(FGameplayTag AttributeTag, FInstancedStruct OldStruct, FInstancedStruct NewStruct) const;

	/**
	 * Sends an event to the event system through the owning ability component. The sender of the event is the AvatarActor of the owning ability component.
	 */
	UFUNCTION(BlueprintCallable, Category = "AbilityComponent|Attributes|AttributeHandler")
	void SendStructEvent(FGameplayTag EventTag, FInstancedStruct Payload, ESimpleEventReplicationPolicy ReplicationPolicy = ESimpleEventReplicationPolicy::NoReplication) const;
	
private:
	UPROPERTY()
	USimpleGameplayAbilityComponent* AbilityComponent = nullptr;
};
