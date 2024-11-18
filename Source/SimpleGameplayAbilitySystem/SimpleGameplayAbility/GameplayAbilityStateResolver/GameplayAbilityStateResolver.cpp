#include "GameplayAbilityStateResolver.h"

#include "SimpleGameplayAbilitySystem/SimpleGameplayAbility/SimpleGameplayAbility.h"
#include "SimpleGameplayAbilitySystem/SimpleGameplayAbilityComponent/SimpleGameplayAbilityComponent.h"

void UGameplayAbilityStateResolver::ResolveState_Implementation(const FAbilityState& AuthorityState, const FAbilityState& PredictedState)
{
}

USimpleGameplayAbilityComponent* UGameplayAbilityStateResolver::GetOwningAbilityComponent() const
{
	if (OwningAbility && OwningAbility->OwningAbilityComponent)
	{
		return OwningAbility->OwningAbilityComponent;
	}

	UE_LOG(LogTemp, Warning, TEXT("Owning Ability Component is null"));
	return nullptr;
}

AActor* UGameplayAbilityStateResolver::GetAvatarActor() const
{
	if (OwningAbility && OwningAbility->OwningAbilityComponent)
	{
		return OwningAbility->OwningAbilityComponent->GetAvatarActor();
	}

	UE_LOG(LogTemp, Warning, TEXT("Avatar Actor is null"));
	return nullptr;
}
