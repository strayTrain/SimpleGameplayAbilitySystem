#include "SimpleGameplayAbilityComponent.h"

#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "SimpleGameplayAbilitySystem/DataAssets/AbilitySet/AbilitySet.h"
#include "SimpleGameplayAbilitySystem/DefaultTags/DefaultTags.h"
#include "SimpleGameplayAbilitySystem/Module/SimpleGameplayAbilitySystem.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleAbilityTypes.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleGameplayAbility/SimpleGameplayAbility.h"
#include "SimpleGameplayAbilitySystem/SimpleEventSubsystem/SimpleEventSubSystem.h"

class USimpleEventSubsystem;

USimpleGameplayAbilityComponent::USimpleGameplayAbilityComponent()
{
	SetIsReplicated(true);
	PrimaryComponentTick.bCanEverTick = false;
	AvatarActor = nullptr;
}

void USimpleGameplayAbilityComponent::BeginPlay()
{
	UActorComponent::BeginPlay();

	if (HasAuthority())
	{
		// Grant abilities to the owning actor
		for (UAbilitySet* AbilitySet : AbilitySets)
		{
			for (const TSubclassOf<USimpleGameplayAbility> AbilityClass : AbilitySet->AbilitiesToGrant)
			{
				GrantAbility(AbilityClass);
			}
		}

		// Create attributes from the provided attribute sets
	}
}

/* Ability Functions */

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
				UE_LOG(LogSimpleGAS, Warning, TEXT("Client cannot activate server only abilities!"));
				return false;
			}
			
			WasAbilityActivated = ActivateAbilityInternal(AbilityClass, AbilityContext, NewAbilityInstanceID);
			break;
	}
	
	return WasAbilityActivated;
}

bool USimpleGameplayAbilityComponent::ActivateAbilityInternal(TSubclassOf<USimpleGameplayAbility>& AbilityClass, const FInstancedStruct& AbilityContext, const FGuid AbilityInstanceID)
{
	if (!AbilityClass)
	{
		UE_LOG(LogSimpleGAS, Warning, TEXT("AbilityClass is null!"));
		return false;
	}

	if (AbilityClass.GetDefaultObject()->bRequireGrantToActivate && !GrantedAbilities.Contains(AbilityClass))
	{
		UE_LOG(LogSimpleGAS, Warning, TEXT("Ability %s requires bring granted on the owning ability component to activate."), *AbilityClass->GetName());
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
					UE_LOG(LogSimpleGAS, Warning, TEXT("Non cancellable single instance ability %s is already active!"), *AbilityClass->GetName());
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
	
	UE_LOG(LogSimpleGAS, Warning, TEXT("Ability with ID %s not found in InstancedAbilities array"), *AbilityInstanceID.ToString());
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

void USimpleGameplayAbilityComponent::GrantAbility(const TSubclassOf<USimpleGameplayAbility> AbilityClass)
{
	GrantedAbilities.AddUnique(AbilityClass);
}

void USimpleGameplayAbilityComponent::RevokeAbility(const TSubclassOf<USimpleGameplayAbility> AbilityClass)
{
	GrantedAbilities.Remove(AbilityClass);
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
	
	UE_LOG(LogSimpleGAS, Warning, TEXT("Ability with ID %s not found in InstancedAbilities array"), *AbilityInstanceID.ToString());
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
				UE_LOG(LogSimpleGAS, Warning, TEXT("Ability with ID %s already exists in AuthorityAbilityStates array, overwriting"), *AbilityInstanceID.ToString());
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
				UE_LOG(LogSimpleGAS, Warning, TEXT("Ability with ID %s already exists in LocalAbilityStates array, overwriting"), *AbilityInstanceID.ToString());
				AbilityState = NewAbilityState;
				return;
			}
		}
		
		LocalAbilityStates.Add(NewAbilityState);
	}
}

/* Attribute Functions */

void USimpleGameplayAbilityComponent::AddFloatAttribute(FFloatAttribute AttributeToAdd, bool OverrideValuesIfExists)
{
	const int32 AttributeIndex = FloatAttributes.Find(AttributeToAdd);

	// This is a new attribute
	if (AttributeIndex == INDEX_NONE)
	{
		FloatAttributes.Add(AttributeToAdd);
		return;
	}

	// Attribute exists but we don't want to override it
	if (!OverrideValuesIfExists)
	{
		return;
	}

	// Attribute exists and we want to override it
	FloatAttributes[AttributeIndex] = AttributeToAdd;
}

void USimpleGameplayAbilityComponent::RemoveFloatAttribute(FGameplayTag AttributeTag)
{
	FloatAttributes.RemoveAll([AttributeTag](const FFloatAttribute& Attribute) { return Attribute.AttributeTag == AttributeTag; });
}

void USimpleGameplayAbilityComponent::AddStructAttribute(FStructAttribute AttributeToAdd, bool OverrideValuesIfExists)
{
	const int32 AttributeIndex = StructAttributes.Find(AttributeToAdd);

	// This is a new attribute
	if (AttributeIndex == INDEX_NONE)
	{
		StructAttributes.Add(AttributeToAdd);
		return;
	}

	// Attribute exists but we don't want to override it
	if (!OverrideValuesIfExists)
	{
		return;
	}

	// Attribute exists and we want to override it
	StructAttributes[AttributeIndex] = AttributeToAdd;
}

void USimpleGameplayAbilityComponent::RemoveStructAttribute(FGameplayTag AttributeTag)
{
	StructAttributes.RemoveAll([AttributeTag](const FStructAttribute& Attribute) { return Attribute.AttributeTag == AttributeTag; });
}

/* Tag Functions */

void USimpleGameplayAbilityComponent::AddGameplayTag(FGameplayTag Tag, FInstancedStruct Payload)
{
	GameplayTags.AddTag(Tag);
}

void USimpleGameplayAbilityComponent::RemoveGameplayTag(FGameplayTag Tag, FInstancedStruct Payload)
{
	GameplayTags.RemoveTag(Tag);
}

/* Event Functions */

void USimpleGameplayAbilityComponent::SendEvent(FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload, AActor* Sender, ESimpleEventReplicationPolicy ReplicationPolicy)
{
	const FGuid EventID = FGuid::NewGuid();

	switch (ReplicationPolicy)
	{
		case ESimpleEventReplicationPolicy::NoReplication:
			SendEventInternal(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy);
			return;
			
		case ESimpleEventReplicationPolicy::ServerAndOwningClient:
			if (!HasAuthority())
			{
				ServerSendEvent(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy);
				return;
			}
		
			SendEventInternal(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy);
			ClientSendEvent(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy);
			break;

		case ESimpleEventReplicationPolicy::ServerAndOwningClientPredicted:
			SendEventInternal(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy);
			ServerSendEvent(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy);
			break;
		
		case ESimpleEventReplicationPolicy::AllConnectedClients:
			if (!HasAuthority())
			{
				ServerSendEvent(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy);
				return;
			}

			MulticastSendEvent(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy);
			break;

		case ESimpleEventReplicationPolicy::AllConnectedClientsPredicted:
			SendEventInternal(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy);
			ServerSendEvent(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy);
			break;
	}
}

void USimpleGameplayAbilityComponent::SendEventInternal(FGuid EventID, FGameplayTag EventTag, FGameplayTag DomainTag,
                                                        const FInstancedStruct& Payload, AActor* Sender, ESimpleEventReplicationPolicy ReplicationPolicy)
{
	USimpleEventSubsystem* EventSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<USimpleEventSubsystem>();
	
	if (!EventSubsystem)
	{
		UE_LOG(LogSimpleGAS, Warning, TEXT("[USimpleGameplayAbilityComponent::SendEventInternal]: No SimpleEventSubsystem found."));
		return;
	}
	
	if (HandledEventIDs.Contains(EventID))
	{
		UE_LOG(LogSimpleGAS, Display, TEXT("Event %s with ID %s has already been handled locally"), *EventTag.ToString(), *EventID.ToString());
		//HandledEventIDs.Remove(EventID);
		return;
	}
	
	EventSubsystem->SendEvent(EventTag, DomainTag, Payload, Sender);

	// No need to keep track of handled events if we're not replicating
	if (ReplicationPolicy == ESimpleEventReplicationPolicy::NoReplication)
	{
		return;
	}
	
	HandledEventIDs.Add(EventID);
}

void USimpleGameplayAbilityComponent::ServerSendEvent_Implementation(FGuid EventID,
                                                                     FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload, AActor* Sender, ESimpleEventReplicationPolicy ReplicationPolicy)
{
	switch (ReplicationPolicy)
	{
		case ESimpleEventReplicationPolicy::ServerAndOwningClient:
			SendEventInternal(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy);
			ClientSendEvent(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy);
			break;
		
		case ESimpleEventReplicationPolicy::ServerAndOwningClientPredicted:
			SendEventInternal(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy);
			ClientSendEvent(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy);
			break;
		
		case ESimpleEventReplicationPolicy::AllConnectedClients:
			MulticastSendEvent(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy);
			break;
		
		case ESimpleEventReplicationPolicy::AllConnectedClientsPredicted:
			MulticastSendEvent(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy);
			break;
		default:
			break;
	}
}

void USimpleGameplayAbilityComponent::ClientSendEvent_Implementation(FGuid EventID,
                                                                     FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload,
                                                                     AActor* Sender, ESimpleEventReplicationPolicy ReplicationPolicy)
{
	SendEventInternal(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy);
}

void USimpleGameplayAbilityComponent::MulticastSendEvent_Implementation(FGuid EventID,
                                                                        FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload, AActor
                                                                        * Sender, ESimpleEventReplicationPolicy ReplicationPolicy)
{
	SendEventInternal(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy);
}

// Utility Functions

void USimpleGameplayAbilityComponent::RemoveInstancedAbility(USimpleGameplayAbility* AbilityToRemove)
{
	InstancedAbilities.Remove(AbilityToRemove);
}

void USimpleGameplayAbilityComponent::UpdateAbilityStatus(const FGuid AbilityInstanceID, const EAbilityStatus NewStatus)
{
	if (HasAuthority())
	{
		for (FAbilityState& AbilityState : AuthorityAbilityStates)
		{
			if (AbilityState.AbilityID == AbilityInstanceID)
			{
				AbilityState.AbilityStatus = NewStatus;
				return;
			}
		}
	}
	else
	{
		for (FAbilityState& AbilityState : LocalAbilityStates)
		{
			if (AbilityState.AbilityID == AbilityInstanceID)
			{
				AbilityState.AbilityStatus = NewStatus;
				return;
			}
		}
	}
	
	UE_LOG(LogSimpleGAS, Warning, TEXT("Ability with ID %s not found in AuthorityAbilityStates array"), *AbilityInstanceID.ToString());
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
		UE_LOG(LogSimpleGAS, Warning, TEXT("GetServerTime called but GetWorld is not valid!"));
		return 0.0;
	}

	if (!GetWorld()->GetGameState())
	{
		UE_LOG(LogSimpleGAS, Warning, TEXT("GetServerTime called but GetGameState is not valid!"));
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

		// TODO: Lastly we check if there are any new snapshots to handle within the local ability
	}
}

// Variable Replication
void USimpleGameplayAbilityComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USimpleGameplayAbilityComponent, AvatarActor);
	DOREPLIFETIME(USimpleGameplayAbilityComponent, GameplayTags);
	DOREPLIFETIME(USimpleGameplayAbilityComponent, AuthorityAbilityStates);
	DOREPLIFETIME(USimpleGameplayAbilityComponent, GrantedAbilities);
}


