#include "SimpleAttributeHandler.h"

#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeComponent.h"
#include "SimpleGameplayAbilitySystem/DefaultTags/DefaultTags.h"
#include "SimpleGameplayAbilitySystem/Module/SimpleGameplayAbilitySystem.h"

void USimpleAttributeHandler::InitializeHandler(USimpleAttributeComponent* NewAttributeOwner, const FGameplayTag NewAttributeTag)
{
	AttributeOwner = NewAttributeOwner;
	AttributeTag = NewAttributeTag;
}

FGameplayTagContainer USimpleAttributeHandler::GetModificationEvents_Implementation(const FInstancedStruct& OldValue, const FInstancedStruct& NewValue)
{
	return FGameplayTagContainer();
}

FInstancedStruct USimpleAttributeHandler::GetStructValue() const
{
	if (!AttributeOwner)
	{
		UE_LOG(LogSimpleGAS, Warning, TEXT("[USimpleAttributeHandler::GetStructValue]: AttributeOwner is null in GetStructValue"));
		return FInstancedStruct();
	}

	bool WasFound = false;
	FInstancedStruct AttributeValue = AttributeOwner->GetStructAttributeValue(AttributeTag, WasFound);

	if (!WasFound)
	{
		UE_LOG(LogSimpleGAS, Warning, TEXT("[USimpleAttributeHandler::GetStructValue]: AttributeTag %s not found in AttributeOwner"), *AttributeTag.ToString());
		return FInstancedStruct();
	}

	// Check if the struct type matches
	if (StructType && StructType != AttributeValue.GetScriptStruct())
	{
		UE_LOG(LogSimpleGAS, Warning, TEXT("[USimpleAttributeHandler::GetStructValue]: Attribute %s does not match expected struct type of %s. It's of type %s"), *AttributeTag.ToString(), *StructType->GetName(), *AttributeValue.GetScriptStruct()->GetName());
		WasFound = false;
		return FInstancedStruct();
	}

	return AttributeValue;
}

bool USimpleAttributeHandler::SetStructValue(const FInstancedStruct& NewValue)
{
	if (!AttributeOwner)
	{
		UE_LOG(LogSimpleGAS, Warning, TEXT("[USimpleAttributeHandler::SetStructValue]: AttributeOwner is null in SetStructValue"));
		return false;
	}
	
	return AttributeOwner->SetStructAttributeValue(AttributeTag, NewValue);
}
