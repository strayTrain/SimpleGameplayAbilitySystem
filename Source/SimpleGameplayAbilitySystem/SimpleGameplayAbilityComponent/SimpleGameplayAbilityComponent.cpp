#include "SimpleGameplayAbilityComponent.h"

#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "SimpleGameplayAbilitySystem/DataAssets/AbilitySet/AbilitySet.h"
#include "SimpleGameplayAbilitySystem/DataAssets/AttributeSet/AttributeSet.h"
#include "SimpleGameplayAbilitySystem/DefaultTags/DefaultTags.h"
#include "SimpleGameplayAbilitySystem/Module/SimpleGameplayAbilitySystem.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleAbilityTypes.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleGameplayAbility/SimpleGameplayAbility.h"
#include "SimpleGameplayAbilitySystem/SimpleGameplayAbilityComponent/SimpleAbilityComponentTypes.h"
#include "SimpleGameplayAbilitySystem/SimpleEventSubsystem/SimpleEventSubSystem.h"
#include "SimpleGameplayAbilitySystem/DataAssets/AbilityOverrideSet/AbilityOverrideSet.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleAttributeModifier/SimpleAttributeModifier.h"

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
	
	if (HasAuthority())
	{
		// For abilities granted directly through the editor
		for (const TSubclassOf<USimpleGameplayAbility> AbilityClass : GrantedAbilities)
		{
			USimpleGameplayAbility::OnGrantedStatic(AbilityClass, this);
		}
		
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
	AuthorityAbilityStates.OnStateAdded.BindUObject(this, &USimpleGameplayAbilityComponent::OnAbilityStateAdded);
	AuthorityAbilityStates.OnStateChanged.BindUObject(this, &USimpleGameplayAbilityComponent::OnAbilityStateChanged);
	AuthorityAbilityStates.OnStateRemoved.BindUObject(this, &USimpleGameplayAbilityComponent::OnAbilityStateRemoved);
	
	AuthorityAttributeModifierStates.OnStateAdded.BindUObject(this, &USimpleGameplayAbilityComponent::OnAttributeModiferStateAdded);
	AuthorityAttributeModifierStates.OnStateChanged.BindUObject(this, &USimpleGameplayAbilityComponent::OnAttributeModifierStateChanged);
	AuthorityAttributeModifierStates.OnStateRemoved.BindUObject(this, &USimpleGameplayAbilityComponent::OnAttributeModiferStateRemoved);

	AuthorityAbilitySnapshots.OnSnapshotAdded.BindUObject(this, &USimpleGameplayAbilityComponent::OnAbilitySnapshotAdded);
	AuthorityAttributeModifierSnapshots.OnSnapshotAdded.BindUObject(this, &USimpleGameplayAbilityComponent::OnAttributeModifierSnapshotAdded);
	
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

void USimpleGameplayAbilityComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Clean up attribute modifiers
	for (USimpleAttributeModifier* AttributeModifier : InstancedAttributeModifiers)
	{
		AttributeModifier->CleanUpAbility();
	}
    
	// Clean up abilities
	for (USimpleGameplayAbility* Ability : InstancedAbilities)
	{
		Ability->CleanUpAbility();
	}
    
	// Clear collections
	InstancedAttributeModifiers.Empty();
	InstancedAbilities.Empty();
    
	// Unsubscribe from events
	if (USimpleEventSubsystem* EventSubsystem = GetWorld() ? GetWorld()->GetGameInstance()->GetSubsystem<USimpleEventSubsystem>() : nullptr)
	{
		EventSubsystem->StopListeningForAllEvents(this);
	}
	
	Super::EndPlay(EndPlayReason);
}

/* Ability Functions */

bool USimpleGameplayAbilityComponent::ActivateAbility(
	const TSubclassOf<USimpleGameplayAbility> AbilityClass,
	const FAbilityContextCollection AbilityContexts,
	FGuid& AbilityID,
	const EAbilityActivationPolicyOverride ActivationPolicyOverride)
{
	AbilityID = FGuid::NewGuid();
	return ActivateAbilityWithID(AbilityID, AbilityClass, AbilityContexts, ActivationPolicyOverride);
}

bool USimpleGameplayAbilityComponent::ActivateAbilityWithID(
	const FGuid AbilityID,
	const TSubclassOf<USimpleGameplayAbility>& AbilityClass,
	const FAbilityContextCollection& AbilityContexts,
	const EAbilityActivationPolicyOverride ActivationPolicyOverride)
{
	if (!GrantedAbilities.Contains(AbilityClass) && AbilityClass->GetDefaultObject<USimpleGameplayAbility>()->RequireGrantToActivate)
	{
		SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::ActivateAbility]: Ability %s is not granted!"), *AbilityClass->GetName()));	
		return false;
	}
	
	EAbilityActivationPolicy ActivationPolicy = EAbilityActivationPolicy::LocalOnly;

	switch (ActivationPolicyOverride)
	{
		case EAbilityActivationPolicyOverride::DontOverride:
			ActivationPolicy = AbilityClass->GetDefaultObject<USimpleGameplayAbility>()->ActivationPolicy;
			break;
		case EAbilityActivationPolicyOverride::ForceLocalOnly:
			ActivationPolicy = EAbilityActivationPolicy::LocalOnly;
			break;
		case EAbilityActivationPolicyOverride::ForceClientOnly:
			ActivationPolicy = EAbilityActivationPolicy::ClientOnly;
			break;
		case EAbilityActivationPolicyOverride::ForceClientPredicted:
			ActivationPolicy = EAbilityActivationPolicy::ClientPredicted;
			break;
		case EAbilityActivationPolicyOverride::ForceServerInitiated:
			ActivationPolicy = EAbilityActivationPolicy::ServerInitiated;
			break;
		case EAbilityActivationPolicyOverride::ForceServerInitiatedFromClient:
			ActivationPolicy = EAbilityActivationPolicy::ServerInitiatedFromClient;
			break;
		case EAbilityActivationPolicyOverride::ForceServerOnly:
			ActivationPolicy = EAbilityActivationPolicy::ServerOnly;
			break;
	}
	
	const bool IsClient = GetNetMode() == NM_Client && !HasAuthority();
	const double ActivationTime = GetServerTime();

	switch (ActivationPolicy)
	{
		case EAbilityActivationPolicy::LocalOnly:
			return ActivateAbilityInternal(AbilityID, AbilityClass, AbilityContexts, ActivationPolicy, false, ActivationTime);
		
		case EAbilityActivationPolicy::ClientOnly:
			if (!IsClient)
			{
				SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::ActivateAbility]: Can't activate ability %s on server with ClientOnly policy."), *AbilityClass->GetName()));
				return false;
			}
		
			return ActivateAbilityInternal(AbilityID, AbilityClass, AbilityContexts, ActivationPolicy, false, ActivationTime);
		
		case EAbilityActivationPolicy::ServerInitiatedFromClient:
			if (IsClient)
			{
				ServerActivateAbility(AbilityID, AbilityClass, AbilityContexts, ActivationPolicy, ActivationTime);
				return true;
			}

			return ActivateAbilityInternal(AbilityID, AbilityClass, AbilityContexts, ActivationPolicy, true, ActivationTime);
		
		case EAbilityActivationPolicy::ClientPredicted:
			if (IsClient)
			{
				ServerActivateAbility(AbilityID, AbilityClass, AbilityContexts, ActivationPolicy, ActivationTime);
			}
		
			return ActivateAbilityInternal(AbilityID, AbilityClass, AbilityContexts, ActivationPolicy, true, ActivationTime);;

		case EAbilityActivationPolicy::ServerInitiated:
			if (IsClient)
			{
				SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::ActivateAbility]: Can't activate ability %s on client with ServerInitiated policy."), *AbilityClass->GetName()));
				return false;
			}
		
			return ActivateAbilityInternal(AbilityID, AbilityClass, AbilityContexts, ActivationPolicy, true, ActivationTime);

		case EAbilityActivationPolicy::ServerOnly:
			if (IsClient)
			{
				SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::ActivateAbility]: Can't activate ability %s on client with ServerOnly policy."), *AbilityClass->GetName()));
				return false;
			}
		
			return ActivateAbilityInternal(AbilityID, AbilityClass, AbilityContexts, ActivationPolicy, false, ActivationTime);
	}
	
	return false;
}

bool USimpleGameplayAbilityComponent::ActivateAbilityInternal(
	const FGuid AbilityID,
	const TSubclassOf<USimpleGameplayAbility>& AbilityClass,
	const FAbilityContextCollection& AbilityContexts,
	const EAbilityActivationPolicy ActivationPolicy,
	const bool TrackState,
	const double ActivationTime)
{
	if (!AbilityClass)
	{
		SIMPLE_LOG(this, TEXT("[USimpleGameplayAbilityComponent::ActivateAbilityInternal]: AbilityClass is null!"));
		return false;
	}
	
	USimpleGameplayAbility* AbilityInstance = GetAbilityInstance(AbilityClass);
	
	if (AbilityInstance->InstancingPolicy == EAbilityInstancingPolicy::SingleInstance)
	{
		if (AbilityInstance->IsActive)
		{
			AbilityInstance->CancelAbility(FDefaultTags::AbilityCancelled(), FInstancedStruct());
		}
	}
	
	if (TrackState)
	{
		FAbilityState NewState;
		NewState.AbilityID = AbilityID;
		NewState.AbilityClass = AbilityClass;
		NewState.ActivationPolicy = ActivationPolicy;
		NewState.ActivationTimeStamp = ActivationTime;
		NewState.ActivationContexts = AbilityContexts;
		
		if (GetNetMode() == NM_Client && !HasAuthority())
		{
			LocalAbilityStates.Add(NewState);
		}
		else
		{
			AuthorityAbilityStates.AbilityStates.Add(NewState);
			AuthorityAbilityStates.MarkItemDirty(NewState);
		}
	}
	
	return AbilityInstance->Activate(this, AbilityID, AbilityContexts);;
}

void USimpleGameplayAbilityComponent::ServerActivateAbility_Implementation(
	const FGuid AbilityID,
	TSubclassOf<USimpleGameplayAbility> AbilityClass,
	const FAbilityContextCollection& AbilityContexts,
	EAbilityActivationPolicy ActivationPolicy,
	const float ActivationTime)
{
	if (!AbilityClass)
	{
		SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::ServerActivateAbility]: AbilityClass is null!")));
		return;
	}
	
	ActivateAbilityInternal(AbilityID, AbilityClass, AbilityContexts, ActivationPolicy, true, ActivationTime);
}

USimpleAttributeHandler* USimpleGameplayAbilityComponent::GetAttributeHandler(const FGameplayTag AttributeTag)
{
	const FStructAttribute* StructAttribute = GetStructAttribute(AttributeTag);

	if (!StructAttribute)
	{
		SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::GetAttributeHandler]: Attribute %s not found."), *AttributeTag.ToString()));
		return nullptr;
	}

	if (!StructAttribute->StructAttributeHandler)
	{
		SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::GetAttributeHandler]: Struct attribute %s has no attribute handler class configured."), *AttributeTag.ToString()));
		return nullptr;
	}

	return GetStructAttributeHandlerInstance(StructAttribute->StructAttributeHandler);
}

USimpleAttributeHandler* USimpleGameplayAbilityComponent::GetAttributeHandlerAs(FGameplayTag AttributeTag, const TSubclassOf<USimpleAttributeHandler> AttributeHandlerClass)
{
	USimpleAttributeHandler* AttributeHandler = GetAttributeHandler(AttributeTag);

	if (!AttributeHandler)
	{
		return nullptr;
	}

	if (!AttributeHandler->IsA(AttributeHandlerClass))
	{
		SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::GetAttributeHandlerAs]: Attribute Handler for attribute %s is not of type %s."), *AttributeTag.ToString(), *AttributeHandlerClass->GetName()));
		return nullptr;
	}

	return AttributeHandler;
}

USimpleGameplayAbility* USimpleGameplayAbilityComponent::GetAbilityInstance(const TSubclassOf<USimpleGameplayAbility>& AbilityClass)
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
	USimpleGameplayAbility* NewAbilityInstance = NewObject<USimpleGameplayAbility>(GetOuter(), AbilityClass);
	NewAbilityInstance->OwningAbilityComponent = this;
	NewAbilityInstance->OnActivationSuccess.BindDynamic(this, &USimpleGameplayAbilityComponent::OnAbilityActivationSuccess);
	NewAbilityInstance->OnActivationFailed.BindDynamic(this, &USimpleGameplayAbilityComponent::OnAbilityActivationFailed);
	NewAbilityInstance->OnAbilityEnded.BindDynamic(this, &USimpleGameplayAbilityComponent::OnAbilityEnded);
	NewAbilityInstance->OnAbilityCancelled.BindDynamic(this, &USimpleGameplayAbilityComponent::OnAbilityCancelled);
	
	InstancedAbilities.Add(NewAbilityInstance);
	
	return NewAbilityInstance;	
}

void USimpleGameplayAbilityComponent::CancelAbility(const FGuid AbilityInstanceID, const FInstancedStruct CancellationContext)
{
	if (USimpleGameplayAbility* AbilityInstance = GetGameplayAbilityInstance(AbilityInstanceID))
	{
		if (!AbilityInstance->IsActive)
		{
			return;
		}
		
		AbilityInstance->CancelAbility(FDefaultTags::AbilityCancelled(), CancellationContext);
	}
}

TArray<FGuid> USimpleGameplayAbilityComponent::CancelAbilitiesWithTags(const FGameplayTagContainer Tags, FInstancedStruct CancellationContext)
{
	TArray<FGuid> CancelledAbilities;
	
	for (USimpleGameplayAbility* AbilityInstance : InstancedAbilities)
	{
		if (AbilityInstance->AbilityTags.HasAnyExact(Tags))
		{
			AbilityInstance->CancelAbility(FDefaultTags::AbilityCancelled(), CancellationContext);
			CancelledAbilities.Add(AbilityInstance->AbilityID);
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
	USimpleGameplayAbility::OnGrantedStatic(AbilityClass, this);
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

int32 USimpleGameplayAbilityComponent::AddGameplayAbilitySnapshot(const FGuid AbilityID, FInstancedStruct SnapshotData)
{
	TArray<FAbilitySnapshot>& SnapshotArray = HasAuthority() ? AuthorityAbilitySnapshots.Snapshots : LocalPendingAbilitySnapshots;
	
	FAbilitySnapshot NewSnapshot;
	NewSnapshot.AbilityID = AbilityID;
	NewSnapshot.SnapshotData = SnapshotData;
	NewSnapshot.TimeStamp = GetServerTime();
	NewSnapshot.SnapshotCounter = 0;

	// Calculate the snapshot counter in case we have multiple snapshots with the same tag
	for (FAbilitySnapshot& SnapShot : SnapshotArray)
	{
		if (SnapShot.AbilityID == AbilityID)
		{
			NewSnapshot.SnapshotCounter += 1;
		}
	}

	SnapshotArray.Add(NewSnapshot);
	
	if (HasAuthority())
	{
		AuthorityAbilitySnapshots.MarkItemDirty(NewSnapshot);
	}

	return NewSnapshot.SnapshotCounter;
}

int32 USimpleGameplayAbilityComponent::AddAttributeModifierSnapshot(const FGuid AbilityID, FInstancedStruct SnapshotData)
{
	TArray<FAbilitySnapshot>& SnapshotArray = HasAuthority() ? AuthorityAttributeModifierSnapshots.Snapshots : LocalPendingAttributeModiferSnapshots;
	
	FAbilitySnapshot NewSnapshot;
	NewSnapshot.AbilityID = AbilityID;
	NewSnapshot.SnapshotData = SnapshotData;
	NewSnapshot.TimeStamp = GetServerTime();
	NewSnapshot.SnapshotCounter = 0;

	// Calculate the snapshot counter in case we have multiple snapshots with the same tag
	for (FAbilitySnapshot& SnapShot : SnapshotArray)
	{
		if (SnapShot.AbilityID == AbilityID)
		{
			NewSnapshot.SnapshotCounter += 1;
		}
	}

	SnapshotArray.Add(NewSnapshot);
	
	if (HasAuthority())
	{
		AuthorityAttributeModifierSnapshots.MarkItemDirty(NewSnapshot);
	}

	return NewSnapshot.SnapshotCounter;
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
		AuthorityGameplayTags.MarkItemDirty(NewTagCounter);
	}
	
	SendEvent(FDefaultTags::GameplayTagAdded(), Tag, Payload, this, {}, ESimpleEventReplicationPolicy::NoReplication);
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
	
	SendEvent(FDefaultTags::GameplayTagRemoved(), Tag, Payload, this, {}, ESimpleEventReplicationPolicy::NoReplication);
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

FGameplayTagContainer USimpleGameplayAbilityComponent::GetActiveGameplayTags() const
{
	FGameplayTagContainer ActiveGameplayTags;
	
	const TArray<FGameplayTagCounter>& TagCounters = HasAuthority() ? AuthorityGameplayTags.Tags : LocalGameplayTags;
	for (const FGameplayTagCounter& TagCounter : TagCounters)
	{
		if (TagCounter.ReferenceCounter > 0)
		{
			ActiveGameplayTags.AddTag(TagCounter.GameplayTag);
		}
	}

	return ActiveGameplayTags;
}

/* Event Functions */

void USimpleGameplayAbilityComponent::SendEvent(
	FGameplayTag EventTag,
	FGameplayTag DomainTag,
	FInstancedStruct Payload,
	UObject* Sender,
	TArray<UObject*> ListenerFilter,
	ESimpleEventReplicationPolicy ReplicationPolicy)
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

void USimpleGameplayAbilityComponent::SendEventInternal(
	FGuid EventID,
	FGameplayTag EventTag,
	FGameplayTag DomainTag,
	const FInstancedStruct& Payload,
	UObject* Sender,
	ESimpleEventReplicationPolicy ReplicationPolicy,
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
	UObject* Sender,
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

void USimpleGameplayAbilityComponent::ClientSendEvent_Implementation(
	FGuid EventID,
	FGameplayTag EventTag,
	FGameplayTag DomainTag,
	FInstancedStruct Payload,
	UObject* Sender,
	ESimpleEventReplicationPolicy ReplicationPolicy,
	const TArray<UObject*>& ListenerFilter)
{
	SendEventInternal(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy, {});
}

void USimpleGameplayAbilityComponent::MulticastSendEvent_Implementation(
	FGuid EventID,
	FGameplayTag EventTag,
	FGameplayTag DomainTag,
	FInstancedStruct Payload,
	UObject* Sender,
	ESimpleEventReplicationPolicy ReplicationPolicy,
	const TArray<UObject*>& ListenerFilter)
{
	SendEventInternal(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy, {});
}

/* Utility Functions */

USimpleGameplayAbility* USimpleGameplayAbilityComponent::GetGameplayAbilityInstance(FGuid AbilityInstanceID)
{
	for (USimpleGameplayAbility* InstancedAbility : InstancedAbilities)
	{
		if (InstancedAbility->AbilityID == AbilityInstanceID)
		{
			return InstancedAbility;
		}
	}
	
	return nullptr;
}

USimpleAttributeModifier* USimpleGameplayAbilityComponent::GetAttributeModifierInstance(FGuid AttributeInstanceID)
{
	for (USimpleAttributeModifier* InstancedModifier : InstancedAttributeModifiers)
	{
		if (InstancedModifier->AbilityID == AttributeInstanceID)
		{
			return InstancedModifier;
		}
	}

	return nullptr;
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

bool USimpleGameplayAbilityComponent::HasAuthority() const
{
	if (GetOwner())
	{
		return GetOwner()->HasAuthority();
	}

	return false;
}

bool USimpleGameplayAbilityComponent::IsAnyAbilityActive() const
{
	for (const USimpleGameplayAbility* AbilityInstance : InstancedAbilities)
	{
		if (AbilityInstance->IsActive)
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

/* Ability Lifecycle */

void USimpleGameplayAbilityComponent::OnAbilityActivationSuccess(FGuid AbilityID)
{
	TArray<FAbilityState>& AbilityStates = HasAuthority() ? AuthorityAbilityStates.AbilityStates : LocalAbilityStates;

	for (FAbilityState& AbilityState : AbilityStates)
	{
		if (AbilityState.AbilityID == AbilityID)
		{
			AbilityState.AbilityStatus = ActivationSuccess;

			if (HasAuthority())
			{
				AuthorityAbilityStates.MarkItemDirty(AbilityState);
			}
			
			return;
		}
	}
}

void USimpleGameplayAbilityComponent::OnAbilityActivationFailed(FGuid AbilityID)
{
	TArray<FAbilityState>& AbilityStates = HasAuthority() ? AuthorityAbilityStates.AbilityStates : LocalAbilityStates;

	for (FAbilityState& AbilityState : AbilityStates)
	{
		if (AbilityState.AbilityID == AbilityID)
		{
			AbilityState.AbilityStatus = ActivationFailed;

			if (HasAuthority())
			{
				AuthorityAbilityStates.MarkItemDirty(AbilityState);
			}
			
			return;
		}
	}
}

void USimpleGameplayAbilityComponent::OnAbilityEnded(FGuid AbilityID, FGameplayTag EndStatus, FInstancedStruct EndingContext)
{
	TArray<FAbilityState>& AbilityStates = HasAuthority() ? AuthorityAbilityStates.AbilityStates : LocalAbilityStates;

	for (FAbilityState& AbilityState : AbilityStates)
	{
		if (AbilityState.AbilityID == AbilityID)
		{
			AbilityState.AbilityStatus = Ended;
			AbilityState.EndingContext = EndingContext;
			AbilityState.EndingTimeStamp = GetServerTime();

			if (HasAuthority())
			{
				AuthorityAbilityStates.MarkItemDirty(AbilityState);
			}
			
			return;
		}
	}
}

void USimpleGameplayAbilityComponent::OnAbilityCancelled(FGuid AbilityID, FGameplayTag CancelStatus, FInstancedStruct CancellationContext)
{
	TArray<FAbilityState>& AbilityStates = HasAuthority() ? AuthorityAbilityStates.AbilityStates : LocalAbilityStates;

	for (int i = 0; i < AbilityStates.Num(); i++)
	{
		if (AbilityStates[i].AbilityID == AbilityID)
		{
			AbilityStates[i].AbilityStatus = Cancelled;
			AbilityStates[i].EndingContext = CancellationContext;
			AbilityStates[i].EndingTimeStamp = GetServerTime();

			if (HasAuthority())
			{
				AuthorityAbilityStates.MarkItemDirty(AbilityStates[i]);
			}
			
			return;
		}
	}
}

/* Replication */

void USimpleGameplayAbilityComponent::OnAbilityStateAdded(const FAbilityState& NewAbilityState)
{
	ResolveLocalAbilityState(NewAbilityState);
}

void USimpleGameplayAbilityComponent::OnAbilityStateChanged(const FAbilityState& ChangedAbilityState)
{
	ResolveLocalAbilityState(ChangedAbilityState);
}

void USimpleGameplayAbilityComponent::ResolveLocalAbilityState(const FAbilityState& UpdatedAbilityState)
{
	// Server-only abilities should not be activated on the client
	if (UpdatedAbilityState.ActivationPolicy == EAbilityActivationPolicy::ServerOnly)
	{
		return;
	}

	FAbilityState* LocalAbilityState = LocalAbilityStates.FindByPredicate([UpdatedAbilityState](const FAbilityState& AbilityState) { return AbilityState.AbilityID == UpdatedAbilityState.AbilityID; });
	
	const TSubclassOf<USimpleGameplayAbility> AbilityClass = static_cast<TSubclassOf<USimpleGameplayAbility>>(UpdatedAbilityState.AbilityClass);
	USimpleGameplayAbility* AbilityInstance = GetAbilityInstance(AbilityClass);
	const bool IsSingleInstanceAbility = AbilityInstance->InstancingPolicy == EAbilityInstancingPolicy::SingleInstance;
	const bool IsInstancedAbilityActive = AbilityInstance->IsActive;
	
	switch (UpdatedAbilityState.AbilityStatus)
	{
		// If an ability ends on the server but never existed locally it probably ended very quickly and we should activate it locally
		// i.e. the ability activated and ended on the server in the same frame and got replicated to the client with the Ended status
		case Ended:
			if (LocalAbilityState)
			{
				break;
			}
		case ActivationSuccess:
			if (IsInstancedAbilityActive)
			{
				// If the ability is already running on the client using the UpdatedAbility state ID we don't need to do anything
				if (AbilityInstance->AbilityID == UpdatedAbilityState.AbilityID)
				{
					break;
				}

				// Otherwise an ability with a different ID is already running and needs to be cancelled first
				AbilityInstance->CancelAbility(FDefaultTags::AbilityCancelled(), FInstancedStruct());
				AbilityInstance->Activate(this, UpdatedAbilityState.AbilityID, UpdatedAbilityState.ActivationContexts);
			}
		
			// Ability instance isn't active so we activate a local copy
			AbilityInstance->Activate(this, UpdatedAbilityState.AbilityID, UpdatedAbilityState.ActivationContexts);

			break;
		
		// In these cases the ability is not running on the server and so if it is running locally we need to cancel it
		case PreActivation:
		case ActivationFailed:
		case Cancelled:
			if (IsSingleInstanceAbility && AbilityInstance->AbilityID == UpdatedAbilityState.AbilityID)
			{
				AbilityInstance->CancelAbility(FDefaultTags::AbilityCancelled(), FInstancedStruct());
			}
			break;
	}

	// Check if there is a local ability state with the same ID and update it
	if (LocalAbilityState)
	{
		*LocalAbilityState = UpdatedAbilityState;
		return;
	}

	// Otherwise add the new ability state to the local array
	LocalAbilityStates.Add(UpdatedAbilityState);
}

void USimpleGameplayAbilityComponent::OnAbilityStateRemoved(const FAbilityState& RemovedAbilityState)
{
	LocalAbilityStates.RemoveAll([RemovedAbilityState](const FAbilityState& AbilityState) { return AbilityState.AbilityID == RemovedAbilityState.AbilityID; });
}

void USimpleGameplayAbilityComponent::OnAttributeModiferStateAdded(const FAbilityState& NewAttributeModiferState)
{
	// A mapping of the local ability states for quick lookups
	TMap<FGuid, int32> LocalStateArrayIndexMap;
	
	for (int32 i = 0; i < LocalAttributeModifierStates.Num(); i++)
	{
		LocalStateArrayIndexMap.Add(LocalAttributeModifierStates[i].AbilityID, i);
	}

	// If the NewAbilityState doesn't exist locally, we create a state and apply side effects if the state history is not empty
	/*if (!LocalStateArrayIndexMap.Contains(NewAttributeModiferState.AbilityID))
	{
		LocalAttributeModifierStates.Add(NewAttributeModiferState);

		if (NewAttributeModiferState.SnapshotHistory.Num() > 0)
		{
			USimpleAttributeModifier* Modifier = GetAttributeModifierInstance(NewAttributeModiferState.AbilityID);

			if (!Modifier)
			{
				UClass* ParentClassPtr = NewAttributeModiferState.AbilityClass.Get();
				const TSubclassOf<USimpleAttributeModifier> AbilityClass = Cast<UClass>(ParentClassPtr);
				Modifier = NewObject<USimpleAttributeModifier>(this, AbilityClass);
				InstancedAttributeModifiers.Add(Modifier);
			}

			Modifier->InitializeAbility(this, NewAttributeModiferState.AbilityID, true);

			if (Modifier->ModifierApplicationPolicy == EAttributeModifierApplicationPolicy::ApplyClientPredicted)
			{
				return;
			}
			
			Modifier->ClientFastForwardState(NewAttributeModiferState.SnapshotHistory.Last().SnapshotTag, NewAttributeModiferState.SnapshotHistory.Last());
		}
	}*/
}

void USimpleGameplayAbilityComponent::OnAttributeModifierStateChanged(const FAbilityState& ChangedAttributeModiferState)
{
	if (!ChangedAttributeModiferState.AbilityClass)
	{
		SIMPLE_LOG(this,
			FString::Printf(TEXT("[USimpleGameplayAbilityComponent::OnStateChanged]: Changed ability state with ID %s has an invalid class upon replication."),
			*ChangedAttributeModiferState.AbilityID.ToString()));

		return;
	}
	
	/*if (ChangedAttributeModiferState.SnapshotHistory.Num() > 0)
	{
		USimpleAttributeModifier* Modifier = GetAttributeModifierInstance(ChangedAttributeModiferState.AbilityID);

		if (!Modifier)
		{
			return;
		}

		TArray<FSimpleAbilitySnapshot>* LocalSnapshots = GetLocalAttributeStateSnapshots(ChangedAttributeModiferState.AbilityID);

		if (!LocalSnapshots)
		{
			SIMPLE_LOG(this,
				FString::Printf(TEXT("[USimpleGameplayAbilityComponent::OnStateChanged]: Attribute modifier with ID %s not found in LocalAttributeModifierStates array"),
				*ChangedAttributeModiferState.AbilityID.ToString()));
			
			return;
		}
		
		for (FSimpleAbilitySnapshot& LocalSnapshot : *LocalSnapshots)
		{
			if (LocalSnapshot.SnapshotTag == ChangedAttributeModiferState.SnapshotHistory.Last().SnapshotTag && !LocalSnapshot.WasClientSnapshotResolved)
			{
				Modifier->ClientResolvePastState(ChangedAttributeModiferState.SnapshotHistory.Last().SnapshotTag, ChangedAttributeModiferState.SnapshotHistory.Last(), LocalSnapshot);
				LocalSnapshot.WasClientSnapshotResolved = true;
				break;
			}
		}
	}*/	
}

void USimpleGameplayAbilityComponent::OnAttributeModiferStateRemoved(const FAbilityState& RemovedAttributeModiferState)
{
	LocalAttributeModifierStates.RemoveAll([RemovedAttributeModiferState](const FAbilityState& AbilityState) { return AbilityState.AbilityID == RemovedAttributeModiferState.AbilityID; });
}

void USimpleGameplayAbilityComponent::OnAbilitySnapshotAdded(const FAbilitySnapshot& NewAbilitySnapshot)
{
	// Get the local version of NewAbilitySnapshot if it exists
	const FAbilitySnapshot* LocalSnapshot = LocalPendingAbilitySnapshots.FindByPredicate(
		[NewAbilitySnapshot](const FAbilitySnapshot& Snapshot)
		{
			return Snapshot.AbilityID == NewAbilitySnapshot.AbilityID && Snapshot.SnapshotCounter == NewAbilitySnapshot.SnapshotCounter;
		});

	// If there's no local snapshot for NewAbilitySnapshot, we've resolved it already, and we don't need to do anything.
	if (!LocalSnapshot)
	{
		return;
	}

	// We only want to resolve the snapshot if the ability is currently running
	USimpleGameplayAbility* LocalRunningAbilityInstance = nullptr;
	for (USimpleGameplayAbility* InstancedAbility : InstancedAbilities)
	{
		if (InstancedAbility->AbilityID == LocalSnapshot->AbilityID)
		{
			LocalRunningAbilityInstance = InstancedAbility;
			break;
		}
	}
	
	if (!LocalRunningAbilityInstance || !LocalRunningAbilityInstance->IsActive)
	{
		return;
	}

	// Call the resolve function on the local running ability instance
	LocalRunningAbilityInstance->OnServerSnapshotReceived(NewAbilitySnapshot.SnapshotCounter, NewAbilitySnapshot.SnapshotData, LocalSnapshot->SnapshotData);

	// Remove the local snapshot from the pending snapshots array now that we've resolved the differences
	if (LocalSnapshot)
	{
		LocalPendingAbilitySnapshots.RemoveAll([LocalSnapshot](const FAbilitySnapshot& Snapshot)
		{
			return Snapshot.AbilityID == LocalSnapshot->AbilityID && Snapshot.SnapshotCounter == LocalSnapshot->SnapshotCounter;
		});
	}
}

void USimpleGameplayAbilityComponent::OnAttributeModifierSnapshotAdded(const FAbilitySnapshot& NewAttributeModifierSnapshot)
{
	// Get the local version of NewAttributeModifierSnapshot if it exists
	const FAbilitySnapshot* LocalSnapshot = LocalPendingAbilitySnapshots.FindByPredicate(
		[NewAttributeModifierSnapshot](const FAbilitySnapshot& Snapshot)
		{
			return Snapshot.AbilityID == NewAttributeModifierSnapshot.AbilityID && Snapshot.SnapshotCounter == NewAttributeModifierSnapshot.SnapshotCounter;
		});

	// If there's no local snapshot for NewAbilitySnapshot, we've resolved it already, and we don't need to do anything.
	if (!LocalSnapshot)
	{
		return;
	}

	// Get a reference to the local running modifier instance for the ability ID in the snapshot. We expect this to exist
	USimpleAttributeModifier* LocalRunningModifierInstance = nullptr;
	for (USimpleAttributeModifier* InstancedModifier : InstancedAttributeModifiers)
	{
		if (InstancedModifier->GetClass() == LocalSnapshot->AbilityClass)
		{
			LocalRunningModifierInstance = InstancedModifier;
			break;
		}
	}

	if (!LocalRunningModifierInstance)
	{
		if (!LocalRunningModifierInstance)
		{
			LocalRunningModifierInstance = NewObject<USimpleAttributeModifier>(this, NewAttributeModifierSnapshot.AbilityClass);
			InstancedAttributeModifiers.Add(LocalRunningModifierInstance);
		}
	}

	// Notify the attribute modifier instance that it has received a server snapshot so it can resolve the differences
	LocalRunningModifierInstance->OnClientReceivedServerActionsResult(NewAttributeModifierSnapshot.SnapshotData, LocalSnapshot->SnapshotData);

	// Remove the local snapshot from the pending snapshots array now that we've resolved the differences
	if (LocalSnapshot)
	{
		LocalPendingAttributeModiferSnapshots.RemoveAll([LocalSnapshot](const FAbilitySnapshot& Snapshot)
		{
			return Snapshot.AbilityID == LocalSnapshot->AbilityID && Snapshot.SnapshotCounter == LocalSnapshot->SnapshotCounter;
		});
	}	
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
		SendEvent(FDefaultTags::GameplayTagAdded(), GameplayTag.GameplayTag, FInstancedStruct(), this, {}, ESimpleEventReplicationPolicy::NoReplication);
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
		SendEvent(FDefaultTags::GameplayTagAdded(), GameplayTag.GameplayTag, FInstancedStruct(), this, {}, ESimpleEventReplicationPolicy::NoReplication);
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
		SendEvent(FDefaultTags::GameplayTagRemoved(), GameplayTag.GameplayTag, FInstancedStruct(), this, {}, ESimpleEventReplicationPolicy::NoReplication);
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
	DOREPLIFETIME(USimpleGameplayAbilityComponent, AuthorityAbilitySnapshots);
	
	DOREPLIFETIME(USimpleGameplayAbilityComponent, AuthorityAttributeModifierStates);
	DOREPLIFETIME(USimpleGameplayAbilityComponent, AuthorityAttributeModifierSnapshots);
}


