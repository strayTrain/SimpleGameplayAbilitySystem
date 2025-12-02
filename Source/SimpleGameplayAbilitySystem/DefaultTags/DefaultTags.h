#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameplayTagsManager.h"
#include "SimpleGameplayAbilitySystem/Module/SimpleGameplayAbilitySystem.h"

class FDefaultTags
{
	public:
		// Attribute Modifier event tags (SimpleEvent system)
		static FGameplayTag AttributeModifierApplied() { return FindTag("SimpleGAS.Events.AttributeModifier.Applied"); }
		static FGameplayTag AttributeModifierTicked() { return FindTag("SimpleGAS.Events.AttributeModifier.Ticked"); }
		static FGameplayTag AttributeModifierTickFailedSkip() { return FindTag("SimpleGAS.Events.AttributeModifier.TickFailedSkip"); }
		static FGameplayTag AttributeModifierTickFailedCancel() { return FindTag("SimpleGAS.Events.AttributeModifier.TickFailedCancel"); }
		static FGameplayTag AttributeModifierEnded() { return FindTag("SimpleGAS.Events.AttributeModifier.Ended"); }
		static FGameplayTag AttributeModifierCancelled() { return FindTag("SimpleGAS.Events.AttributeModifier.Cancelled"); }

		// Stacking event tags
		static FGameplayTag AttributeModifierStackChanged() { return FindTag("SimpleGAS.Events.AttributeModifier.StackChanged"); }
		static FGameplayTag AttributeModifierStackAdded() { return FindTag("SimpleGAS.Events.AttributeModifier.StackAdded"); }
		static FGameplayTag AttributeModifierStackRemoved() { return FindTag("SimpleGAS.Events.AttributeModifier.StackRemoved"); }
		static FGameplayTag AttributeModifierPredictionCorrected() { return FindTag("SimpleGAS.Events.AttributeModifier.PredictionCorrected"); }

		// Domain tags for event categorization
		static FGameplayTag DomainAttributeModifier() { return FindTag("SimpleGAS.Domains.AttributeModifier"); }
		static FGameplayTag DomainAbility() { return FindTag("SimpleGAS.Domains.Ability"); }

		// Scratchpad tags for stacking
		static FGameplayTag ScratchPadStackCount() { return FindTag("SimpleGAS.ScratchPad.StackCount"); }
		static FGameplayTag ScratchPadScaledMagnitude() { return FindTag("SimpleGAS.ScratchPad.ScaledMagnitude"); }

		// Misc scratchpad tags
		static FGameplayTag ScratchPadFloatOverflow() { return FindTag("SimpleGAS.ModifierActionScratchpadTags.FloatAttributeOverflow"); }
		static FGameplayTag ScratchPadRuntimeActionExecutionResult() { return FindTag("SimpleGAS.ModifierActionScratchpadTags.RuntimeActionExecutionResult"); }

		// Async action events
		static FGameplayTag SubAbilityEnded() { return FindTag("SimpleGAS.Events.SubAbility.Ended"); }
		static FGameplayTag SubAbilityCancelled() { return FindTag("SimpleGAS.Events.SubAbility.Cancelled"); }
	
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
