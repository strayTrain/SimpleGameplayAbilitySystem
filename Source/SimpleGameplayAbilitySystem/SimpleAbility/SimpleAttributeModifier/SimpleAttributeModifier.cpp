#include "SimpleAttributeModifier.h"

#include "SimpleGameplayAbilitySystem/BlueprintFunctionLibraries/FunctionSelectors/FunctionSelectors.h"
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
		EndModifier(FDefaultTags::AbilityCancelled(), FInstancedStruct());
		
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
			EventTags.AddTag(FDefaultTags::GameplayTagAdded());
			EventTags.AddTag(FDefaultTags::GameplayTagRemoved());
    
			FSimpleEventDelegate EventDelegate;
			EventDelegate.BindDynamic(this, &USimpleAttributeModifier::OnTagsChanged);
		
			TArray<UObject*> SenderFilter;
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
					EndModifier(FDefaultTags::AbilityEndedSuccessfully(), FInstancedStruct());
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
								EndModifier(FDefaultTags::AbilityCancelled(), FInstancedStruct());
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
		EndModifier(FDefaultTags::AbilityEndedSuccessfully(), FInstancedStruct());
		return true;
	}

	EndModifier(FDefaultTags::AbilityCancelled(), FInstancedStruct());
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

	for (const FFloatAttributeModifier& FloatModifier : FloatAttributeModifications)
	{
		if (ModifierType == EAttributeModifierType::Duration && !FloatModifier.ApplicationRequirements.Contains(TriggerPhase))
		{
			continue;
		}

		if (ApplyFloatAttributeModifier(FloatModifier, TempFloatAttributes, CurrentFloatModifierOverflow))
		{
			ModifiedFloatAttributes.Add(FloatModifier.AttributeToModify);
		}
		else if (FloatModifier.IfAttributeNotFound == EAttributeModiferNotFoundBehaviour::CancelModifier)
		{
			return false;
		}
	}
	
	for (const FStructAttributeModifier& StructModifier : StructAttributeModifications)
	{
		if (ModifierType == EAttributeModifierType::Duration && !StructModifier.ApplicationRequirements.Contains(TriggerPhase))
		{
			continue;
		}
		
		if (ApplyStructAttributeModifier(StructModifier, TempStructAttributes))
		{
			ModifiedStructAttributes.Add(StructModifier.AttributeToModify);
		}
		else if (StructModifier.IfAttributeNotFound == EAttributeModiferNotFoundBehaviour::CancelModifier)
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
				TargetAbilityComponent->OverrideFloatAttribute(Attribute.AttributeTag, Attribute);
			}
		}

		for (const FStructAttribute& Attribute : TempStructAttributes)
		{
			if (ModifiedStructAttributes.Contains(Attribute.AttributeTag))
			{
				TargetAbilityComponent->SetStructAttributeValue(Attribute.AttributeTag, Attribute.AttributeValue);
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

	if (EndingStatus.MatchesTagExact(FDefaultTags::AbilityCancelled()))
	{
		for (const FGameplayTag& Tag : PermanentlyAppliedTags)
		{
			TargetAbilityComponent->RemoveGameplayTag(Tag, FInstancedStruct());
		}
	}

	if (ModifierType == EAttributeModifierType::Instant)
	{
		if (EndingStatus.MatchesTagExact(FDefaultTags::AbilityCancelled()))
		{
			ApplySideEffects(InstigatorAbilityComponent, TargetAbilityComponent, EAttributeModifierSideEffectTrigger::OnInstantModifierEndedCancel);
		}

		if (EndingStatus.MatchesTagExact(FDefaultTags::AbilityEndedSuccessfully()))
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

		if (EndingStatus.MatchesTagExact(FDefaultTags::AbilityCancelled()))
		{
			ApplySideEffects(InstigatorAbilityComponent, TargetAbilityComponent, EAttributeModifierSideEffectTrigger::OnDurationModifierEndedCancel);
		}

		if (EndingStatus.MatchesTagExact(FDefaultTags::AbilityEndedSuccessfully()))
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

bool USimpleAttributeModifier::ApplyFloatAttributeModifier(const FFloatAttributeModifier& FloatModifier, TArray<FFloatAttribute>& TempFloatAttributes, float& CurrentOverflow) const
{
	FFloatAttribute* AttributeToModify = GetTempFloatAttribute(FloatModifier.AttributeToModify, TempFloatAttributes);
	
	if (!AttributeToModify)
	{
		SIMPLE_LOG(OwningAbilityComponent, FString::Printf(TEXT("[USimpleAttributeModifier::ApplyFloatAttributeModifier]: Attribute %s not found."), *FloatModifier.AttributeToModify.ToString()));
		return false;
	}
	
	/**
	 * The formula is NewAttributeValue = CurrentAttributeValue [operation] ModificationInputValue
	 * Where [operation] is one of the following: add, multiply, override (i.e. replace with) or custom (call a function)
	 **/

	// To Start we get the input value for the modification
	float ModificationInputValue = 0;
	bool WasTargetAttributeFound = false;
	bool WasInstigatorAttributeFound = false;
	switch (FloatModifier.ModificationInputValueSource)
	{
		case EAttributeModificationValueSource::Manual:
			ModificationInputValue = FloatModifier.ManualInputValue;
			break;
		
		case EAttributeModificationValueSource::FromOverflow:
			ModificationInputValue = CurrentOverflow;

			if (FloatModifier.ConsumeOverflow)
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
		
			ModificationInputValue = InstigatorAbilityComponent->GetFloatAttributeValue(FloatModifier.SourceAttributeValueType, FloatModifier.SourceAttribute, WasInstigatorAttributeFound);

			if (!WasInstigatorAttributeFound)
			{
				UE_LOG(LogSimpleGAS, Warning, TEXT("USimpleAttributeModifier::ApplyFloatAttributeModifier: Source attribute %s not found on instigator ability component."), *FloatModifier.SourceAttribute.ToString());
				return false;
			}

			break;
		
		case EAttributeModificationValueSource::FromTargetAttribute:
			if (!TargetAbilityComponent)
			{
				UE_LOG(LogSimpleGAS, Warning, TEXT("USimpleAttributeModifier::ApplyFloatAttributeModifier: Target ability component is nullptr."));
				return false;
			}
		
			ModificationInputValue = TargetAbilityComponent->GetFloatAttributeValue(FloatModifier.SourceAttributeValueType, FloatModifier.SourceAttribute, WasTargetAttributeFound);

			if (!WasTargetAttributeFound)
			{
				UE_LOG(LogSimpleGAS, Warning, TEXT("USimpleAttributeModifier::ApplyFloatAttributeModifier: Source attribute %s not found on target ability component."), *FloatModifier.SourceAttribute.ToString());
				return false;
			}
			
			break;
		
	case EAttributeModificationValueSource::CustomInputValue:
			if (!UFunctionSelectors::GetCustomFloatInputValue(
				this,
				FloatModifier.CustomInputFunction,
				AttributeToModify->AttributeTag,
				ModificationInputValue))
			{
				SIMPLE_LOG(OwningAbilityComponent, FString::Printf(TEXT("[USimpleAttributeModifier::ApplyFloatAttributeModifier]: Custom input function failed to activate.")));
				return false;
			}
		
	}

	// Next up we get the current value of the attribute
	float CurrentAttributeValue = 0;
	switch (FloatModifier.ModifiedAttributeValueType)
	{
		case EAttributeValueType::BaseValue:
			CurrentAttributeValue = AttributeToModify->BaseValue;
			break;
		case EAttributeValueType::MinBaseValue:
			CurrentAttributeValue = AttributeToModify->ValueLimits.MinBaseValue;
			break;
		case EAttributeValueType::MaxBaseValue:
			CurrentAttributeValue = AttributeToModify->ValueLimits.MaxBaseValue;
			break;
		case EAttributeValueType::CurrentValue:
			CurrentAttributeValue = AttributeToModify->CurrentValue;
			break;
		case EAttributeValueType::MinCurrentValue:
			CurrentAttributeValue = AttributeToModify->ValueLimits.MinCurrentValue;
			break;
		case EAttributeValueType::MaxCurrentValue:
			CurrentAttributeValue = AttributeToModify->ValueLimits.MaxCurrentValue;
			break;
	}
	
	// Next, modify AttributeToModify based on the input value and the modifier's operation
	float NewAttributeValue = 0;
	FGameplayTag FloatChangedDomainTag = AttributeToModify->AttributeTag;
	switch (FloatModifier.ModificationOperation)
	{
		case EFloatAttributeModificationOperation::Add:
			NewAttributeValue = CurrentAttributeValue + ModificationInputValue;
			break;

		case EFloatAttributeModificationOperation::Subtract:
			NewAttributeValue = CurrentAttributeValue - ModificationInputValue;
			break;
					
		case EFloatAttributeModificationOperation::Multiply:
			NewAttributeValue = CurrentAttributeValue * ModificationInputValue;
			break;

		case EFloatAttributeModificationOperation::Divide:
			if (FMath::IsNearlyZero(ModificationInputValue))
			{
				SIMPLE_LOG(OwningAbilityComponent, TEXT("[USimpleAttributeModifier::ApplyFloatAttributeModifier]: Division by zero."));
				return false;
			}
			NewAttributeValue = CurrentAttributeValue / ModificationInputValue;
			break;

		case EFloatAttributeModificationOperation::Power:
			NewAttributeValue = FMath::Pow(CurrentAttributeValue, ModificationInputValue);
			break;
		
		case EFloatAttributeModificationOperation::Override:
			NewAttributeValue =  ModificationInputValue;
			break;
		
		case EFloatAttributeModificationOperation::Custom:
			if (!UFunctionSelectors::ApplyFloatAttributeOperation(
				this,
				FloatModifier.FloatOperationFunction,
				AttributeToModify->AttributeTag,
				CurrentAttributeValue,
				ModificationInputValue,
				CurrentOverflow,
				FloatChangedDomainTag,
				NewAttributeValue,
				CurrentOverflow))
			{
				SIMPLE_LOG(OwningAbilityComponent, FString::Printf(TEXT("[USimpleAttributeModifier::ApplyFloatAttributeModifier]: Custom operation function %s failed to activate."), *FloatModifier.CustomInputFunction.GetMemberName().ToString()));
				return false;
			}

			break;
	}

	// Lastly, we set the new value to the attribute
	switch (FloatModifier.ModifiedAttributeValueType)
	{
		case EAttributeValueType::BaseValue:
			AttributeToModify->BaseValue = OwningAbilityComponent->ClampFloatAttributeValue(*AttributeToModify, EAttributeValueType::BaseValue, NewAttributeValue, CurrentOverflow);
			break;
		
		case EAttributeValueType::CurrentValue:
			AttributeToModify->CurrentValue = OwningAbilityComponent->ClampFloatAttributeValue(*AttributeToModify, EAttributeValueType::CurrentValue, NewAttributeValue, CurrentOverflow);
			break;
		
		case EAttributeValueType::MaxBaseValue:
			AttributeToModify->ValueLimits.MaxBaseValue = NewAttributeValue;
			break;
		
		case EAttributeValueType::MinBaseValue:
			AttributeToModify->ValueLimits.MinBaseValue = NewAttributeValue;
			break;
		
		case EAttributeValueType::MaxCurrentValue:
			AttributeToModify->ValueLimits.MaxCurrentValue = NewAttributeValue;
			break;
		
		case EAttributeValueType::MinCurrentValue:
			AttributeToModify->ValueLimits.MinCurrentValue = NewAttributeValue;
			break;
	}
	
	return true;
}

bool USimpleAttributeModifier::ApplyStructAttributeModifier(const FStructAttributeModifier& StructModifier, TArray<FStructAttribute>& TempStructAttributes) const
{
	FStructAttribute* AttributeToModify = GetTempStructAttribute(StructModifier.AttributeToModify, TempStructAttributes);
	
	if (!AttributeToModify)
	{
		UE_LOG(LogSimpleGAS, Warning, TEXT("USimpleAttributeModifier::ApplyStructAttributeModifier: Attribute %s not found on target ability component."), *StructModifier.AttributeToModify.ToString());
		return false;
	}

	FInstancedStruct OutStruct;
	
	if (!UFunctionSelectors::ModifyStructAttributeValue(
		this,
		StructModifier.StructModificationFunction,
		AttributeToModify->AttributeTag,
		AttributeToModify->AttributeValue,
		OutStruct))
	{
		SIMPLE_LOG(OwningAbilityComponent, FString::Printf(TEXT("[USimpleAttributeModifier::ApplyStructAttributeModifier]: Struct modifier %s failed to apply."), *StructModifier.ModifierDescription));
		return false;
	}

	AttributeToModify->AttributeValue = OutStruct;
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
	for (FAbilitySideEffect AbilitySideEffect : AbilitySideEffects)
	{
		if (AbilitySideEffect.ApplicationTriggers.Contains(EffectPhase))
		{
			bool IsServer = OwningAbilityComponent->GetNetMode() == NM_ListenServer || OwningAbilityComponent->GetNetMode() == NM_DedicatedServer;
			bool IsListenServer = OwningAbilityComponent->GetNetMode() == NM_ListenServer;
			bool IsClient = OwningAbilityComponent->GetNetMode() == NM_Client && !IsListenServer;

			USimpleGameplayAbilityComponent* ActivatingAbilityComponent = AbilitySideEffect.ActivatingAbilityComponent == EAttributeModifierSideEffectTarget::Instigator ? Instigator : Target;
			FInstancedStruct Payload = FInstancedStruct();

			UFunctionSelectors::GetStructContext(this, AbilitySideEffect.ContextFunction, Payload);

			AbilitySideEffect.AbilityContext = Payload;
			ModifierResult.AppliedAbilitySideEffects.Add(AbilitySideEffect);

			FGuid AbilityID = FGuid::NewGuid();
			
			switch (AbilitySideEffect.ActivationPolicy)
			{
				case EAbilityActivationPolicy::LocalOnly:
					ActivatingAbilityComponent->ActivateAbilityWithID(AbilityID, AbilitySideEffect.AbilityClass, Payload);
					break;

				case EAbilityActivationPolicy::ClientOnly:
					if (IsClient)
					{
						ActivatingAbilityComponent->ActivateAbilityWithID(AbilityID, AbilitySideEffect.AbilityClass, Payload);
					}
					break;

				case EAbilityActivationPolicy::ServerOnly:
					if (IsServer)
					{
						ActivatingAbilityComponent->ActivateAbilityWithID(AbilityID, AbilitySideEffect.AbilityClass, Payload);
					}
					break;
				
				case EAbilityActivationPolicy::ClientPredicted:
					if (IsClient && !(IsListenServer || IsServer))
					{
						ActivatingAbilityComponent->ActivateAbilityWithID(AbilityID, AbilitySideEffect.AbilityClass, Payload);
					}
					break;
				
				case EAbilityActivationPolicy::ServerInitiatedFromClient:
				case EAbilityActivationPolicy::ServerAuthority:
					if (IsServer)
					{
						ActivatingAbilityComponent->ActivateAbilityWithID(AbilityID, AbilitySideEffect.AbilityClass, Payload);
					}
					break;
			}
		}
	}

	// Event side effects
	for (FEventSideEffect& EventSideEffect : EventSideEffects)
	{
		USimpleGameplayAbilityComponent* EventSendingComponent = EventSideEffect.EventSender == EAttributeModifierSideEffectTarget::Instigator ? Instigator : Target;
		FInstancedStruct Payload = FInstancedStruct();

		UFunctionSelectors::GetStructContext(this, EventSideEffect.EventContextFunction, Payload);
		
		if (EventSideEffect.ApplicationTriggers.Contains(EffectPhase))
		{
			EventSideEffect.EventContext = Payload;
			ModifierResult.AppliedEventSideEffects.Add(EventSideEffect);
			
			EventSendingComponent->SendEvent(EventSideEffect.EventTag, EventSideEffect.EventDomain, EventSideEffect.EventContext, EventSendingComponent->GetOwner(), {}, EventSideEffect.EventReplicationPolicy);
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

			UFunctionSelectors::GetAttributeModifierSideEffectTargets(
				this,
				AttributeSideEffect.GetTargetsFunction,
				InstigatingAbilityComponent,
				TargetedAbilityComponent);
			
			FInstancedStruct Payload = FInstancedStruct();
			UFunctionSelectors::GetStructContext(this, AttributeSideEffect.ContextFunction, Payload);

			FGuid AttributeID = FGuid::NewGuid();
			InstigatingAbilityComponent->ApplyAttributeModifierToTarget(TargetedAbilityComponent, AttributeSideEffect.AttributeModifierClass, Payload, AttributeID);
			
			AttributeSideEffect.ModifierContext = Payload;
			AttributeSideEffect.AttributeID = AttributeID;
			ModifierResult.AppliedAttributeModifierSideEffects.Add(AttributeSideEffect);
		}
	}
	
	FSimpleAbilitySnapshot Snapshot;
	Snapshot.AbilityID = AbilityInstanceID;
	Snapshot.SnapshotTag = FDefaultTags::AttributeModifierApplied();
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

void USimpleAttributeModifier::OnTagsChanged(FGameplayTag EventTag, FGameplayTag Domain, FInstancedStruct Payload, UObject* Sender)
{
	if (ModifierType == EAttributeModifierType::Duration && bIsModifierActive)
	{
		if (!CanApplyModifierInternal(InitialModifierContext))
		{
			switch (TickTagRequirementBehaviour)
			{
				case EDurationTickTagRequirementBehaviour::CancelOnTagRequirementFailed:
					EndModifier(FDefaultTags::AbilityCancelled(), FInstancedStruct());
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
		ActivatingAbilityComponent->ActivateAbilityWithID(FGuid::NewGuid(), AuthoritySideEffect.AbilityClass, AuthoritySideEffect.AbilityContext, true, AuthoritySideEffect.ActivationPolicy);
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
            USimpleGameplayAbilityComponent* ActivatingAbilityComponent = AuthoritySideEffect.ActivatingAbilityComponent == EAttributeModifierSideEffectTarget::Instigator ? InstigatorAbilityComponent : TargetAbilityComponent;
            ActivatingAbilityComponent->ActivateAbilityWithID(FGuid::NewGuid(), AuthoritySideEffect.AbilityClass, AuthoritySideEffect.AbilityContext, true, AuthoritySideEffect.ActivationPolicy);
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
            ActivatingAbilityComponent->CancelAbility(PredictedSideEffect.AbilityInstanceID, PredictedSideEffect.AbilityContext, true);
        }
    }
	
}
