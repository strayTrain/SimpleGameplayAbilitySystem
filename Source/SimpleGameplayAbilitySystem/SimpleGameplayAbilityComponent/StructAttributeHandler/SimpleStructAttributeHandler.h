#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SimpleStructAttributeHandler.generated.h"

enum class ESimpleEventReplicationPolicy : uint8;
struct FGameplayTag;
class USimpleGameplayAbilityComponent;
struct FInstancedStruct;

UCLASS(Blueprintable)
class SIMPLEGAMEPLAYABILITYSYSTEM_API USimpleStructAttributeHandler : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(USimpleGameplayAbilityComponent* InAbilityComponent);
	
	UFUNCTION(BlueprintCallable, Category = "AbilityComponent|Attributes|AttributeHandler")
	USimpleGameplayAbilityComponent* GetAbilityComponent() const { return AbilityComponent; }

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Struct Attribute Handler")
	FInstancedStruct OnInitializeStruct();
	virtual FInstancedStruct OnInitializeStruct_Implementation();
	
	/**
	 * This function is called when the struct attribute changes. You can override this function in a blueprint subclass
	 * to handle changes of individual struct members e.g. sending an event
	 * @param OldStruct The previous value of the struct
	 * @param NewStruct The new value of the struct
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Struct Attribute Handler")
	void OnStructChanged(FInstancedStruct OldStruct, FInstancedStruct NewStruct);
	virtual void OnStructChanged_Implementation(FInstancedStruct OldStruct, FInstancedStruct NewStruct);

	/**
 * Sends an event to the event system through the owning ability component. The sender of the event is the AvatarActor of the owning ability component.
 */
	UFUNCTION(BlueprintCallable, Category = "AbilityComponent|Attributes|AttributeHandler")
	void SendEvent(FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload, ESimpleEventReplicationPolicy ReplicationPolicy) const;
	
private:
	UPROPERTY()
	USimpleGameplayAbilityComponent* AbilityComponent = nullptr;
};
