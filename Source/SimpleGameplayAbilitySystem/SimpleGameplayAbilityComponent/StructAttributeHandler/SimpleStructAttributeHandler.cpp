#include "SimpleStructAttributeHandler.h"

#include "GameplayTagContainer.h"
#include "InstancedStruct.h"
#include "SimpleGameplayAbilitySystem/BlueprintFunctionLibraries/SimpleAttributes/SimpleAttributeFunctionLibrary.h"
#include "SimpleGameplayAbilitySystem/DefaultTags/DefaultTags.h"
#include "SimpleGameplayAbilitySystem/Module/SimpleGameplayAbilitySystem.h"
#include "SimpleGameplayAbilitySystem/SimpleGameplayAbilityComponent/SimpleGameplayAbilityComponent.h"

FInstancedStruct USimpleStructAttributeHandler::GetStruct(const FGameplayTag AttributeTag, bool& WasFound) const
{
	return USimpleAttributeFunctionLibrary::GetStructAttributeValue(AbilityComponent, AttributeTag, WasFound);
}

bool USimpleStructAttributeHandler::SetStruct(FGameplayTag AttributeTag, FInstancedStruct NewStruct)
{
	if (!AttributeTag.IsValid())
	{
		SIMPLE_LOG(this, TEXT("[USimpleStructAttributeHandler::SetStruct]: AttributeTag is invalid. Cannot set struct."));
		return false;
	}
	
	if (NewStruct.GetScriptStruct() != StructType)
	{
		SIMPLE_LOG(this, FString::Printf(
			TEXT("[USimpleStructAttributeHandler::SetStruct]: New struct of type %s does not match [%s] required Type %s."),
			*NewStruct.GetScriptStruct()->GetName(), *AttributeTag.GetTagName().ToString(),*StructType->GetName()));
		return false;
	}
	
	return USimpleAttributeFunctionLibrary::SetStructAttributeValue(AbilityComponent, AttributeTag, NewStruct);
}

void USimpleStructAttributeHandler::OnStructChanged_Implementation(FGameplayTag AttributeTag, FInstancedStruct OldStruct, FInstancedStruct NewStruct) const
{
	SendStructEvent(FDefaultTags::StructAttributeValueChanged, NewStruct); 
}

void USimpleStructAttributeHandler::SendStructEvent(FGameplayTag EventTag, FInstancedStruct Payload, ESimpleEventReplicationPolicy ReplicationPolicy) const
{
	AbilityComponent->SendEvent(FDefaultTags::StructAttributeValueChanged, EventTag, Payload, AbilityComponent->GetOwner(), {}, ReplicationPolicy);
}
