#include "DefaultTags.h"

#include "GameplayTagsManager.h"

FGameplayTag FDefaultTags::AbilityAdded;
FGameplayTag FDefaultTags::AbilityRemoved;
FGameplayTag FDefaultTags::AbilityActivated;
FGameplayTag FDefaultTags::AbilityEnded;
FGameplayTag FDefaultTags::AbilityEndedSuccessfully;
FGameplayTag FDefaultTags::AbilityCancelled;
FGameplayTag FDefaultTags::AbilityStateSnapshotTaken;

FGameplayTag FDefaultTags::AttributeAdded;
FGameplayTag FDefaultTags::AttributeChanged;
FGameplayTag FDefaultTags::AttributeRemoved;

FGameplayTag FDefaultTags::LocalDomain;
FGameplayTag FDefaultTags::AuthorityDomain;
FGameplayTag FDefaultTags::AbilityDomain;

void FDefaultTags::InitializeDefaultTags()
{
	// Events
	AbilityAdded = FindTag("SimpleGAS.Events.Ability.AbilityAdded");
	AbilityRemoved = FindTag("SimpleGAS.Events.Ability.AbilityRemoved");
	AbilityActivated = FindTag("SimpleGAS.Events.Ability.AbilityActivated");
	AbilityEnded = FindTag("SimpleGAS.Events.Ability.AbilityEnded");
	AbilityEndedSuccessfully = FindTag("SimpleGAS.Events.Ability.AbilityEndedSuccessfully");
	AbilityCancelled = FindTag("SimpleGAS.Events.Ability.AbilityCancelled");
	AbilityStateSnapshotTaken = FindTag("SimpleGAS.Events.Ability.AbilityStateSnapshotTaken");

	AttributeAdded = FindTag("SimpleGAS.Events.Attributes.AttributeAdded");
	AttributeChanged = FindTag("SimpleGAS.Events.Attributes.AttributeChanged");
	AttributeRemoved = FindTag("SimpleGAS.Events.Attributes.AttributeRemoved");

	// Domains
	LocalDomain = FindTag("SimpleGAS.Domains.Ability.Local");
	AuthorityDomain = FindTag("SimpleGAS.Domains.Ability.Authority");
	AbilityDomain = FindTag("SimpleGAS.Domains.Ability");
}

FGameplayTag FDefaultTags::FindTag(FName TagName)
{
	FGameplayTag Tag = UGameplayTagsManager::Get().RequestGameplayTag(TagName, false);
	if (!Tag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Gameplay Tag %s was not found!"), *TagName.ToString());
	}
	return Tag;
}
