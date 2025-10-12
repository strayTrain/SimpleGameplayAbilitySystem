#include "SimpleAttributeModifier.h"

#include "ModifierActions/ModifierActionTypes.h"
#include "ModifierActions/Base/ModifierAction.h"
#include "SimpleGameplayAbilitySystem/BlueprintFunctionLibraries/FunctionSelectors/FunctionSelectors.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeComponent.h"
#include "SimpleGameplayAbilitySystem/DefaultTags/DefaultTags.h"
#include "SimpleGameplayAbilitySystem/Module/SimpleGameplayAbilitySystem.h"
#include "SimpleGameplayAbilitySystem/SimpleEventSubsystem/SimpleEventSubsystem.h"

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

		switch (Action->PredictionPolicy)
		{
			case EModifierActionPredictionPolicy::PredictIfPossible:
				ShouldRun = Action->SupportsClientPrediction() || IsServer;
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

		if (!ShouldRun && !Action->CanApply())
		{
			return;
		}
		
		const FInstancedStruct ActionResult = Action->ApplyAction();

		if (DoesModifierReplicate && Action->SupportsClientPrediction())
		{
			ActionResults.Add({
				ModifierActions.IndexOfByKey(Action),
				Action->GetClass(),
				ModifierActionScratchPad,
				ActionResult
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
