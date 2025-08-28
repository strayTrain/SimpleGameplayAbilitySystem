#pragma once

#include "CoreMinimal.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleAttributeModifier/ModifierActions/Base/ModifierAction.h"
#include "ChangeStructAttributeAction.generated.h"

UCLASS()
class SIMPLEGAMEPLAYABILITYSYSTEM_API UChangeStructAttributeAction : public UModifierAction
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag AttributeToModify;
	
	UPROPERTY(EditAnywhere, meta=(FunctionReference, AllowFunctionLibraries, PrototypeFunction="/Script/SimpleGameplayAbilitySystem.FunctionSelectors.Prototype_ModifyStructAttributeValue", DefaultBindingName="GetModifiedStructAttribute"))
	FMemberReference StructModificationFunction;
};
