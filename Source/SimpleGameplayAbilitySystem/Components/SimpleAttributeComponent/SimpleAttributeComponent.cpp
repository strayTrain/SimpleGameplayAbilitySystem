#include "SimpleAttributeComponent.h"

#include "VREditorMode.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent//AttributeHandler/SimpleAttributeHandler.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleTimeSynchronizerComponent/ExtendedTimeSynchronizer/SimpleTimeSynchronizerExtended.h"
#include "SimpleGameplayAbilitySystem/Module/SimpleGameplayAbilitySystem.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeModifier/SimpleAttributeModifier.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeModifier/ModifierActions/ChangeFloatAttributeAction/FloatAttributeActionTypes.h"
#include "SimpleGameplayAbilitySystem/DefaultTags/DefaultTags.h"

USimpleAttributeComponent::USimpleAttributeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USimpleAttributeComponent::BeginPlay()
{
	UActorComponent::BeginPlay();

	SetIsReplicated(true);

	if (HasAuthority())
	{
		// Add default attributes defined on the component
		for (const FStructAttribute Attribute : DefaultStructAttributes)
		{
			AddStructAttribute(Attribute);
		}

		for (const FFloatAttribute Attribute : DefaultFloatAttributes)
		{
			AddFloatAttribute(Attribute);
		}

		for (USimpleAttributeSet* AttributeSet : AttributeSets)
		{
			for (const FFloatAttribute Attribute : AttributeSet->FloatAttributes)
			{
				AddFloatAttribute(Attribute);
			}

			for (const FStructAttribute Attribute : AttributeSet->StructAttributes)
			{
				AddStructAttribute(Attribute);
			}
		}

		// Add default gameplay tags defined on the component
		for (const FGameplayTag& Tag : DefaultGameplayTags)
		{
			AddGameplayTag(Tag);
		}

		return;
	}

	// Delegates called on the client to handle replicated data
	AuthorityAttributeModifierStates.OnStateAdded.BindUObject(
		this, &USimpleAttributeComponent::ClientOnAttributeModiferStateAdded);
	AuthorityAttributeModifierStates.OnStateChanged.BindUObject(
		this, &USimpleAttributeComponent::ClientOnAttributeModifierStateChanged);
	AuthorityAttributeModifierStates.OnStateRemoved.BindUObject(
		this, &USimpleAttributeComponent::ClientOnAttributeModiferStateRemoved);

	AuthorityAttributeModifierMutations.OnStateAdded.BindUObject(
		this, &USimpleAttributeComponent::ClientOnAttributeModifierMutationAdded);

	AuthorityFloatAttributes.OnFloatAttributeAdded.BindUObject(
		this, &USimpleAttributeComponent::ClientOnFloatAttributeAdded);
	AuthorityFloatAttributes.OnFloatAttributeChanged.BindUObject(
		this, &USimpleAttributeComponent::ClientOnFloatAttributeChanged);
	AuthorityFloatAttributes.OnFloatAttributeRemoved.BindUObject(
		this, &USimpleAttributeComponent::ClientOnFloatAttributeRemoved);

	AuthorityStructAttributes.OnStructAttributeAdded.BindUObject(
		this, &USimpleAttributeComponent::ClientOnStructAttributeAdded);
	AuthorityStructAttributes.OnStructAttributeChanged.BindUObject(
		this, &USimpleAttributeComponent::ClientOnStructAttributeChanged);
	AuthorityStructAttributes.OnStructAttributeRemoved.BindUObject(
		this, &USimpleAttributeComponent::ClientOnStructAttributeRemoved);

	AuthorityGameplayTags.OnGameplayTagCounterAdded.BindUObject(
		this, &USimpleAttributeComponent::ClientOnGameplayTagAdded);
	AuthorityGameplayTags.OnGameplayTagCounterRemoved.BindUObject(
		this, &USimpleAttributeComponent::ClientOnGameplayTagRemoved);

	// Only initialize local state if we've already received initial replication data
	// Otherwise, the FastArraySerializer callbacks will populate the local state
	if (AuthorityFloatAttributes.Attributes.Num() > 0)
	{
		LocalFloatAttributes = AuthorityFloatAttributes.Attributes;
	}
	if (AuthorityStructAttributes.Attributes.Num() > 0)
	{
		LocalStructAttributes = AuthorityStructAttributes.Attributes;
	}
	if (AuthorityGameplayTags.Tags.Num() > 0)
	{
		LocalGameplayTags = AuthorityGameplayTags.Tags;
	}

	// Set up periodic cleanup of old modifier states (every 10 seconds)
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(
			CleanupTimerHandle,
			this,
			&USimpleAttributeComponent::CleanupOldModifierStates,
			10.0f,
			true
		);
	}
}

void USimpleAttributeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Clear cleanup timer
	if (GetWorld() && CleanupTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(CleanupTimerHandle);
	}

	InstancedAttributeModifiers.Empty();
	Super::EndPlay(EndPlayReason);
}

/* Gameplay Tags */

void USimpleAttributeComponent::AddGameplayTag(FGameplayTag Tag)
{
	TArray<FGameplayTagCounter>& TagCounters = HasAuthority() ? AuthorityGameplayTags.Tags : LocalGameplayTags;
	FGameplayTagCounter* TagCounter = TagCounters.FindByPredicate([Tag](const FGameplayTagCounter& TagCounter)
	{
		return TagCounter.GameplayTag.MatchesTagExact(Tag);
	});

	if (TagCounter)
	{
		TagCounter->ReferenceCounter++;

		if (HasAuthority())
		{
			AuthorityGameplayTags.MarkItemDirty(*TagCounter);
		}

		return;
	}

	FGameplayTagCounter NewTagCounter;
	NewTagCounter.GameplayTag = Tag;
	NewTagCounter.ReferenceCounter = 1;

	TagCounters.AddUnique(NewTagCounter);

	if (HasAuthority())
	{
		AuthorityGameplayTags.MarkItemDirty(NewTagCounter);
	}

	OnGameplayTagAdded.Broadcast(Tag);
}

void USimpleAttributeComponent::RemoveGameplayTag(FGameplayTag Tag)
{
	TArray<FGameplayTagCounter>& TagCounters = HasAuthority() ? AuthorityGameplayTags.Tags : LocalGameplayTags;
	FGameplayTagCounter* TagCounter = TagCounters.FindByPredicate([Tag](const FGameplayTagCounter& TagCounter)
	{
		return TagCounter.GameplayTag.MatchesTagExact(Tag);
	});

	if (!TagCounter)
	{
		return;
	}

	if (TagCounter->ReferenceCounter > 1)
	{
		TagCounter->ReferenceCounter--;

		if (HasAuthority())
		{
			AuthorityGameplayTags.MarkItemDirty(*TagCounter);
		}

		return;
	}

	TagCounters.RemoveSingle(*TagCounter);

	if (HasAuthority())
	{
		AuthorityGameplayTags.MarkArrayDirty();
	}

	OnGameplayTagRemoved.Broadcast(Tag);
}

bool USimpleAttributeComponent::HasGameplayTag(FGameplayTag Tag)
{
	const TArray<FGameplayTagCounter>& TagCounters = HasAuthority() ? AuthorityGameplayTags.Tags : LocalGameplayTags;
	return TagCounters.ContainsByPredicate([Tag](const FGameplayTagCounter& TagCounter)
	{
		return TagCounter.GameplayTag.MatchesTagExact(Tag);
	});
}

bool USimpleAttributeComponent::HasAllGameplayTags(FGameplayTagContainer Tags)
{
	const TArray<FGameplayTagCounter>& TagCounters = HasAuthority() ? AuthorityGameplayTags.Tags : LocalGameplayTags;

	for (const FGameplayTag& Tag : Tags)
	{
		if (!TagCounters.ContainsByPredicate([Tag](const FGameplayTagCounter& TagCounter)
		{
			return TagCounter.GameplayTag.MatchesTagExact(Tag);
		}))
		{
			return false;
		}
	}

	return true;
}

bool USimpleAttributeComponent::HasAnyGameplayTags(FGameplayTagContainer Tags)
{
	const TArray<FGameplayTagCounter>& TagCounters = HasAuthority() ? AuthorityGameplayTags.Tags : LocalGameplayTags;

	for (const FGameplayTag& Tag : Tags)
	{
		if (TagCounters.ContainsByPredicate([Tag](const FGameplayTagCounter& TagCounter)
		{
			return TagCounter.GameplayTag.MatchesTagExact(Tag);
		}))
		{
			return true;
		}
	}

	return false;
}

FGameplayTagContainer USimpleAttributeComponent::GetActiveGameplayTags() const
{
	FGameplayTagContainer ActiveGameplayTags;

	const TArray<FGameplayTagCounter>& TagCounters = HasAuthority() ? AuthorityGameplayTags.Tags : LocalGameplayTags;
	for (const FGameplayTagCounter& TagCounter : TagCounters)
	{
		if (TagCounter.ReferenceCounter > 0)
		{
			ActiveGameplayTags.AddTag(TagCounter.GameplayTag);
		}
	}

	return ActiveGameplayTags;
}

/* Float Attributes */

void USimpleAttributeComponent::AddFloatAttribute(FFloatAttribute AttributeToAdd, bool OverrideValuesIfExists)
{
	if (!HasAuthority())
	{
		SIMPLE_LOG(this, TEXT("[USimpleAttributeComponent::AddFloatAttribute]: Cannot add float attribute on client!"));
		return;
	}

	for (FFloatAttribute& AuthorityAttribute : AuthorityFloatAttributes.Attributes)
	{
		if (AuthorityAttribute.AttributeTag.MatchesTagExact(AttributeToAdd.AttributeTag))
		{
			// Attribute exists but we don't want to override it
			if (!OverrideValuesIfExists)
			{
				return;
			}

			// Attribute exists and we want to override it
			AuthorityAttribute = AttributeToAdd;
			AuthorityFloatAttributes.MarkItemDirty(AuthorityAttribute);
			return;
		}
	}

	AuthorityFloatAttributes.Attributes.Add(AttributeToAdd);
	AuthorityFloatAttributes.MarkItemDirty(AttributeToAdd);

	OnFloatAttributeAdded.Broadcast(AttributeToAdd.AttributeTag, AttributeToAdd);
}

void USimpleAttributeComponent::RemoveFloatAttribute(FGameplayTag AttributeTag)
{
	if (!HasAuthority())
	{
		SIMPLE_LOG(this, TEXT("[USimpleAttributeComponent::RemoveFloatAttribute]: Cannot remove float attribute on client!"));
		return;
	}

	AuthorityFloatAttributes.Attributes.RemoveAll([AttributeTag](const FFloatAttribute& Attribute)
	{
		return Attribute.AttributeTag == AttributeTag;
	});
	AuthorityFloatAttributes.MarkArrayDirty();
	OnFloatAttributeRemoved.Broadcast(AttributeTag);
}

bool USimpleAttributeComponent::HasFloatAttribute(const FGameplayTag AttributeTag)
{
	if (GetFloatAttribute(AttributeTag))
	{
		return true;
	}

	return false;
}

bool USimpleAttributeComponent::HasStructAttribute(const FGameplayTag AttributeTag)
{
	if (GetStructAttribute(AttributeTag))
	{
		return true;
	}

	return false;
}

float USimpleAttributeComponent::GetFloatAttributeValue(EFloatAttributeValueType ValueType, FGameplayTag AttributeTag,
                                                        bool& WasFound)
{
	if (const FFloatAttribute* Attribute = GetFloatAttribute(AttributeTag))
	{
		WasFound = true;

		switch (ValueType)
		{
			case EFloatAttributeValueType::BaseValue:
				return Attribute->BaseValue;
			case EFloatAttributeValueType::CurrentValue:
				return Attribute->CurrentValue;
			case EFloatAttributeValueType::MaxCurrentValue:
				return Attribute->ValueLimits.MaxCurrentValue;
			case EFloatAttributeValueType::MinCurrentValue:
				return Attribute->ValueLimits.MinCurrentValue;
			case EFloatAttributeValueType::MaxBaseValue:
				return Attribute->ValueLimits.MaxBaseValue;
			case EFloatAttributeValueType::MinBaseValue:
				return Attribute->ValueLimits.MinBaseValue;
			default:
				SIMPLE_LOG(this, FString::Printf(
					           TEXT(
						           "[USimpleAttributeFunctionLibrary::GetFloatAttributeValue]: ValueType %d not supported."),
					           static_cast<int32>(ValueType)));
				WasFound = false;
				return 0.0f;
		}
	}

	WasFound = false;
	return 0.0f;
}

bool USimpleAttributeComponent::SetFloatAttributeValue(EFloatAttributeValueType ValueType, FGameplayTag AttributeTag,
                                                       float NewValue, float& Overflow)
{
	FFloatAttribute* Attribute = GetFloatAttribute(AttributeTag);

	if (!Attribute)
	{
		SIMPLE_LOG(this, FString::Printf(
			           TEXT("[USimpleAttributeFunctionLibrary::SetFloatAttributeValue]: Attribute %s not found."),
			           *AttributeTag.ToString()));
		return false;
	}

	float OldValue = 0.0f;
	const float ClampedValue = ClampFloatAttributeValue(*Attribute, ValueType, NewValue, Overflow);

	switch (ValueType)
	{
		case EFloatAttributeValueType::BaseValue:
			OldValue = Attribute->BaseValue;
			Attribute->BaseValue = ClampedValue;
			if (OldValue != ClampedValue)
			{
				OnFloatAttributeBaseValueChanged.Broadcast(AttributeTag, OldValue, ClampedValue);
			}
			// Broadcast limit reached events if the value reached or exceeded a limit
			if (Attribute->ValueLimits.UseMaxBaseValue && ClampedValue == Attribute->ValueLimits.MaxBaseValue && OldValue < ClampedValue)
			{
				OnMaxBaseValueReached.Broadcast(AttributeTag, Overflow);
			}
			else if (Attribute->ValueLimits.UseMinBaseValue && ClampedValue == Attribute->ValueLimits.MinBaseValue && OldValue > ClampedValue)
			{
				OnMinBaseValueReached.Broadcast(AttributeTag, Overflow);
			}
			break;

		case EFloatAttributeValueType::CurrentValue:
			OldValue = Attribute->CurrentValue;
			Attribute->CurrentValue = ClampedValue;
			if (OldValue != ClampedValue)
			{
				OnFloatAttributeCurrentValueChanged.Broadcast(AttributeTag, OldValue, ClampedValue);
			}
			// Broadcast limit reached events if the value reached or exceeded a limit
			if (Attribute->ValueLimits.UseMaxCurrentValue && ClampedValue == Attribute->ValueLimits.MaxCurrentValue && OldValue < ClampedValue)
			{
				OnMaxCurrentValueReached.Broadcast(AttributeTag, Overflow);
			}
			else if (Attribute->ValueLimits.UseMinCurrentValue && ClampedValue == Attribute->ValueLimits.MinCurrentValue && OldValue > ClampedValue)
			{
				OnMinCurrentValueReached.Broadcast(AttributeTag, Overflow);
			}
			break;

		case EFloatAttributeValueType::MaxCurrentValue:
			OldValue = Attribute->ValueLimits.MaxCurrentValue;
			Attribute->ValueLimits.MaxCurrentValue = ClampedValue;
			if (OldValue != ClampedValue)
			{
				OnFloatAttributeMaxCurrentValueChanged.Broadcast(AttributeTag, OldValue, ClampedValue);
			}
			break;

		case EFloatAttributeValueType::MinCurrentValue:
			OldValue = Attribute->ValueLimits.MinCurrentValue;
			Attribute->ValueLimits.MinCurrentValue = ClampedValue;
			if (OldValue != ClampedValue)
			{
				OnFloatAttributeMinCurrentValueChanged.Broadcast(AttributeTag, OldValue, ClampedValue);
			}
			break;

		case EFloatAttributeValueType::MaxBaseValue:
			OldValue = Attribute->ValueLimits.MaxBaseValue;
			Attribute->ValueLimits.MaxBaseValue = ClampedValue;
			if (OldValue != ClampedValue)
			{
				OnFloatAttributeMaxBaseValueChanged.Broadcast(AttributeTag, OldValue, ClampedValue);
			}
			break;

		case EFloatAttributeValueType::MinBaseValue:
			OldValue = Attribute->ValueLimits.MinBaseValue;
			Attribute->ValueLimits.MinBaseValue = ClampedValue;
			if (OldValue != ClampedValue)
			{
				OnFloatAttributeMinBaseValueChanged.Broadcast(AttributeTag, OldValue, ClampedValue);
			}
			break;
	}

	if (HasAuthority())
	{
		AuthorityFloatAttributes.MarkItemDirty(*Attribute);
	}

	return true;
}

bool USimpleAttributeComponent::IncrementFloatAttributeValue(EFloatAttributeValueType ValueType,
                                                             FGameplayTag AttributeTag, float Increment,
                                                             float& Overflow)
{
	bool WasFound = false;
	const float CurrentValue = GetFloatAttributeValue(ValueType, AttributeTag, WasFound);

	if (!WasFound)
	{
		return false;
	}

	return SetFloatAttributeValue(ValueType, AttributeTag, CurrentValue + Increment, Overflow);
}

bool USimpleAttributeComponent::OverrideFloatAttribute(FGameplayTag AttributeTag, FFloatAttribute NewAttribute)
{
	if (!HasAuthority())
	{
		SIMPLE_LOG(this, TEXT("[USimpleAttributeComponent::OverrideFloatAttribute]: Cannot override float attribute on client!"));
		return false;
	}

	for (FFloatAttribute& Attribute : AuthorityFloatAttributes.Attributes)
	{
		if (Attribute.AttributeTag.MatchesTagExact(AttributeTag))
		{
			Attribute.AttributeName = NewAttribute.AttributeName;
			Attribute.AttributeTag = NewAttribute.AttributeTag;
			Attribute.BaseValue = NewAttribute.BaseValue;
			Attribute.CurrentValue = NewAttribute.CurrentValue;
			Attribute.ValueLimits = NewAttribute.ValueLimits;

			AuthorityFloatAttributes.MarkItemDirty(Attribute);

			return true;
		}
	}

	SIMPLE_LOG(this, FString::Printf(
		           TEXT("[USimpleAttributeFunctionLibrary::OverrideFloatAttribute]: Attribute %s not found."),
		           *AttributeTag.ToString()));
	return false;
}

float USimpleAttributeComponent::ClampFloatAttributeValue(const FFloatAttribute& Attribute,
                                                          EFloatAttributeValueType ValueType, float NewValue,
                                                          float& Overflow)
{
	switch (ValueType)
	{
	case EFloatAttributeValueType::BaseValue:
		if (Attribute.ValueLimits.UseMaxBaseValue && NewValue > Attribute.ValueLimits.MaxBaseValue)
		{
			Overflow = NewValue - Attribute.ValueLimits.MaxBaseValue;
			return Attribute.ValueLimits.MaxBaseValue;
		}

		if (Attribute.ValueLimits.UseMinBaseValue && NewValue < Attribute.ValueLimits.MinBaseValue)
		{
			Overflow = NewValue - Attribute.ValueLimits.MinBaseValue;
			return Attribute.ValueLimits.MinBaseValue;
		}

		return NewValue;

	case EFloatAttributeValueType::CurrentValue:
		if (Attribute.ValueLimits.UseMaxCurrentValue && NewValue > Attribute.ValueLimits.MaxCurrentValue)
		{
			Overflow = NewValue - Attribute.ValueLimits.MaxCurrentValue;
			return Attribute.ValueLimits.MaxCurrentValue;
		}

		if (Attribute.ValueLimits.UseMinCurrentValue && NewValue < Attribute.ValueLimits.MinCurrentValue)
		{
			Overflow = NewValue - Attribute.ValueLimits.MinCurrentValue;
			return Attribute.ValueLimits.MinCurrentValue;
		}

		return NewValue;

	default:
		SIMPLE_LOG(this, TEXT("[USimpleGameplayAbilityComponent::ClampFloatAttributeValue]: ValueType not supported."));
		return 0.0f;
	}
}

FFloatAttribute* USimpleAttributeComponent::GetFloatAttribute(FGameplayTag AttributeTag)
{
	TArray<FFloatAttribute>& FloatAttributes = HasAuthority() ? AuthorityFloatAttributes.Attributes : LocalFloatAttributes;

	for (FFloatAttribute& FloatAttribute : FloatAttributes)
	{
		if (FloatAttribute.AttributeTag.MatchesTagExact(AttributeTag))
		{
			return &FloatAttribute;
		}
	}

	return nullptr;
}

FStructAttribute* USimpleAttributeComponent::GetStructAttribute(FGameplayTag AttributeTag)
{
	TArray<FStructAttribute>& StructAttributes = HasAuthority() ? AuthorityStructAttributes.Attributes : LocalStructAttributes;

	for (FStructAttribute& StructAttribute : StructAttributes)
	{
		if (StructAttribute.AttributeTag.MatchesTagExact(AttributeTag))
		{
			return &StructAttribute;
		}
	}

	return nullptr;
}

/* Struct Attributes */

void USimpleAttributeComponent::AddStructAttribute(FStructAttribute AttributeToAdd, bool OverrideValuesIfExists)
{
	if (!HasAuthority())
	{
		SIMPLE_LOG(this, TEXT("[USimpleAttributeComponent::AddStructAttribute]: Cannot add struct attribute on client!"));
		return;
	}

	if (!AttributeToAdd.StructType)
	{
		SIMPLE_LOG(this, FString::Printf(
			           TEXT(
				           "[USimpleGameplayAbilityComponent::AddStructAttribute]: StructType is null for attribute %s! Can't add new attribute"),
			           *AttributeToAdd.AttributeTag.ToString()));
		return;
	}

	const int32 AttributeIndex = AuthorityStructAttributes.Attributes.Find(AttributeToAdd);

	// This is a new attribute
	if (AttributeIndex == INDEX_NONE)
	{
		// Initialise the data within the struct
		if (AttributeToAdd.StructType)
		{
			AttributeToAdd.AttributeValue.InitializeAs(AttributeToAdd.StructType);
		}

		AuthorityStructAttributes.Attributes.AddUnique(AttributeToAdd);
		AuthorityStructAttributes.MarkItemDirty(AttributeToAdd);
		OnStructAttributeAdded.Broadcast(AttributeToAdd.AttributeTag, AttributeToAdd);

		return;
	}

	// Attribute exists but we don't want to override it
	if (!OverrideValuesIfExists)
	{
		return;
	}

	// Attribute exists and we want to override it
	AuthorityStructAttributes.Attributes[AttributeIndex] = AttributeToAdd;
	AuthorityStructAttributes.MarkItemDirty(AuthorityStructAttributes.Attributes[AttributeIndex]);
	OnStructAttributeAdded.Broadcast(AttributeToAdd.AttributeTag, AttributeToAdd);
}

void USimpleAttributeComponent::RemoveStructAttribute(FGameplayTag AttributeTag)
{
	if (!HasAuthority())
	{
		SIMPLE_LOG(this, TEXT("[USimpleAttributeComponent::RemoveStructAttribute]: Cannot remove struct attribute on client!"));
		return;
	}

	AuthorityStructAttributes.Attributes.RemoveAll([AttributeTag](const FStructAttribute& Attribute)
	{
		return Attribute.AttributeTag == AttributeTag;
	});
	AuthorityStructAttributes.MarkArrayDirty();
	OnStructAttributeRemoved.Broadcast(AttributeTag);
}

FInstancedStruct USimpleAttributeComponent::GetStructAttributeValue(FGameplayTag AttributeTag, bool& WasFound)
{
	if (FStructAttribute* Attribute = GetStructAttribute(AttributeTag))
	{
		WasFound = true;
		return Attribute->AttributeValue;
	}

	SIMPLE_LOG(this, FString::Printf(
		           TEXT("[USimpleAttributeFunctionLibrary::GetStructAttributeValue]: Attribute %s not found."),
		           *AttributeTag.ToString()));
	WasFound = false;
	return FInstancedStruct();
}

bool USimpleAttributeComponent::SetStructAttributeValue(FGameplayTag AttributeTag, FInstancedStruct NewValue)
{
	FStructAttribute* Attribute = GetStructAttribute(AttributeTag);

	if (!Attribute)
	{
		SIMPLE_LOG(this, FString::Printf(
			           TEXT("[USimpleAttributeFunctionLibrary::SetStructAttributeValue]: Attribute %s not found."),
			           *AttributeTag.ToString()));
		return false;
	}

	if (NewValue.GetScriptStruct() != Attribute->StructType)
	{
		UE_LOG(LogSimpleGAS, Warning,
		       TEXT(
			       "[USimpleAttributeComponent::SetStructAttributeValue]: NewValue does not match expected struct type of %s. It's of type %s"
		       ), *Attribute->StructType->GetName(), *NewValue.GetScriptStruct()->GetName());
		return false;
	}

	const FInstancedStruct OldValue = Attribute->AttributeValue;
	FGameplayTagContainer ModificationTags = FGameplayTagContainer();

	if (Attribute->StructAttributeHandler)
	{
		ModificationTags = GetStructAttributeHandlerInstance(AttributeTag, Attribute->StructAttributeHandler)->
			GetModificationEvents(OldValue, NewValue);
	}

	Attribute->AttributeValue = NewValue;

	if (HasAuthority())
	{
		AuthorityStructAttributes.MarkItemDirty(*Attribute);
	}

	if (OldValue != NewValue)
	{
		OnStructAttributeChanged.Broadcast(AttributeTag, OldValue, NewValue, ModificationTags);
	}

	return true;
}

/* Attribute Modifiers */

bool USimpleAttributeComponent::ApplyAttributeModifierToTarget(
	FGuid& ModifierID,
	TSubclassOf<USimpleAttributeModifier> ModifierClass,
	USimpleAttributeComponent* ModifierTarget,
	const float Magnitude,
	FInstancedStruct ModifierContext)
{
	ModifierID = FGuid::NewGuid();
	USimpleAttributeModifier* Modifier = GetAttributeModifierInstance(
		ModifierClass,
		ModifierID,
		this,
		ModifierTarget,
		Magnitude,
		ModifierContext,
		false);
	
	return Modifier->ApplyModifier();
}

bool USimpleAttributeComponent::ApplyAttributeModifierToTargetPredicted(
	FGuid& ModifierID,
    const TSubclassOf<USimpleAttributeModifier> ModifierClass,
    USimpleAttributeComponent* ModifierTarget,
    const float Magnitude,
    const FInstancedStruct ModifierContext)
{
	ModifierID = FGuid::NewGuid();

	// Capture snapshot before applying if this is a client prediction
	if (!HasAuthority() && ModifierTarget)
	{
		FPredictedModifierSnapshot Snapshot;
		Snapshot.ModifierID = ModifierID;
		Snapshot.AttributeSnapshot = ModifierTarget->CaptureAttributeSnapshot();
		Snapshot.SnapshotTimestamp = GetServerTime();
		ModifierTarget->PredictedModifierSnapshots.Add(Snapshot);
	}

	USimpleAttributeModifier* Modifier = GetAttributeModifierInstance(
		ModifierClass,
		ModifierID,
		this,
		ModifierTarget,
		Magnitude,
		ModifierContext,
		true);

	const bool WasApplied = Modifier->ApplyModifier();

	if (!HasAuthority())
	{
		if (WasApplied)
		{
			ServerApplyAttributeModifierToTarget(ModifierID, ModifierClass, ModifierTarget, Magnitude, ModifierContext);
		}
		else
		{
			// Application failed on client, remove the snapshot
			if (ModifierTarget)
			{
				ModifierTarget->PredictedModifierSnapshots.RemoveAll([ModifierID](const FPredictedModifierSnapshot& Snapshot)
				{
					return Snapshot.ModifierID == ModifierID;
				});
			}
		}
	}

	return WasApplied;
}

void USimpleAttributeComponent::ApplyAttributeModifierToTargetServerInitiated(FGuid& ModifierID,
	const TSubclassOf<USimpleAttributeModifier> ModifierClass, USimpleAttributeComponent* ModifierTarget, float Magnitude,
	FInstancedStruct ModifierContext)
{
	ModifierID = FGuid::NewGuid();
	ServerApplyAttributeModifierToTarget(ModifierID, ModifierClass, ModifierTarget, Magnitude, ModifierContext);
}

void USimpleAttributeComponent::ServerApplyAttributeModifierToTarget_Implementation(
	const FGuid ModifierID,
	const TSubclassOf<USimpleAttributeModifier> ModifierClass,
	USimpleAttributeComponent* ModifierTarget,
	const float Magnitude,
	const FInstancedStruct ModifierContext)
{
	USimpleAttributeModifier* Modifier = GetAttributeModifierInstance(
		ModifierClass,
		ModifierID,
		this,
		ModifierTarget,
		Magnitude,
		ModifierContext,
		true);
	
	Modifier->ApplyModifier();
}

bool USimpleAttributeComponent::ApplyAttributeModifierToSelf(
	FGuid& ModifierID,
	const TSubclassOf<USimpleAttributeModifier> ModifierClass,
	const float Magnitude, const FInstancedStruct ModifierContext)
{
	ModifierID = FGuid::NewGuid();
	USimpleAttributeModifier* Modifier = GetAttributeModifierInstance(
		ModifierClass,
		ModifierID,
		this,
		this,
		Magnitude,
		ModifierContext,
		false);
	
	return Modifier->ApplyModifier();
}

bool USimpleAttributeComponent::ApplyAttributeModifierToSelfPredicted(
	FGuid& ModifierID,
	const TSubclassOf<USimpleAttributeModifier> ModifierClass,
	const float Magnitude,
	const FInstancedStruct ModifierContext)
{
	ModifierID = FGuid::NewGuid();

	// Capture snapshot before applying if this is a client prediction
	if (!HasAuthority())
	{
		FPredictedModifierSnapshot Snapshot;
		Snapshot.ModifierID = ModifierID;
		Snapshot.AttributeSnapshot = CaptureAttributeSnapshot();
		Snapshot.SnapshotTimestamp = GetServerTime();
		PredictedModifierSnapshots.Add(Snapshot);
	}

	USimpleAttributeModifier* Modifier = GetAttributeModifierInstance(
		ModifierClass,
		ModifierID,
		this,
		this,
		Magnitude,
		ModifierContext,
		true);

	const bool WasApplied = Modifier->ApplyModifier();

	if (!HasAuthority())
	{
		if (WasApplied)
		{
			ServerApplyAttributeModifierToTarget(ModifierID, ModifierClass, this, Magnitude, ModifierContext);
		}
		else
		{
			// Application failed on client, remove the snapshot
			PredictedModifierSnapshots.RemoveAll([ModifierID](const FPredictedModifierSnapshot& Snapshot)
			{
				return Snapshot.ModifierID == ModifierID;
			});
		}
	}

	return WasApplied;
}

void USimpleAttributeComponent::ApplyAttributeModifierToSelfServerInitiated(FGuid& ModifierID,
	const TSubclassOf<USimpleAttributeModifier> ModifierClass, const float Magnitude, const FInstancedStruct ModifierContext)
{
	ModifierID = FGuid::NewGuid();
	ServerApplyAttributeModifierToTarget(ModifierID, ModifierClass, this, Magnitude, ModifierContext);
}

void USimpleAttributeComponent::CancelAttributeModifier(const FGuid ModifierID)
{
	for (USimpleAttributeModifier* Modifier : InstancedAttributeModifiers)
	{
		if (Modifier && Modifier->ModifierID == ModifierID)
		{
			Modifier->CancelModifier(FDefaultTags::AttributeModifierCancelled(), FInstancedStruct());
			return;
		}
	}
}

void USimpleAttributeComponent::CancelAttributeModifierPredicted(const FGuid ModifierID)
{
	CancelAttributeModifier(ModifierID);

	if (!HasAuthority())
	{
		ServerCancelAttributeModifier(ModifierID);
	}
}

void USimpleAttributeComponent::CancelAttributeModifierServerInitiated(const FGuid ModifierID)
{
	if (!HasAuthority())
	{
		ServerCancelAttributeModifier(ModifierID);
		return;
	}

	CancelAttributeModifier(ModifierID);
}

void USimpleAttributeComponent::ServerCancelAttributeModifier_Implementation(const FGuid ModifierID)
{
	CancelAttributeModifier(ModifierID);
}

void USimpleAttributeComponent::CancelAttributeModifiersWithTags(const FGameplayTagContainer ModifierTags)
{
	for (USimpleAttributeModifier* Modifier : InstancedAttributeModifiers)
	{
		if (Modifier && Modifier->ModifierTags.HasAny(ModifierTags))
		{
			Modifier->CancelModifier(FDefaultTags::AttributeModifierCancelled(), FInstancedStruct());
		}
	}
}

void USimpleAttributeComponent::CancelAttributeModifiersWithTagsPredicted(const FGameplayTagContainer ModifierTags)
{
	CancelAttributeModifiersWithTags(ModifierTags);

	if (!HasAuthority())
	{
		ServerCancelAttributeModifiersWithTags(ModifierTags);
	}
}

void USimpleAttributeComponent::CancelAttributeModifiersWithTagsServerInitiated(const FGameplayTagContainer ModifierTags)
{
	if (!HasAuthority())
	{
		ServerCancelAttributeModifiersWithTags(ModifierTags);
		return;
	}

	CancelAttributeModifiersWithTags(ModifierTags);
}

void USimpleAttributeComponent::ServerCancelAttributeModifiersWithTags_Implementation(const FGameplayTagContainer ModifierTags)
{
	CancelAttributeModifiersWithTags(ModifierTags);
}

void USimpleAttributeComponent::CancelAttributeModifiersWithClass(TSubclassOf<USimpleAttributeModifier> ModifierClass)
{
	for (USimpleAttributeModifier* Modifier : InstancedAttributeModifiers)
	{
		if (Modifier && Modifier->IsActive && Modifier->IsA(ModifierClass))
		{
			Modifier->CancelModifier(FDefaultTags::AttributeModifierCancelled(), FInstancedStruct());
		}
	}
}

void USimpleAttributeComponent::CancelAttributeModifiersWithClassPredicted(TSubclassOf<USimpleAttributeModifier> ModifierClass)
{
	CancelAttributeModifiersWithClass(ModifierClass);

	if (!HasAuthority())
	{
		ServerCancelAttributeModifiersWithClass(ModifierClass);
	}
}

void USimpleAttributeComponent::CancelAttributeModifiersWithClassServerInitiated(TSubclassOf<USimpleAttributeModifier> ModifierClass)
{
	if (!HasAuthority())
	{
		ServerCancelAttributeModifiersWithClass(ModifierClass);
		return;
	}

	CancelAttributeModifiersWithClass(ModifierClass);
}

void USimpleAttributeComponent::ServerCancelAttributeModifiersWithClass_Implementation(TSubclassOf<USimpleAttributeModifier> ModifierClass)
{
	CancelAttributeModifiersWithClass(ModifierClass);
}

bool USimpleAttributeComponent::IsModifierWithTagsActive(const FGameplayTagContainer ModifierTags) const
{
	for (const USimpleAttributeModifier* Modifier : InstancedAttributeModifiers)
	{
		if (Modifier && Modifier->IsActive && Modifier->ModifierTags.HasAny(ModifierTags))
		{
			return true;
		}
	}

	return false;
}

USimpleAttributeModifier* USimpleAttributeComponent::GetAttributeModifierInstance(const TSubclassOf<USimpleAttributeModifier>& ModifierClass, const FGuid NewModifierID, USimpleAttributeComponent* Instigator, USimpleAttributeComponent* Target, const float Magnitude, const FInstancedStruct Context, const bool DoesReplicate)
{
	if (!ModifierClass)
	{
		SIMPLE_LOG(this, TEXT("[USimpleGameplayAbilityComponent::GetAttributeModifierInstance]: ModifierClass is null!"));
		return nullptr;
	}

	// Check if an instance with this ModifierID already exists (e.g., from client prediction)
	for (USimpleAttributeModifier* ExistingModifier : InstancedAttributeModifiers)
	{
		if (ExistingModifier && ExistingModifier->ModifierID == NewModifierID)
		{
			// Found existing instance with same ID (likely predicted on client)
			// Don't reinitialize if already active to preserve state like ActivationTime
			if (!ExistingModifier->IsActive)
			{
				ExistingModifier->InitializeModifier(NewModifierID, Instigator, Target, Magnitude, Context, DoesReplicate);
			}
			return ExistingModifier;
		}
	}

	// No existing instance found, create new one
	USimpleAttributeModifier* ModifierInstance = NewObject<USimpleAttributeModifier>(this, ModifierClass);
	ModifierInstance->OnModifierApplied.AddDynamic(this, &USimpleAttributeComponent::OnAttributeModifierInitiallyApplied);
	ModifierInstance->OnActionStackApplied.AddDynamic(this, &USimpleAttributeComponent::OnAttributeModifierActionStackApplied);
	ModifierInstance->OnAttributeModifierEnded.AddDynamic(this, &USimpleAttributeComponent::OnAttributeModifierEnded);
	ModifierInstance->OnAttributeModifierCancelled.AddDynamic(this, &USimpleAttributeComponent::OnAttributeModifierCancelled);
	InstancedAttributeModifiers.Add(ModifierInstance);

	ModifierInstance->InitializeModifier(NewModifierID, Instigator, Target, Magnitude, Context, DoesReplicate);
	return ModifierInstance;
}

/* Attribute Modifier State Callbacks */

void USimpleAttributeComponent::OnAttributeModifierInitiallyApplied(USimpleAttributeModifier* ModifierInstance)
{
	FAttributeModifierState NewState;
	NewState.ModifierID = ModifierInstance->ModifierID;
	NewState.ModifierContext = FInstancedStruct();
	NewState.ModifierMagnitude = ModifierInstance->ModifierMagnitude;
	NewState.InstigatorAttributeComponent = ModifierInstance->InstigatorAttributeComponent;
	NewState.TargetAttributeComponent = ModifierInstance->TargetAttributeComponent;
	NewState.ModifierClass = ModifierInstance->GetClass();
	NewState.ModifierStatus = EModifierStatus::Applied;
	NewState.ApplicationTimestamp = GetServerTime();

	if (!HasAuthority())
	{
		LocalAttributeModifierStates.Add(NewState);
		return;
	}

	AuthorityAttributeModifierStates.ModifierStates.Add(NewState);
	AuthorityAttributeModifierStates.MarkItemDirty(NewState);
}

void USimpleAttributeComponent::OnAttributeModifierEnded(USimpleAttributeModifier* ModifierInstance, FGameplayTag EndStatus, FInstancedStruct EndContext)
{
	if (HasAuthority())
	{
		for (FAttributeModifierState& State : AuthorityAttributeModifierStates.ModifierStates)
		{
			if (State.ModifierID == ModifierInstance->ModifierID)
			{
				State.ModifierStatus = EModifierStatus::Ended;
				State.EndedTimestamp = GetServerTime();
				AuthorityAttributeModifierStates.MarkItemDirty(State);
				InstancedAttributeModifiers.Remove(ModifierInstance);
				return;
			}
		}
	}
	else
	{
		for (FAttributeModifierState& State : LocalAttributeModifierStates)
		{
			if (State.ModifierID == ModifierInstance->ModifierID)
			{
				State.ModifierStatus = EModifierStatus::Ended;
				State.EndedTimestamp = GetServerTime();
				InstancedAttributeModifiers.Remove(ModifierInstance);
				return;
			}
		}
	}

	SIMPLE_LOG(this, FString::Printf(
		TEXT("[USimpleAttributeComponent::OnAttributeModifierEnded]: Modifier with ID %s not found in ModifierStates array"),
		*ModifierInstance->ModifierID.ToString()));
}

void USimpleAttributeComponent::OnAttributeModifierCancelled(USimpleAttributeModifier* ModifierInstance, FGameplayTag EndStatus, FInstancedStruct EndContext)
{
	if (HasAuthority())
	{
		for (FAttributeModifierState& State : AuthorityAttributeModifierStates.ModifierStates)
		{
			if (State.ModifierID == ModifierInstance->ModifierID)
			{
				State.ModifierStatus = EModifierStatus::Cancelled;
				State.EndedTimestamp = GetServerTime();
				AuthorityAttributeModifierStates.MarkItemDirty(State);
				InstancedAttributeModifiers.Remove(ModifierInstance);
				return;
			}
		}
	}
	else
	{
		for (FAttributeModifierState& State : LocalAttributeModifierStates)
		{
			if (State.ModifierID == ModifierInstance->ModifierID)
			{
				State.ModifierStatus = EModifierStatus::Cancelled;
				State.EndedTimestamp = GetServerTime();
				InstancedAttributeModifiers.Remove(ModifierInstance);
				return;
			}
		}
	}

	SIMPLE_LOG(this, FString::Printf(
		TEXT("[USimpleAttributeComponent::OnAttributeModifierCancelled]: Modifier with ID %s not found in ModifierStates array"),
		*ModifierInstance->ModifierID.ToString()));
}

// Modifier Mutation Addition
void USimpleAttributeComponent::OnAttributeModifierActionStackApplied(USimpleAttributeModifier* ModifierInstance, FModifierActionStackResults ActionResult)
{
	FAttributeModifierMutation NewMutation;
	NewMutation.ModifierID = ModifierInstance->ModifierID;
	NewMutation.ModifierClass = ActionResult.ModifierClass;
	NewMutation.ActionStackResult = ActionResult;
	NewMutation.MutationTimestamp = GetServerTime();

	if (!HasAuthority())
	{
		// Find the highest mutation counter for this modifier and increment
		int32 HighestCounter = 0;
		for (const FAttributeModifierMutation& Mutation : LocalAttributeModiferMutations)
		{
			if (Mutation.ModifierID == ModifierInstance->ModifierID && Mutation.MutationCounter > HighestCounter)
			{
				HighestCounter = Mutation.MutationCounter;
			}
		}
		NewMutation.MutationCounter = HighestCounter + 1;

		LocalAttributeModiferMutations.Add(NewMutation);
		return;
	}

	// Find the highest mutation counter for this modifier and increment
	int32 HighestCounter = 0;
	for (const FAttributeModifierMutation& Mutation : AuthorityAttributeModifierMutations.Mutations)
	{
		if (Mutation.ModifierID == ModifierInstance->ModifierID && Mutation.MutationCounter > HighestCounter)
		{
			HighestCounter = Mutation.MutationCounter;
		}
	}
	NewMutation.MutationCounter = HighestCounter + 1;

	FAttributeModifierMutation& AddedRef = AuthorityAttributeModifierMutations.Mutations.Add_GetRef(NewMutation);
	AuthorityAttributeModifierMutations.MarkItemDirty(AddedRef);
}

/* Utility */

bool USimpleAttributeComponent::HasAuthority() const
{
	if (GetOwner())
	{
		return GetOwner()->HasAuthority();
	}

	return false;
}

USimpleTimeSynchronizer* USimpleAttributeComponent::GetTimeSynchronizerComponent_Implementation()
{
	// Default to assuming the owner actor has a time synchronizer component
	return GetOwner()->GetComponentByClass<USimpleTimeSynchronizerExtended>();
}

double USimpleAttributeComponent::GetServerTime()
{
	if (GetTimeSynchronizerComponent())
	{
		return GetTimeSynchronizerComponent()->GetServerTime();
	}

	return GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
}

USimpleAttributeHandler* USimpleAttributeComponent::GetAttributeHandler(const FGameplayTag AttributeTag, const TSubclassOf<USimpleAttributeHandler> AttributeHandlerClass)
{
	return GetStructAttributeHandlerInstance(AttributeTag, AttributeHandlerClass);
}

USimpleAttributeHandler* USimpleAttributeComponent::GetStructAttributeHandlerInstance(const FGameplayTag AttributeTag, const TSubclassOf<USimpleAttributeHandler> HandlerClass)
{
	if (!HasStructAttribute(AttributeTag))
	{
		SIMPLE_LOG(this, FString::Printf(
					   TEXT(
						   "[USimpleGameplayAbilityComponent::GetStructAttributeHandlerInstance]: Struct Attribute %s not found."),
					   *AttributeTag.ToString()));
		return nullptr;
	}

	USimpleAttributeHandler* HandlerInstance = nullptr;

	for (USimpleAttributeHandler* InstancedHandler : InstancedAttributeHandlers)
	{
		if (InstancedHandler->GetClass() == HandlerClass)
		{
			HandlerInstance = InstancedHandler;
			break;
		}
	}

	if (!HandlerInstance)
	{
		HandlerInstance = NewObject<USimpleAttributeHandler>(this, HandlerClass);
		InstancedAttributeHandlers.Add(HandlerInstance);
	}

	HandlerInstance->InitializeHandler(this, AttributeTag);

	return HandlerInstance;
}

/* Replication */

void USimpleAttributeComponent::ClientOnAttributeModiferStateAdded(const FAttributeModifierState& NewAttributeModiferState)
{
	// A mapping of the local modifier states for quick lookups
	TMap<FGuid, int32> LocalAttributeModifierArrayIndexMap;

	for (int32 i = 0; i < LocalAttributeModifierStates.Num(); i++)
	{
		LocalAttributeModifierArrayIndexMap.Add(LocalAttributeModifierStates[i].ModifierID, i);
	}

	USimpleAttributeModifier* Modifier = GetAttributeModifierInstance(
						NewAttributeModiferState.ModifierClass,
						NewAttributeModiferState.ModifierID,
						NewAttributeModiferState.InstigatorAttributeComponent,
						NewAttributeModiferState.TargetAttributeComponent,
						NewAttributeModiferState.ModifierMagnitude,
						NewAttributeModiferState.ModifierContext,
						true);
	
	// Check if we already have this state to prevent double application
	if (LocalAttributeModifierArrayIndexMap.Contains(NewAttributeModiferState.ModifierID))
	{
		// State already exists, this should be handled by ClientOnAttributeModifierStateChanged
		return;
	}

	USimpleAttributeComponent* TargetComponent = NewAttributeModiferState.TargetAttributeComponent;

	switch (NewAttributeModiferState.ModifierStatus)
	{
		case EModifierStatus::Applied:
			// Only apply if not already active (could be client predicted)
			if (!Modifier->IsActive)
			{
				Modifier->ApplyModifier();
			}
			else
			{
				// Server confirmed our prediction, remove the snapshot
				if (TargetComponent)
				{
					TargetComponent->PredictedModifierSnapshots.RemoveAll([NewAttributeModiferState](const FPredictedModifierSnapshot& Snapshot)
					{
						return Snapshot.ModifierID == NewAttributeModiferState.ModifierID;
					});
				}
			}
			LocalAttributeModifierStates.Add(NewAttributeModiferState);
			break;

		case EModifierStatus::Cancelled:
			// Server rejected the modifier, rollback using snapshot
			if (TargetComponent)
			{
				const FPredictedModifierSnapshot* Snapshot = TargetComponent->PredictedModifierSnapshots.FindByPredicate(
					[NewAttributeModiferState](const FPredictedModifierSnapshot& S)
					{
						return S.ModifierID == NewAttributeModiferState.ModifierID;
					});

				if (Snapshot)
				{
					TargetComponent->RestoreAttributeSnapshot(Snapshot->AttributeSnapshot);
					TargetComponent->PredictedModifierSnapshots.RemoveAll([NewAttributeModiferState](const FPredictedModifierSnapshot& S)
					{
						return S.ModifierID == NewAttributeModiferState.ModifierID;
					});
				}
			}

			if (Modifier->IsActive)
			{
				Modifier->CancelModifier(FDefaultTags::AttributeModifierCancelled(), FInstancedStruct());
			}
			LocalAttributeModifierStates.Add(NewAttributeModiferState);
			break;

		case EModifierStatus::Ended:
			if (Modifier->IsActive)
			{
				Modifier->EndModifier(FDefaultTags::AttributeModifierEnded(), FInstancedStruct());
			}

			// Remove snapshot if any
			if (TargetComponent)
			{
				TargetComponent->PredictedModifierSnapshots.RemoveAll([NewAttributeModiferState](const FPredictedModifierSnapshot& Snapshot)
				{
					return Snapshot.ModifierID == NewAttributeModiferState.ModifierID;
				});
			}
			LocalAttributeModifierStates.Add(NewAttributeModiferState);
			break;
	}

	// Process any pending mutations for this modifier
	ProcessPendingMutations(NewAttributeModiferState.ModifierID);
}

void USimpleAttributeComponent::ClientOnAttributeModifierStateChanged(const FAttributeModifierState& ChangedAttributeModiferState)
{
	// A mapping of the local modifier states for quick lookups
	TMap<FGuid, int32> LocalAttributeModifierArrayIndexMap;

	for (int32 i = 0; i < LocalAttributeModifierStates.Num(); i++)
	{
		LocalAttributeModifierArrayIndexMap.Add(LocalAttributeModifierStates[i].ModifierID, i);
	}

	USimpleAttributeModifier* Modifier = GetAttributeModifierInstance(
						ChangedAttributeModiferState.ModifierClass,
						ChangedAttributeModiferState.ModifierID,
						ChangedAttributeModiferState.InstigatorAttributeComponent,
						ChangedAttributeModiferState.TargetAttributeComponent,
						ChangedAttributeModiferState.ModifierMagnitude,
						ChangedAttributeModiferState.ModifierContext,
						true);
	
	switch (ChangedAttributeModiferState.ModifierStatus)
	{
		case EModifierStatus::Applied:
			if (!LocalAttributeModifierArrayIndexMap.Contains(ChangedAttributeModiferState.ModifierID))
			{
				// Only apply if not already active (could be client predicted)
				if (!Modifier->IsActive)
				{
					Modifier->ApplyModifier();
				}

				LocalAttributeModifierStates.Add(ChangedAttributeModiferState);
				return;
			}

			LocalAttributeModifierStates[LocalAttributeModifierArrayIndexMap[ChangedAttributeModiferState.ModifierID]] = ChangedAttributeModiferState;
			break;
			
		case EModifierStatus::Cancelled:
			if (LocalAttributeModifierArrayIndexMap.Contains(ChangedAttributeModiferState.ModifierID))
			{
				Modifier->CancelModifier(FDefaultTags::AttributeModifierCancelled(),FInstancedStruct());
			}
			break;	

		case EModifierStatus::Ended:
			if (LocalAttributeModifierArrayIndexMap.Contains(ChangedAttributeModiferState.ModifierID))
			{
				Modifier->EndModifier(FDefaultTags::AttributeModifierEnded(),FInstancedStruct());
			}
			break;
	}
}

void USimpleAttributeComponent::ClientOnAttributeModiferStateRemoved(const FAttributeModifierState& RemovedAttributeModiferState)
{
	LocalAttributeModifierStates.RemoveAll([RemovedAttributeModiferState](const FAttributeModifierState& ModifierState)
	{
		return ModifierState.ModifierID == RemovedAttributeModiferState.ModifierID;
	});
}

void USimpleAttributeComponent::ClientOnAttributeModifierMutationAdded(const FAttributeModifierMutation& NewModifierMutation)
{
	if (!NewModifierMutation.ModifierClass)
	{
		SIMPLE_LOG(this, FString::Printf(
			TEXT("[USimpleAttributeComponent::ClientOnAttributeModifierMutationAdded]: ModifierClass is null for modifier ID %s"),
			*NewModifierMutation.ModifierID.ToString()));
		return;
	}

	// Get the local version of NewAttributeModifierSnapshot if it exists
	const FAttributeModifierMutation* PredictedMutation = LocalAttributeModiferMutations.FindByPredicate(
		[NewModifierMutation](const FAttributeModifierMutation& Mutation)
		{
			return Mutation.ModifierID == NewModifierMutation.ModifierID;
		});

	FModifierActionStackResults LocalActionStackResults;

	if (PredictedMutation)
	{
		LocalActionStackResults = PredictedMutation->ActionStackResult;
	}
	
	const FAttributeModifierState* AuthorityState = AuthorityAttributeModifierStates.ModifierStates.FindByPredicate(
		[NewModifierMutation](const FAttributeModifierState& State)
		{
			return State.ModifierID == NewModifierMutation.ModifierID;
		});

	if (!AuthorityState)
	{
		SIMPLE_LOG(this, FString::Printf(
			TEXT("[USimpleAttributeComponent::ClientOnAttributeModifierMutationAdded]: Modifier with ID %s not found in AuthorityAttributeModifierStates array. Queueing for later processing."),
			*NewModifierMutation.ModifierID.ToString()));

		// Queue this mutation to be processed when the state arrives
		PendingMutationQueue.Add(NewModifierMutation);
		return;
	}

	USimpleAttributeModifier* Modifier = GetAttributeModifierInstance(
		AuthorityState->ModifierClass,
		AuthorityState->ModifierID,
		AuthorityState->InstigatorAttributeComponent,
		AuthorityState->TargetAttributeComponent,
		AuthorityState->ModifierMagnitude,
		AuthorityState->ModifierContext,
		true);

	Modifier->OnClientReceivedServerActionsResult(NewModifierMutation.ActionStackResult, LocalActionStackResults);

	
	// Remove the local snapshot from the pending snapshots array now that we've resolved the differences
	if (PredictedMutation)
	{
		LocalAttributeModiferMutations.RemoveAll([PredictedMutation](const FAttributeModifierMutation& Snapshot)
		{
			return Snapshot.ModifierID == PredictedMutation->ModifierID && Snapshot.MutationCounter == PredictedMutation->MutationCounter;
		});
	}
}

void USimpleAttributeComponent::ClientOnFloatAttributeAdded(const FFloatAttribute& NewFloatAttribute)
{
	LocalFloatAttributes.AddUnique(NewFloatAttribute);
	OnFloatAttributeAdded.Broadcast(NewFloatAttribute.AttributeTag, NewFloatAttribute);
}

void USimpleAttributeComponent::ClientOnFloatAttributeChanged(const FFloatAttribute& ChangedFloatAttribute)
{
	FFloatAttribute* LocalFloatAttribute = GetFloatAttribute(ChangedFloatAttribute.AttributeTag);

	if (!LocalFloatAttribute)
	{
		LocalFloatAttributes.AddUnique(ChangedFloatAttribute);
		OnFloatAttributeAdded.Broadcast(ChangedFloatAttribute.AttributeTag, ChangedFloatAttribute);
		return;
	}

	float OldFloatValue;
	bool OldBoolValue;

	// Base value changed
	if (LocalFloatAttribute->BaseValue != ChangedFloatAttribute.BaseValue)
	{
		OldFloatValue = LocalFloatAttribute->BaseValue;
		LocalFloatAttribute->BaseValue = ChangedFloatAttribute.BaseValue;
		OnFloatAttributeBaseValueChanged.Broadcast(ChangedFloatAttribute.AttributeTag, OldFloatValue,
		                                           ChangedFloatAttribute.BaseValue);
	}

	// Current value changed
	if (LocalFloatAttribute->CurrentValue != ChangedFloatAttribute.CurrentValue)
	{
		OldFloatValue = LocalFloatAttribute->CurrentValue;
		LocalFloatAttribute->CurrentValue = ChangedFloatAttribute.CurrentValue;
		OnFloatAttributeCurrentValueChanged.Broadcast(ChangedFloatAttribute.AttributeTag, OldFloatValue,
		                                              ChangedFloatAttribute.CurrentValue);
	}

	// Use MinBaseValue changed
	if (LocalFloatAttribute->ValueLimits.UseMinBaseValue != ChangedFloatAttribute.ValueLimits.UseMinBaseValue)
	{
		OldBoolValue = LocalFloatAttribute->ValueLimits.UseMinBaseValue;
		LocalFloatAttribute->ValueLimits.UseMinBaseValue = ChangedFloatAttribute.ValueLimits.UseMinBaseValue;
		OnUseMinBaseValueChanged.Broadcast(ChangedFloatAttribute.AttributeTag, OldBoolValue,
		                                   ChangedFloatAttribute.ValueLimits.UseMinBaseValue);
	}

	// MinBaseValue changed
	if (LocalFloatAttribute->ValueLimits.MinBaseValue != ChangedFloatAttribute.ValueLimits.MinBaseValue)
	{
		OldFloatValue = LocalFloatAttribute->ValueLimits.MinBaseValue;
		LocalFloatAttribute->ValueLimits.MinBaseValue = ChangedFloatAttribute.ValueLimits.MinBaseValue;
		OnFloatAttributeMinBaseValueChanged.Broadcast(ChangedFloatAttribute.AttributeTag, OldFloatValue,
		                                              ChangedFloatAttribute.ValueLimits.MinBaseValue);
	}

	// Use MinCurrentValue changed
	if (LocalFloatAttribute->ValueLimits.UseMinCurrentValue != ChangedFloatAttribute.ValueLimits.UseMinCurrentValue)
	{
		OldBoolValue = LocalFloatAttribute->ValueLimits.UseMinCurrentValue;
		LocalFloatAttribute->ValueLimits.UseMinCurrentValue = ChangedFloatAttribute.ValueLimits.UseMinCurrentValue;
		OnUseMinCurrentValueChanged.Broadcast(ChangedFloatAttribute.AttributeTag, OldBoolValue,
		                                      ChangedFloatAttribute.ValueLimits.UseMinCurrentValue);
	}

	// MinCurrentValue changed
	if (LocalFloatAttribute->ValueLimits.MinCurrentValue != ChangedFloatAttribute.ValueLimits.MinCurrentValue)
	{
		OldFloatValue = LocalFloatAttribute->ValueLimits.MinCurrentValue;
		LocalFloatAttribute->ValueLimits.MinCurrentValue = ChangedFloatAttribute.ValueLimits.MinCurrentValue;
		OnFloatAttributeMinCurrentValueChanged.Broadcast(ChangedFloatAttribute.AttributeTag, OldFloatValue,
		                                                 ChangedFloatAttribute.ValueLimits.MinCurrentValue);
	}

	// Use MaxBaseValue changed
	if (LocalFloatAttribute->ValueLimits.UseMaxBaseValue != ChangedFloatAttribute.ValueLimits.UseMaxBaseValue)
	{
		OldBoolValue = LocalFloatAttribute->ValueLimits.UseMaxBaseValue;
		LocalFloatAttribute->ValueLimits.UseMaxBaseValue = ChangedFloatAttribute.ValueLimits.UseMaxBaseValue;
		OnUseMaxBaseValueChanged.Broadcast(ChangedFloatAttribute.AttributeTag, OldBoolValue,
		                                   ChangedFloatAttribute.ValueLimits.UseMaxBaseValue);
	}

	// MaxBaseValue changed
	if (LocalFloatAttribute->ValueLimits.MaxBaseValue != ChangedFloatAttribute.ValueLimits.MaxBaseValue)
	{
		OldFloatValue = LocalFloatAttribute->ValueLimits.MaxBaseValue;
		LocalFloatAttribute->ValueLimits.MaxBaseValue = ChangedFloatAttribute.ValueLimits.MaxBaseValue;
		OnFloatAttributeMaxBaseValueChanged.Broadcast(ChangedFloatAttribute.AttributeTag, OldFloatValue,
		                                              ChangedFloatAttribute.ValueLimits.MaxBaseValue);
	}

	// Use MaxCurrentValue changed
	if (LocalFloatAttribute->ValueLimits.UseMaxCurrentValue != ChangedFloatAttribute.ValueLimits.UseMaxCurrentValue)
	{
		OldBoolValue = LocalFloatAttribute->ValueLimits.UseMaxCurrentValue;
		LocalFloatAttribute->ValueLimits.UseMaxCurrentValue = ChangedFloatAttribute.ValueLimits.UseMaxCurrentValue;
		OnUseMaxCurrentValueChanged.Broadcast(ChangedFloatAttribute.AttributeTag, OldBoolValue,
		                                      ChangedFloatAttribute.ValueLimits.UseMaxCurrentValue);
	}

	// MaxCurrentValue changed
	if (LocalFloatAttribute->ValueLimits.MaxCurrentValue != ChangedFloatAttribute.ValueLimits.MaxCurrentValue)
	{
		OldFloatValue = LocalFloatAttribute->ValueLimits.MaxCurrentValue;
		LocalFloatAttribute->ValueLimits.MaxCurrentValue = ChangedFloatAttribute.ValueLimits.MaxCurrentValue;
		OnFloatAttributeMaxCurrentValueChanged.Broadcast(ChangedFloatAttribute.AttributeTag, OldFloatValue,
		                                                 ChangedFloatAttribute.ValueLimits.MaxCurrentValue);
	}
}

void USimpleAttributeComponent::ClientOnFloatAttributeRemoved(const FFloatAttribute& RemovedFloatAttribute)
{
	LocalFloatAttributes.Remove(RemovedFloatAttribute);
	OnFloatAttributeRemoved.Broadcast(RemovedFloatAttribute.AttributeTag);
}

void USimpleAttributeComponent::ClientOnStructAttributeAdded(const FStructAttribute& NewStructAttribute)
{
	LocalStructAttributes.AddUnique(NewStructAttribute);
	OnStructAttributeAdded.Broadcast(NewStructAttribute.AttributeTag, NewStructAttribute);
}

void USimpleAttributeComponent::ClientOnStructAttributeChanged(const FStructAttribute& ChangedStructAttribute)
{
	FStructAttribute* LocalStructAttribute = GetStructAttribute(ChangedStructAttribute.AttributeTag);

	if (!LocalStructAttribute)
	{
		LocalStructAttributes.Add(ChangedStructAttribute);
		OnStructAttributeAdded.Broadcast(ChangedStructAttribute.AttributeTag, ChangedStructAttribute);
		return;
	}

	if (LocalStructAttribute->AttributeValue == ChangedStructAttribute.AttributeValue)
	{
		return;
	}

	const FInstancedStruct OldValue = LocalStructAttribute->AttributeValue;
	FGameplayTagContainer ModificationTags = FGameplayTagContainer();
	LocalStructAttribute->AttributeValue = ChangedStructAttribute.AttributeValue;

	if (LocalStructAttribute->StructAttributeHandler)
	{
		ModificationTags = GetStructAttributeHandlerInstance(ChangedStructAttribute.AttributeTag,
		                                                     LocalStructAttribute->StructAttributeHandler)->
			GetModificationEvents(OldValue, ChangedStructAttribute.AttributeValue);
	}

	OnStructAttributeChanged.Broadcast(ChangedStructAttribute.AttributeTag, OldValue,
	                                   ChangedStructAttribute.AttributeValue, ModificationTags);
}

void USimpleAttributeComponent::ClientOnStructAttributeRemoved(const FStructAttribute& RemovedStructAttribute)
{
	LocalStructAttributes.Remove(RemovedStructAttribute);
	OnStructAttributeRemoved.Broadcast(RemovedStructAttribute.AttributeTag);
}

void USimpleAttributeComponent::ClientOnGameplayTagAdded(const FGameplayTagCounter& NewGameplayTag)
{
	if (LocalGameplayTags.Contains(NewGameplayTag))
	{
		return;
	}

	AddGameplayTag(NewGameplayTag.GameplayTag);
}

void USimpleAttributeComponent::ClientOnGameplayTagRemoved(const FGameplayTagCounter& RemovedGameplayTag)
{
	LocalGameplayTags.Remove(RemovedGameplayTag);
	OnGameplayTagRemoved.Broadcast(RemovedGameplayTag.GameplayTag);
}

/* Stack Group Query Functions */

TArray<USimpleAttributeModifier*> USimpleAttributeComponent::GetModifiersInStackGroup(FGameplayTag StackGroupTag) const
{
	TArray<USimpleAttributeModifier*> Result;

	if (!StackGroupTag.IsValid())
		return Result;

	for (USimpleAttributeModifier* Modifier : InstancedAttributeModifiers)
	{
		if (Modifier &&
			Modifier->IsActive &&
			Modifier->bUseStackGroup &&
			Modifier->StackGroupTag.MatchesTagExact(StackGroupTag))
		{
			Result.Add(Modifier);
		}
	}

	return Result;
}

int32 USimpleAttributeComponent::GetModifierStackCountInGroup(FGameplayTag StackGroupTag) const
{
	return GetModifiersInStackGroup(StackGroupTag).Num();
}

USimpleAttributeModifier* USimpleAttributeComponent::GetOldestModifierInGroup(FGameplayTag StackGroupTag) const
{
	TArray<USimpleAttributeModifier*> Modifiers = GetModifiersInStackGroup(StackGroupTag);

	if (Modifiers.Num() == 0)
		return nullptr;

	USimpleAttributeModifier* Oldest = Modifiers[0];
	for (USimpleAttributeModifier* Modifier : Modifiers)
	{
		if (Modifier->GetActivationTime() < Oldest->GetActivationTime())
			Oldest = Modifier;
	}

	return Oldest;
}

USimpleAttributeModifier* USimpleAttributeComponent::GetNewestModifierInGroup(FGameplayTag StackGroupTag) const
{
	TArray<USimpleAttributeModifier*> Modifiers = GetModifiersInStackGroup(StackGroupTag);

	if (Modifiers.Num() == 0)
		return nullptr;

	USimpleAttributeModifier* Newest = Modifiers[0];
	for (USimpleAttributeModifier* Modifier : Modifiers)
	{
		if (Modifier->GetActivationTime() > Newest->GetActivationTime())
			Newest = Modifier;
	}

	return Newest;
}

FAttributeSnapshot USimpleAttributeComponent::CaptureAttributeSnapshot() const
{
	FAttributeSnapshot Snapshot;
	Snapshot.FloatAttributes = HasAuthority() ? AuthorityFloatAttributes.Attributes : LocalFloatAttributes;
	Snapshot.StructAttributes = HasAuthority() ? AuthorityStructAttributes.Attributes : LocalStructAttributes;
	Snapshot.GameplayTags = HasAuthority() ? AuthorityGameplayTags.Tags : LocalGameplayTags;
	return Snapshot;
}

void USimpleAttributeComponent::RestoreAttributeSnapshot(const FAttributeSnapshot& Snapshot)
{
	if (HasAuthority())
	{
		SIMPLE_LOG(this, TEXT("[USimpleAttributeComponent::RestoreAttributeSnapshot]: Attempting to restore snapshot on server - this should only happen on clients!"));
		return;
	}

	// Restore float attributes
	for (const FFloatAttribute& SnapshotAttr : Snapshot.FloatAttributes)
	{
		FFloatAttribute* LocalAttr = GetFloatAttribute(SnapshotAttr.AttributeTag);
		if (LocalAttr)
		{
			const float OldBaseValue = LocalAttr->BaseValue;
			const float OldCurrentValue = LocalAttr->CurrentValue;

			*LocalAttr = SnapshotAttr;

			// Fire events for changes
			if (OldBaseValue != SnapshotAttr.BaseValue)
			{
				OnFloatAttributeBaseValueChanged.Broadcast(SnapshotAttr.AttributeTag, SnapshotAttr.BaseValue, OldBaseValue);
			}
			if (OldCurrentValue != SnapshotAttr.CurrentValue)
			{
				OnFloatAttributeCurrentValueChanged.Broadcast(SnapshotAttr.AttributeTag, SnapshotAttr.CurrentValue, OldCurrentValue);
			}
		}
	}

	// Restore struct attributes
	for (const FStructAttribute& SnapshotAttr : Snapshot.StructAttributes)
	{
		FStructAttribute* LocalAttr = GetStructAttribute(SnapshotAttr.AttributeTag);
		if (LocalAttr)
		{
			const FInstancedStruct OldValue = LocalAttr->AttributeValue;
			*LocalAttr = SnapshotAttr;

			FGameplayTagContainer ModificationTags;
			if (LocalAttr->StructAttributeHandler)
			{
				ModificationTags = GetStructAttributeHandlerInstance(SnapshotAttr.AttributeTag, LocalAttr->StructAttributeHandler)->
					GetModificationEvents(OldValue, SnapshotAttr.AttributeValue);
			}

			if (OldValue != SnapshotAttr.AttributeValue)
			{
				OnStructAttributeChanged.Broadcast(SnapshotAttr.AttributeTag, OldValue, SnapshotAttr.AttributeValue, ModificationTags);
			}
		}
	}

	// Restore gameplay tags
	LocalGameplayTags = Snapshot.GameplayTags;
}

void USimpleAttributeComponent::ProcessPendingMutations(FGuid ModifierID)
{
	TArray<FAttributeModifierMutation> MutationsToProcess;

	// Find all pending mutations for this modifier
	for (int32 i = PendingMutationQueue.Num() - 1; i >= 0; i--)
	{
		if (PendingMutationQueue[i].ModifierID == ModifierID)
		{
			MutationsToProcess.Add(PendingMutationQueue[i]);
			PendingMutationQueue.RemoveAt(i);
		}
	}

	// Process each queued mutation
	for (const FAttributeModifierMutation& Mutation : MutationsToProcess)
	{
		ClientOnAttributeModifierMutationAdded(Mutation);
	}
}

void USimpleAttributeComponent::CleanupOldModifierStates()
{
	const double CurrentTime = GetServerTime();

	// Clean up old authority states (server only)
	if (HasAuthority())
	{
		for (int32 i = AuthorityAttributeModifierStates.ModifierStates.Num() - 1; i >= 0; i--)
		{
			const FAttributeModifierState& State = AuthorityAttributeModifierStates.ModifierStates[i];

			// Remove ended/cancelled states that are older than retention time
			if ((State.ModifierStatus == EModifierStatus::Ended || State.ModifierStatus == EModifierStatus::Cancelled) &&
				(CurrentTime - State.EndedTimestamp) > MaxStateRetentionTime)
			{
				AuthorityAttributeModifierStates.ModifierStates.RemoveAt(i);
				AuthorityAttributeModifierStates.MarkArrayDirty();
			}
		}

		// Clean up old mutations
		for (int32 i = AuthorityAttributeModifierMutations.Mutations.Num() - 1; i >= 0; i--)
		{
			const FAttributeModifierMutation& Mutation = AuthorityAttributeModifierMutations.Mutations[i];

			if ((CurrentTime - Mutation.MutationTimestamp) > MaxStateRetentionTime)
			{
				AuthorityAttributeModifierMutations.Mutations.RemoveAt(i);
				AuthorityAttributeModifierMutations.MarkArrayDirty();
			}
		}
	}
	else
	{
		// Clean up local states (client only)
		for (int32 i = LocalAttributeModifierStates.Num() - 1; i >= 0; i--)
		{
			const FAttributeModifierState& State = LocalAttributeModifierStates[i];

			if ((State.ModifierStatus == EModifierStatus::Ended || State.ModifierStatus == EModifierStatus::Cancelled) &&
				(CurrentTime - State.EndedTimestamp) > MaxStateRetentionTime)
			{
				LocalAttributeModifierStates.RemoveAt(i);
			}
		}

		// Clean up local mutations
		for (int32 i = LocalAttributeModiferMutations.Num() - 1; i >= 0; i--)
		{
			const FAttributeModifierMutation& Mutation = LocalAttributeModiferMutations[i];

			if ((CurrentTime - Mutation.MutationTimestamp) > MaxStateRetentionTime)
			{
				LocalAttributeModiferMutations.RemoveAt(i);
			}
		}
	}

	// Clean up old predicted snapshots (client only)
	if (!HasAuthority())
	{
		// Remove snapshots older than retention time
		for (int32 i = PredictedModifierSnapshots.Num() - 1; i >= 0; i--)
		{
			if ((CurrentTime - PredictedModifierSnapshots[i].SnapshotTimestamp) > MaxStateRetentionTime)
			{
				PredictedModifierSnapshots.RemoveAt(i);
			}
		}

		// If we still have too many snapshots, remove oldest
		while (PredictedModifierSnapshots.Num() > MaxPredictedSnapshots)
		{
			// Find and remove oldest snapshot
			int32 OldestIndex = 0;
			double OldestTime = PredictedModifierSnapshots[0].SnapshotTimestamp;

			for (int32 i = 1; i < PredictedModifierSnapshots.Num(); i++)
			{
				if (PredictedModifierSnapshots[i].SnapshotTimestamp < OldestTime)
				{
					OldestTime = PredictedModifierSnapshots[i].SnapshotTimestamp;
					OldestIndex = i;
				}
			}

			PredictedModifierSnapshots.RemoveAt(OldestIndex);
		}
	}
}

void USimpleAttributeComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	DOREPLIFETIME(USimpleAttributeComponent, AuthorityGameplayTags);
	DOREPLIFETIME(USimpleAttributeComponent, AuthorityFloatAttributes);
	DOREPLIFETIME(USimpleAttributeComponent, AuthorityStructAttributes);
	DOREPLIFETIME(USimpleAttributeComponent, AuthorityAttributeModifierStates);
	DOREPLIFETIME(USimpleAttributeComponent, AuthorityAttributeModifierMutations);

	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}
