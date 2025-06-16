#include "SimpleAttributeModifier.h"

#include "ModifierActions/ModifierActionTypes.h"
#include "ModifierActions/Base/ModifierAction.h"
#include "SimpleGameplayAbilitySystem/BlueprintFunctionLibraries/FunctionSelectors/FunctionSelectors.h"
#include "SimpleGameplayAbilitySystem/DefaultTags/DefaultTags.h"
#include "SimpleGameplayAbilitySystem/Module/SimpleGameplayAbilitySystem.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleGameplayAbility/SimpleGameplayAbility.h"
#include "SimpleGameplayAbilitySystem/SimpleEventSubsystem/SimpleEventSubSystem.h"
#include "SimpleGameplayAbilitySystem/SimpleGameplayAbilityComponent/SimpleGameplayAbilityComponent.h"

class USimpleEventSubsystem;

/* SimpleAbilityBase overrides */

void USimpleAttributeModifier::InitializeModifier(USimpleGameplayAbilityComponent* Target, const float ModifierMagnitude)
{
	TargetAbilityComponent = Target;
	Magnitude = ModifierMagnitude;

	for (const TObjectPtr<UModifierAction>& Action : ModifierActions)
	{
		if (Action)
		{
			Action->InitializeAction(this);
		}
	}
}

bool USimpleAttributeModifier::CanActivate(USimpleGameplayAbilityComponent* ActivatingAbilityComponent, const FAbilityContextCollection ActivationContext)
{
	return CanApplyModifierInternal() && CanApplyModifier();
}

bool USimpleAttributeModifier::Activate(USimpleGameplayAbilityComponent* ActivatingAbilityComponent, const FGuid NewAbilityID, const FAbilityContextCollection ActivationContext)
{
	AbilityID = NewAbilityID;
	OwningAbilityComponent = ActivatingAbilityComponent;
	InstigatorAbilityComponent = ActivatingAbilityComponent;
	AbilityContexts = ActivationContext;
	ModifierActionScratchPad = FAttributeModifierActionScratchPad();
	
	if (!CanActivate(ActivatingAbilityComponent, ActivationContext))
	{
		ApplyModifierActions(this, { EAttributeModifierPhase::OnApplicationFailed });
		OnAbilityCancelled.ExecuteIfBound(AbilityID, FDefaultTags::AbilityCancelled(), FInstancedStruct());
		IsActive = false;
		return false;
	}
	
	ActivationTime = OwningAbilityComponent->GetServerTime();
	IsActive = true;
	OnActivationSuccess.ExecuteIfBound(AbilityID);

	// Add permanent gameplay tags from this modifier
	for (const FGameplayTag& Tag : PermanentlyAppliedTags)
	{
		TargetAbilityComponent->AddGameplayTag(Tag, FInstancedStruct());
	}

	OnPreApplyModifierActions();
	ApplyModifierActions(this, { EAttributeModifierPhase::OnApplied });
	ApplyModifierActions(this, {});
	OnPostApplyModifierActions();
	
	/* If we're an instant modifier we apply the action stack immediately and then end. SetDuration modifiers with a
	duration of 0 also apply immediately and end. */
	if (DurationType == EAttributeModifierType::Instant || (DurationType == EAttributeModifierType::SetDuration && Duration <= 0))
	{
		End(FDefaultTags::AbilityEnded(), FInstancedStruct());
		OnAbilityEnded.ExecuteIfBound(AbilityID, FDefaultTags::AbilityEnded(), FInstancedStruct());
		return true;
	}
	
	// Otherwise we're a duration modifier (either SetDuration with a duration > 0 or InfiniteDuration)

	// Add temporary tags from this modifier
	for (const FGameplayTag& Tag : TemporarilyAppliedTags)
	{
		TargetAbilityComponent->AddGameplayTag(Tag, FInstancedStruct());
	}

	// Set the tick timer
	if (TickInterval > 0)
	{
		GetWorld()->GetTimerManager().SetTimer(TickTimerHandle, this, &USimpleAttributeModifier::OnTickTimerTriggered, TickInterval, true);
	}
	
	if (DurationType == EAttributeModifierType::SetDuration && Duration > 0)
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

bool USimpleAttributeModifier::ApplyModifierActions(USimpleAttributeModifier* OwningModifier, const TArray<EAttributeModifierPhase> PhaseFilter)
{
	TArray<FModifierActionResult> ActionSnapshots;

	for (int i = 0; i < ModifierActions.Num(); i++)
	{
		UModifierAction* Action = ModifierActions[i];
		
		if (!CanApplyAction(Action, OwningModifier, PhaseFilter))
		{
			continue;
		}

		FInstancedStruct ActionSnapshotData;
		Action->ApplyAction(ActionSnapshotData);
		
		// Take a snapshot of the action result if applicable
		if (Action->ApplicationPolicy == EAttributeModifierActionPolicy::ApplyClientPredicted ||
			Action->ApplicationPolicy == EAttributeModifierActionPolicy::ApplyServerInitiated)
		{
			ActionSnapshots.Add({i, Action->GetClass(), ActionSnapshotData});
		}
	}
	
	if (ActionSnapshots.Num() > 0)
	{
		// Prepare snapshot data
		FModifierActionStackResultSnapshot ActionStackSnapshot;
		ActionStackSnapshot.ActionsResults = ActionSnapshots;

		// Save the snapshot and register the callback
		FOnSnapshotResolved ResolutionFunction;
		ResolutionFunction.BindDynamic(this, &USimpleAttributeModifier::OnClientReceivedServerActionsResult);
		TakeStateSnapshot(FInstancedStruct::Make(ActionStackSnapshot), ResolutionFunction);
	}
	
	return true;
}

void USimpleAttributeModifier::Cancel(FGameplayTag CancelStatus, FInstancedStruct CancelContext)
{
	InstigatorAbilityComponent->GetWorld()->GetTimerManager().ClearTimer(DurationTimerHandle);
	InstigatorAbilityComponent->GetWorld()->GetTimerManager().ClearTimer(TickTimerHandle);

	ApplyModifierActions(this, { EAttributeModifierPhase::OnCancelled });
	
	if (USimpleEventSubsystem* EventSubsystem = InstigatorAbilityComponent->GetWorld()->GetGameInstance()->GetSubsystem<USimpleEventSubsystem>())
	{
		EventSubsystem->StopListeningForAllEvents(this);
	}

	if (CancelStatus.MatchesTagExact(FDefaultTags::AbilityCancelled()))
	{
		for (const FGameplayTag& Tag : PermanentlyAppliedTags)
		{
			TargetAbilityComponent->RemoveGameplayTag(Tag, FInstancedStruct());
		}
	}
	
	if (DurationType == EAttributeModifierType::SetDuration || DurationType == EAttributeModifierType::InfiniteDuration)
	{
		for (const FGameplayTag& Tag : TemporarilyAppliedTags)
		{
			TargetAbilityComponent->RemoveGameplayTag(Tag, FInstancedStruct());
		}
	}
	
	for (UModifierAction* Action : ModifierActions)
	{
		Action->OnCancelAction();
	}
	
	OnModifierCancelled(CancelStatus, CancelContext);
	IsActive = false;
}

void USimpleAttributeModifier::End(FGameplayTag EndStatus, FInstancedStruct EndContext)
{
	InstigatorAbilityComponent->GetWorld()->GetTimerManager().ClearTimer(DurationTimerHandle);
	InstigatorAbilityComponent->GetWorld()->GetTimerManager().ClearTimer(TickTimerHandle);

	ApplyModifierActions(this, { EAttributeModifierPhase::OnEnded });
	
	if (USimpleEventSubsystem* EventSubsystem = InstigatorAbilityComponent->GetWorld()->GetGameInstance()->GetSubsystem<USimpleEventSubsystem>())
	{
		EventSubsystem->StopListeningForAllEvents(this);
	}

	if (EndStatus.MatchesTagExact(FDefaultTags::AbilityCancelled()))
	{
		for (const FGameplayTag& Tag : PermanentlyAppliedTags)
		{
			TargetAbilityComponent->RemoveGameplayTag(Tag, FInstancedStruct());
		}
	}
	
	if (DurationType == EAttributeModifierType::SetDuration || DurationType == EAttributeModifierType::InfiniteDuration)
	{
		for (const FGameplayTag& Tag : TemporarilyAppliedTags)
		{
			TargetAbilityComponent->RemoveGameplayTag(Tag, FInstancedStruct());
		}
	}

	for (UModifierAction* Action : ModifierActions)
	{
		Action->OnOwningModifierEnded(this);
	}
	
	OnModifierEnded(EndStatus, EndContext);
	IsActive = false;
}

void USimpleAttributeModifier::TakeSnapshotInternal(const FInstancedStruct SnapshotData, const FOnSnapshotResolved& OnResolved)
{
	if (!OwningAbilityComponent)
	{
		SIMPLE_LOG(GetWorld(), FString::Printf(TEXT("[USimpleAttributeModifier::TakeSnapshot]: OwningAbilityComponent is null. Cannot take snapshot.")));
		return;
	}

	const int32 SnapshotCounter = OwningAbilityComponent->AddAttributeModifierSnapshot(AbilityID, SnapshotData);
	PendingSnapshots.Add(SnapshotCounter, OnResolved);
}

/* Internal functions */

bool USimpleAttributeModifier::CanApplyModifierInternal() const
{
	if (!InstigatorAbilityComponent)
	{
		UE_LOG(LogSimpleGAS, Warning, TEXT("[USimpleAttributeModifier::CanApplyModifierInternal]: InstigatorAbilityComponent is null."));
		return false;
	}
	
	if (!TargetAbilityComponent)
	{
		UE_LOG(LogSimpleGAS, Warning, TEXT("[USimpleAttributeModifier::CanApplyModifierInternal]: TargetAbilityComponent is null."));
		return false;
	}
	
	if (!TargetAbilityComponent->HasAllGameplayTags(TargetRequiredTags))
	{
		UE_LOG(LogSimpleGAS, Warning, TEXT("Target does not have required tags in USimpleAttributeModifier::CanApplyModifierInternal"));
		return false;
	}
	
	if (TargetAbilityComponent->HasAnyGameplayTags(TargetBlockingTags))
	{
		UE_LOG(LogSimpleGAS, Warning, TEXT("Target has blocking tags in USimpleAttributeModifier::CanApplyModifierInternal"));
		return false;
	}

	if (TargetAbilityComponent->HasAttributeModifierWithTags(TargetBlockingModifierTags))
	{
		return false;
	}

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
		if (Stacks + StackCount > MaxStacks)
		{
			Stacks = MaxStacks;
			OnMaxStacksReached();
			return;
		}
	}

	Stacks += StackCount;
	OnStacksAdded(StackCount, Stacks);
}

bool USimpleAttributeModifier::CanApplyAction(const UModifierAction* Action, USimpleAttributeModifier* OwningModifier, const TArray<EAttributeModifierPhase>& PhaseFilter) const
{
	// Check network restrictions on applying the action
	if (Action->ApplicationPolicy == EAttributeModifierActionPolicy::ApplyClientOnly && !IsRunningOnClient())
	{
		return false;
	}

	if (Action->ApplicationPolicy == EAttributeModifierActionPolicy::ApplyServerOnly && !IsRunningOnServer())
	{
		return false;
	}

	if (Action->ApplicationPolicy == EAttributeModifierActionPolicy::ApplyServerInitiated && !IsRunningOnServer())
	{
		return false;
	}

	// Return false if phase filters do not match. If both are empty, they match.
	if (Action->ApplicationPhaseFilter.Num() > 0 || PhaseFilter.Num() > 0)
	{
		if (!Action->ApplicationPhaseFilter.ContainsByPredicate([&](const EAttributeModifierPhase Phase) { return PhaseFilter.Contains(Phase); }))
		{
			return false;
		}
	}
		
	if (!Action->ShouldApply(OwningModifier))
	{
		return false;
	}

	return true;
}

void USimpleAttributeModifier::OnClientReceivedServerActionsResult(FInstancedStruct ServerSnapshot, FInstancedStruct ClientSnapshot)
{
	const TArray<FModifierActionResult> ServerResults = ServerSnapshot.Get<FModifierActionStackResultSnapshot>().ActionsResults;
	const TArray<FModifierActionResult> ClientResults = ClientSnapshot.Get<FModifierActionStackResultSnapshot>().ActionsResults;

	TMap<int32, FModifierActionResult> ServerMap, ClientMap;
	for (const FModifierActionResult& ActionResult : ServerResults) ServerMap.Add(ActionResult.ActionIndex, ActionResult);
	for (const FModifierActionResult& ActionResult : ClientResults) ClientMap.Add(ActionResult.ActionIndex, ActionResult);

	TSet<int32> AllIndices;
	ServerMap.GetKeys(AllIndices);
	ClientMap.GetKeys(AllIndices);

	for (int32 Idx : AllIndices)
	{
		UModifierAction* Action = ModifierActions.IsValidIndex(Idx) ? ModifierActions[Idx] : nullptr;
		if (!Action) continue;

		const bool IsInServerMap = ServerMap.Contains(Idx);
		const bool IsInClientMap = ClientMap.Contains(Idx);

		if (IsInServerMap && !IsInClientMap)
		{
			Action->OnServerInitiatedResultReceived(ServerMap[Idx].ActionResult);
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
				ServerMap[Idx].ActionResult,
				ClientMap[Idx].ActionResult
			);
		}
	}
}

void USimpleAttributeModifier::OnDurationTimerExpired()
{
	End(FDefaultTags::AbilityEnded(), FInstancedStruct());
}

void USimpleAttributeModifier::OnTickTimerTriggered()
{
	// Check if the modifier can still be applied
	if (!CanApplyModifierInternal())
	{
		ApplyModifierActions(this, { EAttributeModifierPhase::OnDurationTickFailed });

		switch (TickTagRequirementBehaviour)
		{
			case EDurationTickTagRequirementBehaviour::CancelOnTagRequirementFailed:
				Cancel(FDefaultTags::AbilityCancelled(), FInstancedStruct());
				return;
			case EDurationTickTagRequirementBehaviour::SkipOnTagRequirementFailed:
				return;
		}
	}

	TickCount += 1;
	ApplyModifierActions(this, { EAttributeModifierPhase::OnDurationTick });
	ApplyModifierActions(this, { });
	OnPostApplyModifierActions();
}
