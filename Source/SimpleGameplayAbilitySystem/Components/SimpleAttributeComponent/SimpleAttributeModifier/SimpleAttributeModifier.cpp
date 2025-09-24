#include "SimpleAttributeModifier.h"

#include "ModifierActions/ModifierActionTypes.h"
#include "ModifierActions/Base/ModifierAction.h"
#include "SimpleGameplayAbilitySystem/BlueprintFunctionLibraries/FunctionSelectors/FunctionSelectors.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeComponent.h"
#include "SimpleGameplayAbilitySystem/DefaultTags/DefaultTags.h"
#include "SimpleGameplayAbilitySystem/Module/SimpleGameplayAbilitySystem.h"

bool USimpleAttributeModifier::ApplyModifier(const FGuid NewModifierID, USimpleAttributeComponent* Instigator, USimpleAttributeComponent* Target, const float Magnitude, const FInstancedStruct Context)
{
	ModifierID = NewModifierID;
	InstigatorAttributeComponent = Instigator;
	TargetAttributeComponent = Target;
	ModifierContext = Context;
	ModifierActionScratchPad = FAttributeModifierActionScratchPad();
	ModifierMagnitude = Magnitude;

	if (!CanApplyModifierInternal())
	{
		CancelModifier(FDefaultTags::AttributeModifierCancelled(), FInstancedStruct());
		return false;
	}
	
	ActivationTime = InstigatorAttributeComponent->GetServerTime();
	IsActive = true;

	// Add permanent gameplay tags from this modifier
	for (const FGameplayTag& Tag : PermanentlyAppliedTags)
	{
		TargetAttributeComponent->AddGameplayTag(Tag);
	}

	OnPreApplyModifierActions();
	
	FGameplayTagContainer Triggers; 
	Triggers.AddTag(FDefaultTags::AttributeModifierApplied()); 
	ApplyModifierActions(this, Triggers);
	
	// If we're an instant modifier we apply the action stack immediately and then end.
	// SetDuration modifiers with a duration of 0 also apply immediately and end.
	
	if (DurationType == EAttributeModifierDurationType::Instant || (DurationType == EAttributeModifierDurationType::SetDuration && Duration <= 0))
	{
		EndModifier(FDefaultTags::AttributeModifierEnded(), FInstancedStruct());
		return true;
	}
	
	// Otherwise we're a duration modifier (either SetDuration with a duration > 0 or InfiniteDuration)

	for (const FGameplayTag& Tag : TemporarilyAppliedTags)
	{
		TargetAttributeComponent->AddGameplayTag(Tag);
	}

	// Set the tick timer
	if (TickInterval > 0)
	{
		GetWorld()->GetTimerManager().SetTimer(TickTimerHandle, this, &USimpleAttributeModifier::OnTickTimerTriggered, TickInterval, true);
	}
	
	if (DurationType == EAttributeModifierDurationType::SetDuration && Duration > 0)
	{
		// Set the duration timer
		GetWorld()->GetTimerManager().SetTimer(
			DurationTimerHandle,
			this,
			&USimpleAttributeModifier::OnDurationTimerExpired,
			Duration,
			false
		);
		
		return true;
	}
	
	return false;
}

void USimpleAttributeModifier::EndModifier(FGameplayTag EndingStatus, FInstancedStruct EndingContext)
{
	{
		FGameplayTagContainer Triggers; 
		Triggers.AddTag(FDefaultTags::AttributeModifierEnded()); 
		ApplyModifierActions(this, Triggers);
	}
	
	if (DurationType == EAttributeModifierDurationType::SetDuration || DurationType == EAttributeModifierDurationType::InfiniteDuration)
	{
		InstigatorAttributeComponent->GetWorld()->GetTimerManager().ClearTimer(DurationTimerHandle);
		InstigatorAttributeComponent->GetWorld()->GetTimerManager().ClearTimer(TickTimerHandle);
		
		for (const FGameplayTag& Tag : TemporarilyAppliedTags)
		{
			TargetAttributeComponent->RemoveGameplayTag(Tag);
		}
	}

	IsActive = false;
	OnModifierEnded(EndingStatus, EndingContext);
	OnAttributeModifierEnded.Broadcast(ModifierID, EndingStatus, EndingContext);
}

void USimpleAttributeModifier::CancelModifier(FGameplayTag EndingStatus, FInstancedStruct EndingContext)
{
	{
		FGameplayTagContainer Triggers; 
		Triggers.AddTag(EndingStatus); 
		ApplyModifierActions(this, Triggers);
	}
	
	if (DurationType == EAttributeModifierDurationType::SetDuration || DurationType == EAttributeModifierDurationType::InfiniteDuration)
	{
		InstigatorAttributeComponent->GetWorld()->GetTimerManager().ClearTimer(DurationTimerHandle);
		InstigatorAttributeComponent->GetWorld()->GetTimerManager().ClearTimer(TickTimerHandle);
		
		for (const FGameplayTag& Tag : TemporarilyAppliedTags)
		{
			TargetAttributeComponent->RemoveGameplayTag(Tag);
		}
	}

	IsActive = false;
	OnModifierCancelled(EndingStatus, EndingContext);
	OnAttributeModifierCancelled.Broadcast(ModifierID, EndingStatus, EndingContext);
}

bool USimpleAttributeModifier::CanApplyModifierInternal()
{
	if (!InstigatorAttributeComponent)
	{
		SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleAttributeModifier::CanApplyModifierInternal]: InstigatorAttributeComponent is null.")));
		return false;
	}
	
	if (!TargetAttributeComponent)
	{
		SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleAttributeModifier::CanApplyModifierInternal]: TargetAttributeComponent is null.")));
		return false;
	}
	
	if (!TargetAttributeComponent->HasAllGameplayTags(TargetRequiredTags))
	{
		SIMPLE_LOG(this, FString::Printf(TEXT("Target does not have required tags in USimpleAttributeModifier::CanApplyModifierInternal")));
		return false;
	}
	
	if (TargetAttributeComponent->HasAnyGameplayTags(TargetBlockingTags))
	{
		SIMPLE_LOG(this, FString::Printf(TEXT("Target has blocking tags in USimpleAttributeModifier::CanApplyModifierInternal")));
		return false;
	}

	if (TargetAttributeComponent->IsModifierWithTagsActive(TargetBlockingModifierTags))
	{
		SIMPLE_LOG(this, FString::Printf(TEXT("Target has blocking modifier tags in USimpleAttributeModifier::CanApplyModifierInternal")));
		return false;
	}

	return CanApplyModifier();
}

bool USimpleAttributeModifier::ApplyModifierActions(USimpleAttributeModifier* OwningModifier, const FGameplayTagContainer ActionTriggers)
{
	TArray<FModifierActionResult> ActionResults;

	for (int i = 0; i < ModifierActions.Num(); i++)
	{
		UModifierAction* Action = ModifierActions[i];
		Action->InitializeAction(ModifierActionScratchPad, OwningModifier);
		
		if (!Action->EventTriggers.HasAnyExact(ActionTriggers) || !Action->CanApply())
		{
			continue;
		}

		const FAttributeModifierActionScratchPad InputScratchpad = ModifierActionScratchPad;
		const FInstancedStruct ActionResult = Action->ApplyAction();
		
		// Add the result of the action if applicable
		if (Action->ApplicationPolicy == EAttributeModifierActionPolicy::ApplyClientPredicted ||
			Action->ApplicationPolicy == EAttributeModifierActionPolicy::ApplyServerInitiated)
		{
			ActionResults.Add({
				i,
				Action->GetClass(),
				InputScratchpad,
				ActionResult
			});
		}
	}
	
	if (ActionResults.Num() > 0)
	{
		FModifierActionStackResults ActionStackResult;
		ActionStackResult.ActionsResults = ActionResults;
		// The attribute component listens for this event to track in AuthorityAttributeModifierMutations and ultimately replicate to clients.
		OnActionStackApplied.Broadcast(ModifierID, ActionStackResult);
	}

	OnPostApplyModifierActions();
	
	return true;
}

void USimpleAttributeModifier::AddModifierStack(int32 StackCount)
{
	if (OnReapplication != EDurationModifierReApplicationConfig::AddStack)
	{
		UE_LOG(LogSimpleGAS, Warning, TEXT("[USimpleAttributeModifier::AddModifierStack]: Modifier %s cannot stack."), *GetName());
		return;
	}

	if (HasMaxStacks)
	{
		if (ModifierStacks + StackCount > MaxStacks)
		{
			ModifierStacks = MaxStacks;
			OnMaxStacksReached();
			return;
		}
	}

	ModifierStacks += StackCount;
	OnStacksAdded(StackCount, ModifierStacks);
}

void USimpleAttributeModifier::OnClientReceivedServerActionsResult(FInstancedStruct ServerSnapshot, FInstancedStruct ClientSnapshot)
{
	const FModifierActionStackResults* ServerSnapshotPtr = ServerSnapshot.GetPtr<FModifierActionStackResults>();
	const FModifierActionStackResults* ClientSnapshotPtr = ClientSnapshot.GetPtr<FModifierActionStackResults>();

	TArray<FModifierActionResult> ServerActionResults;
	TArray<FModifierActionResult> ClientActionResults;
	
	if (!ServerSnapshotPtr)
	{
		UE_LOG(LogSimpleGAS, Warning, TEXT("[USimpleAttributeModifier::OnClientReceivedServerActionsResult]: Server snapshot is null and this should never be the case. Something went wrong :/"));
		return;
	}

	ServerActionResults = ServerSnapshotPtr->ActionsResults;
	
	// It's possible that the client didn't take a snapshot itself but is receiving a snapshot from the server i.e. ActionApplicationPolicy is AppluServerInitiated
	if (!ClientSnapshotPtr)
	{
		ClientActionResults = TArray<FModifierActionResult>();
	}
	else
	{
		ClientActionResults = ClientSnapshotPtr->ActionsResults;
	}
	
	// Create maps for server and client action results to compare them easily.
	TMap<int32, FModifierActionResult> ServerMap, ClientMap;
	for (const FModifierActionResult& ActionResult : ServerActionResults) ServerMap.Add(ActionResult.ActionIndex, ActionResult);
	for (const FModifierActionResult& ActionResult : ClientActionResults) ClientMap.Add(ActionResult.ActionIndex, ActionResult);
	
	TArray<int32> ServerMapKeys, ClientMapKeys;
	ServerMap.GetKeys(ServerMapKeys);
	ClientMap.GetKeys(ClientMapKeys);

	TSet<int32> AllIndices;
	AllIndices.Append(ServerMapKeys);
	AllIndices.Append(ClientMapKeys);

	for (int32 Idx : AllIndices)
	{
		UModifierAction* Action = ModifierActions.IsValidIndex(Idx) ? ModifierActions[Idx] : nullptr;
		if (!Action) continue;

		const bool IsInServerMap = ServerMap.Contains(Idx);
		const bool IsInClientMap = ClientMap.Contains(Idx);

		if (IsInServerMap && !IsInClientMap)
		{
			Action->InitializeAction(ServerMap[Idx].InputScratchpad, this);
			const FInstancedStruct Result = Action->ApplyAction();
		}
		else if (!IsInServerMap && IsInClientMap)
		{
			Action->OnCancelAction();
		}
		else if (IsInServerMap && IsInClientMap)
		{
			// If the snapshots match we don't need to do anything
			if (ServerMap[Idx].ActionResult == ClientMap[Idx].ActionResult)
			{
				continue;
			}
			
			Action->OnClientPredictedCorrection(
				ServerMap[Idx].InputScratchpad,
				ServerMap[Idx].ActionResult,
				ClientMap[Idx].InputScratchpad,
				ClientMap[Idx].ActionResult
			);
		}
	}
}

void USimpleAttributeModifier::OnDurationTimerExpired()
{
	EndModifier(FDefaultTags::AttributeModifierEnded(), FInstancedStruct());
}

void USimpleAttributeModifier::OnTickTimerTriggered()
{
	// Check if the modifier can still be applied
	if (!CanApplyModifierInternal())
	{
		switch (TickRequirementsFailedBehaviour)
		{
			case EDurationTickTagRequirementBehaviour::CancelOnTagRequirementFailed:
				CancelModifier(FDefaultTags::AttributeModifierTickFailedCancel(), FInstancedStruct());
				return;
			
			case EDurationTickTagRequirementBehaviour::SkipOnTagRequirementFailed:
			{
				FGameplayTagContainer Triggers; 
				Triggers.AddTag(FDefaultTags::AttributeModifierTickFailedSkip()); 
				ApplyModifierActions(this, Triggers);
				return;
			}
		}
	}

	TickCount += 1;

	FGameplayTagContainer Triggers; 
	Triggers.AddTag(FDefaultTags::AttributeModifierTicked()); 
	ApplyModifierActions(this, Triggers);
}
