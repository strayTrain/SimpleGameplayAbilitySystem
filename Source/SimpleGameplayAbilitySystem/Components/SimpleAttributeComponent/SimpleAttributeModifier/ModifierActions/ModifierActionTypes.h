#pragma once

#include "CoreMinimal.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeModifier/SimpleAttributeModifierTypes.h"
#include "ModifierActionTypes.generated.h"

UENUM(BlueprintType)
enum class EModifierActionComponentTarget : uint8
{
	Target UMETA(DisplayName = "Target"),
	Instigator UMETA(DisplayName = "Instigator"),
	Both UMETA(DisplayName = "Both")
};

UENUM(BlueprintType)
enum class EModifierActionPredictionPolicy: uint8
{
	// If the action supports client prediction, it will be predicted on the client and generate an FAttributeModifierMutation to be sent to the server
	PredictIfPossible,
	// The action will only run on the server, with the result FAttributeModifierMutation replicated to the client
	ServerInitiate,
	// The action will only run on the server and won't be replicated to the client
	ServerOnly,
	// The action only runs on clients (ListenServer counts as a client) and won't replicate
	ClientOnly,
};

UENUM(BlueprintType)
enum class EActionPredictionMode : uint8
{
	NoPrediction,
	PredictInstantOnly,
	PredictDurationOnly,
	PredictAll
};

UENUM(BlueprintType)
enum class EContextSource : uint8
{
	NoContext,
	FromContextCollection,
	FromFunction
};

/**
 * Stores the scratchpad state before and after a single runtime action branch execution.
 * Used for prediction correction to identify which specific action needs correction.
 */
USTRUCT(BlueprintType)
struct FRuntimeActionBranchResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FAttributeModifierActionScratchPad InputScratchPad;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FAttributeModifierActionScratchPad OutputScratchPad;
};

/**
 * Stores the execution results for all runtime action branches that were executed.
 * This is stored in the scratchpad and used during prediction correction.
 */
USTRUCT(BlueprintType)
struct FRuntimeActionExecutionResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FRuntimeActionBranchResult> ExecutedBranches;
};

USTRUCT(BlueprintType)
struct FRuntimeActionBranch
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Dynamic Action Functions")
	FString Description = "Runtime Action";
	
	UPROPERTY(EditAnywhere, meta=(FunctionReference, AllowFunctionLibraries, PrototypeFunction="/Script/SimpleGameplayAbilitySystem.FunctionSelectors.Prototype_ShouldApplyRuntimeAction", DefaultBindingName="ShouldTriggerRuntimeAction"))
	FMemberReference ShouldTrigger;
	
	UPROPERTY(EditAnywhere, meta=(FunctionReference, AllowFunctionLibraries, PrototypeFunction="/Script/SimpleGameplayAbilitySystem.FunctionSelectors.Prototype_ApplyRuntimeAction", DefaultBindingName="TriggerRuntimeAction"))
	FMemberReference OnTriggerAction;

	UPROPERTY(EditAnywhere, meta=(FunctionReference, AllowFunctionLibraries, PrototypeFunction="/Script/SimpleGameplayAbilitySystem.FunctionSelectors.Prototype_RuntimeActionPredictionCorrection", DefaultBindingName="CorrectRuntimeActionPrediction"))
	FMemberReference OnResolveActionPrediction;
};
