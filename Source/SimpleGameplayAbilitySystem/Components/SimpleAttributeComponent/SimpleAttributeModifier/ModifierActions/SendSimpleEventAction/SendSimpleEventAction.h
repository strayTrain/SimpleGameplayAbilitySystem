#pragma once

#include "CoreMinimal.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeModifier/ModifierActions/Base/ModifierAction.h"
#include "SendSimpleEventAction.generated.h"

UCLASS()
class SIMPLEGAMEPLAYABILITYSYSTEM_API USendSimpleEventAction : public UModifierAction
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Config")
	FGameplayTag EventTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Config")
	FGameplayTag DomainTag;

	/** Optional function that returns the payload to be sent with the event */
	UPROPERTY(EditAnywhere, meta=(FunctionReference, AllowFunctionLibraries, PrototypeFunction="/Script/SimpleGameplayAbilitySystem.FunctionSelectors.Prototype_GetStructContext", DefaultBindingName="GetEventPayload"))
	FMemberReference PayloadFunction;
	
	virtual bool CanApply_Implementation() const override;
	virtual void ApplyAction_Implementation() override;
};
