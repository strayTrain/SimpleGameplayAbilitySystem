#include "SimpleGameplayAbility.h"

#include "SimpleGameplayAbilitySystem/DefaultTags/DefaultTags.h"
#include "SimpleGameplayAbilitySystem/Module/SimpleGameplayAbilitySystem.h"
#include "SimpleGameplayAbilitySystem/SimpleGameplayAbilityComponent/SimpleGameplayAbilityComponent.h"

bool USimpleGameplayAbility::CanActivate_Implementation(FInstancedStruct ActivationContext)
{
	return true;
}

void USimpleGameplayAbility::PreActivate_Implementation(FInstancedStruct ActivationContext)
{
}

bool USimpleGameplayAbility::CanCancel_Implementation()
{
	return true;
}

bool USimpleGameplayAbility::ActivateAbility(FInstancedStruct ActivationContext)
{
	if (!MeetsTagRequirements() || !CanActivate(ActivationContext))
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
	CachedActivationContext = ActivationContext;

	PreActivate(ActivationContext);
	bIsAbilityActive = true;
	OnActivate(ActivationContext);
	
	return true;
}

FGuid USimpleGameplayAbility::ActivateSubAbility(const TSubclassOf<USimpleGameplayAbility> AbilityClass,
	const FInstancedStruct ActivationContext, const bool ShouldOverrideActivationPolicy, const EAbilityActivationPolicy OverridePolicy,
	const bool EndIfParentEnds, const bool EndIfParentCancels)
{
	FGuid SubAbilityID = OwningAbilityComponent->ActivateAbility(AbilityClass, ActivationContext, ShouldOverrideActivationPolicy, OverridePolicy);

	if (EndIfParentEnds)
	{
		EndOnEndedSubAbilities.Add(SubAbilityID);
	}

	if (EndIfParentCancels)
	{
		EndOnCancelledSubAbilities.Add(SubAbilityID);
	}

	return SubAbilityID;
}

void USimpleGameplayAbility::EndAbility(FGameplayTag EndStatus, FInstancedStruct EndingContext)
{
	OnEnd(EndStatus, EndingContext);
	
	for (const FGameplayTag& TempTag : TemporarilyAppliedTags)
	{
		OwningAbilityComponent->RemoveGameplayTag(TempTag, EndingContext);
	}
	
	if (EndStatus == FDefaultTags::AbilityEndedSuccessfully)
	{
		OwningAbilityComponent->ChangeAbilityStatus(AbilityInstanceID, EAbilityStatus::EndedSuccessfully);

		for (const FGuid& SubAbilityID : EndOnEndedSubAbilities)
		{
			OwningAbilityComponent->CancelAbility(SubAbilityID, EndingContext);
		}
	}
	else if (EndStatus == FDefaultTags::AbilityCancelled)
	{
		OwningAbilityComponent->ChangeAbilityStatus(AbilityInstanceID, EAbilityStatus::EndedCancelled);

		for (const FGuid& SubAbilityID : EndOnCancelledSubAbilities)
		{
			OwningAbilityComponent->CancelAbility(SubAbilityID, EndingContext);
		}
	}
	else
	{
		OwningAbilityComponent->ChangeAbilityStatus(AbilityInstanceID, EAbilityStatus::EndedCustomStatus);

		for (const FGuid& SubAbilityID : EndOnEndedSubAbilities)
		{
			OwningAbilityComponent->CancelAbility(SubAbilityID, EndingContext);
		}
	}
	
	if (InstancingPolicy == EAbilityInstancingPolicy::MultipleInstances)
	{
		OwningAbilityComponent->RemoveInstancedAbility(this);
	}

	bIsAbilityActive = false;

	if (IsProxyAbility && !OwningAbilityComponent->HasAuthority())
	{
		return;
	}
	
	OwningAbilityComponent->SetAbilityStateEndingContext(AbilityInstanceID, EndStatus, EndingContext);
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
	return bIsAbilityActive;
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
	return CachedActivationContext;
}

bool USimpleGameplayAbility::WasActivatedOnServer() const
{
	return OwningAbilityComponent->HasAuthority() && !IsProxyAbility;
}

bool USimpleGameplayAbility::WasActivatedOnClient() const
{
	return (!OwningAbilityComponent->HasAuthority() || OwningAbilityComponent->GetNetMode() == NM_ListenServer) && !IsProxyAbility;
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

void USimpleGameplayAbility::ClientResolvePastState(FGameplayTag StateTag, FSimpleAbilitySnapshot AuthorityState, FSimpleAbilitySnapshot PredictedState)
{
	OnClientReceivedAuthorityState(StateTag, AuthorityState, PredictedState);
}
