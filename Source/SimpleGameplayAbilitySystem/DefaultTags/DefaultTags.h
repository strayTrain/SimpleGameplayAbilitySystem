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
	static FGameplayTag AttributeChanged;
	static FGameplayTag AttributeRemoved;

	// Domains
	static FGameplayTag LocalDomain;
	static FGameplayTag AuthorityDomain;
	static FGameplayTag AbilityDomain;
private:
	static FGameplayTag FindTag(FName TagName);
};
