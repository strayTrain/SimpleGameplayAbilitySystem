#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SimpleGameplayAbilitySystem/Module/SimpleGameplayAbilitySystem.h"
#include "SimpleGameplayAbilitySystem/SimpleGameplayAbilityComponent/SimpleGameplayAbilityComponent.h"
#include "SimpleGameplayAbilitySystem/SimpleGameplayAbilityComponent/Interface/SimpleAbilitySystemComponent.h"
#include "NodeHelpers.generated.h"

UCLASS()
class SIMPLEGAMEPLAYABILITYSYSTEM_API UNodeHelpers : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/* This function allows you to drag an actor that implements ISimpleAbilitySystemComponent into an ability component pin */
	UFUNCTION(BlueprintPure, meta=(ImplicitThis="true", BlueprintAutocast))
	static USimpleGameplayAbilityComponent* GetSimpleAbilityComponent(AActor* Actor)
	{
		if (!Actor)
		{
			return nullptr;
		}
        
		// Use the interface to get the ability component
		if (Actor->Implements<USimpleAbilitySystemComponent>())
		{
			return ISimpleAbilitySystemComponent::Execute_GetAbilitySystemComponent(Actor);
		}
        
		// Fallback: directly look for the component
		return Cast<USimpleGameplayAbilityComponent>(Actor->GetComponentByClass(USimpleGameplayAbilityComponent::StaticClass()));
	}

	// Pure style function to convert any struct to an instanced struct
	UFUNCTION(BlueprintPure, CustomThunk, meta=(CustomStructureParam="InStruct", DisplayName="To Instanced Struct", CompactNodeTitle="INSTANCE", Keywords="convert cast make instanced struct"))
	static FInstancedStruct MakeInstancedStructFromAny(const int32& InStruct);
    
	// The actual implementation called by the thunk
	static FInstancedStruct Generic_ConvertToInstancedStruct(const void* StructAddr, const UScriptStruct* StructType)
	{
		if (StructAddr && StructType)
		{
			FInstancedStruct Result;
			Result.InitializeAs(StructType);
			StructType->CopyScriptStruct(Result.GetMutableMemory(), StructAddr);
			return Result;
		}
		return FInstancedStruct();
	}
	
	// The thunk function that Blueprint calls
	DECLARE_FUNCTION(execMakeInstancedStructFromAny)
	{
		// Get the struct property from the stack
		Stack.MostRecentProperty = nullptr;
		Stack.StepCompiledIn<FStructProperty>(nullptr);

		const void* StructPtr = Stack.MostRecentPropertyAddress;
		const FStructProperty* StructProperty = CastField<FStructProperty>(Stack.MostRecentProperty);
	
		// Set up the return value
		P_FINISH;
	
		FInstancedStruct Result;
		if (StructProperty && StructPtr)
		{
			Result = Generic_ConvertToInstancedStruct(StructPtr, StructProperty->Struct);
		}
		else
		{
			FString TypeName = StructProperty ? StructProperty->Struct->GetName() : TEXT("Unknown");
			if (!StructProperty)
			{
				TypeName = Stack.MostRecentProperty ? Stack.MostRecentProperty->GetClass()->GetName() : TEXT("Invalid");
				UE_LOG(LogSimpleGAS, Error, TEXT("[UNodeHelpers::AsInstance]: %s is not a struct, returning empty instanced struct"), *TypeName);
			}
		}
	
		*static_cast<FInstancedStruct*>(RESULT_PARAM) = Result;
	}
};
