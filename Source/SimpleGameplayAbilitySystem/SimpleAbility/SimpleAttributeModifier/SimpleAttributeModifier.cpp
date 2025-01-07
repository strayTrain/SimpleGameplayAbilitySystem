#include "SimpleAttributeModifier.h"

#include "SimpleGameplayAbilitySystem/BlueprintFunctionLibraries/SimpleAttributes/SimpleAttributeFunctionLibrary.h"
#include "SimpleGameplayAbilitySystem/DefaultTags/DefaultTags.h"
#include "SimpleGameplayAbilitySystem/Module/SimpleGameplayAbilitySystem.h"
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

	if (USimpleAttributeFunctionLibrary::HasModifierWithTags(TargetAbilityComponent, TargetBlockingModifierTags))
	{
		UE_LOG(LogSimpleGAS, Warning, TEXT("Target has blocking modifier tags in USimpleAttributeModifier::CanApplyModifierInternal"));
		return false;
	}

	return true;
}

bool USimpleAttributeModifier::CanApplyModifier_Implementation(FInstancedStruct ModifierContext) const
{
	return true;
}

bool USimpleAttributeModifier::ApplyModifier(USimpleGameplayAbilityComponent* Instigator, USimpleGameplayAbilityComponent* Target, FInstancedStruct ModifierContext)
{
	InstigatorAbilityComponent = Instigator;
	TargetAbilityComponent = Target;
	InitialModifierContext = ModifierContext;
	bIsModifierActive = true;
	
	if (!CanApplyModifierInternal(ModifierContext) || !CanApplyModifier(ModifierContext))
	{
		EndModifier(FDefaultTags::AbilityCancelled, FInstancedStruct());
		return false;
	}

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
	
	for (const FGameplayTag& Tag : PermanentlyAppliedTags)
	{
		TargetAbilityComponent->AddGameplayTag(Tag, FInstancedStruct());
	}

	for (const FGameplayTag& Tag : RemoveGameplayTags)
	{
		TargetAbilityComponent->RemoveGameplayTag(Tag, FInstancedStruct());
	}

	OnPreApplyModifier();
	
	if (ModifierType == EAttributeModifierType::Duration)
	{
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
					
					ApplyModifiersInternal(true);
				},
				TickInterval,
				true
			);
		}
		
		return true;
	}
	
	// Instant type modifier
	
	if (ApplyModifiersInternal())
	{
		OnPostApplyModifier();
		EndModifier(FDefaultTags::AbilityEndedSuccessfully, FInstancedStruct());
		return true;
	}

	EndModifier(FDefaultTags::AbilityCancelled, FInstancedStruct());
	return false;
}

bool USimpleAttributeModifier::ApplyModifiersInternal(bool IsTick)
{
	// We process the modifier stack as a transaction to avoid partial changes of attributes
	TArray<FFloatAttribute> TempFloatAttributes = TargetAbilityComponent->FloatAttributes;
	TArray<FStructAttribute> TempStructAttributes = TargetAbilityComponent->StructAttributes;

	TArray<FGameplayTag> ModifiedFloatAttributes;
	TArray<FGameplayTag> ModifiedStructAttributes;

	float CurrentFloatModifierOverflow = 0;
	
	for (const FAttributeModifier& Modifier : AttributeModifications )
	{
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
			default:
				break;
		}
		
		if (!WasModifierApplied && Modifier.CancelIfAttributeNotFound)
		{
			return false;
		}
	}

	// If all the modifiers were applied successfully, we update the target ability component's attributes if we are the authority
	if (InstigatorAbilityComponent->HasAuthority())
	{
		for (FFloatAttribute Attribute : TempFloatAttributes)
		{
			if (ModifiedFloatAttributes.Contains(Attribute.AttributeTag))
			{
				USimpleAttributeFunctionLibrary::OverrideFloatAttribute(TargetAbilityComponent, Attribute.AttributeTag, Attribute);
			}
		}

		for (FStructAttribute Attribute : TempStructAttributes)
		{
			if (ModifiedStructAttributes.Contains(Attribute.AttributeTag))
			{
				TargetAbilityComponent->AddStructAttribute(Attribute);
			}
		}
	}
	
	ApplySideEffects(InstigatorAbilityComponent, TargetAbilityComponent);

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

	OnModifierEnded(EndingStatus, EndingContext);
	
	if (ModifierType == EAttributeModifierType::Duration)
	{
		for (const FGameplayTag& Tag : TemporarilyAppliedTags)
		{
			TargetAbilityComponent->RemoveGameplayTag(Tag, FInstancedStruct());
		}
	}

	if (EndingStatus == FDefaultTags::AbilityCancelled)
	{
		for (const FGameplayTag& Tag : PermanentlyAppliedTags)
		{
			TargetAbilityComponent->RemoveGameplayTag(Tag, FInstancedStruct());
		}
	}

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
		UE_LOG(LogSimpleGAS, Warning, TEXT("USimpleAttributeModifier::ApplyFloatAttributeModifier: Attribute %s not found on target ability component."), *Modifier.ModifiedAttribute.ToString());
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
		default:
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
				default:
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
				default:
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
		
		default:
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

	FInstancedStruct NewValue = AttributeToModify->AttributeValue;
	bool WasHandled = false;
	
	GetModifiedStructAttributeValue(Modifier.MetaAttributeTag, NewValue, WasHandled);

	if (!WasHandled)
	{
		UE_LOG(LogSimpleGAS, Warning, TEXT("USimpleAttributeModifier::ApplyStructAttributeModifier: Struct modifier %s not handled."), *Modifier.StructModifierTag.ToString());
		return false;
	}

	AttributeToModify->AttributeValue = NewValue;
	return true;
}

void USimpleAttributeModifier::ApplySideEffects(USimpleGameplayAbilityComponent* Instigator, USimpleGameplayAbilityComponent* Target)
{
}

/* Meta Attribute Functions */

void USimpleAttributeModifier::GetFloatMetaAttributeValue_Implementation(FGameplayTag MetaAttributeTag, float& OutValue, bool& WasHandled) const
{
	OutValue = 0;
	WasHandled = false;
}

void USimpleAttributeModifier::GetModifiedStructAttributeValue_Implementation(FGameplayTag MetaAttributeTag, FInstancedStruct& OutValue, bool& WasHandled) const
{
	OutValue = FInstancedStruct();
	WasHandled = false;
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

/* Utility Functions */

FFloatAttribute* USimpleAttributeModifier::GetTempFloatAttribute(const FGameplayTag AttributeTag, TArray<FFloatAttribute>& TempFloatAttributes) const
{
	for (int i = 0; i < TempFloatAttributes.Num(); i++)
	{
		if (TempFloatAttributes[i].AttributeTag == AttributeTag)
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

void USimpleAttributeModifier::OnTagsChanged(FGameplayTag EventTag, FGameplayTag Domain, FInstancedStruct Payload)
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
