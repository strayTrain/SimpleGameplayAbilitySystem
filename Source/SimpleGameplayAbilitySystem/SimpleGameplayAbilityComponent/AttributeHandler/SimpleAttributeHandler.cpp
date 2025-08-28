#include "SimpleAttributeHandler.h"

#include "SimpleGameplayAbilitySystem/DefaultTags/DefaultTags.h"
#include "SimpleGameplayAbilitySystem/Module/SimpleGameplayAbilitySystem.h"
#include "SimpleGameplayAbilitySystem/SimpleEventSubsystem/SimpleEventSubSystem.h"
#include "SimpleGameplayAbilitySystem/SimpleGameplayAbilityComponent/SimpleGameplayAbilityComponent.h"

void USimpleAttributeHandler::InitializeHandler(USimpleGameplayAbilityComponent* NewAttributeOwner, const FGameplayTag NewAttributeTag)
{
	AttributeOwner = NewAttributeOwner;
	AttributeTag = NewAttributeTag;
}

FGameplayTagContainer USimpleAttributeHandler::GetModificationEvents_Implementation(const FInstancedStruct& OldValue, const FInstancedStruct& NewValue)
{
	return FGameplayTagContainer();
}

FInstancedStruct USimpleAttributeHandler::GetStruct() const
{
	if (!AttributeOwner)
	{
		UE_LOG(LogSimpleGAS, Warning, TEXT("[USimpleAttributeHandler::GetStruct]: AttributeOwner is null in GetStruct"));
		return FInstancedStruct();
	}

	bool WasFound = false;
	FInstancedStruct AttributeValue = AttributeOwner->GetStructAttributeValue(AttributeTag, WasFound);

	if (!WasFound)
	{
		UE_LOG(LogSimpleGAS, Warning, TEXT("[USimpleAttributeHandler::GetStruct]: AttributeTag %s not found in AttributeOwner"), *AttributeTag.ToString());
		return FInstancedStruct();
	}

	// Check if the struct type matches
	if (StructType && StructType != AttributeValue.GetScriptStruct())
	{
		UE_LOG(LogSimpleGAS, Warning, TEXT("[USimpleAttributeHandler::GetStruct]: Attribute %s does not match expected struct type of %s. It's of type %s"), *AttributeTag.ToString(), *StructType->GetName(), *AttributeValue.GetScriptStruct()->GetName());
		WasFound = false;
		return FInstancedStruct();
	}

	return AttributeValue;
}

bool USimpleAttributeHandler::SetStruct(const FInstancedStruct& NewValue)
{
	if (!AttributeOwner)
	{
		UE_LOG(LogSimpleGAS, Warning, TEXT("[USimpleAttributeHandler::SetStruct]: AttributeOwner is null in SetStruct"));
		return false;
	}

	// Check if the struct type matches
	if (StructType && StructType != NewValue.GetScriptStruct())
	{
		UE_LOG(LogSimpleGAS, Warning, TEXT("[USimpleAttributeHandler::SetStruct]: NewValue does not match expected struct type of %s. It's of type %s"), *StructType->GetName(), *NewValue.GetScriptStruct()->GetName());
		return false;
	}
	
	return AttributeOwner->SetStructAttributeValue(AttributeTag, NewValue);
}
