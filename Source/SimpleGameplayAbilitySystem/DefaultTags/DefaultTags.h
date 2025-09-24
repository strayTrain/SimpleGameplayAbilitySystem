#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameplayTagsManager.h"
#include "SimpleGameplayAbilitySystem/Module/SimpleGameplayAbilitySystem.h"

class FDefaultTags
{
	public:
		// Attribute Modifier trigger events
		static FGameplayTag AttributeModifierApplied() { return FindTag("SimpleGAS.ModiferActionTriggers.OnApplied"); }
		static FGameplayTag AttributeModifierTicked() { return FindTag("SimpleGAS.ModiferActionTriggers.OnTick"); }
		static FGameplayTag AttributeModifierTickFailedSkip() { return FindTag("SimpleGAS.ModiferActionTriggers.OnTickFailedSkip"); }
		static FGameplayTag AttributeModifierTickFailedCancel() { return FindTag("SimpleGAS.ModiferActionTriggers.OnTickFailedCancel"); }
		static FGameplayTag AttributeModifierEnded() { return FindTag("SimpleGAS.ModiferActionTriggers.OnEnded"); }
		static FGameplayTag AttributeModifierCancelled() { return FindTag("SimpleGAS.ModiferActionTriggers.OnCancelled"); }

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
