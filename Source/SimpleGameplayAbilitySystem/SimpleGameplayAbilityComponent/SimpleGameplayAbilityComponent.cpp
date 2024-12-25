#include "SimpleGameplayAbilityComponent.h"

#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "SimpleGameplayAbilitySystem/DefaultTags/DefaultTags.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleAbilityTypes.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleGameplayAbility/SimpleGameplayAbility.h"

USimpleGameplayAbilityComponent::USimpleGameplayAbilityComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	AvatarActor = nullptr;
}

void USimpleGameplayAbilityComponent::AddGameplayTags(FGameplayTagContainer Tags)
{
	GameplayTags.AppendTags(Tags);
}

void USimpleGameplayAbilityComponent::RemoveGameplayTags(FGameplayTagContainer Tags)
{
	GameplayTags.RemoveTags(Tags);
}

bool USimpleGameplayAbilityComponent::ActivateAbility(TSubclassOf<USimpleGameplayAbility> AbilityClass, FInstancedStruct AbilityContext, bool OverrideActivationPolicy, EAbilityActivationPolicy ActivationPolicy)
{
	FGuid NewAbilityInstanceID = FGuid::NewGuid();

	EAbilityActivationPolicy Policy = OverrideActivationPolicy ? ActivationPolicy : AbilityClass.GetDefaultObject()->ActivationPolicy;

	switch (Policy)
	{
		case EAbilityActivationPolicy::LocalOnly:
			return ActivateAbilityInternal(AbilityClass, AbilityContext, NewAbilityInstanceID);
			
		case EAbilityActivationPolicy::LocalPredicted:
			// Case where the client is the server i.e. listen server
			if (GetOwner()->HasAuthority())
			{
				if (ActivateAbilityInternal(AbilityClass, AbilityContext, NewAbilityInstanceID))
				{
					MulticastActivateAbility(AbilityClass, AbilityContext, NewAbilityInstanceID);
					return true;
				}
			}

			// Case where we're the client and we're predicting the ability
			if (ActivateAbilityInternal(AbilityClass, AbilityContext, NewAbilityInstanceID))
			{
				ServerActivateAbility(AbilityClass, AbilityContext, NewAbilityInstanceID);
				return true;
			}

			break;
			
		case EAbilityActivationPolicy::ServerInitiated:
			// If we're not the server, we send a request to the server to activate the ability
			if (!GetOwner()->HasAuthority())
			{
				ServerActivateAbility(AbilityClass, AbilityContext, NewAbilityInstanceID);
				return true;
			}
				
			if (ActivateAbilityInternal(AbilityClass, AbilityContext, NewAbilityInstanceID))
			{
				MulticastActivateAbility(AbilityClass, AbilityContext, NewAbilityInstanceID);
				return true;
			}
			break;
			
		case EAbilityActivationPolicy::ServerInitiatedNonReliable:
			if (!GetOwner()->HasAuthority())
			{
				UE_LOG(LogTemp, Warning, TEXT("Client cannot activate non reliable server initiated abilities!"));
				return false;
			}
				
			if (ActivateAbilityInternal(AbilityClass, AbilityContext, NewAbilityInstanceID))
			{
				MulticastActivateAbilityUnreliable(AbilityClass, AbilityContext, NewAbilityInstanceID);
				return true;
			}
			break;
			
		case EAbilityActivationPolicy::ServerOnly:
			if (!GetOwner()->HasAuthority())
			{
				UE_LOG(LogTemp, Warning, TEXT("Client cannot activate server only abilities!"));
				return false;
			}
			
			return ActivateAbilityInternal(AbilityClass, AbilityContext, NewAbilityInstanceID);
	}
	
	return false;
}

void USimpleGameplayAbilityComponent::SendEvent(FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload,
	ESimpleEventReplicationPolicy ReplicationPolicy)
{
}

double USimpleGameplayAbilityComponent::GetServerTime_Implementation() const
{
	if (!GetWorld())
	{
		UE_LOG(LogTemp, Warning, TEXT("GetServerTime called but GetWorld is not valid!"));
		return 0.0;
	}

	if (!GetWorld()->GetGameState())
	{
		UE_LOG(LogTemp, Warning, TEXT("GetServerTime called but GetGameState is not valid!"));
		return 0.0;
	}
	
	return GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
}

void USimpleGameplayAbilityComponent::PushAbilityState(FGuid AbilityInstanceID, FSimpleAbilityState State)
{
	for (FActiveAbility& ActiveAbility : AuthorityActiveAbilities)
	{
		if (ActiveAbility.AbilityInstanceID == AbilityInstanceID)
		{
			ActiveAbility.AbilityStateHistory.Add(State);
			return;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("Ability with ID %s not found in InstancedAbilities array"), *AbilityInstanceID.ToString());
}

void USimpleGameplayAbilityComponent::OnAbilityActivated(const USimpleGameplayAbility* Ability)
{
	FActiveAbility NewActiveAbility;
	NewActiveAbility.AbilityInstanceID = Ability->AbilityInstanceID;
	NewActiveAbility.AbilityStatus = EAbilityStatus::Running;
	
	if (HasAuthority())
	{
		for (FActiveAbility& ActiveAbility : AuthorityActiveAbilities)
		{
			if (ActiveAbility.AbilityInstanceID == Ability->AbilityInstanceID)
			{
				return;
			}
		}
		
		AuthorityActiveAbilities.Add(NewActiveAbility);
	}
	else
	{
		for (FActiveAbility& ActiveAbility : LocalActiveAbilities)
		{
			if (ActiveAbility.AbilityInstanceID == Ability->AbilityInstanceID)
			{
				return;
			}
		}
		
		LocalActiveAbilities.Add(NewActiveAbility);
	}
}

void USimpleGameplayAbilityComponent::OnAbilityEnded(const USimpleGameplayAbility* Ability)
{
	if (HasAuthority())
	{
		for (FActiveAbility& ActiveAbility : AuthorityActiveAbilities)
		{
			if (ActiveAbility.AbilityInstanceID == Ability->AbilityInstanceID)
			{
				ActiveAbility.AbilityStatus = EAbilityStatus::Ended;
				return;
			}
		}
	}
	else
	{
		for (FActiveAbility& ActiveAbility : LocalActiveAbilities)
		{
			if (ActiveAbility.AbilityInstanceID == Ability->AbilityInstanceID)
			{
				ActiveAbility.AbilityStatus = EAbilityStatus::Ended;
				return;
			}
		}
	}
}

/* Replication */

// Attribute Replication
void USimpleGameplayAbilityComponent::OnRep_FloatAttributes()
{
}

void USimpleGameplayAbilityComponent::OnRep_StructAttributes()
{
}

// Ability Replication
void USimpleGameplayAbilityComponent::OnRep_AuthorityActiveAbilities()
{
}

bool USimpleGameplayAbilityComponent::ActivateAbilityInternal(const TSubclassOf<USimpleGameplayAbility>& AbilityClass, const FInstancedStruct& AbilityContext, FGuid AbilityInstanceID)
{
	// If we've already activated this ability we exit early
	if (HasAuthority())
	{
		for (FActiveAbility& ActiveAbility : AuthorityActiveAbilities)
		{
			if (ActiveAbility.AbilityInstanceID == AbilityInstanceID)
			{
				return false;
			}
		}
	}
	else
	{
		for (FActiveAbility& ActiveAbility : LocalActiveAbilities)
		{
			if (ActiveAbility.AbilityInstanceID == AbilityInstanceID)
			{
				return false;
			}
		}
	}
	
	bool IsSingleInstanceAbility = AbilityClass.GetDefaultObject()->InstancingPolicy == EAbilityInstancingPolicy::SingleInstanceCancellable ||
								   AbilityClass.GetDefaultObject()->InstancingPolicy == EAbilityInstancingPolicy::SingleInstanceNonCancellable;
	
	if (IsSingleInstanceAbility)
	{
		for (USimpleGameplayAbility* Ability : InstancedAbilities)
		{
			if (Ability->StaticClass() == AbilityClass)
			{
				if (Ability->InstancingPolicy == EAbilityInstancingPolicy::SingleInstanceCancellable)
				{
					if (Ability->IsAbilityActive())
					{
						Ability->EndAbility(FDefaultTags::AbilityCancelled, FInstancedStruct());
					}
				}
				else if (Ability->IsAbilityActive())
				{
					UE_LOG(LogTemp, Warning, TEXT("Single instance ability %s is already active!"), *Ability->GetName());
					return false;
				}

				Ability->InitializeAbility(this, AbilityInstanceID);
				return Ability->Activate(AbilityContext);
			}
		}
	}

	USimpleGameplayAbility* NewAbility = NewObject<USimpleGameplayAbility>(this, AbilityClass);
	NewAbility->InitializeAbility(this, AbilityInstanceID);

	if (NewAbility->Activate(AbilityContext))
	{
		InstancedAbilities.Add(NewAbility);
		return true;
	}

	NewAbility = nullptr;
	return false;
}

void USimpleGameplayAbilityComponent::ServerActivateAbility_Implementation(TSubclassOf<USimpleGameplayAbility> AbilityClass, FInstancedStruct AbilityContext, FGuid AbilityInstanceID)
{
	MulticastActivateAbility(AbilityClass, AbilityContext, AbilityInstanceID);
}

void USimpleGameplayAbilityComponent::MulticastActivateAbility_Implementation(TSubclassOf<USimpleGameplayAbility> AbilityClass, FInstancedStruct AbilityContext, FGuid AbilityInstanceID)
{
	ActivateAbilityInternal(AbilityClass, AbilityContext, AbilityInstanceID);
}

void USimpleGameplayAbilityComponent::MulticastActivateAbilityUnreliable_Implementation(TSubclassOf<USimpleGameplayAbility> AbilityClass, FInstancedStruct AbilityContext, FGuid AbilityInstanceID)
{
	ActivateAbilityInternal(AbilityClass, AbilityContext, AbilityInstanceID);
}

void USimpleGameplayAbilityComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USimpleGameplayAbilityComponent, AvatarActor);
	DOREPLIFETIME(USimpleGameplayAbilityComponent, GameplayTags);
	DOREPLIFETIME(USimpleGameplayAbilityComponent, AuthorityActiveAbilities);
}


