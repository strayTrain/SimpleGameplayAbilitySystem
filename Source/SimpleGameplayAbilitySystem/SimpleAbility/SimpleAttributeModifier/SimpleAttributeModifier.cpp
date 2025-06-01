#include "SimpleAttributeModifier.h"

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
}

bool USimpleAttributeModifier::CanActivate(USimpleGameplayAbilityComponent* ActivatingAbilityComponent, const FAbilityContextCollection ActivationContext)
{
	return CanApplyModifier();
}

bool USimpleAttributeModifier::Activate(USimpleGameplayAbilityComponent* ActivatingAbilityComponent, const FGuid NewAbilityID, const FAbilityContextCollection ActivationContext)
{
	AbilityID = NewAbilityID;
	OwningAbilityComponent = ActivatingAbilityComponent;
	InstigatorAbilityComponent = ActivatingAbilityComponent;
	AbilityContexts = ActivationContext;
	ModifierActionScratchPad = FAttributeModifierActionScratchPad();
	
	// Check if we can apply the modifier
	if (!CanApplyModifierInternal() || !CanActivate(ActivatingAbilityComponent, ActivationContext))
	{
		ApplyActionStacks(EAttributeModifierPhase::OnApplicationFailed, this);
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
	
	/* If we're an instant modifier we apply the action stack immediately and then end. SetDuration modifiers with a
	duration of 0 also apply immediately and end. */
	if (DurationType == EAttributeModifierType::Instant || (DurationType == EAttributeModifierType::SetDuration && Duration <= 0))
	{
		ApplyActionStacks(EAttributeModifierPhase::OnApplied, this);
		ApplyActionStacks(EAttributeModifierPhase::Default, this);
		OnPostApplyModifierActions();
		
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

void USimpleAttributeModifier::Cancel(FGameplayTag CancelStatus, FInstancedStruct CancelContext)
{
	End(CancelStatus, CancelContext);
}

void USimpleAttributeModifier::End(FGameplayTag EndStatus, FInstancedStruct EndContext)
{
	InstigatorAbilityComponent->GetWorld()->GetTimerManager().ClearTimer(DurationTimerHandle);
	InstigatorAbilityComponent->GetWorld()->GetTimerManager().ClearTimer(TickTimerHandle);

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
	
	if (DurationType == EAttributeModifierType::SetDuration)
	{
		for (const FGameplayTag& Tag : TemporarilyAppliedTags)
		{
			TargetAbilityComponent->RemoveGameplayTag(Tag, FInstancedStruct());
		}
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

	// if (USimpleAttributeFunctionLibrary::HasModifierWithTags(TargetAbilityComponent, TargetBlockingModifierTags))
	// {
	// 	UE_LOG(LogSimpleGAS, Warning, TEXT("Target has blocking modifier tags in USimpleAttributeModifier::CanApplyModifierInternal"));
	// 	return false;
	// }

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

/*bool USimpleAttributeModifier::ApplyFloatAttributeModifier(const FFloatAttributeModifier& FloatModifier, TArray<FFloatAttribute>& TempFloatAttributes, float& CurrentOverflow)
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
	 *#1#

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
}*/

bool USimpleAttributeModifier::ApplyActionStacks(const EAttributeModifierPhase& Phase, USimpleAttributeModifier* OwningModifier)
{
	for (UModifierAction* Action : ModifierActions)
	{
		if (!Action->ShouldApply(Phase, OwningModifier))
		{
			continue;
		}

		if (!Action->ApplyAction(OwningModifier))
		{
			SIMPLE_LOG(GetWorld(), FString::Printf(TEXT("[USimpleAttributeModifier::ApplyActionStacks]: Action %s failed to apply."), *Action->GetName()));
			return false;
		}
	}
	
	return true;
}

void USimpleAttributeModifier::OnTagsChanged(FGameplayTag EventTag, FGameplayTag Domain, FInstancedStruct Payload, UObject* Sender)
{
	if (DurationType == EAttributeModifierType::SetDuration && IsActive)
	{
		if (!CanApplyModifierInternal())
		{
			switch (TickTagRequirementBehaviour)
			{
				case EDurationTickTagRequirementBehaviour::CancelOnTagRequirementFailed:
					Cancel(FDefaultTags::AbilityCancelled(), FInstancedStruct());
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

void USimpleAttributeModifier::OnDurationTimerExpired()
{
}

void USimpleAttributeModifier::OnTickTimerTriggered()
{
}
