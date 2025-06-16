#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameplayTagsManager.h"
#include "SimpleGameplayAbilitySystem/Module/SimpleGameplayAbilitySystem.h"

class FDefaultTags
{
	public:
		// Events
		static FGameplayTag GameplayTagAdded() { return FindTag("SimpleGAS.Events.AbilityComponent.GameplayTagAdded"); }
		static FGameplayTag GameplayTagRemoved() { return FindTag("SimpleGAS.Events.AbilityComponent.GameplayTagRemoved"); }
	
		static FGameplayTag AbilityActivated() { return FindTag("SimpleGAS.Events.Ability.Activated"); }
		static FGameplayTag AbilityEnded() { return FindTag("SimpleGAS.Events.Ability.Ended"); }
		static FGameplayTag AbilityEndedSuccessfully() { return FindTag("SimpleGAS.Events.Ability.Ended.Success"); }
		static FGameplayTag AbilityCancelled() { return FindTag("SimpleGAS.Events.Ability.Ended.Cancel"); }
	
		static FGameplayTag WaitForAbilityEnded() { return FindTag("SimpleGAS.Events.Ability.WaitForAbilityEnded"); }
	
		static FGameplayTag AttributeModifierApplied() { return FindTag("SimpleGAS.Events.AttributeModifer.Applied"); }
		static FGameplayTag AttributeModifierTicked() { return FindTag("SimpleGAS.Events.AttributeModifer.Ticked"); }
		static FGameplayTag AttributeModifierEnded() { return FindTag("SimpleGAS.Events.AttributeModifer.Ended"); }
	
		static FGameplayTag FloatAttributeAdded() { return FindTag("SimpleGAS.Events.Attributes.Added.Float"); }
		static FGameplayTag FloatAttributeBaseValueChanged() { return FindTag("SimpleGAS.Events.Attributes.Changed.Float.BaseValue"); }
		static FGameplayTag FloatAttributeMinBaseValueChanged() { return FindTag("SimpleGAS.Events.Attributes.Changed.Float.MinBaseValue"); }
		static FGameplayTag FloatAttributeMaxBaseValueChanged() { return FindTag("SimpleGAS.Events.Attributes.Changed.Float.MaxBaseValue"); }
		static FGameplayTag FloatAttributeCurrentValueChanged() { return FindTag("SimpleGAS.Events.Attributes.Changed.Float.CurrentValue"); }
		static FGameplayTag FloatAttributeMinCurrentValueChanged() { return FindTag("SimpleGAS.Events.Attributes.Changed.Float.MinCurrentValue"); }
		static FGameplayTag FloatAttributeMaxCurrentValueChanged() { return FindTag("SimpleGAS.Events.Attributes.Changed.Float.MaxCurrentValue"); }
		static FGameplayTag FloatAttributeRemoved() { return FindTag("SimpleGAS.Events.Attributes.Removed.Float"); }
	
		static FGameplayTag StructAttributeAdded() { return FindTag("SimpleGAS.Events.Attributes.Added.Struct"); }
		static FGameplayTag StructAttributeValueChanged() { return FindTag("SimpleGAS.Events.Attributes.Changed.Struct"); }
		static FGameplayTag StructAttributeRemoved() { return FindTag("SimpleGAS.Events.Attributes.Removed.Struct"); }
		
		// Domains
		static FGameplayTag LocalAbilityDomain() { return FindTag("SimpleGAS.Domains.Ability.Local"); }
		static FGameplayTag AuthorityAbilityDomain() { return FindTag("SimpleGAS.Domains.Ability.Authority"); }
		static FGameplayTag LocalAttributeDomain() { return FindTag("SimpleGAS.Domains.Attribute.Local"); }
		static FGameplayTag AuthorityAttributeDomain() { return FindTag("SimpleGAS.Domains.Attribute.Authority"); }

		// Misc
		static FGameplayTag ScratchPadFloatOverflow() { return FindTag("SimpleGAS.ModifierActionScratchpadTags.FloatAttributeOverflow"); }
	private:
		static FGameplayTag FindTag(const FName TagName)
		{
			const FGameplayTag Tag = UGameplayTagsManager::Get().RequestGameplayTag(TagName, false);

			if (!Tag.IsValid())
			{
				UE_LOG(LogSimpleGAS, Warning, TEXT("Gameplay Tag %s was not found when looking for default tags."), *TagName.ToString());
			}
		
			return Tag;
		}
};
