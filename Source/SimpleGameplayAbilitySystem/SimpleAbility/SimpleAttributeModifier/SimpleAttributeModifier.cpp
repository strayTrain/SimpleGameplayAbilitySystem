#include "SimpleAttributeModifier.h"

#include "SimpleGameplayAbilitySystem/BlueprintFunctionLibraries/SimpleAttributes/SimpleAttributeFunctionLibrary.h"
#include "SimpleGameplayAbilitySystem/DefaultTags/DefaultTags.h"
#include "SimpleGameplayAbilitySystem/Module/SimpleGameplayAbilitySystem.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleGameplayAbility/SimpleGameplayAbility.h"
#include "SimpleGameplayAbilitySystem/SimpleEventSubsystem/SimpleEventSubSystem.h"
#include "SimpleGameplayAbilitySystem/SimpleGameplayAbilityComponent/SimpleGameplayAbilityComponent.h"

class USimpleEventSubsystem;

bool USimpleAttributeModifier::CanApplyModifierInternal(FInstancedStruct ModifierContext) const
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
	
	if (!TargetAbilityComponent->GameplayTags.HasAllExact(TargetRequiredTags))
	{
		UE_LOG(LogSimpleGAS, Warning, TEXT("Target does not have required tags in USimpleAttributeModifier::CanApplyModifierInternal"));
		return false;
	}
	
	if (TargetAbilityComponent->GameplayTags.HasAnyExact(TargetBlockingTags))
	{
		UE_LOG(LogSimpleGAS, Warning, TEXT("Target has blocking tags in USimpleAttributeModifier::CanApplyModifierInternal"));
		return false;
	}

	// if (USimpleAttributeFunctionLibrary::HasModifierWithTags(TargetAbilityComponent, TargetBlockingModifierTags))
	// {
	// 	UE_LOG(LogSimpleGAS, Warning, TEXT("Target has blocking modifier tags in USimpleAttributeModifier::CanApplyModifierInternal"));
	// 	return false;
	// }

	return true;
}

bool USimpleAttributeModifier::CanApplyModifier_Implementation(FInstancedStruct ModifierContext) const
{
	return true;
}

bool USimpleAttributeModifier::ApplyModifier(USimpleGameplayAbilityComponent* Instigator, USimpleGameplayAbilityComponent* Target, FInstancedStruct ModifierContext)
{
	if (!OwningAbilityComponent->HasAuthority())
	{
		if (ModifierApplicationPolicy == EAttributeModifierApplicationPolicy::ApplyServerOnly || ModifierApplicationPolicy == EAttributeModifierApplicationPolicy::ApplyServerOnlyButReplicateSideEffects)
		{
			return false;
		}	
	}
	
	InstigatorAbilityComponent = Instigator;
	TargetAbilityComponent = Target;
	InitialModifierContext = ModifierContext;
	
	// Check if we can apply the modifier
	if (!CanApplyModifierInternal(ModifierContext) || !CanApplyModifier(ModifierContext))
	{
		EndModifier(FDefaultTags::AbilityCancelled, FInstancedStruct());
		
		return false;
	}
	
	bIsModifierActive = true;
	
	for (const FGameplayTag& Tag : PermanentlyAppliedTags)
	{
		TargetAbilityComponent->AddGameplayTag(Tag, FInstancedStruct());
	}

	for (const FGameplayTag& Tag : RemoveGameplayTags)
	{
		TargetAbilityComponent->RemoveGameplayTag(Tag, FInstancedStruct());
	}

	OnPreApplyModifier();

	// Set up for duration type modifiers
	if (ModifierType == EAttributeModifierType::Duration)
	{
		// Listen for tag changes on the target ability component
		if (USimpleEventSubsystem* EventSubsystem = Instigator->GetWorld()->GetGameInstance()->GetSubsystem<USimpleEventSubsystem>())
		{
			FGameplayTagContainer EventTags;
			EventTags.AddTag(FDefaultTags::GameplayTagAdded);
			EventTags.AddTag(FDefaultTags::GameplayTagRemoved);
    
			FSimpleEventDelegate EventDelegate;
			EventDelegate.BindDynamic(this, &USimpleAttributeModifier::OnTagsChanged);
		
			TArray<AActor*> SenderFilter;
			SenderFilter.Add(Target->GetAvatarActor());
    
			EventSubsystem->ListenForEvent(this, false, EventTags, FGameplayTagContainer(), EventDelegate, TArray<UScriptStruct*>(), SenderFilter);
		}
		
		for (const FGameplayTag& Tag : TemporarilyAppliedTags)
		{
			TargetAbilityComponent->AddGameplayTag(Tag, FInstancedStruct());
		}

		// If the whole modifier has a duration we set a timer to end it
		if (Duration > 0 && !HasInfiniteDuration)
		{
			Instigator->GetWorld()->GetTimerManager().SetTimer(
				DurationTimerHandle,
				[this]()
				{
					EndModifier(FDefaultTags::AbilityEndedSuccessfully, FInstancedStruct());
				},
				Duration,
				false
			);	
		}

		if (TickInterval > 0)
		{
			Instigator->GetWorld()->GetTimerManager().SetTimer(
				TickTimerHandle,
				[this]()
				{
					if (!CanApplyModifierInternal(InitialModifierContext))
					{
						switch (TickTagRequirementBehaviour)
						{
							case EDurationTickTagRequirementBehaviour::CancelOnTagRequirementFailed:
								ApplySideEffects(InstigatorAbilityComponent, TargetAbilityComponent, EAttributeModifierSideEffectTrigger::OnDurationModifierTickCancel);
								EndModifier(FDefaultTags::AbilityCancelled, FInstancedStruct());
								return;
							case EDurationTickTagRequirementBehaviour::SkipOnTagRequirementFailed:
								UE_LOG(LogSimpleGAS, Warning, TEXT("[USimpleAttributeModifier::ApplyModifier]: tag requirement failed on tick. Skipping modification."));
								return;
							case EDurationTickTagRequirementBehaviour::PauseOnTagRequirementFailed:
								InstigatorAbilityComponent->GetWorld()->GetTimerManager().PauseTimer(DurationTimerHandle);
								InstigatorAbilityComponent->GetWorld()->GetTimerManager().PauseTimer(TickTimerHandle);
								return;
						}
					}

					InstigatorAbilityComponent->GetWorld()->GetTimerManager().UnPauseTimer(DurationTimerHandle);
					InstigatorAbilityComponent->GetWorld()->GetTimerManager().UnPauseTimer(TickTimerHandle);
					
					ApplyModifiersInternal(EAttributeModifierSideEffectTrigger::OnDurationModifierTickSuccess);
					ApplySideEffects(InstigatorAbilityComponent, TargetAbilityComponent, EAttributeModifierSideEffectTrigger::OnDurationModifierTickSuccess);
				},
				TickInterval,
				true
			);
		}

		if (TickOnApply)
		{
			ApplyModifiersInternal(EAttributeModifierSideEffectTrigger::OnDurationModifierInitiallyAppliedSuccess);
		}
		
		ApplySideEffects(InstigatorAbilityComponent, TargetAbilityComponent, EAttributeModifierSideEffectTrigger::OnDurationModifierInitiallyAppliedSuccess);
		return true;
	}
	
	// Instant type modifier
	
	if (ApplyModifiersInternal(EAttributeModifierSideEffectTrigger::OnInstantModifierEndedSuccess))
	{
		OnPostApplyModifier();
		EndModifier(FDefaultTags::AbilityEndedSuccessfully, FInstancedStruct());
		return true;
	}

	EndModifier(FDefaultTags::AbilityCancelled, FInstancedStruct());
	return false;
}

bool USimpleAttributeModifier::ApplyModifiersInternal(const EAttributeModifierSideEffectTrigger TriggerPhase)
{
	// We process the modifier stack as a transaction to avoid partial changes of attributes
	TArray<FFloatAttribute> TempFloatAttributes = TargetAbilityComponent->AuthorityFloatAttributes.Attributes;
	TArray<FStructAttribute> TempStructAttributes = TargetAbilityComponent->AuthorityStructAttributes.Attributes;

	TArray<FGameplayTag> ModifiedFloatAttributes;
	TArray<FGameplayTag> ModifiedStructAttributes;

	float CurrentFloatModifierOverflow = 0;
	
	for (const FAttributeModifier& Modifier : AttributeModifications )
	{
		if (ModifierType == EAttributeModifierType::Duration && !Modifier.ApplicationTriggers.Contains(TriggerPhase))
		{
			continue;
		}
		
		bool WasModifierApplied = false;
		
		switch (Modifier.AttributeType)
		{
			case EAttributeType::FloatAttribute:
				WasModifierApplied = ApplyFloatAttributeModifier(Modifier, TempFloatAttributes, CurrentFloatModifierOverflow);
				if (WasModifierApplied)
				{
					ModifiedFloatAttributes.Add(Modifier.ModifiedAttribute);
				}
				break;
			case EAttributeType::StructAttribute:
				WasModifierApplied = ApplyStructAttributeModifier(Modifier, TempStructAttributes);
				if (WasModifierApplied)
				{
					ModifiedStructAttributes.Add(Modifier.ModifiedAttribute);
				}
				break;
		}
		
		if (!WasModifierApplied && Modifier.CancelIfAttributeNotFound)
		{
			return false;
		}
	}

	// If all the modifiers were applied successfully, we update the target ability component's attributes
	if (InstigatorAbilityComponent->HasAuthority())
	{
		for (const FFloatAttribute& Attribute : TempFloatAttributes)
		{
			if (ModifiedFloatAttributes.Contains(Attribute.AttributeTag))
			{
				USimpleAttributeFunctionLibrary::OverrideFloatAttribute(TargetAbilityComponent, Attribute.AttributeTag, Attribute);
			}
		}

		for (const FStructAttribute& Attribute : TempStructAttributes)
		{
			if (ModifiedStructAttributes.Contains(Attribute.AttributeTag))
			{
				TargetAbilityComponent->AddStructAttribute(Attribute);
			}
		}
	}
	
	return true;
}

void USimpleAttributeModifier::EndModifier(const FGameplayTag EndingStatus, const FInstancedStruct EndingContext)
{
	InstigatorAbilityComponent->GetWorld()->GetTimerManager().ClearTimer(DurationTimerHandle);
	InstigatorAbilityComponent->GetWorld()->GetTimerManager().ClearTimer(TickTimerHandle);

	if (USimpleEventSubsystem* EventSubsystem = InstigatorAbilityComponent->GetWorld()->GetGameInstance()->GetSubsystem<USimpleEventSubsystem>())
	{
		EventSubsystem->StopListeningForAllEvents(this);
	}

	if (EndingStatus == FDefaultTags::AbilityCancelled)
	{
		for (const FGameplayTag& Tag : PermanentlyAppliedTags)
		{
			TargetAbilityComponent->RemoveGameplayTag(Tag, FInstancedStruct());
		}
	}

	if (ModifierType == EAttributeModifierType::Instant)
	{
		if (EndingStatus == FDefaultTags::AbilityCancelled)
		{
			ApplySideEffects(InstigatorAbilityComponent, TargetAbilityComponent, EAttributeModifierSideEffectTrigger::OnInstantModifierEndedCancel);
		}

		if (EndingStatus == FDefaultTags::AbilityEndedSuccessfully)
		{
			ApplySideEffects(InstigatorAbilityComponent, TargetAbilityComponent, EAttributeModifierSideEffectTrigger::OnInstantModifierEndedSuccess);
		}
	}
	
	if (ModifierType == EAttributeModifierType::Duration)
	{
		for (const FGameplayTag& Tag : TemporarilyAppliedTags)
		{
			TargetAbilityComponent->RemoveGameplayTag(Tag, FInstancedStruct());
		}

		if (EndingStatus == FDefaultTags::AbilityCancelled)
		{
			ApplySideEffects(InstigatorAbilityComponent, TargetAbilityComponent, EAttributeModifierSideEffectTrigger::OnDurationModifierEndedCancel);
		}

		if (EndingStatus == FDefaultTags::AbilityEndedSuccessfully)
		{
			ApplyModifiersInternal(EAttributeModifierSideEffectTrigger::OnDurationModifierEndedSuccess);
			ApplySideEffects(InstigatorAbilityComponent, TargetAbilityComponent, EAttributeModifierSideEffectTrigger::OnDurationModifierEndedSuccess);
		}
	}
	
	OnModifierEnded(EndingStatus, EndingContext);
	bIsModifierActive = false;
}

void USimpleAttributeModifier::AddModifierStack(int32 StackCount)
{
	if (!CanStack)
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

bool USimpleAttributeModifier::ApplyFloatAttributeModifier(const FAttributeModifier& Modifier, TArray<FFloatAttribute>& TempFloatAttributes, float& CurrentOverflow) const
{
	FFloatAttribute* AttributeToModify = GetTempFloatAttribute(Modifier.ModifiedAttribute, TempFloatAttributes);
	
	if (!AttributeToModify)
	{
		SIMPLE_LOG(OwningAbilityComponent, FString::Printf(TEXT("[USimpleAttributeModifier::ApplyFloatAttributeModifier]: Attribute %s not found."), *Modifier.ModifiedAttribute.ToString()));
		return false;
	}

	// Get the input value for the modification
	float ModificationInputValue = 0;
	bool WasInstigatorAttributeFound = false;
	bool WasTargetAttributeFound = false;
	bool WasMetaAttributeHandled = false;
	switch (Modifier.ModificationInputValueSource)
	{
		case EAttributeModificationValueSource::Manual:
			ModificationInputValue = Modifier.ManualInputValue;
			break;
		
		case EAttributeModificationValueSource::FromOverflow:
			ModificationInputValue = CurrentOverflow;

			if (Modifier.ConsumeOverflow)
			{
				CurrentOverflow = 0;
			}
		
			break;
		
		case EAttributeModificationValueSource::FromInstigatorAttribute:
			if (!InstigatorAbilityComponent)
			{
				UE_LOG(LogSimpleGAS, Warning, TEXT("USimpleAttributeModifier::ApplyFloatAttributeModifier: Instigator ability component is nullptr."));
				return false;
			}

			ModificationInputValue = USimpleAttributeFunctionLibrary::GetFloatAttributeValue(InstigatorAbilityComponent, Modifier.SourceAttributeValueType, Modifier.SourceAttribute, WasTargetAttributeFound);

			if (!WasInstigatorAttributeFound)
			{
				UE_LOG(LogSimpleGAS, Warning, TEXT("USimpleAttributeModifier::ApplyFloatAttributeModifier: Source attribute %s not found on instigator ability component."), *Modifier.SourceAttribute.ToString());
				return false;
			}

			break;
		
		case EAttributeModificationValueSource::FromTargetAttribute:
			if (!TargetAbilityComponent)
			{
				UE_LOG(LogSimpleGAS, Warning, TEXT("USimpleAttributeModifier::ApplyFloatAttributeModifier: Target ability component is nullptr."));
				return false;
			}

			ModificationInputValue = USimpleAttributeFunctionLibrary::GetFloatAttributeValue(TargetAbilityComponent, Modifier.SourceAttributeValueType, Modifier.SourceAttribute, WasTargetAttributeFound);

			if (!WasTargetAttributeFound)
			{
				UE_LOG(LogSimpleGAS, Warning, TEXT("USimpleAttributeModifier::ApplyFloatAttributeModifier: Source attribute %s not found on target ability component."), *Modifier.SourceAttribute.ToString());
				return false;
			}
			
			break;
		
		case EAttributeModificationValueSource::FromMetaAttribute:
			GetFloatMetaAttributeValue(Modifier.MetaAttributeTag, ModificationInputValue, WasMetaAttributeHandled);

			if (!WasMetaAttributeHandled)
			{
				UE_LOG(LogSimpleGAS, Warning, TEXT("USimpleAttributeModifier::ApplyFloatAttributeModifier: Meta attribute %s not handled."), *Modifier.MetaAttributeTag.ToString());
				return false;
			}
		
			break;
	}

	// Finally, modify AttributeToModify based on the input value and the modifier's operation
	switch (Modifier.ModifiedAttributeValueType)
	{
		case EAttributeValueType::BaseValue:
			switch (Modifier.ModificationOperation)
			{
				case EFloatAttributeModificationOperation::Add:
					AttributeToModify->BaseValue = USimpleAttributeFunctionLibrary::ClampFloatAttributeValue(*AttributeToModify, EAttributeValueType::BaseValue, AttributeToModify->BaseValue + ModificationInputValue, CurrentOverflow);
					break;
				
				case EFloatAttributeModificationOperation::Multiply:
					AttributeToModify->BaseValue = USimpleAttributeFunctionLibrary::ClampFloatAttributeValue(*AttributeToModify, EAttributeValueType::BaseValue, AttributeToModify->BaseValue * ModificationInputValue, CurrentOverflow);
					break;
				
				case EFloatAttributeModificationOperation::Override:
					AttributeToModify->BaseValue = USimpleAttributeFunctionLibrary::ClampFloatAttributeValue(*AttributeToModify, EAttributeValueType::BaseValue, ModificationInputValue, CurrentOverflow);
					break;
			}
			break;
		
		case EAttributeValueType::CurrentValue:
			switch (Modifier.ModificationOperation)
			{
				case EFloatAttributeModificationOperation::Add:
					AttributeToModify->CurrentValue = USimpleAttributeFunctionLibrary::ClampFloatAttributeValue(*AttributeToModify, EAttributeValueType::CurrentValue, AttributeToModify->CurrentValue + ModificationInputValue, CurrentOverflow);
					break;
				
				case EFloatAttributeModificationOperation::Multiply:
					AttributeToModify->CurrentValue = USimpleAttributeFunctionLibrary::ClampFloatAttributeValue(*AttributeToModify, EAttributeValueType::CurrentValue, AttributeToModify->CurrentValue * ModificationInputValue, CurrentOverflow);
					break;
				
				case EFloatAttributeModificationOperation::Override:
					AttributeToModify->CurrentValue = USimpleAttributeFunctionLibrary::ClampFloatAttributeValue(*AttributeToModify, EAttributeValueType::CurrentValue, ModificationInputValue, CurrentOverflow);
					break;
			}
			break;
		
		case EAttributeValueType::MaxBaseValue:
			AttributeToModify->ValueLimits.MaxBaseValue = ModificationInputValue;
			break;
		
		case EAttributeValueType::MinBaseValue:
			AttributeToModify->ValueLimits.MinBaseValue = ModificationInputValue;
			break;
		
		case EAttributeValueType::MaxCurrentValue:
			AttributeToModify->ValueLimits.MaxCurrentValue = ModificationInputValue;
			break;
		
		case EAttributeValueType::MinCurrentValue:
			AttributeToModify->ValueLimits.MinCurrentValue = ModificationInputValue;
			break;
	}
	
	return true;
}

bool USimpleAttributeModifier::ApplyStructAttributeModifier(const FAttributeModifier& Modifier, TArray<FStructAttribute>& TempStructAttributes) const
{
	FStructAttribute* AttributeToModify = GetTempStructAttribute(Modifier.ModifiedAttribute, TempStructAttributes);
	
	if (!AttributeToModify)
	{
		UE_LOG(LogSimpleGAS, Warning, TEXT("USimpleAttributeModifier::ApplyStructAttributeModifier: Attribute %s not found on target ability component."), *Modifier.ModifiedAttribute.ToString());
		return false;
	}

	bool WasHandled = false;
	const FInstancedStruct NewValue = GetModifiedStructAttributeValue(Modifier.StructOperationTag, AttributeToModify->AttributeValue, WasHandled);

	if (!WasHandled)
	{
		UE_LOG(LogSimpleGAS, Warning, TEXT("USimpleAttributeModifier::ApplyStructAttributeModifier: Struct modifier %s not handled."), *Modifier.StructOperationTag.ToString());
		return false;
	}

	AttributeToModify->AttributeValue = NewValue;
	return true;
}

void USimpleAttributeModifier::ApplySideEffects(USimpleGameplayAbilityComponent* Instigator, USimpleGameplayAbilityComponent* Target, EAttributeModifierSideEffectTrigger EffectPhase)
{
	if (!OwningAbilityComponent->HasAuthority())
	{
		if (ModifierApplicationPolicy == EAttributeModifierApplicationPolicy::ApplyServerOnly || ModifierApplicationPolicy == EAttributeModifierApplicationPolicy::ApplyServerOnlyButReplicateSideEffects)
		{
			SIMPLE_LOG(this, TEXT("[USimpleAttributeModifier::ApplySideEffects]: ModifierApplicationPolicy is ApplyServerOnly but the ability component does not have authority. Can't locally apply side effects."));
			return;
		}	
	}

	FAttributeModifierResult ModifierResult;
	ModifierResult.Instigator = Instigator;
	ModifierResult.Target = Target;
	
	// Ability side effects
	for (FAbilitySideEffect& AbilitySideEffect : AbilitySideEffects)
	{
		if (AbilitySideEffect.ApplicationTriggers.Contains(EffectPhase))
		{
			// if (ModifierApplicationPolicy != EAttributeModifierApplicationPolicy::ApplyClientPredicted)
			// {
			// 	if (AbilitySideEffect.ActivationPolicy != EAbilityActivationPolicy::ClientPredicted)
			// 	{
			// 		SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleAttributeModifier::ApplySideEffects]: Ability side effect %s has ActivationPolicy ApplyClientPredicted but ModifierApplicationPolicy is not ApplyClientPredicted."), *AbilitySideEffect.AbilityClass->GetName()));
			// 		continue;
			// 	}
			// }
			
			USimpleGameplayAbilityComponent* ActivatingAbilityComponent = AbilitySideEffect.ActivatingAbilityComponent == EAttributeModifierSideEffectTarget::Instigator ? Instigator : Target;

			FInstancedStruct Payload;

			if (AbilitySideEffect.AbilityContextTag.IsValid())
			{
				bool IsPayloadValid = true;
				Payload = GetAbilitySideEffectContext(AbilitySideEffect.AbilityContextTag, IsPayloadValid);
			}

			AbilitySideEffect.AbilityContext = Payload;
			ModifierResult.AppliedAbilitySideEffects.Add(AbilitySideEffect);

			ActivatingAbilityComponent->ActivateAbility(AbilitySideEffect.AbilityClass, Payload, true, AbilitySideEffect.ActivationPolicy);
		}
	}

	// Event side effects
	for (FEventSideEffect& EventSideEffect : EventSideEffects)
	{
		USimpleGameplayAbilityComponent* EventSendingComponent = EventSideEffect.EventSender == EAttributeModifierSideEffectTarget::Instigator ? Instigator : Target;
		
		if (EventSideEffect.ApplicationTriggers.Contains(EffectPhase))
		{
			FInstancedStruct Payload;

			if (EventSideEffect.EventContextTag.IsValid())
			{
				bool IsPayloadValid = true;
				Payload = GetEventSideEffectContext(EventSideEffect.EventContextTag, IsPayloadValid);
			}

			EventSideEffect.EventContext = Payload;
			ModifierResult.AppliedEventSideEffects.Add(EventSideEffect);
			
			EventSendingComponent->SendEvent(EventSideEffect.EventTag, EventSideEffect.EventDomain, Payload, EventSendingComponent->GetOwner(), EventSideEffect.EventReplicationPolicy);
		}
	}

	// Attribute modifier side effects
	for (FAttributeModifierSideEffect& AttributeSideEffect : AttributeModifierSideEffects)
	{
		if (ModifierApplicationPolicy == EAttributeModifierApplicationPolicy::ApplyClientPredicted)
		{
			if (AttributeSideEffect.AttributeModifierClass->GetDefaultObject<USimpleAttributeModifier>()->ModifierApplicationPolicy != EAttributeModifierApplicationPolicy::ApplyClientPredicted)
			{
				SIMPLE_LOG(this, FString::Printf(TEXT(
					"[USimpleAttributeModifier::ApplySideEffects]: Attribute side effect %s has ModifierApplicationPolicy ApplyClientPredicted but %s is not ApplyClientPredicted."),
					*AttributeSideEffect.AttributeModifierClass->GetName(), *GetName()));
				continue;
			}
		}
		
		if (AttributeSideEffect.ApplicationTriggers.Contains(EffectPhase))
		{
			USimpleGameplayAbilityComponent* InstigatingAbilityComponent = AttributeSideEffect.ModifierInstigator == EAttributeModifierSideEffectTarget::Instigator ? Instigator : Target;
			USimpleGameplayAbilityComponent* TargetedAbilityComponent = AttributeSideEffect.ModifierTarget == EAttributeModifierSideEffectTarget::Instigator ? Instigator : Target;

			if (AttributeSideEffect.ModifierTargetsTag.IsValid())
			{
				GetAttributeModifierSideEffectTargets(AttributeSideEffect.ModifierTargetsTag, InstigatingAbilityComponent, TargetedAbilityComponent);

				if (!InstigatingAbilityComponent || !TargetedAbilityComponent)
				{
					SIMPLE_LOG(InstigatingAbilityComponent, FString::Printf(TEXT("[USimpleAttributeModifier::ApplySideEffects]: InstigatingAbilityComponent or TargetedAbilityComponent is null. Can't apply Attribute Modifier side effect.")));
					return;
				}
			}
			
			FInstancedStruct Payload;

			if (AttributeSideEffect.ModifierContextTag.IsValid())
			{
				bool IsPayloadValid = true;
				Payload = GetAttributeModifierSideEffectContext(AttributeSideEffect.ModifierContextTag, IsPayloadValid);
			}

			FGuid AttributeID = FGuid::NewGuid();
			InstigatingAbilityComponent->ApplyAttributeModifierToTarget(TargetedAbilityComponent, AttributeSideEffect.AttributeModifierClass, Payload, AttributeID);
			
			AttributeSideEffect.ModifierContext = Payload;
			AttributeSideEffect.AttributeID = AttributeID;
			ModifierResult.AppliedAttributeModifierSideEffects.Add(AttributeSideEffect);
		}
	}
	
	FSimpleAbilitySnapshot Snapshot;
	Snapshot.AbilityID = AbilityInstanceID;
	Snapshot.StateTag  = FDefaultTags::AttributeModifierApplied;
	Snapshot.TimeStamp = OwningAbilityComponent->GetServerTime();

	// ApplyServerOnly means we don't want to replicate any side effects so we clear the modifier result
	if (OwningAbilityComponent->HasAuthority())
	{
		if (ModifierApplicationPolicy == EAttributeModifierApplicationPolicy::ApplyServerOnly)
		{
			SIMPLE_LOG(this, TEXT("[USimpleAttributeModifier::ApplySideEffects]: ModifierApplicationPolicy is ApplyServerOnly. Skipping side effect replication."));
			ModifierResult = FAttributeModifierResult();
		}
	} 

	Snapshot.StateData = FInstancedStruct::Make(ModifierResult);
	OwningAbilityComponent->AddAttributeStateSnapshot(AbilityInstanceID, Snapshot);
}

/* Meta Attribute Functions */

void USimpleAttributeModifier::GetFloatMetaAttributeValue_Implementation(FGameplayTag MetaAttributeTag, float& OutValue, bool& WasHandled) const
{
	OutValue = 0;
	WasHandled = false;
}

FInstancedStruct USimpleAttributeModifier::GetModifiedStructAttributeValue_Implementation(FGameplayTag OperationTag, FInstancedStruct StructToModify, bool& WasHandled) const
{
	WasHandled = false;
	return StructToModify;
}

FInstancedStruct USimpleAttributeModifier::GetAbilitySideEffectContext_Implementation(FGameplayTag MetaTag, bool& WasHandled) const
{
	WasHandled = false;
	return FInstancedStruct();
}

FInstancedStruct USimpleAttributeModifier::GetEventSideEffectContext_Implementation(FGameplayTag MetaTag, bool& WasHandled) const
{
	WasHandled = false;
	return FInstancedStruct();
}

FInstancedStruct USimpleAttributeModifier::GetAttributeModifierSideEffectContext_Implementation(FGameplayTag MetaTag, bool& WasHandled) const
{
	WasHandled = false;
	return FInstancedStruct();
}

void USimpleAttributeModifier::GetAttributeModifierSideEffectTargets_Implementation(FGameplayTag TargetsTag, USimpleGameplayAbilityComponent*& OutInstigator, USimpleGameplayAbilityComponent*& OutTarget) const
{
	OutInstigator = nullptr;
	OutTarget = nullptr;
}

/* Utility Functions */

FFloatAttribute* USimpleAttributeModifier::GetTempFloatAttribute(const FGameplayTag AttributeTag, TArray<FFloatAttribute>& TempFloatAttributes) const
{
	for (int i = 0; i < TempFloatAttributes.Num(); i++)
	{
		if (TempFloatAttributes[i].AttributeTag.MatchesTagExact(AttributeTag))
		{
			return &TempFloatAttributes[i];
		}
	}

	return nullptr;
}

FStructAttribute* USimpleAttributeModifier::GetTempStructAttribute(const FGameplayTag AttributeTag, TArray<FStructAttribute>& TempStructAttributes) const
{
	for (int i = 0; i < TempStructAttributes.Num(); i++)
	{
		if (TempStructAttributes[i].AttributeTag == AttributeTag)
		{
			return &TempStructAttributes[i];
		}
	}

	return nullptr;
}

void USimpleAttributeModifier::OnTagsChanged(FGameplayTag EventTag, FGameplayTag Domain, FInstancedStruct Payload, AActor* Sender)
{
	if (ModifierType == EAttributeModifierType::Duration && bIsModifierActive)
	{
		if (!CanApplyModifierInternal(InitialModifierContext))
		{
			switch (TickTagRequirementBehaviour)
			{
				case EDurationTickTagRequirementBehaviour::CancelOnTagRequirementFailed:
					EndModifier(FDefaultTags::AbilityCancelled, FInstancedStruct());
					return;;
				case EDurationTickTagRequirementBehaviour::SkipOnTagRequirementFailed:
					return;
				case EDurationTickTagRequirementBehaviour::PauseOnTagRequirementFailed:
					InstigatorAbilityComponent->GetWorld()->GetTimerManager().PauseTimer(DurationTimerHandle);
					InstigatorAbilityComponent->GetWorld()->GetTimerManager().PauseTimer(TickTimerHandle);
					return;
			}
		}
		
		InstigatorAbilityComponent->GetWorld()->GetTimerManager().UnPauseTimer(DurationTimerHandle);
		InstigatorAbilityComponent->GetWorld()->GetTimerManager().UnPauseTimer(TickTimerHandle);
	}
}

void USimpleAttributeModifier::ClientFastForwardState(FGameplayTag StateTag, FSimpleAbilitySnapshot LatestAuthorityState)
{
	const FAttributeModifierResult* AuthorityModifierResult = LatestAuthorityState.StateData.GetPtr<FAttributeModifierResult>();

	if (!AuthorityModifierResult)
	{
		SIMPLE_LOG(this, TEXT("ClientFastForwardState: Authority modifier result is null."));
		return;
	}
	
	for (const FAbilitySideEffect& AuthoritySideEffect : AuthorityModifierResult->AppliedAbilitySideEffects)
	{
		USimpleGameplayAbilityComponent* ActivatingAbilityComponent = AuthoritySideEffect.ActivatingAbilityComponent == EAttributeModifierSideEffectTarget::Instigator ? AuthorityModifierResult->Instigator : AuthorityModifierResult->Target;
		ActivatingAbilityComponent->ActivateAbility(AuthoritySideEffect.AbilityClass, AuthoritySideEffect.AbilityContext, true, AuthoritySideEffect.ActivationPolicy);
	}
}

void USimpleAttributeModifier::ClientResolvePastState(FGameplayTag StateTag, FSimpleAbilitySnapshot AuthorityState, FSimpleAbilitySnapshot PredictedState)
{
	const FAttributeModifierResult* AuthorityModifierResult = AuthorityState.StateData.GetPtr<FAttributeModifierResult>();
    const FAttributeModifierResult* PredictedModifierResult = PredictedState.StateData.GetPtr<FAttributeModifierResult>();

    if (!AuthorityModifierResult || !PredictedModifierResult)
    {
        SIMPLE_LOG(this, TEXT("ClientResolvePastState: Authority or Predicted modifier result is null."));
        return;
    }

    // Check for ability side effects that were not predicted
    for (const FAbilitySideEffect& AuthoritySideEffect : AuthorityModifierResult->AppliedAbilitySideEffects)
    {
        bool bFound = false;
        for (const FAbilitySideEffect& PredictedSideEffect : PredictedModifierResult->AppliedAbilitySideEffects)
        {
            if (AuthoritySideEffect.AbilityClass == PredictedSideEffect.AbilityClass)
            {
                bFound = true;
                break;
            }
        }

        if (!bFound)
        {
            // Activate the side effect that was not predicted
        	SIMPLE_LOG(this, FString::Printf(TEXT("ClientResolvePastState: Activating ability side effect %s"), *AuthoritySideEffect.AbilityClass->GetName()));
            USimpleGameplayAbilityComponent* ActivatingAbilityComponent = AuthoritySideEffect.ActivatingAbilityComponent == EAttributeModifierSideEffectTarget::Instigator ? InstigatorAbilityComponent : TargetAbilityComponent;
            ActivatingAbilityComponent->ActivateAbility(AuthoritySideEffect.AbilityClass, AuthoritySideEffect.AbilityContext, true, AuthoritySideEffect.ActivationPolicy);
        }
    }

    // Check for ability side effects that were predicted but not run on the server
    for (const FAbilitySideEffect& PredictedSideEffect : PredictedModifierResult->AppliedAbilitySideEffects)
    {
        bool bFound = false;
        for (const FAbilitySideEffect& AuthoritySideEffect : AuthorityModifierResult->AppliedAbilitySideEffects)
        {
            if (PredictedSideEffect.AbilityClass == AuthoritySideEffect.AbilityClass)
            {
                bFound = true;
                break;
            }
        }

        if (!bFound)
        {
            // Cancel the side effect that was predicted but not run on the server
            USimpleGameplayAbilityComponent* ActivatingAbilityComponent = PredictedSideEffect.ActivatingAbilityComponent == EAttributeModifierSideEffectTarget::Instigator ? InstigatorAbilityComponent : TargetAbilityComponent;
            //ActivatingAbilityComponent->CancelAbility(PredictedSideEffect.AbilityClass, PredictedSideEffect.AbilityContext);
        }
    }
	
}
