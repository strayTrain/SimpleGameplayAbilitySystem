#include "SimpleAbilityComponent.h"

#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "SimpleGameplayAbilitySystem/BlueprintFunctionLibraries/SimpleAttributes/SimpleAttributeFunctionLibrary.h"
#include "SimpleGameplayAbilitySystem/DataAssets/AbilitySet/AbilitySet.h"
#include "SimpleGameplayAbilitySystem/DataAssets/AttributeSet/AttributeSet.h"
#include "SimpleGameplayAbilitySystem/SimpleEventSubsystem/SimpleEventSubsystem.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleAbility.h"
#include "SimpleGameplayAbilitySystem/DefaultTags/DefaultTags.h"
#include "SimpleGameplayAbilitySystem/SimpleAttributeModifier/SimpleAttributeModifier.h"
#include "SimpleGameplayAbilitySystem/SimpleAttributeModifier/SimpleAttributeModifierTypes.h"

USimpleAbilityComponent::USimpleAbilityComponent()
{	
	SetIsReplicated(true);
	bReplicateUsingRegisteredSubObjectList = true;
}

void USimpleAbilityComponent::BeginPlay()
{
	Super::BeginPlay();
	
	if (USimpleEventSubsystem* EventSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<USimpleEventSubsystem>())
	{
		FGameplayTagContainer EventTags;
		FGameplayTagContainer DomainTags;
		DomainTags.AddTag(FDefaultTags::AbilityDomain);

		// Listen for ability started event
		EventTags.AddTag(FDefaultTags::AbilityActivated);
		FSimpleEventDelegate StartedEventDelegate;
		StartedEventDelegate.BindDynamic(this, &USimpleAbilityComponent::OnAbilityStartedInternal);
		EventSubsystem->ListenForEvent(this, false, EventTags, DomainTags, StartedEventDelegate, TArray<UScriptStruct*>(), TArray<AActor*>());

		// Listen for ability ended event
		EventTags.Reset();
		EventTags.AddTag(FDefaultTags::AbilityEnded);
		FSimpleEventDelegate EndedEventDelegate;
		EndedEventDelegate.BindDynamic(this, &USimpleAbilityComponent::OnAbilityEndedInternal);
		EventSubsystem->ListenForEvent(this, false, EventTags, DomainTags, EndedEventDelegate, TArray<UScriptStruct*>(), TArray<AActor*>());
	}
	
	if (GetOwner()->HasAuthority())
	{
		/* Grant abilities from ability sets */
		for (UAbilitySet* AbilitySet : GrantedAbilitySets)
		{
			for (TSubclassOf<USimpleAbility> Ability : AbilitySet->AbilitiesToGrant)
			{
				GrantAbility(Ability, false, FInstancedStruct());
			}
		}

		/* Add attributes from attribute sets */
		for (UAttributeSet* AttributeSet : AttributeSets)
		{
			for (const FFloatAttribute& Attribute : AttributeSet->FloatAttributes)
			{
				AddFloatAttribute(Attribute);
			}

			for (const FStructAttribute& Attribute : AttributeSet->StructAttributes)
			{
				AddStructAttribute(Attribute);
			}
		}
	}
}

/* Ability */

void USimpleAbilityComponent::GrantAbility(TSubclassOf<USimpleAbility> Ability, bool AutoActivateGrantedAbility, FInstancedStruct AutoActivatedAbilityContext)
{
	if (!GetOwner()->HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("Client cannot grant abilities!"));
	}

	/* Make sure we only grant unique abilities i.e. can't have the same class twice or two abilities
	 * with the same tag (even if they are different classes)
	 */ 
	FSimpleGameplayAbilityConfig IncomingAbilityConfig = GetAbilityConfig(Ability);
	for (TSubclassOf<USimpleAbility> GrantedAbility : GrantedAbilities)
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

void USimpleAbilityComponent::RevokeAbility(TSubclassOf<USimpleAbility> Ability, bool CancelRunningInstances)
{
	if (!GetOwner()->HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("Client cannot revoke abilities!"));
	}

	if (CancelRunningInstances)
	{
		FSimpleGameplayAbilityConfig AbilityConfig = GetAbilityConfig(Ability);
		for (USimpleAbility* RunningAbility : RunningAbilities)
		{
			if (RunningAbility->AbilityConfig.AbilityName.MatchesTagExact(AbilityConfig.AbilityName))
			{
				RunningAbility->EndAbilityCancel();
			}
		}
	}
	
	GrantedAbilities.Remove(Ability);
}

bool USimpleAbilityComponent::ActivateAbility(TSubclassOf<USimpleAbility> AbilityClass, FInstancedStruct AbilityContext, bool OnlyActivateIfGranted)
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

void USimpleAbilityComponent::ServerActivateAbility_Implementation(TSubclassOf<USimpleAbility> AbilityClass, FInstancedStruct AbilityContext, FGuid AbilityInstanceID)
{
	MulticastActivateAbility(AbilityClass, AbilityContext, AbilityInstanceID);
}

void USimpleAbilityComponent::MulticastActivateAbility_Implementation(TSubclassOf<USimpleAbility> AbilityClass, FInstancedStruct AbilityContext, FGuid AbilityInstanceID)
{
	ActivateAbilityInternal(AbilityClass, AbilityContext, AbilityInstanceID);
}

void USimpleAbilityComponent::MulticastActivateAbilityUnreliable_Implementation(TSubclassOf<USimpleAbility> AbilityClass, FInstancedStruct AbilityContext, FGuid AbilityInstanceID)
{
	ActivateAbilityInternal(AbilityClass, AbilityContext, AbilityInstanceID);
}

bool USimpleAbilityComponent::ActivateAbilityInternal(const TSubclassOf<USimpleAbility>& AbilityClass, const FInstancedStruct& AbilityContext, FGuid AbilityInstanceID)
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
		for (USimpleAbility* Ability : RunningAbilities)
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

	USimpleAbility* NewAbility = NewObject<USimpleAbility>(this, AbilityClass);
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

/* Attribute manipulation */

bool USimpleAbilityComponent::ApplyAttributeModifier(USimpleAbilityComponent* Instigator, const TSubclassOf<USimpleAttributeModifier> AttributeModifier, FInstancedStruct EffectContext)
{
	const FGuid ModifierID = FGuid::NewGuid();
	USimpleAttributeModifier* NewModifier = NewObject<USimpleAttributeModifier>(this, AttributeModifier);
	NewModifier->InitializeModifier(ModifierID, Instigator, this);

	if (HasAuthority())
	{
		//AddReplicatedSubObject(NewModifier);
		RunningAttributeModifiers.Add(NewModifier);
	}
	else
	{
		LocalRunningAttributeModifiers.Add(NewModifier);
	}

	return NewModifier->ApplyModifier(EffectContext);
}

bool USimpleAbilityComponent::ApplyPendingAttributeModifier(const FPendingAttributeModifier PendingAttributeModifier)
{
	return ApplyAttributeModifier(PendingAttributeModifier.Instigator, PendingAttributeModifier.AttributeModifierClass, PendingAttributeModifier.AttributeModifierContext);
}

void USimpleAbilityComponent::AddFloatAttribute(FFloatAttribute Attribute)
{
	// Check if the attribute already exists
	for (int i = 0; i < FloatAttributes.Num(); i++)
	{
		if (FloatAttributes[i].AttributeTag == Attribute.AttributeTag)
		{
			// If it does, update the attribute
			USimpleAttributeFunctionLibrary::OverrideFloatAttribute(this, Attribute.AttributeTag, Attribute);
			return;
		}
	}

	FloatAttributes.Add(Attribute);
}

void USimpleAbilityComponent::RemoveFloatAttributes(FGameplayTagContainer AttributeTags)
{
	FloatAttributes.RemoveAll([&AttributeTags](const FFloatAttribute& Attribute)
	{
		return AttributeTags.HasTag(Attribute.AttributeTag);
	});
}

void USimpleAbilityComponent::AddStructAttribute(FStructAttribute Attribute)
{
	// Check if the attribute already exists
	for (int i = 0; i < StructAttributes.Num(); i++)
	{
		if (StructAttributes[i].AttributeTag == Attribute.AttributeTag)
		{
			// If it does, update the attribute
			StructAttributes[i] = Attribute;
			UE_LOG(LogTemp, Warning, TEXT("Attribute %s already exists. Replacing attribute data with new attribute."), *Attribute.AttributeTag.ToString());
			return;
		}
	}

	StructAttributes.Add(Attribute);
}

void USimpleAbilityComponent::RemoveStructAttributes(FGameplayTagContainer AttributeTags)
{
	StructAttributes.RemoveAll([&AttributeTags](const FStructAttribute& Attribute)
	{
		return AttributeTags.HasTagExact(Attribute.AttributeTag);
	});
}

/* Replicated Events */

void USimpleAbilityComponent::SendEvent(FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload, ESimpleEventReplicationPolicy ReplicationPolicy)
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

void USimpleAbilityComponent::SendEventInternal(FGameplayTag EventTag, FGameplayTag DomainTag, const FInstancedStruct& Payload) const
{
	USimpleEventSubsystem* EventSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<USimpleEventSubsystem>();

	if (!EventSubsystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("EventSubsystem is not valid!"));
		return;
	}
	
	EventSubsystem->SendEvent(EventTag, DomainTag, Payload, GetOwner());
}

void USimpleAbilityComponent::ClientSendEvent_Implementation(FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload)
{
	SendEventInternal(EventTag, DomainTag, Payload);
}

void USimpleAbilityComponent::ServerSendEvent_Implementation(FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload)
{
	SendEventInternal(EventTag, DomainTag, Payload);
}

void USimpleAbilityComponent::ServerSendMulticastEvent_Implementation(FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload, FGuid EventID)
{
	MulticastSendEvent(EventTag, DomainTag, Payload, EventID);
}

void USimpleAbilityComponent::ServerSendClientEvent_Implementation(FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload)
{
	SendEventInternal(EventTag, DomainTag, Payload);
	ClientSendEvent(EventTag, DomainTag, Payload);
}

void USimpleAbilityComponent::MulticastSendEvent_Implementation(FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload, FGuid EventID)
{
	if (LocalPredictedEventIDs.Contains(EventID))
	{
		LocalPredictedEventIDs.Remove(EventID);
		return;
	}
	
	SendEventInternal(EventTag, DomainTag, Payload);
}

/* Gameplay tag */

void USimpleAbilityComponent::AddGameplayTags(FGameplayTagContainer Tags)
{
	GameplayTags.AppendTags(Tags);
}

void USimpleAbilityComponent::RemoveGameplayTags(FGameplayTagContainer Tags)
{
	GameplayTags.RemoveTags(Tags);
}

/* Avatar actor */

AActor* USimpleAbilityComponent::GetAvatarActor() const
{
	return AvatarActor;
}

void USimpleAbilityComponent::SetAvatarActor(AActor* NewAvatarActor)
{
	if (!GetOwner()->HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("Client cannot set avatar actor!"));
		return;
	}
	
	AvatarActor = NewAvatarActor;
}

/* Overridable functions */

double USimpleAbilityComponent::GetServerTime_Implementation() const
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

void USimpleAbilityComponent::OnAbilityStartedInternal(FGameplayTag AbilityTag, FGameplayTag DomainTag, FInstancedStruct Payload)
{
	if (const FAbilityActivationStateChangedData* StateChangeData = Payload.GetPtr<FAbilityActivationStateChangedData>())
	{
		if (USimpleAbility* Ability = FindAnyAbilityInstanceByID(StateChangeData->AbilityInstanceID))
		{
			FGameplayTagContainer AbilityTagsToCancel = Ability->AbilityConfig.TagConfig.CancelAbilitiesWithAbilityTags;

			if (GetOwner()->HasAuthority())
			{
				for (USimpleAbility* RunningAbility : RunningAbilities)
				{
					if (AbilityTagsToCancel.HasAnyExact(RunningAbility->AbilityConfig.TagConfig.AbilityTags))
					{
						RunningAbility->EndAbilityCancel();
					}
				}
			}
			else
			{
				for (USimpleAbility* RunningAbility : LocalRunningAbilities)
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

void USimpleAbilityComponent::OnAbilityEndedInternal(FGameplayTag AbilityTag, FGameplayTag DomainTag,  FInstancedStruct Payload)
{
	/* Delete multi instance abilities on the server once they're done.
	   Local predicted multi instance abilities are deleted on the client when the RunningAbilities array is replicated */
	if (const FAbilityActivationStateChangedData* StateChangeData = Payload.GetPtr<FAbilityActivationStateChangedData>())
	{
		if (USimpleAbility* Ability = FindAnyAbilityInstanceByID(StateChangeData->AbilityInstanceID))
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

/* Utility */

FSimpleGameplayAbilityConfig USimpleAbilityComponent::GetAbilityConfig(TSubclassOf<USimpleAbility> Ability)
{
	if (!Ability)
	{
		UE_LOG(LogTemp, Warning, TEXT("Ability class in GetAbilityConfig is not valid. Returning default config"));
		return FSimpleGameplayAbilityConfig();
	}
	
	return Ability.GetDefaultObject()->AbilityConfig;
}

USimpleAbility* USimpleAbilityComponent::FindAnyAbilityInstanceByID(const FGuid AbilityInstanceID)
{
	for (USimpleAbility* Ability : RunningAbilities)
	{
		if (Ability->AbilityInstanceID == AbilityInstanceID)
		{
			return Ability;
		}
	}

	for (USimpleAbility* Ability : LocalRunningAbilities)
	{
		if (Ability->AbilityInstanceID == AbilityInstanceID)
		{
			return Ability;
		}
	}

	return nullptr;
}

USimpleAbility* USimpleAbilityComponent::FindAuthorityAbilityInstanceByID(const FGuid AbilityInstanceID)
{
	for (USimpleAbility* Ability : RunningAbilities)
	{
		if (Ability->AbilityInstanceID == AbilityInstanceID)
		{
			return Ability;
		}
	}

	return nullptr;
}

USimpleAbility* USimpleAbilityComponent::FindPredictedAbilityInstanceByID(const FGuid AbilityInstanceID)
{
	for (USimpleAbility* Ability : LocalRunningAbilities)
	{
		if (Ability->AbilityInstanceID == AbilityInstanceID)
		{
			return Ability;
		}
	}

	return nullptr;
}

TSubclassOf<USimpleAbility> USimpleAbilityComponent::FindGrantedAbilityByTag(FGameplayTag AbilityTag)
{
	for (TSubclassOf<USimpleAbility> Ability : GrantedAbilities)
	{
		if (Ability.GetDefaultObject()->AbilityConfig.AbilityName == AbilityTag)
		{
			return Ability;
		}
	}

	return nullptr;
}

/* Replication */

void USimpleAbilityComponent::OnRep_RunningAbilities()
{
	/* We go through each local ability and check if it still has
	 * a corresponding ability in the authoritative RunningAbilities array*/
	for (USimpleAbility* Ability : LocalRunningAbilities)
	{
		// We only want to do cleanup when the local ability is no longer running
		if (Ability->IsAbilityActive())
		{
			continue;
		}

		// Check if this ability still exists on the server
		for (USimpleAbility* RunningAbility : RunningAbilities)
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

void USimpleAbilityComponent::OnRep_RunningAttributeModifiers()
{
	/*/* We go through each local modifier and check if it still has
	 * a corresponding modifier in the authoritative RunningAttributeModifiers array#1#
	for (USimpleAttributeModifier* Modifier : LocalRunningAttributeModifiers)
	{
		// We only want to do cleanup when the local modifier is no longer running
		if (Modifier->IsModifierActive())
		{
			continue;
		}

		// Check if this modifier still exists on the server
		for (USimpleAttributeModifier* RunningModifier : RunningAttributeModifiers)
		{
			if (RunningModifier->ModifierID == Modifier->ModifierID)
			{
				return;
			}
		}

		// Finally, remove the local modifier if it's no longer running on the server or the client
		LocalRunningAttributeModifiers.Remove(Modifier);
		UE_LOG(LogTemp, Warning, TEXT("Local modifier %s is no longer running on the server. Removing."), *Modifier->ModifierID.ToString());
	}*/
}

void USimpleAbilityComponent::OnRep_FloatAttributes(TArray<FFloatAttribute> OldAttributes)
{
	// Check if any new attributes were added
	FGameplayTagContainer OldAttributeTags;
	FGameplayTagContainer NewAttributeTags;
	
	for (int i = 0; i < OldAttributes.Num(); i++)
	{
		OldAttributeTags.AddTag(OldAttributes[i].AttributeTag);
	}
	
	for (int i = 0; i < FloatAttributes.Num(); i++)
	{
		if (!OldAttributeTags.HasTagExact(FloatAttributes[i].AttributeTag))
		{
			USimpleAttributeFunctionLibrary::SendFloatAttributeChangedEvent(this, FDefaultTags::AttributeAdded, FloatAttributes[i].AttributeTag, EAttributeValueType::BaseValue, FloatAttributes[i].BaseValue);
		}

		NewAttributeTags.AddTag(FloatAttributes[i].AttributeTag);
	}

	// Check if any old attributes were removed and check if any existing attributes were updated
	for (int i = 0; i < OldAttributes.Num(); i++)
	{
		if (!NewAttributeTags.HasTagExact(OldAttributes[i].AttributeTag))
		{
			USimpleAttributeFunctionLibrary::SendFloatAttributeChangedEvent(this, FDefaultTags::AttributeRemoved, OldAttributes[i].AttributeTag, EAttributeValueType::BaseValue, 0.0f);
		}
		else
		{
			const int32 NewAttributeIndex = USimpleAttributeFunctionLibrary::GetFloatAttributeIndex(this, OldAttributes[i].AttributeTag);

			if (NewAttributeIndex < 0)
			{
				UE_LOG(LogTemp, Warning, TEXT("USimpleGameplayAttributes::OnRep_FloatAttributes: Attribute %s not found."), *OldAttributes[i].AttributeTag.ToString());
				continue;
			}

			USimpleAttributeFunctionLibrary::CompareFloatAttributesAndSendEvents(this, OldAttributes[i], FloatAttributes[NewAttributeIndex]);
		}
	}
}

void USimpleAbilityComponent::OnRep_StructAttributes(TArray<FStructAttribute> OldAttributes)
{
	/*// Check if any new attributes were added
	FGameplayTagContainer OldAttributeTags;
	FGameplayTagContainer NewAttributeTags;
	
	for (int i = 0; i < OldAttributes.Num(); i++)
	{
		OldAttributeTags.AddTag(OldAttributes[i].AttributeTag);
	}
	
	for (int i = 0; i < StructAttributes.Num(); i++)
	{
		if (!OldAttributeTags.HasTagExact(StructAttributes[i].AttributeTag))
		{
			//SendFloatAttributeChangedEvent(FDefaultTags::AttributeAdded, StructAttributes[i].AttributeTag, EAttributeValueType::BaseValue, 0.0f);
		}

		NewAttributeTags.AddTag(StructAttributes[i].AttributeTag);
	}

	// Check if any old attributes were removed and check if any existing attributes were updated
	for (int i = 0; i < OldAttributes.Num(); i++)
	{
		if (!NewAttributeTags.HasTagExact(OldAttributes[i].AttributeTag))
		{
			SendFloatAttributeChangedEvent(FDefaultTags::AttributeRemoved, OldAttributes[i].AttributeTag, EAttributeValueType::BaseValue, 0.0f);
		}
		else
		{
			const int32 NewAttributeIndex = GetFloatAttributeIndex(OldAttributes[i].AttributeTag);

			if (NewAttributeIndex < 0)
			{
				UE_LOG(LogTemp, Warning, TEXT("USimpleGameplayAttributes::OnRep_StructAttributes: Attribute %s not found."), *OldAttributes[i].AttributeTag.ToString());
				continue;
			}
			
			if (StructAttributes[NewAttributeIndex].AttributeValue != OldAttributes[i].AttributeValue)
			{
				//SendFloatAttributeChangedEvent(FDefaultTags::AttributeChanged, OldAttributes[i].AttributeTag, EAttributeValueType::BaseValue, 0.0f);
			}
		}
	}*/
}

void USimpleAbilityComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USimpleAbilityComponent, AvatarActor);
	DOREPLIFETIME(USimpleAbilityComponent, GameplayTags);
	DOREPLIFETIME(USimpleAbilityComponent, FloatAttributes);
	DOREPLIFETIME(USimpleAbilityComponent, StructAttributes);
	DOREPLIFETIME(USimpleAbilityComponent, RunningAbilities);
	DOREPLIFETIME(USimpleAbilityComponent, RunningAttributeModifiers);
	DOREPLIFETIME(USimpleAbilityComponent, GrantedAbilities);
}
