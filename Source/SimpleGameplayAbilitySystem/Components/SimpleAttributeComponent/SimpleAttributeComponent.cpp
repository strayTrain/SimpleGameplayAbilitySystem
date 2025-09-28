//  Copyright 2025 Ahmed Elgoni

#include "SimpleAttributeComponent.h"

#include "VREditorMode.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent//AttributeHandler/SimpleAttributeHandler.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleTimeSynchronizerComponent/SimpleTimeSynchronizer.h"
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

	LocalFloatAttributes = AuthorityFloatAttributes.Attributes;
	LocalStructAttributes = AuthorityStructAttributes.Attributes;
	LocalGameplayTags = AuthorityGameplayTags.Tags;
}

void USimpleAttributeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	InstancedAttributeModifiers.Empty();
	Super::EndPlay(EndPlayReason);
}

USimpleTimeSynchronizer* USimpleAttributeComponent::GetTimeSynchronizerComponent_Implementation()
{
	// Default to assuming the owner actor has a time synchronizer component
	return GetOwner()->GetComponentByClass<USimpleTimeSynchronizer>();
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

	if (TagCounter && TagCounter->ReferenceCounter > 1)
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
	TArray<FGameplayTagCounter>& TagCounters = HasAuthority() ? AuthorityGameplayTags.Tags : LocalGameplayTags;

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
	TArray<FGameplayTagCounter>& TagCounters = HasAuthority() ? AuthorityGameplayTags.Tags : LocalGameplayTags;

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
			break;

		case EFloatAttributeValueType::CurrentValue:
			OldValue = Attribute->CurrentValue;
			Attribute->CurrentValue = ClampedValue;
			if (OldValue != ClampedValue)
			{
				OnFloatAttributeCurrentValueChanged.Broadcast(AttributeTag, OldValue, ClampedValue);
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
			Overflow = Attribute.ValueLimits.MinBaseValue + NewValue;
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
			Overflow = Attribute.ValueLimits.MinCurrentValue + NewValue;
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
	return GetAttributeModifierInstance(ModifierClass, false)->ApplyModifier(ModifierID, this, ModifierTarget, Magnitude,ModifierContext);
}

bool USimpleAttributeComponent::ApplyAttributeModifierToTargetPredicted(
	FGuid& ModifierID,
    const TSubclassOf<USimpleAttributeModifier> ModifierClass,
    USimpleAttributeComponent* ModifierTarget,
    const float Magnitude,
    const FInstancedStruct ModifierContext) 
{
	ModifierID = FGuid::NewGuid();
	const bool WasApplied = GetAttributeModifierInstance(ModifierClass, true)->ApplyModifier(ModifierID, this, ModifierTarget, Magnitude,ModifierContext);

	if (!HasAuthority() && WasApplied)
	{
		ServerApplyAttributeModifierToTarget(ModifierID, ModifierClass, ModifierTarget, Magnitude, ModifierContext);
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
	GetAttributeModifierInstance(ModifierClass, true)->ApplyModifier(ModifierID, this, ModifierTarget, Magnitude,ModifierContext);
}

bool USimpleAttributeComponent::ApplyAttributeModifierToSelf(
	FGuid& ModifierID,
	const TSubclassOf<USimpleAttributeModifier> ModifierClass,
	const float Magnitude, const FInstancedStruct ModifierContext)
{
	ModifierID = FGuid::NewGuid();
	return GetAttributeModifierInstance(ModifierClass, false)->ApplyModifier(ModifierID, this, this, Magnitude,ModifierContext);
}

bool USimpleAttributeComponent::ApplyAttributeModifierToSelfPredicted(
	FGuid& ModifierID,
	const TSubclassOf<USimpleAttributeModifier> ModifierClass,
	const float Magnitude,
	const FInstancedStruct ModifierContext) 
{
	ModifierID = FGuid::NewGuid();
	const bool WasApplied = GetAttributeModifierInstance(ModifierClass, true)->ApplyModifier(ModifierID, this, this, Magnitude,ModifierContext);

	if (!HasAuthority() && WasApplied)
	{
		ServerApplyAttributeModifierToTarget(ModifierID, ModifierClass, this, Magnitude, ModifierContext);
	}

	return WasApplied;
}

void USimpleAttributeComponent::ApplyAttributeModifierToSelfServerInitiated(FGuid& ModifierID,
	const TSubclassOf<USimpleAttributeModifier> ModifierClass, const float Magnitude, const FInstancedStruct ModifierContext)
{
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

USimpleAttributeModifier* USimpleAttributeComponent::GetAttributeModifierInstance(const TSubclassOf<USimpleAttributeModifier>& ModifierClass, const bool ShouldReplicate)
{
	if (!ModifierClass)
	{
		SIMPLE_LOG(this, TEXT("[USimpleGameplayAbilityComponent::GetAttributeModifierInstance]: ModifierClass is null!"));
		return nullptr;
	}
	
	USimpleAttributeModifier* LocalRunningModifierInstance = nullptr;
	for (USimpleAttributeModifier* InstancedModifier : InstancedAttributeModifiers)
	{
		if (InstancedModifier->GetClass() == ModifierClass)
		{
			LocalRunningModifierInstance = InstancedModifier;
			break;
		}
	}

	if (!LocalRunningModifierInstance)
	{
		LocalRunningModifierInstance = NewObject<USimpleAttributeModifier>(this, ModifierClass);
		LocalRunningModifierInstance->OnModifierApplied.AddDynamic(this, &USimpleAttributeComponent::OnAttributeModifierInitiallyApplied);
		LocalRunningModifierInstance->OnActionStackApplied.AddDynamic(this, &USimpleAttributeComponent::OnAttributeModifierActionStackApplied);
		LocalRunningModifierInstance->OnAttributeModifierEnded.AddDynamic(this, &USimpleAttributeComponent::OnAttributeModifierEnded);
		LocalRunningModifierInstance->OnAttributeModifierCancelled.AddDynamic(this, &USimpleAttributeComponent::OnAttributeModifierCancelled);
		InstancedAttributeModifiers.Add(LocalRunningModifierInstance);
	}

	LocalRunningModifierInstance->SetModifierCanReplicate(ShouldReplicate);
	return LocalRunningModifierInstance;
}

/* Attribute Modifier State Callbacks */

void USimpleAttributeComponent::OnAttributeModifierInitiallyApplied(USimpleAttributeModifier* ModifierInstance)
{
	FAttributeModifierState NewState;
	NewState.ModifierID = ModifierInstance->ModifierID;
	NewState.ModifierContext = FInstancedStruct();
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
		LocalAttributeModiferMutations.Add(NewMutation);
		return;
	}

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
	
	switch (NewAttributeModiferState.ModifierStatus)
	{
		case EModifierStatus::Applied:
			if (!LocalAttributeModifierArrayIndexMap.Contains(NewAttributeModiferState.ModifierID))
			{
				GetAttributeModifierInstance(NewAttributeModiferState.ModifierClass, true)->ApplyModifier(
					NewAttributeModiferState.ModifierID,
					NewAttributeModiferState.InstigatorAttributeComponent,
					NewAttributeModiferState.TargetAttributeComponent,
					0.0f,
					NewAttributeModiferState.ModifierContext);

				LocalAttributeModifierStates.Add(NewAttributeModiferState);
				return;
			}

			LocalAttributeModifierStates[LocalAttributeModifierArrayIndexMap[NewAttributeModiferState.ModifierID]] = NewAttributeModiferState;
			break;
		
		case EModifierStatus::Cancelled:
			if (LocalAttributeModifierArrayIndexMap.Contains(NewAttributeModiferState.ModifierID))
			{
				GetAttributeModifierInstance(NewAttributeModiferState.ModifierClass, true)->CancelModifier(
				FDefaultTags::AttributeModifierCancelled(),
				FInstancedStruct());
			}
			break;	

		case EModifierStatus::Ended:
			if (LocalAttributeModifierArrayIndexMap.Contains(NewAttributeModiferState.ModifierID))
			{
				GetAttributeModifierInstance(NewAttributeModiferState.ModifierClass, true)->EndModifier(
					FDefaultTags::AttributeModifierEnded(),
					FInstancedStruct());
			}
			break;
	}

}

void USimpleAttributeComponent::ClientOnAttributeModifierStateChanged(const FAttributeModifierState& ChangedAttributeModiferState)
{
	// A mapping of the local modifier states for quick lookups
	TMap<FGuid, int32> LocalAttributeModifierArrayIndexMap;

	for (int32 i = 0; i < LocalAttributeModifierStates.Num(); i++)
	{
		LocalAttributeModifierArrayIndexMap.Add(LocalAttributeModifierStates[i].ModifierID, i);
	}
	
	switch (ChangedAttributeModiferState.ModifierStatus)
	{
		case EModifierStatus::Applied:
			if (!LocalAttributeModifierArrayIndexMap.Contains(ChangedAttributeModiferState.ModifierID))
			{
				GetAttributeModifierInstance(ChangedAttributeModiferState.ModifierClass, true)->ApplyModifier(
					ChangedAttributeModiferState.ModifierID,
					ChangedAttributeModiferState.InstigatorAttributeComponent,
					ChangedAttributeModiferState.TargetAttributeComponent,
					0.0f,
					ChangedAttributeModiferState.ModifierContext);

				LocalAttributeModifierStates.Add(ChangedAttributeModiferState);
				return;
			}

			LocalAttributeModifierStates[LocalAttributeModifierArrayIndexMap[ChangedAttributeModiferState.ModifierID]] = ChangedAttributeModiferState;
			break;
			
		case EModifierStatus::Cancelled:
			if (LocalAttributeModifierArrayIndexMap.Contains(ChangedAttributeModiferState.ModifierID))
			{
				GetAttributeModifierInstance(ChangedAttributeModiferState.ModifierClass, true)->CancelModifier(
				FDefaultTags::AttributeModifierCancelled(),
				FInstancedStruct());
			}
			break;	

		case EModifierStatus::Ended:
			if (LocalAttributeModifierArrayIndexMap.Contains(ChangedAttributeModiferState.ModifierID))
			{
				GetAttributeModifierInstance(ChangedAttributeModiferState.ModifierClass, true)->EndModifier(
					FDefaultTags::AttributeModifierEnded(),
					FInstancedStruct());
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

	USimpleAttributeModifier* LocalRunningModifierInstance = GetAttributeModifierInstance(NewModifierMutation.ModifierClass, false);
	LocalRunningModifierInstance->ModifierID = NewModifierMutation.ModifierID;
	const FAttributeModifierState* AuthorityState = AuthorityAttributeModifierStates.ModifierStates.FindByPredicate(
		[NewModifierMutation](const FAttributeModifierState& State)
		{
			return State.ModifierID == NewModifierMutation.ModifierID;
		});

	if (AuthorityState)
	{
		LocalRunningModifierInstance->InstigatorAttributeComponent = AuthorityState->InstigatorAttributeComponent;
		LocalRunningModifierInstance->TargetAttributeComponent = AuthorityState->TargetAttributeComponent;
		LocalRunningModifierInstance->OnClientReceivedServerActionsResult(NewModifierMutation.ActionStackResult, LocalActionStackResults);
	}
	else
	{
		SIMPLE_LOG(this, FString::Printf(
			TEXT("[USimpleAttributeComponent::ClientOnAttributeModifierMutationAdded]: Modifier with ID %s not found in AuthorityAttributeModifierStates array"),
			*NewModifierMutation.ModifierID.ToString()));
	}
	
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
	OnGameplayTagAdded.Broadcast(NewGameplayTag.GameplayTag);
}

void USimpleAttributeComponent::ClientOnGameplayTagRemoved(const FGameplayTagCounter& RemovedGameplayTag)
{
	LocalGameplayTags.Remove(RemovedGameplayTag);
	OnGameplayTagRemoved.Broadcast(RemovedGameplayTag.GameplayTag);
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
