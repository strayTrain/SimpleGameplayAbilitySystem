#include "AbilityStateResolver.h"

#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleAbility.h"
#include "SimpleGameplayAbilitySystem/SimpleAbilityComponent/SimpleAbilityComponent.h"

void UAbilityStateResolver::ResolveState_Implementation(const FAbilityState& AuthorityState, const FAbilityState& PredictedState)
{
}

USimpleAbilityComponent* UAbilityStateResolver::GetOwningAbilityComponent() const
{
	if (OwningAbility && OwningAbility->OwningAbilityComponent)
	{
		return OwningAbility->OwningAbilityComponent;
	}

	UE_LOG(LogTemp, Warning, TEXT("Owning Ability Component is null"));
	return nullptr;
}

AActor* UAbilityStateResolver::GetAvatarActor() const
{
	if (OwningAbility && OwningAbility->OwningAbilityComponent)
	{
		return OwningAbility->OwningAbilityComponent->GetAvatarActor();
	}

	UE_LOG(LogTemp, Warning, TEXT("Avatar Actor is null"));
	return nullptr;
}
