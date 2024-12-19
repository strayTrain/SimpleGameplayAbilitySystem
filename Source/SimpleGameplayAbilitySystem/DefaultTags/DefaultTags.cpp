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
FGameplayTag FDefaultTags::AttributeRemoved;

FGameplayTag FDefaultTags::FloatAttributeBaseValueChanged;
FGameplayTag FDefaultTags::FloatAttributeMinBaseValueChanged;
FGameplayTag FDefaultTags::FloatAttributeMaxBaseValueChanged;
FGameplayTag FDefaultTags::FloatAttributeCurrentValueChanged;
FGameplayTag FDefaultTags::FloatAttributeMinCurrentValueChanged;
FGameplayTag FDefaultTags::FloatAttributeMaxCurrentValueChanged;

FGameplayTag FDefaultTags::LocalDomain;
FGameplayTag FDefaultTags::AuthorityDomain;
FGameplayTag FDefaultTags::AbilityDomain;
FGameplayTag FDefaultTags::AttributeDomain;

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

	// Attributes
	AttributeAdded = FindTag("SimpleGAS.Events.Attributes.AttributeAdded");
	AttributeRemoved = FindTag("SimpleGAS.Events.Attributes.AttributeRemoved");

	FloatAttributeBaseValueChanged = FindTag("SimpleGAS.Events.Attributes.AttributeChanged.BaseValue");
	FloatAttributeMinBaseValueChanged = FindTag("SimpleGAS.Events.Attributes.AttributeChanged.MinBaseValue");
	FloatAttributeMaxBaseValueChanged = FindTag("SimpleGAS.Events.Attributes.AttributeChanged.MaxBaseValue");
	FloatAttributeCurrentValueChanged = FindTag("SimpleGAS.Events.Attributes.AttributeChanged.CurrentValue");
	FloatAttributeMinCurrentValueChanged = FindTag("SimpleGAS.Events.Attributes.AttributeChanged.MinCurrentValue");
	FloatAttributeMaxCurrentValueChanged = FindTag("SimpleGAS.Events.Attributes.AttributeChanged.MaxCurrentValue");

	// Domains
	LocalDomain = FindTag("SimpleGAS.Domains.Ability.Local");
	AuthorityDomain = FindTag("SimpleGAS.Domains.Ability.Authority");
	AbilityDomain = FindTag("SimpleGAS.Domains.Ability");
	AttributeDomain = FindTag("SimpleGAS.Domains.Attribute");
}

FGameplayTag FDefaultTags::FindTag(FName TagName)
{
	const FGameplayTag Tag = UGameplayTagsManager::Get().RequestGameplayTag(TagName, false);

	if (!Tag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Gameplay Tag %s was not found when looking for default tags."), *TagName.ToString());
	}
	
	return Tag;
}
