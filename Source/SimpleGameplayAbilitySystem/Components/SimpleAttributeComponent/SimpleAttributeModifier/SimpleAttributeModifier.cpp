#include "SimpleAttributeModifier.h"

#include "ModifierActions/ModifierActionTypes.h"
#include "ModifierActions/Base/ModifierAction.h"
#include "SimpleGameplayAbilitySystem/BlueprintFunctionLibraries/FunctionSelectors/FunctionSelectors.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeComponent.h"
#include "SimpleGameplayAbilitySystem/DefaultTags/DefaultTags.h"
#include "SimpleGameplayAbilitySystem/Module/SimpleGameplayAbilitySystem.h"

bool USimpleAttributeModifier::ApplyModifier()
{
	if (!WasInitialized)
	{
		UE_LOG(LogSimpleGAS, Warning, TEXT("[USimpleAttributeModifier::ApplyModifier]: Modifier %s was not initialized before applying."), *GetName());
		return false;
	}

	// Check for stack group overflow and reapplication
	if (bUseStackGroup && StackGroupTag.IsValid() && DurationType == EAttributeModifierDurationType::SetDuration)
	{
		if (!HandleStackGroupReapplication())
		{
			return false; // Denied by reapplication logic
		}
	}

	if (!CanApplyModifierInternal())
	{
		CancelModifier(FDefaultTags::AttributeModifierCancelled(), FInstancedStruct());
		return false;
	}
	
	ActivationTime = InstigatorAttributeComponent->GetServerTime();
	IsActive = true;
	
	OnPreApplyModifierActions();

	if (DoesModifierReplicate)
	{
		OnModifierApplied.Broadcast(this);
	}

	// Add permanent gameplay tags from this modifier
	for (const FGameplayTag& Tag : PermanentlyAppliedTags)
	{
		TargetAttributeComponent->AddGameplayTag(Tag);
	}
	
	ModifierActionScratchPad = InitialScratchPadValues;
	ApplyModifierActions(this, FGameplayTagContainer::CreateFromArray(TArray({ FDefaultTags::AttributeModifierApplied() })));
	
	switch (DurationType)
	{
		case EAttributeModifierDurationType::Instant:
			EndModifier(FDefaultTags::AttributeModifierEnded(), FInstancedStruct());
			return true;
		
		case EAttributeModifierDurationType::SetDuration:
			if (Duration <= 0)
			{
				EndModifier(FDefaultTags::AttributeModifierEnded(), FInstancedStruct());
				return true;
			}
		
			// Set the duration timer
			GetWorld()->GetTimerManager().SetTimer(
				DurationTimerHandle,
				this,
				&USimpleAttributeModifier::OnDurationTimerExpired,
				Duration,
				false
			);
		
			// No break in the case of SetDuration because it also shares the same logic as InfiniteDuration
		case EAttributeModifierDurationType::InfiniteDuration:
			for (const FGameplayTag& Tag : TemporarilyAppliedTags)
			{
				TargetAttributeComponent->AddGameplayTag(Tag);
			}

			// Set the tick timer if applicable
			if (TickInterval > 0)
			{
				GetWorld()->GetTimerManager().SetTimer(TickTimerHandle, this, &USimpleAttributeModifier::OnTickTimerTriggered, TickInterval, true);
			}
		
			return true;
	}

	return false;
}

void USimpleAttributeModifier::EndModifier(FGameplayTag EndingStatus, FInstancedStruct EndingContext)
{
	ApplyModifierActions(this, FGameplayTagContainer::CreateFromArray(TArray<FGameplayTag>({ FDefaultTags::AttributeModifierEnded() })));
	
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

	if (DoesModifierReplicate)
	{
		OnAttributeModifierEnded.Broadcast(this, EndingStatus, EndingContext);
	}
}

void USimpleAttributeModifier::CancelModifier(FGameplayTag EndingStatus, FInstancedStruct EndingContext)
{
	ApplyModifierActions(this, FGameplayTagContainer::CreateFromArray(TArray<FGameplayTag>({ EndingStatus })));
	
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

	if (DoesModifierReplicate)
	{
		OnAttributeModifierCancelled.Broadcast(this, EndingStatus, EndingContext);
	}
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

		const bool CanRunOnServer = (Action->ActivationPolicy & static_cast<uint8>(EModifierActionActivationPolicy::RunOnServer)) != 0;
		const bool CanRunOnClient = (Action->ActivationPolicy & static_cast<uint8>(EModifierActionActivationPolicy::RunOnClient)) != 0;
		const bool IsServer = InstigatorAttributeComponent->HasAuthority();
		
		if (IsServer && !CanRunOnServer)
		{
			continue;
		}

		if (!IsServer && !CanRunOnClient)
		{
			continue;
		}
		
		if (!Action->EventTriggers.HasAnyExact(ActionTriggers) || !Action->CanApply())
		{
			continue;
		}

		const FAttributeModifierActionScratchPad InputScratchpad = ModifierActionScratchPad;
		const FInstancedStruct ActionResult = Action->ApplyAction();
		
		ActionResults.Add({
			i,
			Action->GetClass(),
			InputScratchpad,
			ActionResult
		});
	}
	
	if (ActionResults.Num() > 0)
	{
		FModifierActionStackResults ActionStackResult;
		ActionStackResult.ModifierClass = GetClass();
		ActionStackResult.ActionsResults = ActionResults;

		if (DoesModifierReplicate)
		{
			// The attribute component listens for this event to track in AuthorityAttributeModifierMutations and ultimately replicate to clients.
			OnActionStackApplied.Broadcast(this, ActionStackResult);
		}
	}

	OnPostApplyModifierActions();
	
	return true;
}


void USimpleAttributeModifier::OnClientReceivedServerActionsResult(FModifierActionStackResults ServerMutation, FModifierActionStackResults ClientMutation)
{
	// Create maps for server and client action results to compare them easily.
	TMap<int32, FModifierActionResult> ServerMap, ClientMap;
	for (const FModifierActionResult& ActionResult : ServerMutation.ActionsResults) ServerMap.Add(ActionResult.ActionIndex, ActionResult);
	for (const FModifierActionResult& ActionResult : ClientMutation.ActionsResults) ClientMap.Add(ActionResult.ActionIndex, ActionResult);
	
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
			if (ServerMap[Idx].ActionResult == ClientMap[Idx].ActionResult && 
				ServerMap[Idx].InputScratchpad == ClientMap[Idx].InputScratchpad)
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
				ApplyModifierActions(this, FGameplayTagContainer::CreateFromArray(TArray<FGameplayTag>({ FDefaultTags::AttributeModifierTickFailedSkip() })));
				return;
			}
		}
	}

	TickCount += 1;

	if (ResetScratchPadOnTick)
	{
		ModifierActionScratchPad = InitialScratchPadValues;
	}

	ApplyModifierActions(this, FGameplayTagContainer::CreateFromArray(TArray<FGameplayTag>({FDefaultTags::AttributeModifierTicked()})));
}

bool USimpleAttributeModifier::HandleStackGroupReapplication()
{
	TArray<USimpleAttributeModifier*> ExistingInGroup =
		TargetAttributeComponent->GetModifiersInStackGroup(StackGroupTag);

	int32 CurrentStackCount = ExistingInGroup.Num();

	// Handle reapplication behavior first
	if (CurrentStackCount > 0)
	{
		switch (OnReapplication)
		{
			case EDurationModifierReApplicationConfig::ResetDurationTimer:
			{
				// Cancel oldest, allow new application
				USimpleAttributeModifier* Oldest = TargetAttributeComponent->GetOldestModifierInGroup(StackGroupTag);
				if (Oldest)
				{
					Oldest->CancelModifier(FDefaultTags::AttributeModifierCancelled(), FInstancedStruct());
				}
				return true; // Allow new application
			}

			case EDurationModifierReApplicationConfig::ExtendDurationTimer:
			{
				// Extend oldest, deny new
				USimpleAttributeModifier* Oldest = TargetAttributeComponent->GetOldestModifierInGroup(StackGroupTag);
				if (Oldest)
				{
					Oldest->ExtendDuration(Duration);
				}
				return false; // Deny new application
			}

			case EDurationModifierReApplicationConfig::RefreshAll:
			{
				// Reset duration on all instances
				for (USimpleAttributeModifier* Existing : ExistingInGroup)
				{
					if (Existing)
					{
						Existing->SetRemainingDuration(Duration);
					}
				}
				return false; // Deny new application
			}

			case EDurationModifierReApplicationConfig::AllowMultiple:
			default:
				// Continue to overflow check below
				break;
		}
	}

	// Check max stacks (after reapplication, count may have changed)
	CurrentStackCount = TargetAttributeComponent->GetModifierStackCountInGroup(StackGroupTag);

	if (bHasMaxStacksInGroup && CurrentStackCount >= MaxStacksInGroup)
	{
		switch (OverflowBehavior)
		{
			case EStackGroupOverflowBehavior::DenyNew:
				SIMPLE_LOG(this, FString::Printf(TEXT("Max stacks (%d) reached for group %s"), MaxStacksInGroup, *StackGroupTag.ToString()));
				return false;

			case EStackGroupOverflowBehavior::ReplaceOldest:
			{
				USimpleAttributeModifier* Oldest = TargetAttributeComponent->GetOldestModifierInGroup(StackGroupTag);
				if (Oldest)
				{
					Oldest->CancelModifier(FDefaultTags::AttributeModifierCancelled(), FInstancedStruct());
				}
				return true;
			}

			case EStackGroupOverflowBehavior::ReplaceNewest:
			{
				USimpleAttributeModifier* Newest = TargetAttributeComponent->GetNewestModifierInGroup(StackGroupTag);
				if (Newest)
				{
					Newest->CancelModifier(FDefaultTags::AttributeModifierCancelled(), FInstancedStruct());
				}
				return true;
			}

			case EStackGroupOverflowBehavior::ExtendOldest:
			{
				USimpleAttributeModifier* Oldest = TargetAttributeComponent->GetOldestModifierInGroup(StackGroupTag);
				if (Oldest)
				{
					Oldest->ExtendDuration(Duration);
				}
				return false;
			}
		}
	}

	return true; // Allow application
}

float USimpleAttributeModifier::GetRemainingDuration() const
{
	if (DurationType != EAttributeModifierDurationType::SetDuration || !DurationTimerHandle.IsValid())
		return 0.0f;

	return GetWorld()->GetTimerManager().GetTimerRemaining(DurationTimerHandle);
}

void USimpleAttributeModifier::ExtendDuration(float AdditionalSeconds)
{
	if (DurationType != EAttributeModifierDurationType::SetDuration)
		return;

	float CurrentRemaining = GetRemainingDuration();
	SetRemainingDuration(CurrentRemaining + AdditionalSeconds);
}

void USimpleAttributeModifier::SetRemainingDuration(float NewDuration)
{
	if (DurationType != EAttributeModifierDurationType::SetDuration)
		return;

	// Clear existing timer
	if (DurationTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(DurationTimerHandle);
	}

	// Set new timer
	if (NewDuration > 0)
	{
		GetWorld()->GetTimerManager().SetTimer(
			DurationTimerHandle,
			this,
			&USimpleAttributeModifier::OnDurationTimerExpired,
			NewDuration,
			false
		);
	}
	else
	{
		// Duration expired, end immediately
		EndModifier(FDefaultTags::AttributeModifierEnded(), FInstancedStruct());
	}
}
