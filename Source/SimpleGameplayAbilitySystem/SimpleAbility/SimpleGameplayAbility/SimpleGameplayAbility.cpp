#include "SimpleGameplayAbility.h"

#include "SimpleGameplayAbilitySystem/SimpleGameplayAbilityComponent/SimpleGameplayAbilityComponent.h"

bool USimpleGameplayAbility::CanActivate_Implementation(FInstancedStruct AbilityContext)
{
	return true;
}

bool USimpleGameplayAbility::Activate(FInstancedStruct AbilityContext)
{
	if (CanActivate(AbilityContext))
	{
		bIsAbilityActive = true;
		ActivationTime = OwningAbilityComponent->GetServerTime();
		OwningAbilityComponent->OnAbilityActivated(this);
		OnActivate(AbilityContext);
		
		return true;
	}

	return false;
}

void USimpleGameplayAbility::EndAbility(FGameplayTag EndStatus, FInstancedStruct EndingContext)
{
	OnEnd(EndStatus, EndingContext);
	bIsAbilityActive = false;
}

AActor* USimpleGameplayAbility::GetAvatarActorAs(TSubclassOf<AActor> AvatarClass) const
{
	if (AActor* AvatarActor = OwningAbilityComponent->GetAvatarActor())
	{
		return AvatarActor;
	}

	UE_LOG(LogTemp, Warning, TEXT("Ability %s owning component has no avatar actor set. Did you remember to call SetAvatarActor?"), *AbilityInstanceID.ToString());
	return nullptr;
}

UWorld* USimpleGameplayAbility::GetWorld() const
{
	if (OwningAbilityComponent)
	{
		return OwningAbilityComponent->GetWorld();
	}

	return nullptr;
}
