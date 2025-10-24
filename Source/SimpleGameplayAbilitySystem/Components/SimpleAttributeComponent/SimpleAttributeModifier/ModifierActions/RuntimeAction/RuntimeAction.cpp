#include "RuntimeAction.h"

#include "SimpleGameplayAbilitySystem/BlueprintFunctionLibraries/FunctionSelectors/FunctionSelectors.h"
#include "SimpleGameplayAbilitySystem/DefaultTags/DefaultTags.h"

void URuntimeAction::ApplyAction_Implementation()
{
	// Initialize the execution result to track all branch executions
	FRuntimeActionExecutionResult ExecutionResult;

	for (const FRuntimeActionBranch& ActionBranch : ActionBranches)
	{
		bool bShouldApply = true;
		UFunctionSelectors::ShouldApplyRuntimeAction(this, ActionBranch.ShouldTrigger, bShouldApply);

		if (bShouldApply)
		{
			// Capture the scratchpad state before executing this branch
			FRuntimeActionBranchResult BranchResult;
			BranchResult.InputScratchPad = GetScratchPad();

			// Execute the runtime action
			UFunctionSelectors::ApplyRuntimeAction(this, ActionBranch.OnTriggerAction);

			// Capture the scratchpad state after executing this branch
			BranchResult.OutputScratchPad = GetScratchPad();

			// Store this branch execution result
			ExecutionResult.ExecutedBranches.Add(BranchResult);
		}
	}

	// Store the execution result in the scratchpad for prediction correction
	if (ExecutionResult.ExecutedBranches.Num() > 0)
	{
		FInstancedStruct ExecutionResultStruct;
		ExecutionResultStruct.InitializeAs<FRuntimeActionExecutionResult>(ExecutionResult);
		SetScratchPadStruct(FDefaultTags::ScratchPadRuntimeActionExecutionResult(), ExecutionResultStruct);
	}
}

void URuntimeAction::OnClientPredictedCorrection_Implementation(
	FAttributeModifierActionScratchPad ServerInputScratchPad,
	FAttributeModifierActionScratchPad ServerOutputScratchPad,
	FAttributeModifierActionScratchPad ClientInputScratchPad,
	FAttributeModifierActionScratchPad ClientOutputScratchPad)
{
	// Helper lambda to retrieve execution result from scratchpad
	auto GetExecutionResult = [](const FAttributeModifierActionScratchPad& ScratchPad) -> const FRuntimeActionExecutionResult*
	{
		for (const FAttributeModifierActionScratchPadStruct& StructEntry : ScratchPad.ScratchpadStructs)
		{
			if (StructEntry.ScratchpadTag == FDefaultTags::ScratchPadRuntimeActionExecutionResult())
			{
				if (StructEntry.ScratchpadStruct.IsValid())
				{
					return StructEntry.ScratchpadStruct.GetPtr<FRuntimeActionExecutionResult>();
				}
			}
		}
		return nullptr;
	};

	// Retrieve the server and client execution results
	const FRuntimeActionExecutionResult* ServerExecutionResult = GetExecutionResult(ServerOutputScratchPad);
	const FRuntimeActionExecutionResult* ClientExecutionResult = GetExecutionResult(ClientOutputScratchPad);

	if (!ServerExecutionResult || !ClientExecutionResult)
	{
		// No execution results stored, nothing to correct
		return;
	}

	// Iterate through each executed branch and call the correction function
	const int32 NumBranches = FMath::Min(ServerExecutionResult->ExecutedBranches.Num(), ClientExecutionResult->ExecutedBranches.Num());

	for (int32 i = 0; i < NumBranches; ++i)
	{
		const FRuntimeActionBranchResult& ServerBranchResult = ServerExecutionResult->ExecutedBranches[i];
		const FRuntimeActionBranchResult& ClientBranchResult = ClientExecutionResult->ExecutedBranches[i];

		// Check if this branch has a prediction correction function configured
		// We need to look up the original ActionBranch to find the correction function
		if (i < ActionBranches.Num())
		{
			const FRuntimeActionBranch& ActionBranch = ActionBranches[i];

			// Call the prediction correction function for this specific branch
			UFunctionSelectors::RuntimeActionPredictionCorrection(
				this,
				ActionBranch.OnResolveActionPrediction,
				ServerBranchResult.InputScratchPad,
				ServerBranchResult.OutputScratchPad,
				ClientBranchResult.InputScratchPad,
				ClientBranchResult.OutputScratchPad
			);
		}
	}
}
