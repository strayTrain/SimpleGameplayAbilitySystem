#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

class FDefaultTags
{
	public:
		static void InitializeDefaultTags();

		// Events
		static FGameplayTag AbilityAdded;
		static FGameplayTag AbilityRemoved;
		static FGameplayTag AbilityActivated;
		static FGameplayTag AbilityEnded;
		static FGameplayTag AbilityEndedSuccessfully;
		static FGameplayTag AbilityCancelled;

		static FGameplayTag AbilityStateSnapshotTaken;

		static FGameplayTag AttributeAdded;
		static FGameplayTag AttributeRemoved;

		static FGameplayTag AttributeModifierInitiallyApplied;
		static FGameplayTag AttributeModifierTicked;
		static FGameplayTag AttributeModifierEnded;

		static FGameplayTag AttributeModifierApplied;

		static FGameplayTag GameplayTagAdded;
		static FGameplayTag GameplayTagRemoved;

		static FGameplayTag FloatAttributeBaseValueChanged;
		static FGameplayTag FloatAttributeMinBaseValueChanged;
		static FGameplayTag FloatAttributeMaxBaseValueChanged;
		static FGameplayTag FloatAttributeCurrentValueChanged;
		static FGameplayTag FloatAttributeMinCurrentValueChanged;
		static FGameplayTag FloatAttributeMaxCurrentValueChanged;
		
		// Domains
		static FGameplayTag LocalDomain;
		static FGameplayTag AuthorityDomain;
		static FGameplayTag AbilityDomain;
		static FGameplayTag AttributeDomain;
	private:
		static FGameplayTag FindTag(FName TagName);
};
