#include "SimpleGameplayAttributes.h"

#include "Net/UnrealNetwork.h"
#include "SimpleGameplayAbilitySystem/DataAssets/AttributeSet/AttributeSet.h"
#include "SimpleGameplayAbilitySystem/SimpleEventSubsystem/SimpleEventSubSystem.h"
#include "SimpleGameplayAbilitySystem/SimpleGameplayAbilityComponent/SimpleGameplayAbilityComponent.h"
#include "SimpleGameplayAbilitySystem/SimpleGASTypes/DefaultTags/DefaultTags.h"

USimpleGameplayAttributes::USimpleGameplayAttributes()
{
}

void USimpleGameplayAttributes::InitializeAttributeSets(USimpleGameplayAbilityComponent* NewOwningAbilityComponent)
{
	if (!NewOwningAbilityComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("USimpleGameplayAttributes::InitializeAttributeSets: NewOwningAbilityComponent is null."));
		return;
	}
	
	OwningAbilityComponent = NewOwningAbilityComponent;
	
	if (!OwningAbilityComponent->HasAuthority())
	{
		return;
	}
	
	for (UAttributeSet* AttributeSet : AttributeSets)
	{
		for (const FFloatAttribute& Attribute : AttributeSet->FloatAttributes)
		{
			AddFloatAttribute(Attribute);
		}

		for (const FStructAttribute& Attribute : AttributeSet->StructAttributes)
		{
			AddStructAttribute(Attribute);
		}
	}
}

// Float attribute functions

void USimpleGameplayAttributes::AddFloatAttribute(FFloatAttribute Attribute)
{
	// Check if the attribute already exists
	for (int i = 0; i < FloatAttributes.Num(); i++)
	{
		if (FloatAttributes[i].AttributeName == Attribute.AttributeName)
		{
			// If it does, update the attribute
			FloatAttributes[i] = Attribute;
			UE_LOG(LogTemp, Warning, TEXT("Attribute %s already exists. Replacing attribute data with new attribute."), *Attribute.AttributeName.ToString());
			return;
		}
	}

	FloatAttributes.Add(Attribute);
}

void USimpleGameplayAttributes::RemoveFloatAttributes(FGameplayTagContainer AttributeTags)
{
	FloatAttributes.RemoveAll([&AttributeTags](const FFloatAttribute& Attribute)
	{
		return AttributeTags.HasTag(Attribute.AttributeName);
	});
}

FFloatAttribute USimpleGameplayAttributes::GetFloatAttribute(FGameplayTag AttributeTag, bool& WasFound) const
{
	for (int i = 0; i < FloatAttributes.Num(); i++)
	{
		if (FloatAttributes[i].AttributeName == AttributeTag)
		{
			WasFound = true;
			return FloatAttributes[i];
		}
	}

	WasFound = false;
	return FFloatAttribute();
}

float USimpleGameplayAttributes::GetFloatAttributeValue(EAttributeValueType ValueType, FGameplayTag AttributeTag, bool& WasFound) const
{
	const FFloatAttribute Attribute = GetFloatAttribute(AttributeTag, WasFound);
	
	if (!WasFound)
	{
		UE_LOG(LogTemp, Warning, TEXT("Attribute %s not found."), *AttributeTag.ToString());
		return 0.0f;
	}

	switch (ValueType)
	{
		case EAttributeValueType::BaseValue:
			return Attribute.BaseValue;
		case EAttributeValueType::CurrentValue:
			return Attribute.CurrentValue;
		default:
			WasFound = false;
			return 0.0f;
	}
}

bool USimpleGameplayAttributes::SetFloatAttributeValue(EAttributeValueType ValueType, FGameplayTag AttributeTag, float NewValue, float& Overflow)
{
	const int32 AttributeIndex = GetFloatAttributeIndex(AttributeTag);

	if (AttributeIndex < 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Attribute %s not found."), *AttributeTag.ToString());
		return false;
	}
	
	switch (ValueType)
	{
		case EAttributeValueType::BaseValue:
			FloatAttributes[AttributeIndex].BaseValue = ClampValue(FloatAttributes[AttributeIndex], BaseValue, NewValue, Overflow);
			SendFloatAttributeChangedEvent(FDefaultTags::AttributeChanged, AttributeTag, BaseValue, FloatAttributes[AttributeIndex].BaseValue);
			return true;
		case EAttributeValueType::CurrentValue:
			FloatAttributes[AttributeIndex].CurrentValue = ClampValue(FloatAttributes[AttributeIndex], CurrentValue, NewValue, Overflow);
			SendFloatAttributeChangedEvent(FDefaultTags::AttributeChanged, AttributeTag, CurrentValue, FloatAttributes[AttributeIndex].CurrentValue);
			return true;
		default:
			return false;
	}
}

bool USimpleGameplayAttributes::SetFloatAttributeMaxValue(EAttributeValueType ValueType, FGameplayTag AttributeTag, float NewMaxValue, float& Overflow)
{
	const int32 AttributeIndex = GetFloatAttributeIndex(AttributeTag);

	if (AttributeIndex < 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Attribute %s not found."), *AttributeTag.ToString());
		return false;
	}

	switch (ValueType)
	{
		case BaseValue:
			FloatAttributes[AttributeIndex].ValueLimits.MaxBaseValue = NewMaxValue;
			return true;
		
		case CurrentValue:
			FloatAttributes[AttributeIndex].ValueLimits.MaxCurrentValue = NewMaxValue;
			return true;
		
		default:
			return false;
	}
}

bool USimpleGameplayAttributes::SetFloatAttributeMinValue(EAttributeValueType ValueType, FGameplayTag AttributeTag, float NewMinValue, float& Overflow)
{
	const int32 AttributeIndex = GetFloatAttributeIndex(AttributeTag);

	if (AttributeIndex < 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Attribute %s not found."), *AttributeTag.ToString());
		return false;
	}

	switch (ValueType)
	{
		case BaseValue:
			FloatAttributes[AttributeIndex].ValueLimits.MinBaseValue = NewMinValue;
			return true;
		
		case CurrentValue:
			FloatAttributes[AttributeIndex].ValueLimits.MinCurrentValue = NewMinValue;
			return true;
		
		default:
			return false;
	}
}

bool USimpleGameplayAttributes::SetFloatAttributeUseMinValue(EAttributeValueType ValueType, FGameplayTag AttributeTag, bool UseMinValue)
{
	const int32 AttributeIndex = GetFloatAttributeIndex(AttributeTag);

	if (AttributeIndex < 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Attribute %s not found."), *AttributeTag.ToString());
		return false;
	}

	switch (ValueType)
	{
		case BaseValue:
			FloatAttributes[AttributeIndex].ValueLimits.UseMinBaseValue = UseMinValue;
			return true;
		
		case CurrentValue:
			FloatAttributes[AttributeIndex].ValueLimits.UseMinCurrentValue = UseMinValue;
			return true;
		
		default:
			return false;
	}
}

bool USimpleGameplayAttributes::SetFloatAttributeUseMaxValue(EAttributeValueType ValueType, FGameplayTag AttributeTag, bool UseMaxValue)
{
	const int32 AttributeIndex = GetFloatAttributeIndex(AttributeTag);

	if (AttributeIndex < 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Attribute %s not found."), *AttributeTag.ToString());
		return false;
	}

	switch (ValueType)
	{
		case BaseValue:
			FloatAttributes[AttributeIndex].ValueLimits.UseMaxBaseValue = UseMaxValue;
			return true;
		
		case CurrentValue:
			FloatAttributes[AttributeIndex].ValueLimits.UseMaxCurrentValue = UseMaxValue;
			return true;
		
		default:
			return false;
	}
}

bool USimpleGameplayAttributes::AddToFloatAttributeValue(EAttributeValueType ValueType, FGameplayTag AttributeTag, float Amount, float& Overflow)
{
	const int32 AttributeIndex = GetFloatAttributeIndex(AttributeTag);

	if (AttributeIndex < 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Attribute %s not found."), *AttributeTag.ToString());
		return false;
	}

	float NewValue;
	switch (ValueType)
	{
		case BaseValue:
			NewValue = FloatAttributes[AttributeIndex].BaseValue + Amount;
			FloatAttributes[AttributeIndex].BaseValue = ClampValue(FloatAttributes[AttributeIndex], BaseValue, NewValue, Overflow);
			SendFloatAttributeChangedEvent(FDefaultTags::AttributeChanged, AttributeTag, BaseValue, FloatAttributes[AttributeIndex].BaseValue);
			return true;
		
		case CurrentValue:
			NewValue = FloatAttributes[AttributeIndex].CurrentValue + Amount;
			FloatAttributes[AttributeIndex].CurrentValue = ClampValue(FloatAttributes[AttributeIndex], CurrentValue, NewValue, Overflow);
			SendFloatAttributeChangedEvent(FDefaultTags::AttributeChanged, AttributeTag, CurrentValue, FloatAttributes[AttributeIndex].CurrentValue);
			return true;
		
		default:
			return false;
	}
}

bool USimpleGameplayAttributes::MultiplyFloatAttributeValue(EAttributeValueType ValueType, FGameplayTag AttributeTag, float Multiplier, float& Overflow)
{
	const int32 AttributeIndex = GetFloatAttributeIndex(AttributeTag);

	if (AttributeIndex < 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Attribute %s not found."), *AttributeTag.ToString());
		return false;
	}

	float NewValue;
	switch (ValueType)
	{
		case BaseValue:
			NewValue = FloatAttributes[AttributeIndex].BaseValue * Multiplier;
			FloatAttributes[AttributeIndex].BaseValue = ClampValue(FloatAttributes[AttributeIndex], BaseValue, NewValue, Overflow);
			SendFloatAttributeChangedEvent(FDefaultTags::AttributeChanged, AttributeTag, BaseValue, FloatAttributes[AttributeIndex].BaseValue);
			return true;
		
		case CurrentValue:
			NewValue = FloatAttributes[AttributeIndex].CurrentValue * Multiplier;
			FloatAttributes[AttributeIndex].CurrentValue = ClampValue(FloatAttributes[AttributeIndex], CurrentValue, NewValue, Overflow);
			SendFloatAttributeChangedEvent(FDefaultTags::AttributeChanged, AttributeTag, CurrentValue, FloatAttributes[AttributeIndex].CurrentValue);
			return true;
		
		default:
			return false;
	}
}

// Struct attribute functions

void USimpleGameplayAttributes::AddStructAttribute(FStructAttribute Attribute)
{
	// Check if the attribute already exists
	for (int i = 0; i < StructAttributes.Num(); i++)
	{
		if (StructAttributes[i].AttributeName == Attribute.AttributeName)
		{
			// If it does, update the attribute
			StructAttributes[i] = Attribute;
			UE_LOG(LogTemp, Warning, TEXT("Attribute %s already exists. Replacing attribute data with new attribute."), *Attribute.AttributeName.ToString());
			return;
		}
	}

	StructAttributes.Add(Attribute);
}

void USimpleGameplayAttributes::RemoveStructAttributes(FGameplayTagContainer AttributeTags)
{
	StructAttributes.RemoveAll([&AttributeTags](const FStructAttribute& Attribute)
	{
		return AttributeTags.HasTagExact(Attribute.AttributeName);
	});
}

FStructAttribute USimpleGameplayAttributes::GetStructAttribute(FGameplayTag AttributeTag, bool& WasFound) const
{
	for (int i = 0; i < StructAttributes.Num(); i++)
	{
		if (StructAttributes[i].AttributeName == AttributeTag)
		{
			WasFound = true;
			return StructAttributes[i];
		}
	}

	WasFound = false;
	return FStructAttribute();
}

FInstancedStruct USimpleGameplayAttributes::GetStructAttributeValue(FGameplayTag AttributeTag, bool& WasFound) const
{
	FStructAttribute Attribute = GetStructAttribute(AttributeTag, WasFound);

	if (WasFound)
	{
		return Attribute.AttributeValue;
	}

	return FInstancedStruct();
}

bool USimpleGameplayAttributes::SetStructAttributeValue(FGameplayTag AttributeTag, FInstancedStruct NewValue)
{
	for (int i = 0; i < StructAttributes.Num(); i++)
	{
		if (StructAttributes[i].AttributeName == AttributeTag)
		{
			if (StructAttributes[i].AttributeType != NewValue.GetScriptStruct())
			{
				UE_LOG(LogTemp, Warning, TEXT("USimpleGameplayAttributes::SetStructAttributeValue: New value type does not match attribute type."));
				return false;
			}
			
			StructAttributes[i].AttributeValue = NewValue;

			if (USimpleEventSubsystem* EventSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<USimpleEventSubsystem>())
			{
				FStructAttributeModification Payload;
				Payload.AttributeTag = AttributeTag;
				Payload.Value = NewValue;

				const FInstancedStruct EventPayload = FInstancedStruct::Make(Payload);
		
				EventSubsystem->SendEvent(FDefaultTags::AttributeChanged, FDefaultTags::AuthorityDomain, EventPayload, OwningAbilityComponent->GetOwner());
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("USimpleGameplayAttributes::SetStructAttributeValue: No event subsystem found."));
			}
			
			return true;
		}
	}

	return false;
}

// Utility functions

int32 USimpleGameplayAttributes::GetFloatAttributeIndex(FGameplayTag AttributeTag) const
{
	for (int i = 0; i < FloatAttributes.Num(); i++)
	{
		if (FloatAttributes[i].AttributeName == AttributeTag)
		{
			return i;
		}
	}

	return -1;
}

float USimpleGameplayAttributes::ClampValue(const FFloatAttribute& Attribute, EAttributeValueType ValueType, float NewValue, float& Overflow)
{
	switch (ValueType)
	{
		case EAttributeValueType::BaseValue:
			if (Attribute.ValueLimits.UseMaxBaseValue && NewValue > Attribute.ValueLimits.MaxBaseValue)
			{
				Overflow = NewValue - Attribute.ValueLimits.MaxBaseValue;
				return Attribute.ValueLimits.MaxBaseValue;
			}

			if (Attribute.ValueLimits.UseMinBaseValue && NewValue < Attribute.ValueLimits.MinBaseValue)
			{
				Overflow = Attribute.ValueLimits.MinBaseValue - NewValue;
				return Attribute.ValueLimits.MinBaseValue;
			}

			return NewValue;
		
		case EAttributeValueType::CurrentValue:
			if (Attribute.ValueLimits.UseMaxCurrentValue && NewValue > Attribute.ValueLimits.MaxCurrentValue)
			{
				Overflow = NewValue - Attribute.ValueLimits.MaxCurrentValue;
				return Attribute.ValueLimits.MaxCurrentValue;
			}

			if (Attribute.ValueLimits.UseMinCurrentValue && NewValue < Attribute.ValueLimits.MinCurrentValue)
			{
				Overflow = Attribute.ValueLimits.MinCurrentValue - NewValue;
				return Attribute.ValueLimits.MinCurrentValue;
			}

			return NewValue;
		
		default:
			UE_LOG(LogTemp, Warning, TEXT("USimpleGameplayAttributes::ClampValue: Invalid attribute value type."));
			return 0.0f;
	}
}

void USimpleGameplayAttributes::SendFloatAttributeChangedEvent(FGameplayTag EventTag, FGameplayTag AttributeTag, EAttributeValueType ValueType, float NewValue)
{
	if (USimpleEventSubsystem* EventSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<USimpleEventSubsystem>())
	{
		FFloatAttributeModification Payload;
		Payload.AttributeTag = AttributeTag;
		Payload.ValueType = ValueType;
		Payload.NewValue = NewValue;

		const FInstancedStruct EventPayload = FInstancedStruct::Make(Payload);
		
		EventSubsystem->SendEvent(EventTag, FDefaultTags::AuthorityDomain, EventPayload, OwningAbilityComponent->GetOwner());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("USimpleGameplayAttributes::SendFloatAttributeChangedEvent: No event subsystem found."));
	}
}

// Replication

void USimpleGameplayAttributes::OnRep_FloatAttributes(TArray<FFloatAttribute> OldAttributes)
{
	// Check if any new attributes were added
	FGameplayTagContainer OldAttributeTags;
	FGameplayTagContainer NewAttributeTags;
	
	for (int i = 0; i < OldAttributes.Num(); i++)
	{
		OldAttributeTags.AddTag(OldAttributes[i].AttributeName);
	}
	
	for (int i = 0; i < FloatAttributes.Num(); i++)
	{
		if (!OldAttributeTags.HasTagExact(FloatAttributes[i].AttributeName))
		{
			SendFloatAttributeChangedEvent(FDefaultTags::AttributeAdded, FloatAttributes[i].AttributeName, EAttributeValueType::BaseValue, FloatAttributes[i].BaseValue);
		}

		NewAttributeTags.AddTag(FloatAttributes[i].AttributeName);
	}

	// Check if any old attributes were removed and check if any existing attributes were updated
	for (int i = 0; i < OldAttributes.Num(); i++)
	{
		if (!NewAttributeTags.HasTagExact(OldAttributes[i].AttributeName))
		{
			SendFloatAttributeChangedEvent(FDefaultTags::AttributeRemoved, OldAttributes[i].AttributeName, EAttributeValueType::BaseValue, 0.0f);
		}
		else
		{
			const int32 NewAttributeIndex = GetFloatAttributeIndex(OldAttributes[i].AttributeName);

			if (NewAttributeIndex < 0)
			{
				UE_LOG(LogTemp, Warning, TEXT("USimpleGameplayAttributes::OnRep_FloatAttributes: Attribute %s not found."), *OldAttributes[i].AttributeName.ToString());
				continue;
			}
			
			if (FloatAttributes[NewAttributeIndex].BaseValue != OldAttributes[i].BaseValue)
			{
				SendFloatAttributeChangedEvent(FDefaultTags::AttributeChanged, OldAttributes[i].AttributeName, EAttributeValueType::BaseValue, FloatAttributes[NewAttributeIndex].BaseValue);
			}

			if (FloatAttributes[NewAttributeIndex].CurrentValue != OldAttributes[i].CurrentValue)
			{
				SendFloatAttributeChangedEvent(FDefaultTags::AttributeChanged, OldAttributes[i].AttributeName, EAttributeValueType::CurrentValue, FloatAttributes[NewAttributeIndex].CurrentValue);
			}
		}
	}
}

void USimpleGameplayAttributes::OnRep_StructAttributes(TArray<FStructAttribute> OldAttributes)
{
	// Check if any new attributes were added
	FGameplayTagContainer OldAttributeTags;
	FGameplayTagContainer NewAttributeTags;
	
	for (int i = 0; i < OldAttributes.Num(); i++)
	{
		OldAttributeTags.AddTag(OldAttributes[i].AttributeName);
	}
	
	for (int i = 0; i < StructAttributes.Num(); i++)
	{
		if (!OldAttributeTags.HasTagExact(StructAttributes[i].AttributeName))
		{
			SendFloatAttributeChangedEvent(FDefaultTags::AttributeAdded, StructAttributes[i].AttributeName, EAttributeValueType::BaseValue, 0.0f);
		}

		NewAttributeTags.AddTag(StructAttributes[i].AttributeName);
	}

	// Check if any old attributes were removed and check if any existing attributes were updated
	for (int i = 0; i < OldAttributes.Num(); i++)
	{
		if (!NewAttributeTags.HasTagExact(OldAttributes[i].AttributeName))
		{
			SendFloatAttributeChangedEvent(FDefaultTags::AttributeRemoved, OldAttributes[i].AttributeName, EAttributeValueType::BaseValue, 0.0f);
		}
		else
		{
			const int32 NewAttributeIndex = GetFloatAttributeIndex(OldAttributes[i].AttributeName);

			if (NewAttributeIndex < 0)
			{
				UE_LOG(LogTemp, Warning, TEXT("USimpleGameplayAttributes::OnRep_StructAttributes: Attribute %s not found."), *OldAttributes[i].AttributeName.ToString());
				continue;
			}
			
			if (StructAttributes[NewAttributeIndex].AttributeValue != OldAttributes[i].AttributeValue)
			{
				SendFloatAttributeChangedEvent(FDefaultTags::AttributeChanged, OldAttributes[i].AttributeName, EAttributeValueType::BaseValue, 0.0f);
			}
		}
	}
}

UWorld* USimpleGameplayAttributes::GetWorld() const
{
	if (OwningAbilityComponent)
	{
		return OwningAbilityComponent->GetWorld();
	}

	return nullptr;
}

void USimpleGameplayAttributes::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USimpleGameplayAttributes, FloatAttributes);
	DOREPLIFETIME(USimpleGameplayAttributes, StructAttributes);
}
