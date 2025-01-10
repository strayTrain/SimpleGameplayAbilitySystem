#include "SimpleGameplayAbility.h"

#include "SimpleGameplayAbilitySystem/DefaultTags/DefaultTags.h"
#include "SimpleGameplayAbilitySystem/Module/SimpleGameplayAbilitySystem.h"
#include "SimpleGameplayAbilitySystem/SimpleGameplayAbilityComponent/SimpleGameplayAbilityComponent.h"

bool USimpleGameplayAbility::CanActivate_Implementation(FInstancedStruct ActivationContext)
{
	return true;
}

bool USimpleGameplayAbility::ActivateAbility(FInstancedStruct ActivationContext)
{
	if (!MeetsTagRequirements() && CanActivate(ActivationContext))
	{
		OwningAbilityComponent->ChangeAbilityStatus(AbilityInstanceID, EAbilityStatus::EndedActivationFailed);
		return false;	
	}

	for (const FGameplayTag& TempTag : TemporarilyAppliedTags)
	{
		OwningAbilityComponent->AddGameplayTag(TempTag, ActivationContext);
	}

	for (const FGameplayTag& PermTag : PermanentlyAppliedTags)
	{
		OwningAbilityComponent->AddGameplayTag(PermTag, ActivationContext);
	}

	OwningAbilityComponent->ChangeAbilityStatus(AbilityInstanceID, EAbilityStatus::ActivationSuccess);
	OnActivate(ActivationContext);
	
	return true;
}

void USimpleGameplayAbility::EndAbility(FGameplayTag EndStatus, FInstancedStruct EndingContext)
{
	OnEnd(EndStatus, EndingContext);

	for (const FGameplayTag& TempTag : TemporarilyAppliedTags)
	{
		OwningAbilityComponent->RemoveGameplayTag(TempTag, EndingContext);
	}
	
	if (ActivationPolicy == EAbilityActivationPolicy::LocalPredicted || ActivationPolicy == EAbilityActivationPolicy::ServerInitiated)
	{
		if (EndStatus == FDefaultTags::AbilityEndedSuccessfully)
		{
			OwningAbilityComponent->ChangeAbilityStatus(AbilityInstanceID, EAbilityStatus::EndedSuccessfully);
		}
		else if (EndStatus == FDefaultTags::AbilityCancelled)
		{
			OwningAbilityComponent->ChangeAbilityStatus(AbilityInstanceID, EAbilityStatus::EndedCancelled);
		}
		else
		{
			OwningAbilityComponent->ChangeAbilityStatus(AbilityInstanceID, EAbilityStatus::EndedCustomStatus);
		}
	}
	
	if (InstancingPolicy == EAbilityInstancingPolicy::MultipleInstances)
	{
		OwningAbilityComponent->RemoveInstancedAbility(this);
	}
}

void USimpleGameplayAbility::EndSuccess(FInstancedStruct EndingContext)
{
	EndAbility(FDefaultTags::AbilityEndedSuccessfully, EndingContext);
}

void USimpleGameplayAbility::EndCancel(FInstancedStruct EndingContext)
{
	EndAbility(FDefaultTags::AbilityCancelled, EndingContext);
}

AActor* USimpleGameplayAbility::GetAvatarActorAs(TSubclassOf<AActor> AvatarClass) const
{
	if (AActor* AvatarActor = OwningAbilityComponent->GetAvatarActor())
	{
		return AvatarActor;
	}

	UE_LOG(LogSimpleGAS, Warning, TEXT("Ability %s owning component has no avatar actor set. Did you remember to call SetAvatarActor?"), *AbilityInstanceID.ToString());
	return nullptr;
}

bool USimpleGameplayAbility::IsAbilityActive() const
{
	bool WasStateFound;
	const FAbilityState AbilityState = OwningAbilityComponent->GetAbilityState(AbilityInstanceID, WasStateFound);

	if (WasStateFound)
	{
		return AbilityState.AbilityStatus == EAbilityStatus::ActivationSuccess;
	}

	//UE_LOG(LogSimpleGAS, Warning, TEXT("Ability with ID %s not found in AbilityState array"), *AbilityInstanceID.ToString());
	return false;
}

double USimpleGameplayAbility::GetActivationTime() const
{
	bool WasStateFound;
	const FAbilityState AbilityState = OwningAbilityComponent->GetAbilityState(AbilityInstanceID, WasStateFound);

	if (WasStateFound)
	{
		return AbilityState.ActivationTimeStamp;
	}

	UE_LOG(LogSimpleGAS, Warning, TEXT("Ability with ID %s not found in AbilityState array"), *AbilityInstanceID.ToString());
	return 0;
}

FInstancedStruct USimpleGameplayAbility::GetActivationContext() const
{
	bool WasStateFound;
	FAbilityState AbilityState = OwningAbilityComponent->GetAbilityState(AbilityInstanceID, WasStateFound);

	if (WasStateFound)
	{
		return AbilityState.ActivationContext;
	}

	UE_LOG(LogSimpleGAS, Warning, TEXT("Ability with ID %s not found in AbilityState array"), *AbilityInstanceID.ToString());
	return FInstancedStruct();
}

UWorld* USimpleGameplayAbility::GetWorld() const
{
	if (OwningAbilityComponent)
	{
		return OwningAbilityComponent->GetWorld();
	}

	return nullptr;
}

bool USimpleGameplayAbility::MeetsTagRequirements() const
{
	if (ActivationBlockingTags.Num() > 0)
	{
		for (const FGameplayTag& BlockingTag : ActivationBlockingTags)
		{
			if (OwningAbilityComponent->GameplayTags.HasTagExact(BlockingTag))
			{
				UE_LOG(LogSimpleGAS, Warning, TEXT("Ability %s blocked by tag %s"), *GetName(), *BlockingTag.ToString());
				return false;
			}
		}
	}

	if (ActivationRequiredTags.Num() > 0)
	{
		for (const FGameplayTag& RequiredTag : ActivationRequiredTags)
		{
			if (!OwningAbilityComponent->GameplayTags.HasTagExact(RequiredTag))
			{
				UE_LOG(LogSimpleGAS, Warning, TEXT("Ability %s requires tag %s"), *GetName(), *RequiredTag.ToString());
				return false;
			}
		}
	}

	return true;
}
