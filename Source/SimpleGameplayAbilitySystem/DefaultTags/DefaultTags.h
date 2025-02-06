#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

class FDefaultTags
{
	public:
		static void InitializeDefaultTags();

		// Events
		static FGameplayTag GameplayTagAdded;
		static FGameplayTag GameplayTagRemoved;
	
		static FGameplayTag AbilityAdded;
		static FGameplayTag AbilityRemoved;
		static FGameplayTag AbilityActivated;
		static FGameplayTag AbilityEnded;
		static FGameplayTag AbilityEndedSuccessfully;
		static FGameplayTag AbilityCancelled;

		static FGameplayTag AbilityStateSnapshotTaken;

		static FGameplayTag AttributeModifierApplied;
		static FGameplayTag AttributeModifierInitiallyApplied; // For duration modifiers
		static FGameplayTag AttributeModifierTicked;
		static FGameplayTag AttributeModifierEnded;
	
		static FGameplayTag FloatAttributeAdded;
		static FGameplayTag FloatAttributeRemoved;
		
		static FGameplayTag FloatAttributeBaseValueChanged;
		static FGameplayTag FloatAttributeMinBaseValueChanged;
		static FGameplayTag FloatAttributeMaxBaseValueChanged;
		static FGameplayTag FloatAttributeCurrentValueChanged;
		static FGameplayTag FloatAttributeMinCurrentValueChanged;
		static FGameplayTag FloatAttributeMaxCurrentValueChanged;

		static FGameplayTag StructAttributeAdded;
		static FGameplayTag StructAttributeValueChanged;
		static FGameplayTag StructAttributeRemoved;
		
		// Domains
		static FGameplayTag LocalDomain;
		static FGameplayTag AuthorityDomain;
		static FGameplayTag AbilityDomain;
		static FGameplayTag AttributeDomain;
	private:
		static FGameplayTag FindTag(FName TagName);
};
