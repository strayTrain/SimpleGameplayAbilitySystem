#include "SimpleAttributeModifier.h"

#include "ModifierActions/ModifierActionTypes.h"
#include "ModifierActions/Base/ModifierAction.h"
#include "SimpleGameplayAbilitySystem/BlueprintFunctionLibraries/FunctionSelectors/FunctionSelectors.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeComponent.h"
#include "SimpleGameplayAbilitySystem/DefaultTags/DefaultTags.h"
#include "SimpleGameplayAbilitySystem/Module/SimpleGameplayAbilitySystem.h"
#include "SimpleGameplayAbilitySystem/SimpleEventSubsystem/SimpleEventSubsystem.h"

void USimpleAttributeModifier::PostLoad()
{
	Super::PostLoad();

	// Migrate deprecated stacking properties to the new StackingConfig struct
	if (bUseStackGroup && !StackingConfig.bEnableStacking)
	{
		UE_LOG(LogSimpleGAS, Log, TEXT("Migrating deprecated stacking properties to StackingConfig for modifier: %s"), *GetName());

		StackingConfig.bEnableStacking = true;
		StackingConfig.StackGroupTag = StackGroupTag;
		StackingConfig.OnReapplication = OnReapplication;

		if (bHasMaxStacksInGroup)
		{
			StackingConfig.MaxStacks = MaxStacksInGroup;
		}
		else
		{
			StackingConfig.MaxStacks = 0; // 0 means unlimited
		}

		StackingConfig.OverflowBehavior = OverflowBehavior;

		// Clear deprecated properties to avoid re-migration
		bUseStackGroup = false;

		// Mark package as dirty so the migration is saved
		if (!HasAnyFlags(RF_ClassDefaultObject))
		{
			MarkPackageDirty();
		}
	}
}

void USimpleAttributeModifier::InitializeModifier(FGuid NewModifierID, USimpleAttributeComponent* Instigator,
	USimpleAttributeComponent* Target, float Magnitude, const FInstancedStruct Context, const bool DoesReplicate)
{
	ModifierID = NewModifierID;
	InstigatorAttributeComponent = Instigator;
	TargetAttributeComponent = Target;
	ModifierContext = Context;
	ModifierMagnitude = Magnitude;
	WasInitialized = true;
	DoesModifierReplicate = DoesReplicate;

	// Subscribe to SimpleEventSubsystem for all events
	if (USimpleEventSubsystem* EventSubsystem = TargetAttributeComponent->GetWorld()->GetGameInstance()->GetSubsystem<USimpleEventSubsystem>())
	{
		FSimpleEventDelegate EventCallback;
		EventCallback.BindDynamic(this, &USimpleAttributeModifier::OnModifierEventReceived);
		GlobalEventSubscriptionID = EventSubsystem->ListenForEvent(
			this,
			false,
			FGameplayTagContainer(),
			FGameplayTagContainer(),
			EventCallback,
			{},
			{},
			false,
			false);
	}
}

bool USimpleAttributeModifier::ApplyModifier()
{
	if (!WasInitialized)
	{
		UE_LOG(LogSimpleGAS, Warning, TEXT("[USimpleAttributeModifier::ApplyModifier]: Modifier %s was not initialized before applying."), *GetName());
		return false;
	}

	// Check for stack group overflow and reapplication (using new StackingConfig)
	if (UsesConsolidatedStacking() && DurationType == EAttributeModifierDurationType::SetDuration)
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

	OnPreApplyModifier();

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
	
	TriggerActionsForEvents(FGameplayTagContainer::CreateFromArray(TArray({ FDefaultTags::AttributeModifierApplied() })));
	
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
	// Send the Ended event
	TriggerActionsForEvents(FGameplayTagContainer::CreateFromArray(TArray({ FDefaultTags::AttributeModifierEnded() })));

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
	// Send the Cancelled event
	TriggerActionsForEvents(FGameplayTagContainer::CreateFromArray(TArray({ FDefaultTags::AttributeModifierCancelled() })));

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

	if (RequiredContextType)
	{
		if (ModifierContext.IsValid() && ModifierContext.GetScriptStruct() != RequiredContextType)
		{
			return false;
		}
	}
	
	return CanApplyModifier();
}

void USimpleAttributeModifier::OnCleanupModifier_Implementation()
{
	// Unsubscribe from SimpleEventSubsystem
	if (GlobalEventSubscriptionID.IsValid())
	{
		if (USimpleEventSubsystem* EventSubsystem = TargetAttributeComponent->GetWorld()->GetGameInstance()->GetSubsystem<USimpleEventSubsystem>())
		{
			EventSubsystem->StopListeningForEventSubscriptionByID(GlobalEventSubscriptionID);
			GlobalEventSubscriptionID.Invalidate();
		}
	}
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
			Action->InitializeAction(this);
			Action->ApplyAction();
		}
		else if (!IsInServerMap && IsInClientMap)
		{
			Action->OnCancelAction();
		}
		else if (IsInServerMap && IsInClientMap)
		{
			// If the snapshots match we don't need to do anything
			if (ServerMap[Idx] == ClientMap[Idx])
			{
				continue;
			}
			
			Action->OnClientPredictedCorrection(
				ServerMap[Idx].InputScratchpad,
				ServerMap[Idx].OutputScratchpad,
				ClientMap[Idx].InputScratchpad,
				ClientMap[Idx].OutputScratchpad
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
				TriggerActionsForEvents(FGameplayTagContainer::CreateFromArray(TArray({ FDefaultTags::AttributeModifierTickFailedSkip() })));
				return;
			}
		}
	}

	TickCount += 1;

	if (ResetScratchPadOnTick)
	{
		ModifierActionScratchPad = InitialScratchPadValues;
	}

	TriggerActionsForEvents(FGameplayTagContainer::CreateFromArray(TArray({ FDefaultTags::AttributeModifierTicked() })));
}

bool USimpleAttributeModifier::HandleStackGroupReapplication()
{
	// Determine grouping method: tag-based or class-based
	const bool bUseTagBasedGrouping = StackingConfig.StackGroupTag.IsValid();

	TArray<USimpleAttributeModifier*> ExistingInGroup;
	if (bUseTagBasedGrouping)
	{
		ExistingInGroup = TargetAttributeComponent->GetModifiersInStackGroup(StackingConfig.StackGroupTag);
	}
	else
	{
		// Class-based grouping (default)
		ExistingInGroup = TargetAttributeComponent->GetModifiersByClass(GetClass());
	}

	int32 ExistingStackCount = ExistingInGroup.Num();

	// Handle reapplication behavior when stacks already exist
	if (ExistingStackCount > 0)
	{
		switch (StackingConfig.OnReapplication)
		{
			case EStackReapplicationBehavior::ReplaceOldest:
			{
				// Cancel oldest, allow new application
				USimpleAttributeModifier* Oldest = bUseTagBasedGrouping
					? TargetAttributeComponent->GetOldestModifierInGroup(StackingConfig.StackGroupTag)
					: TargetAttributeComponent->GetOldestModifierByClass(GetClass());
				if (Oldest)
				{
					Oldest->CancelModifier(FDefaultTags::AttributeModifierCancelled(), FInstancedStruct());
				}
				return true; // Allow new application
			}

			case EStackReapplicationBehavior::ExtendDuration:
			{
				// Extend oldest, deny new
				USimpleAttributeModifier* Oldest = bUseTagBasedGrouping
					? TargetAttributeComponent->GetOldestModifierInGroup(StackingConfig.StackGroupTag)
					: TargetAttributeComponent->GetOldestModifierByClass(GetClass());
				if (Oldest)
				{
					Oldest->ExtendDuration(Duration);
				}
				return false; // Deny new application
			}
		}
	}

	// Check max stacks (after reapplication, count may have changed)
	ExistingStackCount = bUseTagBasedGrouping
		? TargetAttributeComponent->GetModifierStackCountInGroup(StackingConfig.StackGroupTag)
		: TargetAttributeComponent->GetModifierCountByClass(GetClass());

	// MaxStacks of 0 means unlimited
	if (StackingConfig.MaxStacks > 0 && ExistingStackCount >= StackingConfig.MaxStacks)
	{
		switch (StackingConfig.OverflowBehavior)
		{
			case EStackGroupOverflowBehavior::DenyNew:
			{
				FString GroupName = bUseTagBasedGrouping ? StackingConfig.StackGroupTag.ToString() : GetClass()->GetName();
				SIMPLE_LOG(this, FString::Printf(TEXT("Max stacks (%d) reached for group %s"), StackingConfig.MaxStacks, *GroupName));
				return false;
			}

			case EStackGroupOverflowBehavior::ReplaceOldest:
			{
				USimpleAttributeModifier* Oldest = bUseTagBasedGrouping
					? TargetAttributeComponent->GetOldestModifierInGroup(StackingConfig.StackGroupTag)
					: TargetAttributeComponent->GetOldestModifierByClass(GetClass());
				if (Oldest)
				{
					Oldest->CancelModifier(FDefaultTags::AttributeModifierCancelled(), FInstancedStruct());
				}
				return true;
			}

			case EStackGroupOverflowBehavior::ReplaceNewest:
			{
				USimpleAttributeModifier* Newest = bUseTagBasedGrouping
					? TargetAttributeComponent->GetNewestModifierInGroup(StackingConfig.StackGroupTag)
					: TargetAttributeComponent->GetNewestModifierByClass(GetClass());
				if (Newest)
				{
					Newest->CancelModifier(FDefaultTags::AttributeModifierCancelled(), FInstancedStruct());
				}
				return true;
			}

			case EStackGroupOverflowBehavior::ExtendOldest:
			{
				USimpleAttributeModifier* Oldest = bUseTagBasedGrouping
					? TargetAttributeComponent->GetOldestModifierInGroup(StackingConfig.StackGroupTag)
					: TargetAttributeComponent->GetOldestModifierByClass(GetClass());
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

void USimpleAttributeModifier::OnModifierEventReceived(FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload, UObject* Sender)
{
	TArray<UModifierAction*> ActionsToTrigger;
	for (int i = 0; i < ModifierActions.Num(); i++)
	{
		UModifierAction* Action = ModifierActions[i];
		if (!Action)
		{
			continue;
		}

		// Check if the action is set up to respond to this SimpleEvent
		if (Action->SimpleEventTriggers.GetMemberName() == NAME_None)
		{
			continue;
		}
		
		bool ShouldRespond;
		UFunctionSelectors::ShouldRespondToEvent(
			this,
			Action->SimpleEventTriggers,
			EventTag,
			DomainTag,
			Payload,
			Sender,
			ShouldRespond
		);

		if (!ShouldRespond)
		{
			continue;
		}

		ActionsToTrigger.Add(Action);
	}

	TriggerActions(ActionsToTrigger);
}

void USimpleAttributeModifier::TriggerActionsForEvents(FGameplayTagContainer EventTags)
{
	TArray<UModifierAction*> ActionsToTrigger;
	
	for (int i = 0; i < ModifierActions.Num(); i++)
	{
		UModifierAction* Action = ModifierActions[i];
		if (!Action)
		{
			continue;
		}

		// Check if this action wants to respond to this event tag
		if (!Action->EventTriggers.HasAny(EventTags))
		{
			continue;
		}

		ActionsToTrigger.Add(Action);
	}

	TriggerActions(ActionsToTrigger);
}

void USimpleAttributeModifier::TriggerActions(TArray<UModifierAction*>& Actions)
{
	const bool IsServer = InstigatorAttributeComponent->HasAuthority();

	TArray<FModifierActionResult> ActionResults;
	for (UModifierAction* Action : Actions)
	{
		bool ShouldRun = false;

		// Determine if the action supports prediction in the current context (Instant vs Duration)
		const EActionPredictionMode Mode = Action->GetPredictionMode();
		bool bIsPredictable = false;
		if (Mode == EActionPredictionMode::PredictAll)
		{
			bIsPredictable = true;
		}
		else if (Mode == EActionPredictionMode::PredictInstantOnly && DurationType == EAttributeModifierDurationType::Instant)
		{
			bIsPredictable = true;
		}
		else if (Mode == EActionPredictionMode::PredictDurationOnly && DurationType != EAttributeModifierDurationType::Instant)
		{
			bIsPredictable = true;
		}

		switch (Action->PredictionPolicy)
		{
			case EModifierActionPredictionPolicy::PredictIfPossible:
				ShouldRun = bIsPredictable || IsServer;
				break;
			case EModifierActionPredictionPolicy::ServerInitiate:
			case EModifierActionPredictionPolicy::ServerOnly:
				ShouldRun = IsServer;
				break;
			case EModifierActionPredictionPolicy::ClientOnly:
				ShouldRun = !IsServer;
				break;
		}

		Action->InitializeAction(this);

		if (!ShouldRun || !Action->CanApply())
		{
			continue;
		}

		const FAttributeModifierActionScratchPad InputScratchpad = GetModifierActionScratchPad();
		Action->ApplyAction();
		const FAttributeModifierActionScratchPad OutputScratchPad = GetModifierActionScratchPad();

		if (DoesModifierReplicate && bIsPredictable)
		{
			ActionResults.Add({
				ModifierActions.IndexOfByKey(Action),
				Action->GetClass(),
				InputScratchpad,
				OutputScratchPad
			});
		}
	}

	if (DoesModifierReplicate && ActionResults.Num() > 0)
	{
		FModifierActionStackResults ActionStackResult;
		ActionStackResult.ModifierClass = GetClass();
		ActionStackResult.ActionsResults = ActionResults;

		// The attribute component listens for this event to track in AuthorityAttributeModifierMutations and ultimately replicate to clients.
		OnActionStackApplied.Broadcast(this, ActionStackResult);
	}
}

/* Stacking Implementation */

bool USimpleAttributeModifier::AddStacks(int32 Count)
{
	if (Count <= 0)
	{
		return false;
	}

	if (!StackingConfig.bEnableStacking)
	{
		SIMPLE_LOG(this, FString::Printf(TEXT("AddStacks called on modifier %s which doesn't use consolidated stacking"), *GetName()));
		return false;
	}

	const int32 OldCount = CurrentStackCount;
	int32 NewCount = OldCount + Count;

	// Check max stacks and apply overflow behavior
	if (StackingConfig.MaxStacks > 0 && NewCount > StackingConfig.MaxStacks)
	{
		switch (StackingConfig.OverflowBehavior)
		{
			case EStackGroupOverflowBehavior::DenyNew:
				SIMPLE_LOG(this, FString::Printf(TEXT("AddStacks denied - max stacks (%d) reached for %s"), StackingConfig.MaxStacks, *GetName()));
				return false;

			case EStackGroupOverflowBehavior::ReplaceOldest:
			case EStackGroupOverflowBehavior::ReplaceNewest:
				// For consolidated stacking, these behave the same - cap at max
				NewCount = StackingConfig.MaxStacks;
				break;

			case EStackGroupOverflowBehavior::ExtendOldest:
				// Extend duration instead of adding stacks
				if (DurationType == EAttributeModifierDurationType::SetDuration)
				{
					ExtendDuration(Duration * Count);
				}
				return false;
		}
	}

	CurrentStackCount = NewCount;

	// Handle duration based on mode
	switch (StackingConfig.DurationMode)
	{
		case EStackDurationMode::SharedDuration:
			// Apply reapplication config
			switch (StackingConfig.OnReapplication)
			{
				case EStackReapplicationBehavior::ReplaceOldest:
					if (DurationType == EAttributeModifierDurationType::SetDuration)
					{
						SetRemainingDuration(Duration);
					}
					break;

				case EStackReapplicationBehavior::ExtendDuration:
					if (DurationType == EAttributeModifierDurationType::SetDuration)
					{
						ExtendDuration(Duration);
					}
					break;
			}
			break;

		case EStackDurationMode::AdditiveDuration:
			if (DurationType == EAttributeModifierDurationType::SetDuration && StackingConfig.DurationPerStack > 0)
			{
				ExtendDuration(StackingConfig.DurationPerStack * Count);
			}
			break;

		case EStackDurationMode::IndependentDurations:
			// Add new stack duration entries
			for (int32 i = 0; i < Count; i++)
			{
				FStackDurationEntry Entry;
				Entry.StackIndex = OldCount + i;
				Entry.ExpirationTime = InstigatorAttributeComponent->GetServerTime() + Duration;
				StackDurations.Add(Entry);

				// Set up timer for this stack
				FTimerHandle& TimerHandle = StackDurationTimerHandles.Add(Entry.StackIndex);
				FTimerDelegate TimerDelegate;
				TimerDelegate.BindUObject(this, &USimpleAttributeModifier::HandleStackDurationExpired, Entry.StackIndex);
				GetWorld()->GetTimerManager().SetTimer(TimerHandle, TimerDelegate, Duration, false);
			}
			break;
	}

	// Recalculate magnitude based on new stack count
	RecalculateMagnitude();

	// Check threshold crossings
	CheckThresholdCrossings(OldCount, NewCount);

	// Always inject stack count to scratchpad for stacking modifiers
	InjectStackCountToScratchpad();

	// Fire global OnStackAdded event
	TriggerActionsForEvents(FGameplayTagContainer::CreateFromArray(TArray({ FDefaultTags::AttributeModifierStackAdded() })));

	// Re-run actions if configured
	if (StackingConfig.bRerunActionsOnStackChange)
	{
		TriggerActionsForEvents(FGameplayTagContainer::CreateFromArray(TArray({ FDefaultTags::AttributeModifierStackChanged() })));
	}

	// Broadcast stack count changed event
	OnStackCountChanged.Broadcast(this, CurrentStackCount);

	// Notify component for replication
	if (TargetAttributeComponent)
	{
		TargetAttributeComponent->OnModifierStackCountChanged(this);
	}

	return true;
}

bool USimpleAttributeModifier::RemoveStacks(int32 Count)
{
	if (Count <= 0)
	{
		return false;
	}

	if (!StackingConfig.bEnableStacking)
	{
		SIMPLE_LOG(this, FString::Printf(TEXT("RemoveStacks called on modifier %s which doesn't use consolidated stacking"), *GetName()));
		return false;
	}

	const int32 OldCount = CurrentStackCount;
	const int32 NewCount = FMath::Max(0, OldCount - Count);

	if (NewCount == 0)
	{
		// No stacks left, end the modifier
		CurrentStackCount = 0;
		CheckThresholdCrossings(OldCount, 0);
		EndModifier(FDefaultTags::AttributeModifierEnded(), FInstancedStruct());
		return true;
	}

	CurrentStackCount = NewCount;

	// Handle duration cleanup for IndependentDurations mode
	if (StackingConfig.DurationMode == EStackDurationMode::IndependentDurations)
	{
		// Sort by expiration time (oldest first) and remove the oldest entries
		StackDurations.Sort([](const FStackDurationEntry& A, const FStackDurationEntry& B)
		{
			return A.ExpirationTime < B.ExpirationTime;
		});

		for (int32 i = 0; i < Count && StackDurations.Num() > 0; i++)
		{
			const int32 StackIndex = StackDurations[0].StackIndex;

			// Clear the timer for this stack
			if (FTimerHandle* TimerHandle = StackDurationTimerHandles.Find(StackIndex))
			{
				GetWorld()->GetTimerManager().ClearTimer(*TimerHandle);
				StackDurationTimerHandles.Remove(StackIndex);
			}

			StackDurations.RemoveAt(0);
		}
	}

	// Recalculate magnitude
	RecalculateMagnitude();

	// Check threshold crossings
	CheckThresholdCrossings(OldCount, NewCount);

	// Always inject stack count to scratchpad for stacking modifiers
	InjectStackCountToScratchpad();

	// Fire global OnStackRemoved event
	TriggerActionsForEvents(FGameplayTagContainer::CreateFromArray(TArray({ FDefaultTags::AttributeModifierStackRemoved() })));

	// Re-run actions if configured
	if (StackingConfig.bRerunActionsOnStackChange)
	{
		TriggerActionsForEvents(FGameplayTagContainer::CreateFromArray(TArray({ FDefaultTags::AttributeModifierStackChanged() })));
	}

	// Broadcast stack count changed event
	OnStackCountChanged.Broadcast(this, CurrentStackCount);

	// Notify component for replication
	if (TargetAttributeComponent)
	{
		TargetAttributeComponent->OnModifierStackCountChanged(this);
	}

	return true;
}

void USimpleAttributeModifier::RecalculateMagnitude()
{
	if (!StackingConfig.bEnableStacking)
	{
		ScaledMagnitude = ModifierMagnitude;
		return;
	}

	switch (StackingConfig.MagnitudeScalingSource)
	{
		case EMagnitudeScalingSource::Linear:
			ScaledMagnitude = ModifierMagnitude * CurrentStackCount;
			break;

		case EMagnitudeScalingSource::CurveAsset:
			if (StackingConfig.MagnitudeScalingCurve)
			{
				const float Multiplier = StackingConfig.MagnitudeScalingCurve->GetFloatValue(static_cast<float>(CurrentStackCount));
				ScaledMagnitude = ModifierMagnitude * Multiplier;
			}
			else
			{
				// Fallback to linear if curve not set
				ScaledMagnitude = ModifierMagnitude * CurrentStackCount;
			}
			break;

		case EMagnitudeScalingSource::FunctionCallback:
			if (MagnitudeScalingFunction.GetMemberName() != NAME_None)
			{
				// Call the custom function via FunctionSelectors
				UFunctionSelectors::CalculateStackMagnitude(
					this,
					MagnitudeScalingFunction,
					CurrentStackCount,
					ModifierMagnitude,
					ScaledMagnitude
				);
			}
			else
			{
				// Fallback to linear if function not set
				ScaledMagnitude = ModifierMagnitude * CurrentStackCount;
			}
			break;
	}
}

void USimpleAttributeModifier::CheckThresholdCrossings(int32 OldCount, int32 NewCount)
{
	if (!StackingConfig.bEnableStacking || !TargetAttributeComponent)
	{
		return;
	}

	for (const FStackThresholdTrigger& Threshold : StackingConfig.ThresholdTriggers)
	{
		const bool WasBelowThreshold = OldCount < Threshold.ThresholdCount;
		const bool IsNowAtOrAboveThreshold = NewCount >= Threshold.ThresholdCount;
		const bool WasAtOrAboveThreshold = OldCount >= Threshold.ThresholdCount;
		const bool IsNowBelowThreshold = NewCount < Threshold.ThresholdCount;

		// Crossed upward
		if (WasBelowThreshold && IsNowAtOrAboveThreshold)
		{
			// Fire threshold reached event
			if (Threshold.OnThresholdReachedTag.IsValid())
			{
				TriggerActionsForEvents(FGameplayTagContainer::CreateFromArray(TArray({ Threshold.OnThresholdReachedTag })));
			}

			// Add threshold granted tags
			for (const FGameplayTag& Tag : Threshold.ThresholdGrantedTags)
			{
				TargetAttributeComponent->AddGameplayTag(Tag);
			}
		}
		// Crossed downward
		else if (WasAtOrAboveThreshold && IsNowBelowThreshold)
		{
			// Fire threshold lost event
			if (Threshold.OnThresholdLostTag.IsValid())
			{
				TriggerActionsForEvents(FGameplayTagContainer::CreateFromArray(TArray({ Threshold.OnThresholdLostTag })));
			}

			// Remove threshold granted tags
			for (const FGameplayTag& Tag : Threshold.ThresholdGrantedTags)
			{
				TargetAttributeComponent->RemoveGameplayTag(Tag);
			}
		}
	}
}

void USimpleAttributeModifier::InjectStackCountToScratchpad()
{
	// Find or add the stack count entry
	bool bFoundStackCount = false;
	for (FAttributeModifierActionScratchPadValue& Entry : ModifierActionScratchPad.ScratchpadValues)
	{
		if (Entry.ScratchpadTag == FDefaultTags::ScratchPadStackCount())
		{
			Entry.ScratchpadValue = static_cast<float>(CurrentStackCount);
			bFoundStackCount = true;
			break;
		}
	}
	if (!bFoundStackCount)
	{
		FAttributeModifierActionScratchPadValue StackCountEntry;
		StackCountEntry.ScratchpadTag = FDefaultTags::ScratchPadStackCount();
		StackCountEntry.ScratchpadValue = static_cast<float>(CurrentStackCount);
		ModifierActionScratchPad.ScratchpadValues.Add(StackCountEntry);
	}

	// Find or add the scaled magnitude entry
	bool bFoundScaledMagnitude = false;
	for (FAttributeModifierActionScratchPadValue& Entry : ModifierActionScratchPad.ScratchpadValues)
	{
		if (Entry.ScratchpadTag == FDefaultTags::ScratchPadScaledMagnitude())
		{
			Entry.ScratchpadValue = ScaledMagnitude;
			bFoundScaledMagnitude = true;
			break;
		}
	}
	if (!bFoundScaledMagnitude)
	{
		FAttributeModifierActionScratchPadValue ScaledMagnitudeEntry;
		ScaledMagnitudeEntry.ScratchpadTag = FDefaultTags::ScratchPadScaledMagnitude();
		ScaledMagnitudeEntry.ScratchpadValue = ScaledMagnitude;
		ModifierActionScratchPad.ScratchpadValues.Add(ScaledMagnitudeEntry);
	}
}

void USimpleAttributeModifier::HandleStackDurationExpired(int32 StackIndex)
{
	// Remove the expired stack
	StackDurations.RemoveAll([StackIndex](const FStackDurationEntry& Entry)
	{
		return Entry.StackIndex == StackIndex;
	});

	// Clean up timer handle
	StackDurationTimerHandles.Remove(StackIndex);

	// Decrement stack count
	const int32 OldCount = CurrentStackCount;
	CurrentStackCount = FMath::Max(0, CurrentStackCount - 1);

	if (CurrentStackCount == 0)
	{
		// No stacks left, end the modifier
		CheckThresholdCrossings(OldCount, 0);
		EndModifier(FDefaultTags::AttributeModifierEnded(), FInstancedStruct());
		return;
	}

	// Recalculate magnitude
	RecalculateMagnitude();

	// Check threshold crossings
	CheckThresholdCrossings(OldCount, CurrentStackCount);

	// Always inject stack count to scratchpad for stacking modifiers
	InjectStackCountToScratchpad();

	// Fire global OnStackRemoved event
	TriggerActionsForEvents(FGameplayTagContainer::CreateFromArray(TArray({ FDefaultTags::AttributeModifierStackRemoved() })));

	// Re-run actions if configured
	if (StackingConfig.bRerunActionsOnStackChange)
	{
		TriggerActionsForEvents(FGameplayTagContainer::CreateFromArray(TArray({ FDefaultTags::AttributeModifierStackChanged() })));
	}

	// Broadcast stack count changed event
	OnStackCountChanged.Broadcast(this, CurrentStackCount);

	// Notify component for replication
	if (TargetAttributeComponent)
	{
		TargetAttributeComponent->OnModifierStackCountChanged(this);
	}
}

void USimpleAttributeModifier::UpdateStackDurationTimers()
{
	// This is called to sync timers after prediction correction
	// Clear all existing timers
	for (auto& Pair : StackDurationTimerHandles)
	{
		GetWorld()->GetTimerManager().ClearTimer(Pair.Value);
	}
	StackDurationTimerHandles.Empty();

	// Set up new timers based on current StackDurations
	const float CurrentTime = InstigatorAttributeComponent ? InstigatorAttributeComponent->GetServerTime() : 0.0f;

	for (const FStackDurationEntry& Entry : StackDurations)
	{
		const float RemainingTime = Entry.ExpirationTime - CurrentTime;
		if (RemainingTime > 0)
		{
			FTimerHandle& TimerHandle = StackDurationTimerHandles.Add(Entry.StackIndex);
			FTimerDelegate TimerDelegate;
			TimerDelegate.BindUObject(this, &USimpleAttributeModifier::HandleStackDurationExpired, Entry.StackIndex);
			GetWorld()->GetTimerManager().SetTimer(TimerHandle, TimerDelegate, RemainingTime, false);
		}
		else
		{
			// Timer already expired, handle immediately
			HandleStackDurationExpired(Entry.StackIndex);
		}
	}
}
