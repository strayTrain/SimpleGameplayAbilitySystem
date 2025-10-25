#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SimpleGameplayAbilitySystem/Module/SimpleGameplayAbilitySystem.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleGameplayAbilityComponent/SimpleGameplayAbilityComponent.h"
#include "InstancedStruct.h"
#include "NodeHelpers.generated.h"

UENUM(BlueprintType)
enum class EGetInstancedStructResult : uint8
{
	Valid		UMETA(DisplayName = "Valid"),
	Invalid		UMETA(DisplayName = "Invalid")
};

UCLASS()
class SIMPLEGAMEPLAYABILITYSYSTEM_API UNodeHelpers : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
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

	// Function to extract struct from FInstancedStruct with type validation
	// This is the blueprint-callable declaration that K2Node_GetInstancedStruct uses
	UFUNCTION(BlueprintCallable, CustomThunk, Category = "Utilities|InstancedStruct",
		meta=(BlueprintInternalUseOnly="true", CustomStructureParam="OutStruct"))
	static EGetInstancedStructResult GetInstancedStruct(
		const FInstancedStruct& InstancedStruct,
		const UScriptStruct* StructType,
		int32& OutStruct);

	// The thunk implementation
	DECLARE_FUNCTION(execGetInstancedStruct)
	{
		// Get the InstancedStruct parameter
		P_GET_STRUCT_REF(FInstancedStruct, InstancedStruct);

		// Get the UScriptStruct* parameter
		P_GET_OBJECT(UScriptStruct, StructType);

		// Get the wildcard output struct parameter
		Stack.MostRecentProperty = nullptr;
		Stack.StepCompiledIn<FStructProperty>(nullptr);
		void* OutStructPtr = Stack.MostRecentPropertyAddress;
		FStructProperty* OutStructProperty = CastField<FStructProperty>(Stack.MostRecentProperty);

		P_FINISH;

		// Perform validation and copy
		EGetInstancedStructResult Result = EGetInstancedStructResult::Invalid;

		if (!StructType)
		{
			UE_LOG(LogSimpleGAS, Warning, TEXT("[UNodeHelpers::GetInstancedStruct]: StructType is null"));
			*static_cast<EGetInstancedStructResult*>(RESULT_PARAM) = Result;
			return;
		}

		if (!InstancedStruct.IsValid())
		{
			UE_LOG(LogSimpleGAS, Warning, TEXT("[UNodeHelpers::GetInstancedStruct]: InstancedStruct is not valid"));
			*static_cast<EGetInstancedStructResult*>(RESULT_PARAM) = Result;
			return;
		}

		// Check if the instanced struct contains the expected type
		const UScriptStruct* ActualStructType = InstancedStruct.GetScriptStruct();
		if (ActualStructType != StructType)
		{
			UE_LOG(LogSimpleGAS, Warning,
				TEXT("[UNodeHelpers::GetInstancedStruct]: Type mismatch. Expected: %s, Actual: %s"),
				*StructType->GetName(),
				ActualStructType ? *ActualStructType->GetName() : TEXT("None"));
			*static_cast<EGetInstancedStructResult*>(RESULT_PARAM) = Result;
			return;
		}

		// Verify the output property matches
		if (!OutStructProperty || OutStructProperty->Struct != StructType)
		{
			UE_LOG(LogSimpleGAS, Error,
				TEXT("[UNodeHelpers::GetInstancedStruct]: Output struct property mismatch"));
			*static_cast<EGetInstancedStructResult*>(RESULT_PARAM) = Result;
			return;
		}

		// Copy the data from the instanced struct to the output
		if (OutStructPtr && InstancedStruct.GetMemory())
		{
			StructType->CopyScriptStruct(OutStructPtr, InstancedStruct.GetMemory());
			Result = EGetInstancedStructResult::Valid;
		}

		*static_cast<EGetInstancedStructResult*>(RESULT_PARAM) = Result;
	}
};
