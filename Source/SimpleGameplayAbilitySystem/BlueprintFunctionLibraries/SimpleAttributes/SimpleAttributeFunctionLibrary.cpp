#include "SimpleAttributeFunctionLibrary.h"

#include "GameplayTagContainer.h"
#include "SimpleGameplayAbilitySystem/SimpleAbilityComponent/SimpleAbilityComponent.h"

bool USimpleAttributeFunctionLibrary::HasFloatAttribute(const FGameplayTag AttributeTag, const USimpleAbilityComponent* AbilityComponent)
{
	for (int i = 0; i < AbilityComponent->FloatAttributes.Num(); i++)
	{
		if (AbilityComponent->FloatAttributes[i].AttributeTag.MatchesTagExact(AttributeTag))
		{
			return true;
		}
	}

	return false;
}

bool USimpleAttributeFunctionLibrary::HasStructAttribute(const FGameplayTag AttributeTag, const USimpleAbilityComponent* AbilityComponent)
{
	for (int i = 0; i < AbilityComponent->StructAttributes.Num(); i++)
	{
		if (AbilityComponent->StructAttributes[i].AttributeTag.MatchesTagExact(AttributeTag))
		{
			return true;
		}
	}

	return false;
}

FFloatAttribute USimpleAttributeFunctionLibrary::GetFloatAttributeCopy(const FGameplayTag AttributeTag, const USimpleAbilityComponent* AbilityComponent, bool& WasFound)
{
	for (int i = 0; i < AbilityComponent->FloatAttributes.Num(); i++)
	{
		if (AbilityComponent->FloatAttributes[i].AttributeTag.MatchesTagExact(AttributeTag))
		{
			WasFound = true;
			return AbilityComponent->FloatAttributes[i];
		}
	}

	return FFloatAttribute();
}
