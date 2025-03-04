#include "DefaultTags.h"

#include "GameplayTagsManager.h"
#include "SimpleGameplayAbilitySystem/Module/SimpleGameplayAbilitySystem.h"

FGameplayTag FDefaultTags::GameplayTagAdded;
FGameplayTag FDefaultTags::GameplayTagRemoved;

FGameplayTag FDefaultTags::AbilityAdded;
FGameplayTag FDefaultTags::AbilityRemoved;
FGameplayTag FDefaultTags::AbilityActivated;
FGameplayTag FDefaultTags::AbilityEnded;
FGameplayTag FDefaultTags::AbilityEndedSuccessfully;
FGameplayTag FDefaultTags::AbilityCancelled;
FGameplayTag FDefaultTags::AbilityStateSnapshotTaken;
FGameplayTag FDefaultTags::WaitForAbilityEnded;

FGameplayTag FDefaultTags::AttributeModifierApplied;
FGameplayTag FDefaultTags::AttributeModifierInitiallyApplied;
FGameplayTag FDefaultTags::AttributeModifierTicked;
FGameplayTag FDefaultTags::AttributeModifierEnded;

FGameplayTag FDefaultTags::FloatAttributeAdded;
FGameplayTag FDefaultTags::FloatAttributeRemoved;

FGameplayTag FDefaultTags::FloatAttributeBaseValueChanged;
FGameplayTag FDefaultTags::FloatAttributeMinBaseValueChanged;
FGameplayTag FDefaultTags::FloatAttributeMaxBaseValueChanged;
FGameplayTag FDefaultTags::FloatAttributeCurrentValueChanged;
FGameplayTag FDefaultTags::FloatAttributeMinCurrentValueChanged;
FGameplayTag FDefaultTags::FloatAttributeMaxCurrentValueChanged;

FGameplayTag FDefaultTags::StructAttributeAdded;
FGameplayTag FDefaultTags::StructAttributeValueChanged;
FGameplayTag FDefaultTags::StructAttributeRemoved;

FGameplayTag FDefaultTags::LocalDomain;
FGameplayTag FDefaultTags::AuthorityDomain;
FGameplayTag FDefaultTags::AbilityDomain;
FGameplayTag FDefaultTags::AttributeDomain;

void FDefaultTags::InitializeDefaultTags()
{
	// Ability Component Events
	GameplayTagAdded = FindTag("SimpleGAS.Events.AbilityComponent.GameplayTagAdded");
	GameplayTagRemoved = FindTag("SimpleGAS.Events.AbilityComponent.GameplayTagRemoved");
	
	// Ability Events
	AbilityAdded = FindTag("SimpleGAS.Events.Ability.AbilityAdded");
	AbilityRemoved = FindTag("SimpleGAS.Events.Ability.AbilityRemoved");
	AbilityActivated = FindTag("SimpleGAS.Events.Ability.AbilityActivated");
	AbilityEnded = FindTag("SimpleGAS.Events.Ability.AbilityEnded");
	AbilityEndedSuccessfully = FindTag("SimpleGAS.Events.Ability.AbilityEnded.Success");
	AbilityCancelled = FindTag("SimpleGAS.Events.Ability.AbilityEnded.Cancelled");
	AbilityStateSnapshotTaken = FindTag("SimpleGAS.Events.Ability.AbilityStateSnapshotTaken");
	WaitForAbilityEnded = FindTag("SimpleGAS.Events.Ability.WaitForAbilityEnded");
	
	// Float Attributes
	FloatAttributeAdded = FindTag("SimpleGAS.Events.Attributes.FloatAttributeAdded");
	FloatAttributeRemoved = FindTag("SimpleGAS.Events.Attributes.FloatAttributeRemoved");
	
	FloatAttributeBaseValueChanged = FindTag("SimpleGAS.Events.Attributes.AttributeChanged.BaseValue");
	FloatAttributeMinBaseValueChanged = FindTag("SimpleGAS.Events.Attributes.AttributeChanged.MinBaseValue");
	FloatAttributeMaxBaseValueChanged = FindTag("SimpleGAS.Events.Attributes.AttributeChanged.MaxBaseValue");
	FloatAttributeCurrentValueChanged = FindTag("SimpleGAS.Events.Attributes.AttributeChanged.CurrentValue");
	FloatAttributeMinCurrentValueChanged = FindTag("SimpleGAS.Events.Attributes.AttributeChanged.MinCurrentValue");
	FloatAttributeMaxCurrentValueChanged = FindTag("SimpleGAS.Events.Attributes.AttributeChanged.MaxCurrentValue");

	// Struct Attributes
	StructAttributeAdded = FindTag("SimpleGAS.Events.Attributes.StructAttributeAdded");
	StructAttributeValueChanged = FindTag("SimpleGAS.Events.Attributes.StructAttributeChanged");
	StructAttributeRemoved = FindTag("SimpleGAS.Events.Attributes.StructAttributeRemoved");

	// Attribute Modifiers
	AttributeModifierInitiallyApplied = FindTag("SimpleGAS.Events.AttributeModifer.ModifierInitiallyApplied");
	AttributeModifierTicked = FindTag("SimpleGAS.Events.AttributeModifer.ModifierTicked");
	AttributeModifierEnded = FindTag("SimpleGAS.Events.AttributeModifer.ModifierEnded");

	AttributeModifierApplied = FindTag("SimpleGAS.Events.AttributeModifer.States.ModifierApplied");
	
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
		UE_LOG(LogSimpleGAS, Warning, TEXT("Gameplay Tag %s was not found when looking for default tags."), *TagName.ToString());
	}
	
	return Tag;
}
