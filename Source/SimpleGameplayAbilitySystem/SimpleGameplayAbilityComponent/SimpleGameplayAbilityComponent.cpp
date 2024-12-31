#include "SimpleGameplayAbilityComponent.h"

#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "SimpleGameplayAbilitySystem/DefaultTags/DefaultTags.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleAbilityTypes.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleGameplayAbility/SimpleGameplayAbility.h"

USimpleGameplayAbilityComponent::USimpleGameplayAbilityComponent()
{
	SetIsReplicated(true);
	PrimaryComponentTick.bCanEverTick = false;
	AvatarActor = nullptr;
}

bool USimpleGameplayAbilityComponent::ActivateAbility(TSubclassOf<USimpleGameplayAbility> AbilityClass, FInstancedStruct AbilityContext, bool OverrideActivationPolicy, EAbilityActivationPolicy ActivationPolicy)
{
	const FGuid NewAbilityInstanceID = FGuid::NewGuid();
	const EAbilityActivationPolicy Policy = OverrideActivationPolicy ? ActivationPolicy : AbilityClass.GetDefaultObject()->ActivationPolicy;
	bool WasAbilityActivated = false;
	
	switch (Policy)
	{
		case EAbilityActivationPolicy::LocalOnly:
			WasAbilityActivated = ActivateAbilityInternal(AbilityClass, AbilityContext, NewAbilityInstanceID);
			break;
			
		case EAbilityActivationPolicy::LocalPredicted:
			WasAbilityActivated = ActivateAbilityInternal(AbilityClass, AbilityContext, NewAbilityInstanceID);

			if (WasAbilityActivated)
			{
				AddNewAbilityState(AbilityClass, AbilityContext, NewAbilityInstanceID, true);

				/* If we successfully activate a predicted ability from the client
				we send a request to the server to activate the same ability */
				if (!GetOwner()->HasAuthority())
				{
					ServerActivateAbility(AbilityClass, AbilityContext, NewAbilityInstanceID);
				}
			}

			break;
			
		case EAbilityActivationPolicy::ServerInitiated:
			// If we're not the server, we send a request to the server to activate the ability
			if (!GetOwner()->HasAuthority())
			{
				ServerActivateAbility(AbilityClass, AbilityContext, NewAbilityInstanceID);
				// Returning false because technically we didn't activate the ability for the client
				return false;
			}

			// If we're a listen server we can activate the ability directly 
			WasAbilityActivated = ActivateAbilityInternal(AbilityClass, AbilityContext, NewAbilityInstanceID);
			AddNewAbilityState(AbilityClass, AbilityContext, NewAbilityInstanceID, WasAbilityActivated);
				
			break;
			
		case EAbilityActivationPolicy::ServerOnly:
			if (!GetOwner()->HasAuthority())
			{
				UE_LOG(LogTemp, Warning, TEXT("Client cannot activate server only abilities!"));
				return false;
			}
			
			WasAbilityActivated = ActivateAbilityInternal(AbilityClass, AbilityContext, NewAbilityInstanceID);
			break;
	}
	
	return WasAbilityActivated;
}

void USimpleGameplayAbilityComponent::ServerActivateAbility_Implementation(TSubclassOf<USimpleGameplayAbility> AbilityClass, FInstancedStruct AbilityContext, FGuid AbilityInstanceID)
{
	const bool WasAbilityActivated = ActivateAbilityInternal(AbilityClass, AbilityContext, AbilityInstanceID);
	AddNewAbilityState(AbilityClass, AbilityContext, AbilityInstanceID, WasAbilityActivated);
}

bool USimpleGameplayAbilityComponent::CancelAbility(const FGuid AbilityInstanceID, FInstancedStruct CancellationContext)
{
	if (USimpleGameplayAbility* AbilityInstance = GetAbilityInstance(AbilityInstanceID))
	{
		AbilityInstance->EndCancel(CancellationContext);
		return true;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("Ability with ID %s not found in InstancedAbilities array"), *AbilityInstanceID.ToString());
	return false;
}

TArray<FGuid> USimpleGameplayAbilityComponent::CancelAbilitiesWithTags(const FGameplayTagContainer Tags, FInstancedStruct CancellationContext)
{
	TArray<FGuid> CancelledAbilities;
	
	for (USimpleGameplayAbility* AbilityInstance : InstancedAbilities)
	{
		if (AbilityInstance->AbilityTags.HasAnyExact(Tags))
		{
			AbilityInstance->EndCancel(CancellationContext);
			CancelledAbilities.Add(AbilityInstance->AbilityInstanceID);
		}
	}
	
	return CancelledAbilities;
}

void USimpleGameplayAbilityComponent::AddAbilityStateSnapshot(FGuid AbilityInstanceID, FSimpleAbilitySnapshot State)
{
	if (HasAuthority())
	{
		for (FAbilityState& ActiveAbility : AuthorityAbilityStates)
		{
			if (ActiveAbility.AbilityID == AbilityInstanceID)
			{
				ActiveAbility.SnapshotHistory.Add(State);
				return;
			}
		}	
	}
	else
	{
		for (FAbilityState& ActiveAbility : LocalAbilityStates)
		{
			if (ActiveAbility.AbilityID == AbilityInstanceID)
			{
				ActiveAbility.SnapshotHistory.Add(State);
				return;
			}
		}	
	}
	
	UE_LOG(LogTemp, Warning, TEXT("Ability with ID %s not found in InstancedAbilities array"), *AbilityInstanceID.ToString());
}

void USimpleGameplayAbilityComponent::GrantAbility(const TSubclassOf<USimpleGameplayAbility> AbilityClass)
{
	GrantedAbilities.AddUnique(AbilityClass);
}

void USimpleGameplayAbilityComponent::RevokeAbility(const TSubclassOf<USimpleGameplayAbility> AbilityClass)
{
	GrantedAbilities.Remove(AbilityClass);
}

void USimpleGameplayAbilityComponent::AddGameplayTags(FGameplayTagContainer Tags)
{
	GameplayTags.AppendTags(Tags);
}

void USimpleGameplayAbilityComponent::RemoveGameplayTags(FGameplayTagContainer Tags)
{
	GameplayTags.RemoveTags(Tags);
}

void USimpleGameplayAbilityComponent::SendEvent(FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload, ESimpleEventReplicationPolicy ReplicationPolicy)
{
}

// Utility Functions

bool USimpleGameplayAbilityComponent::ActivateAbilityInternal(TSubclassOf<USimpleGameplayAbility>& AbilityClass, const FInstancedStruct& AbilityContext, const FGuid AbilityInstanceID)
{
	if (!AbilityClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("AbilityClass is null!"));
		return false;
	}

	if (AbilityClass.GetDefaultObject()->bRequireGrantToActivate && !GrantedAbilities.Contains(AbilityClass))
	{
		UE_LOG(LogTemp, Warning, TEXT("Ability %s requires bring granted on the owning ability component to activate."), *AbilityClass->GetName());
		return false;
	}

	const EAbilityInstancingPolicy InstancingPolicy = AbilityClass.GetDefaultObject()->InstancingPolicy;

	if (InstancingPolicy == EAbilityInstancingPolicy::SingleInstanceCancellable)
	{
		for (USimpleGameplayAbility* InstancedAbility : InstancedAbilities)
		{
			if (InstancedAbility->GetClass() == AbilityClass)
			{
				if (InstancedAbility->IsAbilityActive())
				{
					InstancedAbility->EndCancel(FInstancedStruct());
				}
				
				InstancedAbility->InitializeAbility(this, AbilityInstanceID);
				return InstancedAbility->ActivateAbility(AbilityContext);
			}
		}
	}

	if (InstancingPolicy == EAbilityInstancingPolicy::SingleInstanceNonCancellable)
	{
		for (USimpleGameplayAbility* InstancedAbility : InstancedAbilities)
		{
			if (InstancedAbility->GetClass() == AbilityClass)
			{
				if (InstancedAbility->IsAbilityActive())
				{
					UE_LOG(LogTemp, Warning, TEXT("Non cancellable single instance ability %s is already active!"), *AbilityClass->GetName());
					return false;
				}
				
				InstancedAbility->InitializeAbility(this, AbilityInstanceID);
				return InstancedAbility->ActivateAbility(AbilityContext);
			}
		}
	}
	
	USimpleGameplayAbility* AbilityToActivate = NewObject<USimpleGameplayAbility>(this, AbilityClass);
	AbilityToActivate->InitializeAbility(this, AbilityInstanceID);

	const bool WasAbilityActivated = AbilityToActivate->ActivateAbility(AbilityContext);

	if (WasAbilityActivated)
	{
		InstancedAbilities.Add(AbilityToActivate);
	}
	
	return WasAbilityActivated;
}

void USimpleGameplayAbilityComponent::AddNewAbilityState(const TSubclassOf<USimpleGameplayAbility>& AbilityClass, const FInstancedStruct& AbilityContext, FGuid AbilityInstanceID, bool DidActivateSuccessfully)
{
	FAbilityState NewAbilityState;
	
	NewAbilityState.AbilityID = AbilityInstanceID;
	NewAbilityState.AbilityClass = AbilityClass;
	NewAbilityState.ActivationTimeStamp = GetServerTime();
	NewAbilityState.ActivationContext = AbilityContext;
	NewAbilityState.AbilityStatus = DidActivateSuccessfully ? EAbilityStatus::ActivationSuccess : EAbilityStatus::EndedActivationFailed;
	
	if (HasAuthority())
	{
		for (FAbilityState& AbilityState : AuthorityAbilityStates)
		{
			if (AbilityState.AbilityID == AbilityInstanceID)
			{
				UE_LOG(LogTemp, Warning, TEXT("Ability with ID %s already exists in AuthorityAbilityStates array, overwriting"), *AbilityInstanceID.ToString());
				AbilityState = NewAbilityState;
				return;
			}
		}
		
		AuthorityAbilityStates.Add(NewAbilityState);
	}
	else
	{
		for (FAbilityState& AbilityState : LocalAbilityStates)
		{
			if (AbilityState.AbilityID == AbilityInstanceID)
			{
				UE_LOG(LogTemp, Warning, TEXT("Ability with ID %s already exists in LocalAbilityStates array, overwriting"), *AbilityInstanceID.ToString());
				AbilityState = NewAbilityState;
				return;
			}
		}
		
		LocalAbilityStates.Add(NewAbilityState);
	}
}

USimpleGameplayAbility* USimpleGameplayAbilityComponent::GetAbilityInstance(FGuid AbilityInstanceID)
{
	for (USimpleGameplayAbility* InstancedAbility : InstancedAbilities)
	{
		if (InstancedAbility->AbilityInstanceID == AbilityInstanceID)
		{
			return InstancedAbility;
		}
	}
	
	return nullptr;
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

FAbilityState USimpleGameplayAbilityComponent::GetAbilityState(const FGuid AbilityInstanceID, bool& WasFound) const
{
	if (HasAuthority())
	{
		for (const FAbilityState& AbilityState : AuthorityAbilityStates)
		{
			if (AbilityState.AbilityID == AbilityInstanceID)
			{
				WasFound = true;
				return AbilityState;
			}
		}
	}
	else
	{
		for (const FAbilityState& AbilityState : LocalAbilityStates)
		{
			if (AbilityState.AbilityID == AbilityInstanceID)
			{
				WasFound = true;
				return AbilityState;
			}
		}
	}
	
	WasFound = false;
	return FAbilityState();
}

// Attribute Replication
void USimpleGameplayAbilityComponent::OnRep_FloatAttributes()
{
}

void USimpleGameplayAbilityComponent::OnRep_StructAttributes()
{
}

// Ability Replication
void USimpleGameplayAbilityComponent::OnRep_AuthorityAbilityStates()
{
	TMap<FGuid, int32> AuthorityAbilityStateMap;
	TMap<FGuid, int32> LocalAbilityStateMap;

	for (int32 i = 0; i < AuthorityAbilityStates.Num(); i++)
	{
		AuthorityAbilityStateMap.Add(AuthorityAbilityStates[i].AbilityID, i);
	}

	for (int32 i = 0; i < LocalAbilityStates.Num(); i++)
	{
		LocalAbilityStateMap.Add(LocalAbilityStates[i].AbilityID, i);
	}

	for (FAbilityState& AuthorityAbilityState : AuthorityAbilityStates)
	{
		/* If an ability exists in AuthorityAbilityStates but not in LocalAbilityStates,
		we run it locally (if it's running on the server) and then add the ability to LocalAbilityStates */
		if (!LocalAbilityStateMap.Contains(AuthorityAbilityState.AbilityID))
		{
			if (AuthorityAbilityState.AbilityStatus == EAbilityStatus::ActivationSuccess)
			{
				TSubclassOf<USimpleGameplayAbility> GameplayAbilityClass = TSubclassOf<USimpleGameplayAbility>(AuthorityAbilityState.AbilityClass);
				ActivateAbilityInternal(GameplayAbilityClass, AuthorityAbilityState.ActivationContext, AuthorityAbilityState.AbilityID);
			}
			
			LocalAbilityStates.Add(AuthorityAbilityState);
			LocalAbilityStateMap.Add(AuthorityAbilityState.AbilityID, LocalAbilityStates.Num() - 1);
		}

		// The ability state exists locally, let's cache a reference to it
		FAbilityState& LocalAbilityState = LocalAbilityStates[LocalAbilityStateMap[AuthorityAbilityState.AbilityID]];
		
		// Check if the ability activated locally but failed activation on the server
		if (AuthorityAbilityState.AbilityStatus == EAbilityStatus::EndedActivationFailed)
		{
			if (LocalAbilityState.AbilityStatus != EAbilityStatus::EndedActivationFailed)
			{
				if (USimpleGameplayAbility* AbilityInstance = GetAbilityInstance(AuthorityAbilityState.AbilityID))
				{
					AbilityInstance->EndCancel(FInstancedStruct());
				}
			}
		}

		// Lastly we check if there are any new snapshots to handle within the local ability
	}
}

// Variable Replication
void USimpleGameplayAbilityComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USimpleGameplayAbilityComponent, AvatarActor);
	DOREPLIFETIME(USimpleGameplayAbilityComponent, GameplayTags);
	DOREPLIFETIME(USimpleGameplayAbilityComponent, AuthorityAbilityStates);
}


