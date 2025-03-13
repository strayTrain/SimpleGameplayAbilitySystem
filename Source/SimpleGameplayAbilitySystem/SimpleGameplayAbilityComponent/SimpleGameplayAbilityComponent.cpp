#include "SimpleGameplayAbilityComponent.h"

#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "SimpleGameplayAbilitySystem/DataAssets/AbilitySet/AbilitySet.h"
#include "SimpleGameplayAbilitySystem/DataAssets/AttributeSet/AttributeSet.h"
#include "SimpleGameplayAbilitySystem/DefaultTags/DefaultTags.h"
#include "SimpleGameplayAbilitySystem/Module/SimpleGameplayAbilitySystem.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleAbilityTypes.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleGameplayAbility/SimpleGameplayAbility.h"
#include "SimpleGameplayAbilitySystem/SimpleEventSubsystem/SimpleEventSubSystem.h"
#include "SimpleGameplayAbilitySystem/DataAssets/AbilityOverrideSet/AbilityOverrideSet.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleAttributeModifier/SimpleAttributeModifier.h"
#include "Windows/WindowsTextInputMethodSystem.h"

class USimpleEventSubsystem;

using enum EAbilityStatus;

USimpleGameplayAbilityComponent::USimpleGameplayAbilityComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	AvatarActor = nullptr;
}

void USimpleGameplayAbilityComponent::BeginPlay()
{
	UActorComponent::BeginPlay();

	SetIsReplicated(true);

	// Listen for ability ended event
	if (USimpleEventSubsystem* EventSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<USimpleEventSubsystem>())
	{
		FGameplayTagContainer EventTags;
		EventTags.AddTag(FDefaultTags::AbilityEnded());
    
		FSimpleEventDelegate EventDelegate;
		EventDelegate.BindDynamic(this, &USimpleGameplayAbilityComponent::OnAbilityEnded);
		
		EventSubsystem->ListenForEvent(this, false, EventTags, {}, EventDelegate, {}, {});
	}
	
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

		// Create attributes from the directly added attributes and attribute sets
		for (const FStructAttribute Attribute : StructAttributes)
		{
			AddStructAttribute(Attribute);
		}

		for (const FFloatAttribute Attribute : FloatAttributes)
		{
			AddFloatAttribute(Attribute);
		}
		
		for (UAttributeSet* AttributeSet : AttributeSets)
		{
			for (const FFloatAttribute Attribute : AttributeSet->FloatAttributes)
			{
				AddFloatAttribute(Attribute);
			}
			
			for (const FStructAttribute Attribute : AttributeSet->StructAttributes)
			{
				AddStructAttribute(Attribute);
			}
		}

		// Add ability overrides from the ability override sets
		for (UAbilityOverrideSet* AbilityOverrideSet : AbilityOverrideSets)
		{
			for (const FAbilityOverride AbilityOverride : AbilityOverrideSet->AbilityOverrides)
			{
				AddAbilityOverride(AbilityOverride.OriginalAbility, AbilityOverride.OverrideAbility);
			}
		}

		return;
	}

	// Delegates called on the client to handle replicated data
	AuthorityAbilityStates.OnAbilityStateAdded.BindUObject(this, &USimpleGameplayAbilityComponent::OnStateAdded);
	AuthorityAbilityStates.OnAbilityStateChanged.BindUObject(this, &USimpleGameplayAbilityComponent::OnStateChanged);
	AuthorityAbilityStates.OnAbilityStateRemoved.BindUObject(this, &USimpleGameplayAbilityComponent::OnStateRemoved);
	
	AuthorityAttributeStates.OnAbilityStateAdded.BindUObject(this, &USimpleGameplayAbilityComponent::OnStateAdded);
	AuthorityAttributeStates.OnAbilityStateChanged.BindUObject(this, &USimpleGameplayAbilityComponent::OnStateChanged);
	AuthorityAttributeStates.OnAbilityStateRemoved.BindUObject(this, &USimpleGameplayAbilityComponent::OnStateRemoved);

	AuthorityFloatAttributes.OnFloatAttributeAdded.BindUObject(this, &USimpleGameplayAbilityComponent::OnFloatAttributeAdded);
	AuthorityFloatAttributes.OnFloatAttributeChanged.BindUObject(this, &USimpleGameplayAbilityComponent::OnFloatAttributeChanged);
	AuthorityFloatAttributes.OnFloatAttributeRemoved.BindUObject(this, &USimpleGameplayAbilityComponent::OnFloatAttributeRemoved);

	AuthorityStructAttributes.OnStructAttributeAdded.BindUObject(this, &USimpleGameplayAbilityComponent::OnStructAttributeAdded);
	AuthorityStructAttributes.OnStructAttributeChanged.BindUObject(this, &USimpleGameplayAbilityComponent::OnStructAttributeChanged);
	AuthorityStructAttributes.OnStructAttributeRemoved.BindUObject(this, &USimpleGameplayAbilityComponent::OnStructAttributeRemoved);

	AuthorityGameplayTags.OnGameplayTagCounterAdded.BindUObject(this, &USimpleGameplayAbilityComponent::OnGameplayTagAdded);
	AuthorityGameplayTags.OnGameplayTagCounterChanged.BindUObject(this, &USimpleGameplayAbilityComponent::OnGameplayTagChanged);
	AuthorityGameplayTags.OnGameplayTagCounterRemoved.BindUObject(this, &USimpleGameplayAbilityComponent::OnGameplayTagRemoved);

	LocalFloatAttributes = AuthorityFloatAttributes.Attributes;
	LocalStructAttributes = AuthorityStructAttributes.Attributes;

	LocalGameplayTags = AuthorityGameplayTags.Tags;
}

/* Ability Functions */

bool USimpleGameplayAbilityComponent::ActivateAbility(
	const TSubclassOf<USimpleGameplayAbility> AbilityClass,
	const FInstancedStruct AbilityContext,
	FGuid& AbilityID, const bool OverrideActivationPolicy, const EAbilityActivationPolicy ActivationPolicyOverride)
{
	AbilityID = FGuid::NewGuid();
	return ActivateAbilityWithID(AbilityID, AbilityClass, AbilityContext, OverrideActivationPolicy, ActivationPolicyOverride);
}

bool USimpleGameplayAbilityComponent::ActivateAbilityWithID(
	const FGuid AbilityID,
	TSubclassOf<USimpleGameplayAbility> AbilityClass,
	const FInstancedStruct& AbilityContext,
	const bool OverrideActivationPolicy,
	const EAbilityActivationPolicy ActivationPolicyOverride)
{
	const EAbilityActivationPolicy ActivationPolicy = OverrideActivationPolicy ? ActivationPolicyOverride : AbilityClass.GetDefaultObject()->ActivationPolicy;
	
	const bool IsClient = GetNetMode() == NM_Client;
	const float ActivationTime = GetServerTime();

	switch (ActivationPolicy)
	{
		case EAbilityActivationPolicy::LocalOnly:
			return ActivateAbilityInternal(AbilityID, AbilityClass, AbilityContext, ActivationPolicy, false, ActivationTime);
		
		case EAbilityActivationPolicy::ClientOnly:
			if (!IsClient)
			{
				SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::ActivateAbility]: Can't activate ability %s on server with ClientOnly policy."), *AbilityClass->GetName()));
			}
		
			return ActivateAbilityInternal(AbilityID, AbilityClass, AbilityContext, ActivationPolicy, false, ActivationTime);
		
		case EAbilityActivationPolicy::ServerInitiatedFromClient:
			if (IsClient)
			{
				ServerActivateAbility(AbilityID, AbilityClass, AbilityContext, ActivationPolicy, ActivationTime);
				return true;
			}

			return ActivateAbilityInternal(AbilityID, AbilityClass, AbilityContext, ActivationPolicy, true, ActivationTime);
		
		case EAbilityActivationPolicy::ClientPredicted:
			if (IsClient)
			{
				ServerActivateAbility(AbilityID, AbilityClass, AbilityContext, ActivationPolicy, ActivationTime);
			}
		
			return ActivateAbilityInternal(AbilityID, AbilityClass, AbilityContext, ActivationPolicy, true, ActivationTime);

		case EAbilityActivationPolicy::ServerAuthority:
			if (IsClient)
			{
				SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::ActivateAbility]: Can't activate ability %s on client with ServerAuthority policy."), *AbilityClass->GetName()));
				return false;
			}
		
			return ActivateAbilityInternal(AbilityID, AbilityClass, AbilityContext, ActivationPolicy, true, ActivationTime);

		case EAbilityActivationPolicy::ServerOnly:
			if (IsClient)
			{
				SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::ActivateAbility]: Can't activate ability %s on client with ServerOnly policy."), *AbilityClass->GetName()));
				return false;
			}
		
			return ActivateAbilityInternal(AbilityID, AbilityClass, AbilityContext, ActivationPolicy, false, ActivationTime);
	}
	
	return false;
}

bool USimpleGameplayAbilityComponent::ActivateAbilityInternal(
	const FGuid AbilityID,
	TSubclassOf<USimpleGameplayAbility> AbilityClass,
	const FInstancedStruct& AbilityContext,
	const EAbilityActivationPolicy ActivationPolicy,
	const bool TrackState, const float ActivationTime)
{
	if (!AbilityClass)
	{
		SIMPLE_LOG(this, TEXT("[USimpleGameplayAbilityComponent::ActivateAbilityInternal]: AbilityClass is null!"));
		return false;
	}

	// Check if the ability is on cooldown
	if (LastActivatedAbilityTimeStamps.Contains(AbilityClass))
	{
		const float LastActivatedTime = LastActivatedAbilityTimeStamps[AbilityClass];
		const float Cooldown = AbilityClass.GetDefaultObject()->Cooldown;

		if (LastActivatedTime + Cooldown > GetServerTime())
		{
			SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::ActivateAbilityInternal]: Failed to activate ability %s because it is on cooldown."), *AbilityClass->GetName()));
			return false;
		}
	}
	

	CreateAbilityState(AbilityID, ActivationPolicy, AbilityClass, AbilityContext, HasAuthority(), ActivationTime);
	
	USimpleGameplayAbility* AbilityInstance = GetAbilityInstance(AbilityClass);
	
	if (AbilityInstance->InstancingPolicy == EAbilityInstancingPolicy::SingleInstance)
	{
		if (AbilityInstance->IsAbilityActive() && !AbilityInstance->CanCancel())
		{
			SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::ActivateAbilityWithID]: Failed to activate ability %s because it is already running and cannot be cancelled."), *AbilityClass->GetName()));
			return false;
		}

		AbilityInstance->CancelAbility(FDefaultTags::AbilityCancelled(), FInstancedStruct(), true);
	}
	
	AbilityInstance->InitializeAbility(this, AbilityID, false);
	const bool WasActivated = AbilityInstance->ActivateAbility(AbilityID, AbilityContext);

	if (WasActivated)
	{
		LastActivatedAbilityTimeStamps.Add(AbilityClass, GetServerTime());
	}
	
	return WasActivated;
}

void USimpleGameplayAbilityComponent::ServerActivateAbility_Implementation(const FGuid AbilityID, TSubclassOf<USimpleGameplayAbility> AbilityClass,
	const FInstancedStruct& AbilityContext, EAbilityActivationPolicy ActivationPolicy, const float ActivationTime)
{
	if (!AbilityClass)
	{
		SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::ServerActivateAbility]: AbilityClass is null!")));
		return;
	}
	
	ActivateAbilityInternal(AbilityID, AbilityClass, AbilityContext, ActivationPolicy, true, ActivationTime);
}

void USimpleGameplayAbilityComponent::OnAbilityEnded(FGameplayTag EventTag, FGameplayTag Domain, FInstancedStruct Payload, AActor* Sender)
{
	const FSimpleAbilityEndedEvent* EndedEvent = Payload.GetPtr<FSimpleAbilityEndedEvent>();

	if (!EndedEvent)
	{
		SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::OnAbilityEnded]: Ability end event was sent with malformed payload. Payload was not of type FSimpleAbilityEndedEvent.")));
		return;
	}
	
	FAbilityState* AbilityState = GetAbilityState(EndedEvent->AbilityID, HasAuthority());

	if (!AbilityState)
	{
		return;
	}
	
	const EAbilityStatus StatusToSet = EndedEvent->WasCancelled ? EndedCancelled : EndedSuccessfully;
	AbilityState->EndingContext = Payload;
	AbilityState->AbilityStatus = StatusToSet;
	
	if (HasAuthority())
	{
		AuthorityAbilityStates.MarkItemDirty(*AbilityState);
	}
}

FAbilityState& USimpleGameplayAbilityComponent::CreateAbilityState(
	const FGuid AbilityID,
	EAbilityActivationPolicy ActivationPolicy,
	const TSubclassOf<USimpleGameplayAbility>& AbilityClass,
	const FInstancedStruct& ActivationContext, const bool IsAuthorityState, const float ActivationTime)
{
	FAbilityState NewAbilityState;
	NewAbilityState.AbilityID = AbilityID;
	NewAbilityState.AbilityClass = AbilityClass;
	NewAbilityState.ActivationTimeStamp = ActivationTime > 0 ? ActivationTime : GetServerTime();
	NewAbilityState.ActivationContext = ActivationContext;
	NewAbilityState.AbilityStatus = PreActivation;
	
	if (IsAuthorityState)
	{
		AuthorityAbilityStates.AbilityStates.Add(NewAbilityState);
		AuthorityAbilityStates.MarkArrayDirty();
		return AuthorityAbilityStates.AbilityStates.Last();
	}

	LocalAbilityStates.Add(NewAbilityState);
	return LocalAbilityStates.Last();
}

FAbilityState* USimpleGameplayAbilityComponent::GetAbilityState(const FGuid AbilityID, const bool IsAuthorityState)
{
	TArray<FAbilityState>& AbilityStates = IsAuthorityState ? AuthorityAbilityStates.AbilityStates : LocalAbilityStates;
	
	for (FAbilityState& AbilityState : AbilityStates)
	{
		if (AbilityState.AbilityID == AbilityID)
		{
			return &AbilityState;
		}
	}

	return nullptr;
}

bool USimpleGameplayAbilityComponent::SetAbilityStatus(const FGuid AbilityID, const EAbilityStatus NewStatus)
{
	FAbilityState* AbilityState = GetAbilityState(AbilityID, HasAuthority());

	if (!AbilityState)
	{
		SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::SetAbilityStatus]: Ability with ID %s not found in AbilityState array"), *AbilityID.ToString()));
		return false;
	}

	AbilityState->AbilityStatus = NewStatus;

	if (HasAuthority())
	{
		AuthorityAbilityStates.MarkItemDirty(*AbilityState);
	}

	return true;
}

USimpleGameplayAbility* USimpleGameplayAbilityComponent::GetAbilityInstance(TSubclassOf<USimpleGameplayAbility> AbilityClass)
{
	if (AbilityClass.GetDefaultObject()->InstancingPolicy == EAbilityInstancingPolicy::SingleInstance)
	{
		// Check if we already have an instance of the ability created
		for (USimpleGameplayAbility* InstancedAbility : InstancedAbilities)
		{
			if (InstancedAbility->GetClass() == AbilityClass)
			{
				return InstancedAbility;
			}
		}
	}

	// If we don't have an instance of the ability created (or a MultipleInstance policy) we create one
	USimpleGameplayAbility* NewAbilityInstance = NewObject<USimpleGameplayAbility>(this, AbilityClass);
	InstancedAbilities.Add(NewAbilityInstance);
	
	return InstancedAbilities.Last();	
}

bool USimpleGameplayAbilityComponent::CancelAbility(const FGuid AbilityInstanceID, const FInstancedStruct CancellationContext)
{
	if (USimpleGameplayAbility* AbilityInstance = GetGameplayAbilityInstance(AbilityInstanceID))
	{
		if (!AbilityInstance->IsAbilityActive())
		{
			return true;
		}
		
		if (!AbilityInstance->CanCancel())
		{
			SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::CancelAbility]: Ability %s CanCancel() returned false"), *AbilityInstance->GetName()));
			return false;
		}
		
		AbilityInstance->CancelAbility(FDefaultTags::AbilityCancelled(), CancellationContext);
		return true;
	}

	SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::CancelAbility]: Ability with ID %s not found in InstancedAbilities array"), *AbilityInstanceID.ToString()));
	return false;
}

TArray<FGuid> USimpleGameplayAbilityComponent::CancelAbilitiesWithTags(const FGameplayTagContainer Tags, FInstancedStruct CancellationContext)
{
	TArray<FGuid> CancelledAbilities;
	
	for (USimpleGameplayAbility* AbilityInstance : InstancedAbilities)
	{
		if (AbilityInstance->AbilityTags.HasAnyExact(Tags))
		{
			if (!AbilityInstance->CanCancel())
			{
				SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::CancelAbilitiesWithTags]: Ability %s CanCancel() returned false"), *AbilityInstance->GetName()));
				continue;
			}
			
			AbilityInstance->CancelAbility(FDefaultTags::AbilityCancelled(), CancellationContext);
			CancelledAbilities.Add(AbilityInstance->AbilityInstanceID);
		}
	}
	
	return CancelledAbilities;
}

bool USimpleGameplayAbilityComponent::IsAvatarActorOfType(TSubclassOf<AActor> AvatarClass) const
{
	if (AvatarActor && AvatarActor->IsA(AvatarClass))
	{
		return true;
	}

	return false;
}

void USimpleGameplayAbilityComponent::GrantAbility(const TSubclassOf<USimpleGameplayAbility> AbilityClass)
{
	GrantedAbilities.AddUnique(AbilityClass);
}

void USimpleGameplayAbilityComponent::RevokeAbility(const TSubclassOf<USimpleGameplayAbility> AbilityClass)
{
	GrantedAbilities.Remove(AbilityClass);
}

void USimpleGameplayAbilityComponent::AddAbilityOverride(TSubclassOf<USimpleGameplayAbility> Ability, TSubclassOf<USimpleGameplayAbility> OverrideAbility)
{
	FAbilityOverride AbilityOverride;
	AbilityOverride.OriginalAbility = Ability;
	AbilityOverride.OverrideAbility = OverrideAbility;
	
	ActiveAbilityOverrides.AddUnique(AbilityOverride);
}

void USimpleGameplayAbilityComponent::RemoveAbilityOverride(TSubclassOf<USimpleGameplayAbility> Ability)
{
	FAbilityOverride* FoundAbilityOverride = ActiveAbilityOverrides.FindByPredicate([Ability](const FAbilityOverride& Override) { return Override.OriginalAbility == Ability; });

	if (FoundAbilityOverride)
	{
		ActiveAbilityOverrides.Remove(*FoundAbilityOverride);
	}
}

void USimpleGameplayAbilityComponent::AddAbilityStateSnapshot(FGuid AbilityInstanceID, FSimpleAbilitySnapshot State)
{
	if (HasAuthority())
	{
		for (FAbilityState& AuthorityAbilityState : AuthorityAbilityStates.AbilityStates)
		{
			if (AuthorityAbilityState.AbilityID == AbilityInstanceID)
			{
				AuthorityAbilityState.SnapshotHistory.Add(State);
				AuthorityAbilityStates.MarkItemDirty(AuthorityAbilityState);
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

	SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::AddAbilityStateSnapshot]: Ability with ID %s not found in InstancedAbilities array"), *AbilityInstanceID.ToString()));
}

/* Tag Functions */

void USimpleGameplayAbilityComponent::AddGameplayTag(FGameplayTag Tag, FInstancedStruct Payload)
{
	TArray<FGameplayTagCounter>& TagCounters = HasAuthority() ? AuthorityGameplayTags.Tags : LocalGameplayTags;
	FGameplayTagCounter* TagCounter = TagCounters.FindByPredicate([Tag](const FGameplayTagCounter& TagCounter) { return TagCounter.GameplayTag.MatchesTagExact(Tag); });

	if (TagCounter)
	{
		TagCounter->ReferenceCounter++;
		
		if (HasAuthority())
		{
			AuthorityGameplayTags.MarkItemDirty(*TagCounter);
		}
		
		return;
	}

	FGameplayTagCounter NewTagCounter;
	NewTagCounter.GameplayTag = Tag;
	NewTagCounter.ReferenceCounter = 1;
	
	TagCounters.AddUnique(NewTagCounter);

	if (HasAuthority())
	{
		AuthorityGameplayTags.MarkArrayDirty();
	}
	
	SendEvent(FDefaultTags::GameplayTagAdded(), Tag, Payload, GetOwner(), {}, ESimpleEventReplicationPolicy::NoReplication);
}

void USimpleGameplayAbilityComponent::RemoveGameplayTag(FGameplayTag Tag, FInstancedStruct Payload)
{
	TArray<FGameplayTagCounter>& TagCounters = HasAuthority() ? AuthorityGameplayTags.Tags : LocalGameplayTags;
	FGameplayTagCounter* TagCounter = TagCounters.FindByPredicate([Tag](const FGameplayTagCounter& TagCounter) { return TagCounter.GameplayTag.MatchesTagExact(Tag); });

	if (TagCounter && TagCounter->ReferenceCounter > 1)
	{
		TagCounter->ReferenceCounter--;
		
		if (HasAuthority())
		{
			AuthorityGameplayTags.MarkItemDirty(*TagCounter);
		}
		
		return;
	}

	TagCounters.RemoveSingle(*TagCounter);

	if (HasAuthority())
	{
		AuthorityGameplayTags.MarkArrayDirty();
	}
	
	SendEvent(FDefaultTags::GameplayTagRemoved(), Tag, Payload, GetOwner(), {}, ESimpleEventReplicationPolicy::NoReplication);
}

bool USimpleGameplayAbilityComponent::HasGameplayTag(FGameplayTag Tag)
{
	TArray<FGameplayTagCounter>& TagCounters = HasAuthority() ? AuthorityGameplayTags.Tags : LocalGameplayTags;

	return TagCounters.ContainsByPredicate([Tag](const FGameplayTagCounter& TagCounter) { return TagCounter.GameplayTag.MatchesTagExact(Tag); });
}

bool USimpleGameplayAbilityComponent::HasAllGameplayTags(FGameplayTagContainer Tags)
{
	TArray<FGameplayTagCounter>& TagCounters = HasAuthority() ? AuthorityGameplayTags.Tags : LocalGameplayTags;

	for (const FGameplayTag& Tag : Tags)
	{
		if (!TagCounters.ContainsByPredicate([Tag](const FGameplayTagCounter& TagCounter) { return TagCounter.GameplayTag.MatchesTagExact(Tag); }))
		{
			return false;
		}
	}

	return true;
}

bool USimpleGameplayAbilityComponent::HasAnyGameplayTags(FGameplayTagContainer Tags)
{
	TArray<FGameplayTagCounter>& TagCounters = HasAuthority() ? AuthorityGameplayTags.Tags : LocalGameplayTags;

	for (const FGameplayTag& Tag : Tags)
	{
		if (TagCounters.ContainsByPredicate([Tag](const FGameplayTagCounter& TagCounter) { return TagCounter.GameplayTag.MatchesTagExact(Tag); }))
		{
			return true;
		}
	}

	return false;
}

/* Event Functions */

void USimpleGameplayAbilityComponent::SendEvent(FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload,
                                                AActor* Sender, TArray<UObject*> ListenerFilter, ESimpleEventReplicationPolicy ReplicationPolicy)
{
	const FGuid EventID = FGuid::NewGuid();

	switch (ReplicationPolicy)
	{
		case ESimpleEventReplicationPolicy::NoReplication:
			SendEventInternal(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy, ListenerFilter);
			return;
			
		case ESimpleEventReplicationPolicy::ServerAndOwningClient:
			if (!HasAuthority() && GetOwner()->HasLocalNetOwner())
			{
				ServerSendEvent(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy, ListenerFilter);
				return;
			}
		
			SendEventInternal(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy, ListenerFilter);
			ClientSendEvent(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy, ListenerFilter);
			break;

		case ESimpleEventReplicationPolicy::ServerAndOwningClientPredicted:
			SendEventInternal(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy, ListenerFilter);

			if (HasAuthority())
			{
				ClientSendEvent(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy, ListenerFilter);
			}
		
			if (!HasAuthority() && GetOwner()->HasLocalNetOwner())
			{
				ServerSendEvent(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy, ListenerFilter);
			}
			break;
		
		case ESimpleEventReplicationPolicy::AllConnectedClients:
			if (!HasAuthority() && GetOwner()->HasLocalNetOwner())
			{
				ServerSendEvent(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy, ListenerFilter);
				return;
			}

			MulticastSendEvent(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy, ListenerFilter);
			break;

		case ESimpleEventReplicationPolicy::AllConnectedClientsPredicted:

			if (HasAuthority())
			{
				MulticastSendEvent(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy, ListenerFilter);
				break;
			}

			SendEventInternal(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy, ListenerFilter);
		
			if (!HasAuthority() && GetOwner()->HasLocalNetOwner())
			{
				ServerSendEvent(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy, ListenerFilter);
			}
			break;
	}
}

void USimpleGameplayAbilityComponent::SendEventInternal(FGuid EventID, FGameplayTag EventTag, FGameplayTag DomainTag,
	const FInstancedStruct& Payload, AActor* Sender, ESimpleEventReplicationPolicy ReplicationPolicy,
	const TArray<UObject*>& ListenerFilter)
{
	USimpleEventSubsystem* EventSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<USimpleEventSubsystem>();
	
	if (!EventSubsystem)
	{
		SIMPLE_LOG(this, TEXT("[USimpleGameplayAbilityComponent::SendEventInternal]: No SimpleEventSubsystem found."));
		return;
	}
	
	if (HandledEventIDs.Contains(EventID))
	{
		HandledEventIDs.Remove(EventID);
		return;
	}

	EventSubsystem->SendEvent(EventTag, DomainTag, Payload, Sender, ListenerFilter);

	// No need to keep track of handled events if we're not replicating
	if (ReplicationPolicy == ESimpleEventReplicationPolicy::NoReplication)
	{
		return;
	}
	
	HandledEventIDs.Add(EventID);
}

void USimpleGameplayAbilityComponent::ServerSendEvent_Implementation(
	FGuid EventID,
    FGameplayTag EventTag,
    FGameplayTag DomainTag,
    FInstancedStruct Payload,
    AActor* Sender,
    ESimpleEventReplicationPolicy ReplicationPolicy,
    const TArray<UObject*>& ListenerFilter)
{
	switch (ReplicationPolicy)
	{
		case ESimpleEventReplicationPolicy::ServerAndOwningClient:
			SendEventInternal(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy, {});
			ClientSendEvent(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy, ListenerFilter);
			break;
		
		case ESimpleEventReplicationPolicy::ServerAndOwningClientPredicted:
			SendEventInternal(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy, {});
			ClientSendEvent(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy, ListenerFilter);
			break;
		
		case ESimpleEventReplicationPolicy::AllConnectedClients:
			MulticastSendEvent(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy, ListenerFilter);
			break;
		
		case ESimpleEventReplicationPolicy::AllConnectedClientsPredicted:
			MulticastSendEvent(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy, ListenerFilter);
			break;
		default:
			break;
	}
}

void USimpleGameplayAbilityComponent::ClientSendEvent_Implementation(FGuid EventID,
                                                                     FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload,
                                                                     AActor* Sender, ESimpleEventReplicationPolicy ReplicationPolicy, const TArray<UObject*>& ListenerFilter)
{
	SendEventInternal(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy, {});
}

void USimpleGameplayAbilityComponent::MulticastSendEvent_Implementation(FGuid EventID,
                                                                        FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload, AActor*Sender, ESimpleEventReplicationPolicy ReplicationPolicy, const
                                                                        TArray<UObject*>& ListenerFilter)
{
	SendEventInternal(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy, {});
}

/* Utility Functions */

void USimpleGameplayAbilityComponent::RemoveInstancedAbility(USimpleGameplayAbility* AbilityToRemove)
{
	InstancedAbilities.Remove(AbilityToRemove);
}

USimpleGameplayAbility* USimpleGameplayAbilityComponent::GetGameplayAbilityInstance(FGuid AbilityInstanceID)
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

USimpleAttributeModifier* USimpleGameplayAbilityComponent::GetAttributeModifierInstance(FGuid AttributeInstanceID)
{
	for (USimpleAttributeModifier* InstancedModifier : InstancedAttributes)
	{
		if (InstancedModifier->AbilityInstanceID == AttributeInstanceID)
		{
			return InstancedModifier;
		}
	}

	return nullptr;
}

TArray<FSimpleAbilitySnapshot>* USimpleGameplayAbilityComponent::GetLocalAttributeStateSnapshots(const FGuid AttributeInstanceID)
{
	for (FAbilityState& AttributeState : LocalAttributeStates)
	{
		if (AttributeState.AbilityID == AttributeInstanceID)
		{
			return &AttributeState.SnapshotHistory;
		}
	}

	return nullptr;
}

bool USimpleGameplayAbilityComponent::IsAbilityOnCooldown(TSubclassOf<USimpleGameplayAbility> AbilityClass)
{
	return LastActivatedAbilityTimeStamps.Contains(AbilityClass) && LastActivatedAbilityTimeStamps[AbilityClass] + AbilityClass.GetDefaultObject()->Cooldown > GetServerTime();
}

float USimpleGameplayAbilityComponent::GetAbilityCooldownTimeRemaining(TSubclassOf<USimpleGameplayAbility> AbilityClass)
{
	if (!LastActivatedAbilityTimeStamps.Contains(AbilityClass))
	{
		return 0.0f;
	}

	return FMath::Clamp(LastActivatedAbilityTimeStamps[AbilityClass] + AbilityClass.GetDefaultObject()->Cooldown - GetServerTime(), 0.0f, AbilityClass.GetDefaultObject()->Cooldown);
}

double USimpleGameplayAbilityComponent::GetServerTime_Implementation()
{
	if (!GetWorld())
	{
		SIMPLE_LOG(this, TEXT("GetServerTime called but GetWorld is not valid!"));
		return 0.0;
	}

	if (!GetWorld()->GetGameState())
	{
		SIMPLE_LOG(this, TEXT("GetServerTime called but GetGameState is not valid!"));
		return 0.0;
	}
	
	return GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
}

bool USimpleGameplayAbilityComponent::IsAnyAbilityActive() const
{
	for (const USimpleGameplayAbility* AbilityInstance : InstancedAbilities)
	{
		if (AbilityInstance->IsAbilityActive())
		{
			return true;
		}
	}

	return false;
}

bool USimpleGameplayAbilityComponent::DoesAbilityHaveOverride(TSubclassOf<USimpleGameplayAbility> AbilityClass) const
{
	for (const FAbilityOverride& AbilityOverride : ActiveAbilityOverrides)
	{
		if (AbilityOverride.OriginalAbility == AbilityClass)
		{
			return true;
		}
	}

	return false;
}

/* Replication */

void USimpleGameplayAbilityComponent::OnStateAdded(const FAbilityState& NewAbilityState)
{
	if (HasAuthority())
	{
		return;
	}
	
	// A mapping of the local ability states for quick lookups
	TMap<FGuid, int32> LocalStateArrayIndexMap;

	// Added a new gameplay ability state
	if (NewAbilityState.AbilityClass->IsChildOf(USimpleGameplayAbility::StaticClass()))
	{
		// Get a mapping of the local ability states for quick lookups
		for (int32 i = 0; i < LocalAbilityStates.Num(); i++)
		{
			LocalStateArrayIndexMap.Add(LocalAbilityStates[i].AbilityID, i);
		}
	
		// If the NewAbilityState doesn't exist locally, we activate it
		if (!LocalStateArrayIndexMap.Contains(NewAbilityState.AbilityID))
		{
			const TSubclassOf<USimpleGameplayAbility> AbilityClass = static_cast<TSubclassOf<USimpleGameplayAbility>>(NewAbilityState.AbilityClass);

			LocalAbilityStates.AddUnique(NewAbilityState);
			LocalStateArrayIndexMap.Add(NewAbilityState.AbilityID, LocalAbilityStates.Num() - 1);

			if (NewAbilityState.AbilityStatus != ActivationSuccess && NewAbilityState.AbilityStatus != EndedSuccessfully)
			{
				return;
			}
			
			USimpleGameplayAbility* NewAbilityInstance = GetAbilityInstance(AbilityClass);

			if (NewAbilityInstance->IsAbilityActive())
			{
				NewAbilityInstance->CancelAbility(FDefaultTags::AbilityCancelled(), FInstancedStruct());
			}
			
			NewAbilityInstance->InitializeAbility(this, NewAbilityState.AbilityID, true);
			NewAbilityInstance->ActivateAbility(NewAbilityState.AbilityID, NewAbilityState.ActivationContext);
			return;
		}

		FAbilityState& LocalState = LocalAbilityStates[LocalStateArrayIndexMap[NewAbilityState.AbilityID]];
		CompareSnapshots(NewAbilityState, LocalState);
	}

	// Added a new attribute state
	if (NewAbilityState.AbilityClass->IsChildOf(USimpleAttributeModifier::StaticClass()))
	{
		for (int32 i = 0; i < LocalAttributeStates.Num(); i++)
		{
			LocalStateArrayIndexMap.Add(LocalAttributeStates[i].AbilityID, i);
		}

		// If the NewAbilityState doesn't exist locally, we create a state and apply side effects if the state history is not empty
		if (!LocalStateArrayIndexMap.Contains(NewAbilityState.AbilityID))
		{
			LocalAttributeStates.Add(NewAbilityState);

			if (NewAbilityState.SnapshotHistory.Num() > 0)
			{
				USimpleAttributeModifier* Modifier = GetAttributeModifierInstance(NewAbilityState.AbilityID);

				if (!Modifier)
				{
					UClass* ParentClassPtr = NewAbilityState.AbilityClass.Get();
					const TSubclassOf<USimpleAttributeModifier> AbilityClass = Cast<UClass>(ParentClassPtr);
					Modifier = NewObject<USimpleAttributeModifier>(this, AbilityClass);
					InstancedAttributes.Add(Modifier);
				}

				Modifier->InitializeAbility(this, NewAbilityState.AbilityID, true);

				if (Modifier->ModifierApplicationPolicy == EAttributeModifierApplicationPolicy::ApplyClientPredicted)
				{
					return;
				}
				
				Modifier->ClientFastForwardState(NewAbilityState.SnapshotHistory.Last().SnapshotTag, NewAbilityState.SnapshotHistory.Last());
			}
		}
		
	}
}

void USimpleGameplayAbilityComponent::OnStateChanged(const FAbilityState& AuthorityAbilityState)
{
	if (HasAuthority())
	{
		return;
	}
	
	if (!AuthorityAbilityState.AbilityClass)
	{
		SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::OnStateChanged]: Changed ability state with ID %s has an invalid class upon replication."), *AuthorityAbilityState.AbilityID.ToString()));
		return;
	}
	
	// Changed an ability state
	if (AuthorityAbilityState.AbilityClass->IsChildOf(USimpleGameplayAbility::StaticClass()))
	{
		// Get a reference to the local version of the changed ability on the server
		FAbilityState* LocalAbilityState = LocalAbilityStates.FindByPredicate([AuthorityAbilityState](const FAbilityState& AbilityState)
		{
			return AbilityState.AbilityID == AuthorityAbilityState.AbilityID;
		});

		// If the ability doesn't exist locally, we don't need to do anything
		if (!LocalAbilityState)
		{
			SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::OnStateChanged]: Ability with ID %s not found in LocalAbilityStates array"), *AuthorityAbilityState.AbilityID.ToString()));
			return;
		}

		// Check if the latest SnapshotHistory of the ability has changed
		if (AuthorityAbilityState.SnapshotHistory.Num() > 0)
		{
			CompareSnapshots(AuthorityAbilityState, *LocalAbilityState);
		}
		
		// Check if the server cancelled the ability
		if (AuthorityAbilityState.AbilityStatus == EndedCancelled && LocalAbilityState->AbilityStatus != EndedCancelled)
		{
			CancelAbility(AuthorityAbilityState.AbilityID, AuthorityAbilityState.EndingContext);
		}
	}

	// Changed an attribute state
	if (AuthorityAbilityState.AbilityClass->IsChildOf(USimpleAttributeModifier::StaticClass()))
	{
		if (AuthorityAbilityState.SnapshotHistory.Num() > 0)
		{
			USimpleAttributeModifier* Modifier = GetAttributeModifierInstance(AuthorityAbilityState.AbilityID);

			if (!Modifier)
			{
				SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::OnStateChanged]: Attribute modifier with ID %s not found in InstancedAttributes array"), *AuthorityAbilityState.AbilityID.ToString()));
				return;
			}

			TArray<FSimpleAbilitySnapshot>* LocalSnapshots = GetLocalAttributeStateSnapshots(AuthorityAbilityState.AbilityID);

			if (!LocalSnapshots)
			{
				SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::OnStateChanged]: Attribute modifier with ID %s not found in LocalAttributeStates array"), *AuthorityAbilityState.AbilityID.ToString()));
				return;
			}
			
			for (FSimpleAbilitySnapshot& LocalSnapshot : *LocalSnapshots)
			{
				if (LocalSnapshot.SnapshotTag == AuthorityAbilityState.SnapshotHistory.Last().SnapshotTag && !LocalSnapshot.WasClientSnapshotResolved)
				{
					Modifier->ClientResolvePastState(AuthorityAbilityState.SnapshotHistory.Last().SnapshotTag, AuthorityAbilityState.SnapshotHistory.Last(), LocalSnapshot);
					LocalSnapshot.WasClientSnapshotResolved = true;
					break;
				}
			}
		}	
	}
}
  
void USimpleGameplayAbilityComponent::OnStateRemoved(const FAbilityState& RemovedAbilityState)
{
	if (RemovedAbilityState.AbilityClass->IsChildOf(USimpleGameplayAbility::StaticClass()))
	{
		LocalAbilityStates.RemoveAll([RemovedAbilityState](const FAbilityState& AbilityState) { return AbilityState.AbilityID == RemovedAbilityState.AbilityID; });
		return;
	}

	if (RemovedAbilityState.AbilityClass->IsChildOf(USimpleAttributeModifier::StaticClass()))
	{
		LocalAttributeStates.RemoveAll([RemovedAbilityState](const FAbilityState& AbilityState) { return AbilityState.AbilityID == RemovedAbilityState.AbilityID; });
	}
}

void USimpleGameplayAbilityComponent::CompareSnapshots(const FAbilityState& AuthorityAbilityState, FAbilityState& LocalAbilityState)
{
	if (AuthorityAbilityState.SnapshotHistory.Num() == 0)
	{
		return;
	}
	
	const FSimpleAbilitySnapshot& AuthoritySnapshot = AuthorityAbilityState.SnapshotHistory.Last();
        
	// Try to find matching snapshot by sequence number first
	FSimpleAbilitySnapshot* MatchingSnapshot = nullptr;
        
	for (FSimpleAbilitySnapshot& ClientSnapshot : LocalAbilityState.SnapshotHistory)
	{
		if (ClientSnapshot.SnapshotTag == AuthoritySnapshot.SnapshotTag && 
			ClientSnapshot.SequenceNumber == AuthoritySnapshot.SequenceNumber &&
			!ClientSnapshot.WasClientSnapshotResolved)
		{
			MatchingSnapshot = &ClientSnapshot;
			break;
		}
	}
        
	// Fall back to just matching by tag if sequence matching fails
	if (!MatchingSnapshot)
	{
		for (FSimpleAbilitySnapshot& ClientSnapshot : LocalAbilityState.SnapshotHistory)
		{
			if (ClientSnapshot.SnapshotTag == AuthoritySnapshot.SnapshotTag && 
				!ClientSnapshot.WasClientSnapshotResolved)
			{
				MatchingSnapshot = &ClientSnapshot;
				break;
			}
		}
	}

	USimpleGameplayAbility* AbilityInstance = GetGameplayAbilityInstance(AuthorityAbilityState.AbilityID);
			
	if (MatchingSnapshot && AbilityInstance)
	{
		AbilityInstance->ClientResolvePastState(AuthoritySnapshot.SnapshotTag, AuthoritySnapshot, *MatchingSnapshot);
		MatchingSnapshot->WasClientSnapshotResolved = true;
	}
}

void USimpleGameplayAbilityComponent::OnFloatAttributeAdded(const FFloatAttribute& NewFloatAttribute)
{
	LocalFloatAttributes.AddUnique(NewFloatAttribute);
	SendEvent(FDefaultTags::FloatAttributeAdded(), NewFloatAttribute.AttributeTag, FInstancedStruct(), GetOwner(), {}, ESimpleEventReplicationPolicy::NoReplication);
}

void USimpleGameplayAbilityComponent::OnFloatAttributeChanged(const FFloatAttribute& ChangedFloatAttribute)
{
	for (FFloatAttribute& LocalFloatAttribute : LocalFloatAttributes)
	{
		if (LocalFloatAttribute.AttributeTag.MatchesTagExact(ChangedFloatAttribute.AttributeTag))
		{
			CompareFloatAttributesAndSendEvents(LocalFloatAttribute, ChangedFloatAttribute);
			LocalFloatAttribute = ChangedFloatAttribute;
			return;
		}
	}

	LocalFloatAttributes.AddUnique(ChangedFloatAttribute);
	SendEvent(FDefaultTags::FloatAttributeAdded(), ChangedFloatAttribute.AttributeTag, FInstancedStruct(), GetOwner(), {}, ESimpleEventReplicationPolicy::NoReplication);
}

void USimpleGameplayAbilityComponent::OnFloatAttributeRemoved(const FFloatAttribute& RemovedFloatAttribute)
{
	LocalFloatAttributes.Remove(RemovedFloatAttribute);
	SendEvent(FDefaultTags::FloatAttributeRemoved(), RemovedFloatAttribute.AttributeTag, FInstancedStruct(), GetOwner(), {}, ESimpleEventReplicationPolicy::NoReplication);
}

void USimpleGameplayAbilityComponent::OnStructAttributeAdded(const FStructAttribute& NewStructAttribute)
{
	if (!LocalStructAttributes.Contains(NewStructAttribute))
	{
		LocalStructAttributes.Add(NewStructAttribute);
		SendEvent(FDefaultTags::StructAttributeAdded(), NewStructAttribute.AttributeTag, NewStructAttribute.AttributeValue, GetOwner(), {}, ESimpleEventReplicationPolicy::NoReplication);
	}
}

void USimpleGameplayAbilityComponent::OnStructAttributeChanged(const FStructAttribute& ChangedStructAttribute)
{
	for (FStructAttribute& LocalStructAttribute : LocalStructAttributes)
	{
		if (LocalStructAttribute.AttributeTag.MatchesTagExact(ChangedStructAttribute.AttributeTag))
		{
			LocalStructAttribute = ChangedStructAttribute;
			return;
		}
	}

	LocalStructAttributes.Add(ChangedStructAttribute);
	SendEvent(FDefaultTags::StructAttributeAdded(), ChangedStructAttribute.AttributeTag, ChangedStructAttribute.AttributeValue, GetOwner(), {}, ESimpleEventReplicationPolicy::NoReplication);
}

void USimpleGameplayAbilityComponent::OnStructAttributeRemoved(const FStructAttribute& RemovedStructAttribute)
{
	LocalStructAttributes.Remove(RemovedStructAttribute);
	SendEvent(FDefaultTags::StructAttributeRemoved(), RemovedStructAttribute.AttributeTag, FInstancedStruct(), GetOwner(), {}, ESimpleEventReplicationPolicy::NoReplication);
}

void USimpleGameplayAbilityComponent::OnGameplayTagAdded(const FGameplayTagCounter& GameplayTag)
{
	FGameplayTagCounter* LocalTagCounter = LocalGameplayTags.FindByPredicate(
		[GameplayTag](const FGameplayTagCounter& TagCounter)
		{
			return TagCounter.GameplayTag.MatchesTagExact(GameplayTag.GameplayTag);
		});

	if (!LocalTagCounter)
	{
		LocalGameplayTags.AddUnique(GameplayTag);
		SendEvent(FDefaultTags::GameplayTagAdded(), GameplayTag.GameplayTag, FInstancedStruct(), GetOwner(), {}, ESimpleEventReplicationPolicy::NoReplication);
		return;
	}

	LocalTagCounter->ReferenceCounter = GameplayTag.ReferenceCounter;
}

void USimpleGameplayAbilityComponent::OnGameplayTagChanged(const FGameplayTagCounter& GameplayTag)
{
	FGameplayTagCounter* LocalTagCounter = LocalGameplayTags.FindByPredicate(
		[GameplayTag](const FGameplayTagCounter& TagCounter)
		{
			return TagCounter.GameplayTag.MatchesTagExact(GameplayTag.GameplayTag);
		});

	if (!LocalTagCounter)
	{
		LocalGameplayTags.AddUnique(GameplayTag);
		SendEvent(FDefaultTags::GameplayTagAdded(), GameplayTag.GameplayTag, FInstancedStruct(), GetOwner(), {}, ESimpleEventReplicationPolicy::NoReplication);
		return;
	}

	LocalTagCounter->ReferenceCounter = GameplayTag.ReferenceCounter;
}

void USimpleGameplayAbilityComponent::OnGameplayTagRemoved(const FGameplayTagCounter& GameplayTag)
{
	FGameplayTagCounter* LocalTagCounter = LocalGameplayTags.FindByPredicate(
		[GameplayTag](const FGameplayTagCounter& TagCounter)
		{
			return TagCounter.GameplayTag.MatchesTagExact(GameplayTag.GameplayTag);
		});

	if (LocalTagCounter)
	{
		LocalGameplayTags.RemoveSingle(GameplayTag);
		SendEvent(FDefaultTags::GameplayTagRemoved(), GameplayTag.GameplayTag, FInstancedStruct(), GetOwner(), {}, ESimpleEventReplicationPolicy::NoReplication);
	}
}

void USimpleGameplayAbilityComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USimpleGameplayAbilityComponent, AvatarActor);
	DOREPLIFETIME(USimpleGameplayAbilityComponent, AuthorityGameplayTags);
	DOREPLIFETIME(USimpleGameplayAbilityComponent, GrantedAbilities);
	DOREPLIFETIME(USimpleGameplayAbilityComponent, ActiveAbilityOverrides);
	DOREPLIFETIME(USimpleGameplayAbilityComponent, AuthorityFloatAttributes);
	DOREPLIFETIME(USimpleGameplayAbilityComponent, AuthorityStructAttributes);
	DOREPLIFETIME(USimpleGameplayAbilityComponent, AuthorityAbilityStates);
	DOREPLIFETIME(USimpleGameplayAbilityComponent, AuthorityAttributeStates);
}


