#include "SimpleGameplayAbilityComponent.h"

#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "SimpleGameplayAbilitySystem/DataAssets/AbilitySet/AbilitySet.h"
#include "SimpleGameplayAbilitySystem/SimpleEventSubsystem/SimpleEventSubsystem.h"
#include "SimpleGameplayAbilitySystem/SimpleGameplayAbility/SimpleGameplayAbility.h"
#include "SimpleGameplayAbilitySystem/SimpleGameplayAttributes/SimpleGameplayAttributes.h"
#include "SimpleGameplayAbilitySystem/SimpleGASTypes/DefaultTags/DefaultTags.h"

USimpleGameplayAbilityComponent::USimpleGameplayAbilityComponent()
{	
	SetIsReplicated(true);
	bReplicateUsingRegisteredSubObjectList = true;
	Attributes = CreateDefaultSubobject<USimpleGameplayAttributes>(TEXT("GameplayAttributes"));
}

void USimpleGameplayAbilityComponent::BeginPlay()
{
	Super::BeginPlay();
	
	if (USimpleEventSubsystem* EventSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<USimpleEventSubsystem>())
	{
		FGameplayTagContainer EventTags;
		FGameplayTagContainer DomainTags;
		DomainTags.AddTag(FDefaultTags::AbilityDomain);
		FSimpleEventDelegate EndedEventDelegate;

		// Listen for ability started event
		EventTags.AddTag(FDefaultTags::AbilityActivated);
		FSimpleEventDelegate StartedEventDelegate;
		EndedEventDelegate.BindDynamic(this, &USimpleGameplayAbilityComponent::OnAbilityStartedInternal);
		EventSubsystem->ListenForEvent(this, false, EventTags, DomainTags, EndedEventDelegate, TArray<UScriptStruct*>(), TArray<AActor*>());

		// Listen for ability ended event
		EventTags.Reset();
		EventTags.AddTag(FDefaultTags::AbilityEnded);
		EndedEventDelegate.BindDynamic(this, &USimpleGameplayAbilityComponent::OnAbilityEndedInternal);
		EventSubsystem->ListenForEvent(this, false, EventTags, DomainTags, EndedEventDelegate, TArray<UScriptStruct*>(), TArray<AActor*>());
	}

	/* Grant abilities from ability sets */
	if (GetOwner()->HasAuthority())
	{
		for (UAbilitySet* AbilitySet : GrantedAbilitySets)
		{
			for (TSubclassOf<USimpleGameplayAbility> Ability : AbilitySet->AbilitiesToGrant)
			{
				GrantAbility(Ability, false, FInstancedStruct());
			}
		}

		AddReplicatedSubObject(Attributes);
	}
}

void USimpleGameplayAbilityComponent::AddGameplayTags(FGameplayTagContainer Tags)
{
	GameplayTags.AppendTags(Tags);
}

void USimpleGameplayAbilityComponent::RemoveGameplayTags(FGameplayTagContainer Tags)
{
	GameplayTags.RemoveTags(Tags);
}

AActor* USimpleGameplayAbilityComponent::GetAvatarActor() const
{
	return AvatarActor;
}

void USimpleGameplayAbilityComponent::SetAvatarActor(AActor* NewAvatarActor)
{
	if (!GetOwner()->HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("Client cannot set avatar actor!"));
		return;
	}
	
	AvatarActor = NewAvatarActor;
}

void USimpleGameplayAbilityComponent::GrantAbility(TSubclassOf<USimpleGameplayAbility> Ability, bool AutoActivateGrantedAbility, FInstancedStruct AutoActivatedAbilityContext)
{
	if (!GetOwner()->HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("Client cannot grant abilities!"));
	}

	/* Make sure we only grant unique abilities i.e. can't have the same class twice or two abilities
	 * with the same tag (even if they are different classes)
	 */ 
	FSimpleGameplayAbilityConfig IncomingAbilityConfig = GetAbilityConfig(Ability);
	for (TSubclassOf<USimpleGameplayAbility> GrantedAbility : GrantedAbilities)
	{
		FSimpleGameplayAbilityConfig GrantedAbilityConfig = GetAbilityConfig(GrantedAbility);
		if (GrantedAbilityConfig.AbilityName.MatchesTagExact(IncomingAbilityConfig.AbilityName))
		{
			UE_LOG(LogTemp, Warning, TEXT("An ability with tag %s is already granted to this component"), *IncomingAbilityConfig.AbilityName.ToString());
			return;
		}
	}
	
	GrantedAbilities.AddUnique(Ability);

	if (AutoActivateGrantedAbility)
	{
		ActivateAbility(Ability, AutoActivatedAbilityContext);
	}
}

void USimpleGameplayAbilityComponent::RevokeAbility(TSubclassOf<USimpleGameplayAbility> Ability, bool CancelRunningInstances)
{
	if (!GetOwner()->HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("Client cannot revoke abilities!"));
	}

	if (CancelRunningInstances)
	{
		FSimpleGameplayAbilityConfig AbilityConfig = GetAbilityConfig(Ability);
		for (USimpleGameplayAbility* RunningAbility : RunningAbilities)
		{
			if (RunningAbility->AbilityConfig.AbilityName.MatchesTagExact(AbilityConfig.AbilityName))
			{
				RunningAbility->EndAbilityCancel();
			}
		}
	}
	
	GrantedAbilities.Remove(Ability);
}

/* Ability activation functions */

bool USimpleGameplayAbilityComponent::ActivateAbility(TSubclassOf<USimpleGameplayAbility> AbilityClass, FInstancedStruct AbilityContext, bool OnlyActivateIfGranted)
{
	if (OnlyActivateIfGranted && !GrantedAbilities.Contains(AbilityClass))
	{
		return false;
	}
	
	FSimpleGameplayAbilityConfig AbilityConfig = GetAbilityConfig(AbilityClass);
	const FGuid NewAbilityInstanceID = FGuid::NewGuid();
	
	switch (AbilityConfig.ActivationPolicy)
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
		default:
			break;
	}

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

bool USimpleGameplayAbilityComponent::ActivateAbilityInternal(const TSubclassOf<USimpleGameplayAbility>& AbilityClass, const FInstancedStruct& AbilityContext, FGuid AbilityInstanceID)
{
	// If we've already activated this ability we exit early
	if (FindAnyAbilityInstanceByID(AbilityInstanceID))
	{
		return false;
	}
	
	FSimpleGameplayAbilityConfig AbilityConfig = GetAbilityConfig(AbilityClass);
	bool IsSingleInstanceAbility = AbilityConfig.InstancingPolicy == EAbilityInstancingPolicy::SingleInstanceCancellable ||
	                               AbilityConfig.InstancingPolicy == EAbilityInstancingPolicy::SingleInstanceNonCancellable;
	
	if (IsSingleInstanceAbility)
	{
		for (USimpleGameplayAbility* Ability : RunningAbilities)
		{
			if (Ability->AbilityConfig.AbilityName.MatchesTagExact(AbilityConfig.AbilityName))
			{
				if (AbilityConfig.InstancingPolicy == EAbilityInstancingPolicy::SingleInstanceCancellable)
				{
					if (Ability->IsAbilityActive())
					{
						Ability->EndAbilityCancel();
					}
				}
				else if (Ability->IsAbilityActive())
				{
					UE_LOG(LogTemp, Warning, TEXT("Single instance ability %s is already active!"), *AbilityConfig.AbilityName.ToString());
					return false;
				}

				Ability->InitializeAbility(this, AbilityInstanceID, GetServerTime());
				return Ability->Activate(AbilityContext);
			}
		}
	}

	USimpleGameplayAbility* NewAbility = NewObject<USimpleGameplayAbility>(this, AbilityClass);
	NewAbility->InitializeAbility(this, AbilityInstanceID, GetServerTime());

	if (GetOwner()->HasAuthority())
	{
		AddReplicatedSubObject(NewAbility);
		RunningAbilities.Add(NewAbility);
	}
	else
	{
		LocalRunningAbilities.Add(NewAbility);
	}

	if (NewAbility->Activate(AbilityContext))
	{
		return true;
	}
	
	return false;
}

/* Event functions */

void USimpleGameplayAbilityComponent::SendEvent(FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload, ESimpleEventReplicationPolicy ReplicationPolicy)
{
	USimpleEventSubsystem* EventSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<USimpleEventSubsystem>();

	if (!EventSubsystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("EventSubsystem is not valid!"));
		return;
	}

	FGuid EventID = FGuid::NewGuid();
	
	switch (ReplicationPolicy)
	{
		case ESimpleEventReplicationPolicy::NoReplication:
			EventSubsystem->SendEvent(EventTag, DomainTag, Payload, GetOwner());
			break;
		case ESimpleEventReplicationPolicy::ClientToServer:
			if (GetOwner()->HasAuthority())
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to send event %s. Server cannot send ClientToServer events!"), *EventTag.ToString());
				return;;
			}
		
			EventSubsystem->SendEvent(EventTag, DomainTag, Payload, GetOwner());
			ServerSendEvent(EventTag, DomainTag, Payload);
			break;
		case ESimpleEventReplicationPolicy::ServerToClient:
			if (!GetOwner()->HasAuthority())
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to send event %s. Client cannot send ServerToClient events!"), *EventTag.ToString());
				return;;
			}
		
			EventSubsystem->SendEvent(EventTag, DomainTag, Payload, GetOwner());
			ClientSendEvent(EventTag, DomainTag, Payload);
			break;
		case ESimpleEventReplicationPolicy::ServerToClientPredicted:
			if (!GetOwner()->HasAuthority())
			{
				ServerSendClientEvent(EventTag, DomainTag, Payload);
				return;
			}
		
			EventSubsystem->SendEvent(EventTag, DomainTag, Payload, GetOwner());
			ClientSendEvent(EventTag, DomainTag, Payload);
			break;
		case ESimpleEventReplicationPolicy::ServerToAll:
			if (!GetOwner()->HasAuthority())
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to send event %s. Client cannot send ServerToAll events!"), *EventTag.ToString());
				return;
			}
		
			MulticastSendEvent(EventTag, DomainTag, Payload, EventID);
			break;
		case ESimpleEventReplicationPolicy::ServerToAllPredicted:
			if (GetOwner()->HasAuthority())
			{
				ServerSendMulticastEvent(EventTag, DomainTag, Payload, EventID);
				return;
			}

			SendEventInternal(EventTag, DomainTag, Payload);
			LocalPredictedEventIDs.Add(EventID);
			ServerSendMulticastEvent(EventTag, DomainTag, Payload, EventID);
			break;
	}
}

bool USimpleGameplayAbilityComponent::ApplyGameplayEffect(const USimpleGameplayAbilityComponent* Instigator,
	const TSubclassOf<USimpleGameplayEffect> GameplayEffect, FInstancedStruct EffectContext)
{
	return false;	
}

bool USimpleGameplayAbilityComponent::ApplyPendingGameplayEffect(const FPendingGameplayEffect PendingEffect)
{
	return ApplyGameplayEffect(PendingEffect.Instigator, PendingEffect.EffectClass, PendingEffect.EffectContext);
}

void USimpleGameplayAbilityComponent::SendEventInternal(FGameplayTag EventTag, FGameplayTag DomainTag, const FInstancedStruct& Payload) const
{
	USimpleEventSubsystem* EventSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<USimpleEventSubsystem>();

	if (!EventSubsystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("EventSubsystem is not valid!"));
		return;
	}
	
	EventSubsystem->SendEvent(EventTag, DomainTag, Payload, GetOwner());
}

void USimpleGameplayAbilityComponent::ClientSendEvent_Implementation(FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload)
{
	SendEventInternal(EventTag, DomainTag, Payload);
}

void USimpleGameplayAbilityComponent::ServerSendEvent_Implementation(FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload)
{
	SendEventInternal(EventTag, DomainTag, Payload);
}

void USimpleGameplayAbilityComponent::ServerSendMulticastEvent_Implementation(FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload, FGuid EventID)
{
	MulticastSendEvent(EventTag, DomainTag, Payload, EventID);
}

void USimpleGameplayAbilityComponent::ServerSendClientEvent_Implementation(FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload)
{
	SendEventInternal(EventTag, DomainTag, Payload);
	ClientSendEvent(EventTag, DomainTag, Payload);
}

void USimpleGameplayAbilityComponent::MulticastSendEvent_Implementation(FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload, FGuid EventID)
{
	if (LocalPredictedEventIDs.Contains(EventID))
	{
		LocalPredictedEventIDs.Remove(EventID);
		return;
	}
	
	SendEventInternal(EventTag, DomainTag, Payload);
}

/* Utility functions */

FSimpleGameplayAbilityConfig USimpleGameplayAbilityComponent::GetAbilityConfig(TSubclassOf<USimpleGameplayAbility> Ability)
{
	if (!Ability)
	{
		UE_LOG(LogTemp, Warning, TEXT("Ability class in GetAbilityConfig is not valid. Returning default config"));
		return FSimpleGameplayAbilityConfig();
	}
	
	return Ability.GetDefaultObject()->AbilityConfig;
}

USimpleGameplayAbility* USimpleGameplayAbilityComponent::FindAnyAbilityInstanceByID(const FGuid AbilityInstanceID)
{
	for (USimpleGameplayAbility* Ability : RunningAbilities)
	{
		if (Ability->AbilityInstanceID == AbilityInstanceID)
		{
			return Ability;
		}
	}

	for (USimpleGameplayAbility* Ability : LocalRunningAbilities)
	{
		if (Ability->AbilityInstanceID == AbilityInstanceID)
		{
			return Ability;
		}
	}

	return nullptr;
}

USimpleGameplayAbility* USimpleGameplayAbilityComponent::FindAuthorityAbilityInstanceByID(const FGuid AbilityInstanceID)
{
	for (USimpleGameplayAbility* Ability : RunningAbilities)
	{
		if (Ability->AbilityInstanceID == AbilityInstanceID)
		{
			return Ability;
		}
	}

	return nullptr;
}

USimpleGameplayAbility* USimpleGameplayAbilityComponent::FindPredictedAbilityInstanceByID(const FGuid AbilityInstanceID)
{
	for (USimpleGameplayAbility* Ability : LocalRunningAbilities)
	{
		if (Ability->AbilityInstanceID == AbilityInstanceID)
		{
			return Ability;
		}
	}

	return nullptr;
}

TSubclassOf<USimpleGameplayAbility> USimpleGameplayAbilityComponent::FindGrantedAbilityByTag(FGameplayTag AbilityTag)
{
	for (TSubclassOf<USimpleGameplayAbility> Ability : GrantedAbilities)
	{
		if (Ability.GetDefaultObject()->AbilityConfig.AbilityName == AbilityTag)
		{
			return Ability;
		}
	}

	return nullptr;
}

/* Override functions */

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

void USimpleGameplayAbilityComponent::OnAbilityStartedInternal(FGameplayTag AbilityTag, FGameplayTag DomainTag, FInstancedStruct Payload)
{
	if (const FAbilityActivationStateChangedData* StateChangeData = Payload.GetPtr<FAbilityActivationStateChangedData>())
	{
		if (USimpleGameplayAbility* Ability = FindAnyAbilityInstanceByID(StateChangeData->AbilityInstanceID))
		{
			FGameplayTagContainer AbilityTagsToCancel = Ability->AbilityConfig.TagConfig.CancelAbilitiesWithTag;

			if (GetOwner()->HasAuthority())
			{
				for (USimpleGameplayAbility* RunningAbility : RunningAbilities)
				{
					if (AbilityTagsToCancel.HasAnyExact(RunningAbility->AbilityConfig.TagConfig.AbilityTags))
					{
						RunningAbility->EndAbilityCancel();
					}
				}
			}
			else
			{
				for (USimpleGameplayAbility* RunningAbility : LocalRunningAbilities)
				{
					if (AbilityTagsToCancel.HasAnyExact(RunningAbility->AbilityConfig.TagConfig.AbilityTags))
					{
						RunningAbility->EndAbilityCancel();
					}
				}
			}

			OnAbilityStarted(Ability);
		}
	}
}

void USimpleGameplayAbilityComponent::OnAbilityEndedInternal(FGameplayTag AbilityTag, FGameplayTag DomainTag,  FInstancedStruct Payload)
{
	/* Delete multi instance abilities on the server once they're done.
	   Local predicted multi instance abilities are deleted on the client when the RunningAbilities array is replicated */
	if (const FAbilityActivationStateChangedData* StateChangeData = Payload.GetPtr<FAbilityActivationStateChangedData>())
	{
		if (USimpleGameplayAbility* Ability = FindAnyAbilityInstanceByID(StateChangeData->AbilityInstanceID))
		{
			// Call the BP event before we remove and delete the ability
			OnAbilityEnded(Ability);
			
			if (Ability->AbilityConfig.InstancingPolicy == EAbilityInstancingPolicy::MultipleInstances)
			{
				if (GetOwner()->HasAuthority())
				{
					RunningAbilities.Remove(Ability);
					RemoveReplicatedSubObject(Ability);

					UE_LOG(LogTemp, Warning, TEXT("Multi instance ability %s is no longer running. Removing."), *Ability->AbilityConfig.AbilityName.ToString());
				}
			}
		}
	}
}

/* Replication functions */

void USimpleGameplayAbilityComponent::OnRep_RunningAbilities()
{
	/* We go through each local ability and check if it still has
	 * a corresponding ability in the authoritative RunningAbilities array*/
	for (USimpleGameplayAbility* Ability : LocalRunningAbilities)
	{
		// We only want to do cleanup when the local ability is no longer running
		if (Ability->IsAbilityActive())
		{
			continue;
		}

		// Check if this ability still exists on the server
		for (USimpleGameplayAbility* RunningAbility : RunningAbilities)
		{
			if (RunningAbility->AbilityInstanceID == Ability->AbilityInstanceID)
			{
				return;
			}
		}

		// Finally, remove the local ability if it's no longer running on the server or the client
		LocalRunningAbilities.Remove(Ability);
		UE_LOG(LogTemp, Warning, TEXT("Local ability %s is no longer running on the server. Removing."), *Ability->AbilityConfig.AbilityName.ToString());
	}
}

void USimpleGameplayAbilityComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USimpleGameplayAbilityComponent, AvatarActor);
	DOREPLIFETIME(USimpleGameplayAbilityComponent, GameplayTags);
	DOREPLIFETIME(USimpleGameplayAbilityComponent, Attributes);
	DOREPLIFETIME(USimpleGameplayAbilityComponent, RunningAbilities);
	DOREPLIFETIME(USimpleGameplayAbilityComponent, GrantedAbilities);
}

