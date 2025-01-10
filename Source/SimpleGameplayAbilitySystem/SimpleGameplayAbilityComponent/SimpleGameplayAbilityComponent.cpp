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
#include "SimpleGameplayAbilitySystem/BlueprintFunctionLibraries/SimpleAttributes/SimpleAttributeFunctionLibrary.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleAttributeModifier/SimpleAttributeModifier.h"

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
		for (UAttributeSet* AttributeSet : AttributeSets)
		{
			for (FFloatAttribute Attribute : AttributeSet->FloatAttributes)
			{
				AddFloatAttribute(Attribute);
			}

			for (FStructAttribute Attribute : AttributeSet->StructAttributes)
			{
				AddStructAttribute(Attribute);
			}
		}

		return;
	}

	// Delegates called on the client to handle replicated ability states
	AuthorityAbilityStates.OnAbilityStateAdded.BindUObject(this, &USimpleGameplayAbilityComponent::OnStateAdded);
	AuthorityAbilityStates.OnAbilityStateChanged.BindUObject(this, &USimpleGameplayAbilityComponent::OnStateChanged);
	AuthorityAbilityStates.OnAbilityStateRemoved.BindUObject(this, &USimpleGameplayAbilityComponent::OnStateRemoved);
	AuthorityAttributeStates.OnAbilityStateAdded.BindUObject(this, &USimpleGameplayAbilityComponent::OnStateAdded);
	AuthorityAttributeStates.OnAbilityStateChanged.BindUObject(this, &USimpleGameplayAbilityComponent::OnStateChanged);
	AuthorityAttributeStates.OnAbilityStateRemoved.BindUObject(this, &USimpleGameplayAbilityComponent::OnStateRemoved);
}

/* Ability Functions */

bool USimpleGameplayAbilityComponent::ActivateAbility(const TSubclassOf<USimpleGameplayAbility> AbilityClass, FInstancedStruct AbilityContext, bool OverrideActivationPolicy, EAbilityActivationPolicy ActivationPolicy)
{
	if (!AbilityClass)
	{
		SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::ActivateAbility]: AbilityClass is null!")));
		return false;
	}

	if (AbilityClass.GetDefaultObject()->bRequireGrantToActivate && !GrantedAbilities.Contains(AbilityClass))
	{
		SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::ActivateAbility]: Ability %s requires bring granted on the owning ability component to activate."), *AbilityClass->GetName()));
		return false;
	}
	
	const FGuid NewAbilityInstanceID = FGuid::NewGuid();
	const EAbilityActivationPolicy Policy = OverrideActivationPolicy ? ActivationPolicy : AbilityClass.GetDefaultObject()->ActivationPolicy;
	bool WasAbilityActivated = false;
	
	switch (Policy)
	{
		case EAbilityActivationPolicy::LocalOnly:
			WasAbilityActivated = ActivateAbilityInternal(AbilityClass, AbilityContext, NewAbilityInstanceID);
			break;
			
		case EAbilityActivationPolicy::LocalPredicted:
			AddNewAbilityState(AbilityClass, AbilityContext, NewAbilityInstanceID);
			WasAbilityActivated = ActivateAbilityInternal(AbilityClass, AbilityContext, NewAbilityInstanceID);
		
			if (!GetOwner()->HasAuthority())
			{
				ServerActivateAbility(AbilityClass, AbilityContext, NewAbilityInstanceID);
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
			AddNewAbilityState(AbilityClass, AbilityContext, NewAbilityInstanceID);
			WasAbilityActivated = ActivateAbilityInternal(AbilityClass, AbilityContext, NewAbilityInstanceID);
				
			break;
			
		case EAbilityActivationPolicy::ServerOnly:
			if (!GetOwner()->HasAuthority())
			{
				SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::ActivateAbility]: Client cannot activate server only abilities!")));
				return false;
			}
			
			WasAbilityActivated = ActivateAbilityInternal(AbilityClass, AbilityContext, NewAbilityInstanceID);
			break;
	}
	
	return WasAbilityActivated;
}

bool USimpleGameplayAbilityComponent::ActivateAbilityInternal(const TSubclassOf<USimpleGameplayAbility>& AbilityClass, const FInstancedStruct& AbilityContext, const FGuid AbilityInstanceID)
{
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
					SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::ActivateAbilityInternal]: Non cancellable single instance ability %s is already active!"), *AbilityClass->GetName()));
					return false;
				}
				
				InstancedAbility->InitializeAbility(this, AbilityInstanceID);
				return InstancedAbility->ActivateAbility(AbilityContext);
			}
		}
	}
	
	USimpleGameplayAbility* AbilityToActivate = NewObject<USimpleGameplayAbility>(this, AbilityClass);
	AbilityToActivate->InitializeAbility(this, AbilityInstanceID);
	InstancedAbilities.Add(AbilityToActivate);

	return AbilityToActivate->ActivateAbility(AbilityContext);
}

void USimpleGameplayAbilityComponent::ServerActivateAbility_Implementation(TSubclassOf<USimpleGameplayAbility> AbilityClass, FInstancedStruct AbilityContext, FGuid AbilityInstanceID)
{
	AddNewAbilityState(AbilityClass, AbilityContext, AbilityInstanceID);
	ActivateAbilityInternal(AbilityClass, AbilityContext, AbilityInstanceID);
}

bool USimpleGameplayAbilityComponent::CancelAbility(const FGuid AbilityInstanceID, FInstancedStruct CancellationContext)
{
	if (USimpleGameplayAbility* AbilityInstance = GetAbilityInstance(AbilityInstanceID))
	{
		AbilityInstance->EndCancel(CancellationContext);
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

void USimpleGameplayAbilityComponent::AddNewAbilityState(const TSubclassOf<USimpleGameplayAbility>& AbilityClass, const FInstancedStruct& AbilityContext, FGuid AbilityInstanceID)
{
	FAbilityState NewAbilityState;
	
	NewAbilityState.AbilityID = AbilityInstanceID;
	NewAbilityState.AbilityClass = AbilityClass;
	NewAbilityState.ActivationTimeStamp = GetServerTime();
	NewAbilityState.ActivationContext = AbilityContext;
	
	if (HasAuthority())
	{
		for (FSimpleAbilityStateItem& AuthorityAbilityState : AuthorityAbilityStates.AbilityStates)
		{
			if (AuthorityAbilityState.AbilityState.AbilityID == AbilityInstanceID)
			{
				SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::AddNewAbilityState]: Ability with ID %s already exists in AuthorityAbilityStates array."), *AbilityInstanceID.ToString()));
				return;
			}
		}

		FSimpleAbilityStateItem NewAbilityStateItem;
		NewAbilityStateItem.AbilityState = NewAbilityState;
		
		AuthorityAbilityStates.AbilityStates.Add(NewAbilityStateItem);
		AuthorityAbilityStates.MarkArrayDirty();
		SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::AddNewAbilityState]: Added new ability state with ID %s to AuthorityAbilityStates array."), *AbilityInstanceID.ToString()));
	}
	else
	{
		for (FAbilityState& AbilityState : LocalAbilityStates)
		{
			if (AbilityState.AbilityID == AbilityInstanceID)
			{
				SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::AddNewAbilityState]: Ability with ID %s already exists in LocalAbilityStates array."), *AbilityInstanceID.ToString()));
				return;
			}
		}
		
		LocalAbilityStates.Add(NewAbilityState);
		SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::AddNewAbilityState]: Added new ability state with ID %s to LocalAbilityStates array."), *AbilityInstanceID.ToString()));
	}
}

void USimpleGameplayAbilityComponent::AddAbilityStateSnapshot(FGuid AbilityInstanceID, FSimpleAbilitySnapshot State)
{
	if (HasAuthority())
	{
		for (FSimpleAbilityStateItem& AuthorityAbilityState : AuthorityAbilityStates.AbilityStates)
		{
			if (AuthorityAbilityState.AbilityState.AbilityID == AbilityInstanceID)
			{
				AuthorityAbilityState.AbilityState.SnapshotHistory.Add(State);
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

void USimpleGameplayAbilityComponent::ChangeAbilityStatus(FGuid AbilityInstanceID, EAbilityStatus NewStatus)
{
	if (HasAuthority())
	{
		for (FSimpleAbilityStateItem& AuthorityAbilityState : AuthorityAbilityStates.AbilityStates)
		{
			if (AuthorityAbilityState.AbilityState.AbilityID == AbilityInstanceID)
			{
				AuthorityAbilityState.AbilityState.AbilityStatus = NewStatus;
				AuthorityAbilityStates.MarkItemDirty(AuthorityAbilityState);
				return;
			}
		}

		SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::ChangeAbilityStatus]: Ability with ID %s not found in AuthorityAbilityStates array"), *AbilityInstanceID.ToString()));
	}
	else
	{
		for (FAbilityState& ActiveAbility : LocalAbilityStates)
		{
			if (ActiveAbility.AbilityID == AbilityInstanceID)
			{
				ActiveAbility.AbilityStatus = NewStatus;
				return;
			}
		}

		SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::ChangeAbilityStatus]: Ability with ID %s not found in LocalAbilityStates array"), *AbilityInstanceID.ToString()));
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

bool USimpleGameplayAbilityComponent::ApplyAttributeModifierToTarget(USimpleGameplayAbilityComponent* ModifierTarget,
	TSubclassOf<USimpleAttributeModifier> ModifierClass, FInstancedStruct ModifierContext)
{
	if (!ModifierClass)
	{
		SIMPLE_LOG(this, TEXT("[USimpleGameplayAbilityComponent::ApplyAttributeModifierToTarget]: ModifierClass is null!"));
		return false;
	}
	
	const FGuid ModifierInstanceID = FGuid::NewGuid();
	USimpleAttributeModifier* Modifier = nullptr;

	for (USimpleAttributeModifier* InstancedModifier : InstancedAttributes)
	{
		if (InstancedModifier->GetClass() == ModifierClass)
		{
			if (InstancedModifier->ModifierType == EAttributeModifierType::Duration && InstancedModifier->IsModifierActive())
			{
				if (InstancedModifier->CanStack)
				{
					InstancedModifier->AddModifierStack(1);
					return true;
				}

				InstancedModifier->EndModifier(FDefaultTags::AbilityCancelled, FInstancedStruct());
			}

			Modifier = InstancedModifier;
			break;
		}
	}

	if (!Modifier)
	{
		Modifier = NewObject<USimpleAttributeModifier>(this, ModifierClass);
		InstancedAttributes.Add(Modifier);
	}
	
	Modifier->InitializeAbility(this, ModifierInstanceID);
	AddNewAttributeState(ModifierClass, ModifierContext, ModifierInstanceID);
	
	return Modifier->ApplyModifier(this, ModifierTarget, ModifierContext);
}

bool USimpleGameplayAbilityComponent::ApplyAttributeModifierToSelf(TSubclassOf<USimpleAttributeModifier> ModifierClass,
	FInstancedStruct ModifierContext)
{
	return ApplyAttributeModifierToTarget(this, ModifierClass, ModifierContext);
}

void USimpleGameplayAbilityComponent::AddNewAttributeState(const TSubclassOf<USimpleAttributeModifier>& AttributeClass,
	const FInstancedStruct& AttributeContext, FGuid AttributeInstanceID)
{
	FAbilityState NewAttributeState;
	
	NewAttributeState.AbilityID = AttributeInstanceID;
	NewAttributeState.AbilityClass = AttributeClass;
	NewAttributeState.ActivationTimeStamp = GetServerTime();
	NewAttributeState.ActivationContext = AttributeContext;
	NewAttributeState.AbilityStatus = EAbilityStatus::ActivationSuccess;
	
	if (HasAuthority())
	{
		for (FSimpleAbilityStateItem& AuthorityAttributeState : AuthorityAttributeStates.AbilityStates)
		{
			if (AuthorityAttributeState.AbilityState.AbilityID == AttributeInstanceID)
			{
				SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::AddNewAttributeState]: Attribute with ID %s already exists in AuthorityAttributeStates array."), *AttributeInstanceID.ToString()));
				return;
			}
		}
		
		FSimpleAbilityStateItem NewAttributeStateItem;
		NewAttributeStateItem.AbilityState = NewAttributeState;

		AuthorityAttributeStates.AbilityStates.Add(NewAttributeStateItem);
		AuthorityAttributeStates.MarkArrayDirty();
	}
	else
	{
		for (FAbilityState& AbilityState : LocalAbilityStates)
		{
			if (AbilityState.AbilityID == AttributeInstanceID)
			{
				SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::AddNewAttributeState]: Attribute with ID %s already exists in LocalAttributeStates array."), *AttributeInstanceID.ToString()));
				return;
			}
		}
		
		LocalAttributeStates.Add(NewAttributeState);
	}
}

/* Tag Functions */

void USimpleGameplayAbilityComponent::AddGameplayTag(FGameplayTag Tag, FInstancedStruct Payload)
{
	GameplayTags.AddTag(Tag);
	SendEvent(FDefaultTags::GameplayTagAdded, Tag, Payload, GetOwner(), ESimpleEventReplicationPolicy::ServerAndOwningClientPredicted);
}

void USimpleGameplayAbilityComponent::RemoveGameplayTag(FGameplayTag Tag, FInstancedStruct Payload)
{
	GameplayTags.RemoveTag(Tag);
	SendEvent(FDefaultTags::GameplayTagRemoved, Tag, Payload, GetOwner(), ESimpleEventReplicationPolicy::ServerAndOwningClientPredicted);
}

/* Event Functions */

void USimpleGameplayAbilityComponent::SendEvent(FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload,
	AActor* Sender, ESimpleEventReplicationPolicy ReplicationPolicy)
{
	const FGuid EventID = FGuid::NewGuid();

	switch (ReplicationPolicy)
	{
		case ESimpleEventReplicationPolicy::NoReplication:
			SendEventInternal(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy);
			return;
			
		case ESimpleEventReplicationPolicy::ServerAndOwningClient:
			if (!HasAuthority() && GetOwner()->HasLocalNetOwner())
			{
				ServerSendEvent(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy);
				return;
			}
		
			SendEventInternal(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy);
			ClientSendEvent(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy);
			break;

		case ESimpleEventReplicationPolicy::ServerAndOwningClientPredicted:
			SendEventInternal(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy);
			if (!HasAuthority() && GetOwner()->HasLocalNetOwner())
			{
				ServerSendEvent(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy);
			}
			break;
		
		case ESimpleEventReplicationPolicy::AllConnectedClients:
			if (!HasAuthority() && GetOwner()->HasLocalNetOwner())
			{
				ServerSendEvent(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy);
				return;
			}

			MulticastSendEvent(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy);
			break;

		case ESimpleEventReplicationPolicy::AllConnectedClientsPredicted:
			SendEventInternal(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy);
			if (!HasAuthority() && GetOwner()->HasLocalNetOwner())
			{
				ServerSendEvent(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy);
			}
			break;
	}
}

void USimpleGameplayAbilityComponent::SendEventInternal(FGuid EventID, FGameplayTag EventTag, FGameplayTag DomainTag,
	const FInstancedStruct& Payload, AActor* Sender, ESimpleEventReplicationPolicy ReplicationPolicy)
{
	USimpleEventSubsystem* EventSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<USimpleEventSubsystem>();
	
	if (!EventSubsystem)
	{
		SIMPLE_LOG(this, TEXT("[USimpleGameplayAbilityComponent::SendEventInternal]: No SimpleEventSubsystem found."));
		return;
	}
	
	if (HandledEventIDs.Contains(EventID))
	{
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
	FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload, AActor*Sender, ESimpleEventReplicationPolicy ReplicationPolicy)
{
	SendEventInternal(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy);
}

/* Utility Functions */

void USimpleGameplayAbilityComponent::RemoveInstancedAbility(USimpleGameplayAbility* AbilityToRemove)
{
	InstancedAbilities.Remove(AbilityToRemove);
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

FAbilityState USimpleGameplayAbilityComponent::GetAbilityState(const FGuid AbilityInstanceID, bool& WasFound) const
{
	if (HasAuthority())
	{
		for (const FSimpleAbilityStateItem& AbilityStateItem : AuthorityAbilityStates.AbilityStates)
		{
			if (AbilityStateItem.AbilityState.AbilityID == AbilityInstanceID)
			{
				WasFound = true;
				return AbilityStateItem.AbilityState;
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

/* Replication */

void USimpleGameplayAbilityComponent::OnStateAdded(const FAbilityState& NewAbilityState)
{
	// A mapping of the local ability states for quick lookups
	TMap<FGuid, int32> LocalStateArrayIndexMap;

	// Added a new ability state
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
			UClass* ParentClassPtr = NewAbilityState.AbilityClass.Get();
			const TSubclassOf<USimpleGameplayAbility> AbilityClass = Cast<UClass>(ParentClassPtr);

			LocalAbilityStates.Add(NewAbilityState);
			ActivateAbilityInternal(AbilityClass, NewAbilityState.ActivationContext, NewAbilityState.AbilityID);

			SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::OnStateAdded]: New ability state with ID %s created locally."), *NewAbilityState.AbilityID.ToString()));
		}
	}

	// Added a new attribute state
	if (NewAbilityState.AbilityClass->IsChildOf(USimpleAttributeModifier::StaticClass()))
	{
		for (int32 i = 0; i < LocalAttributeStates.Num(); i++)
		{
			LocalStateArrayIndexMap.Add(LocalAttributeStates[i].AbilityID, i);
		}
	
		// If the NewAbilityState doesn't exist locally, we activate it
		if (!LocalStateArrayIndexMap.Contains(NewAbilityState.AbilityID))
		{
			UClass* ParentClassPtr = NewAbilityState.AbilityClass.Get();
			const TSubclassOf<USimpleAttributeModifier> AttributeClass = Cast<UClass>(ParentClassPtr);

			LocalAttributeStates.Add(NewAbilityState);
			ApplyAttributeModifierToSelf(AttributeClass, NewAbilityState.ActivationContext);

			SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::OnStateAdded]: New attribute state with ID %s has an invalid class upon replication."), *NewAbilityState.AbilityID.ToString()));
		}
	}
}

void USimpleGameplayAbilityComponent::OnStateChanged(const FAbilityState& ChangedAbilityState)
{
	if (!ChangedAbilityState.AbilityClass)
	{
		SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::OnStateChanged]: Changed ability state with ID %s has an invalid class upon replication."), *ChangedAbilityState.AbilityID.ToString()));
		return;
	}

	// Changed an ability state
	if (ChangedAbilityState.AbilityClass->IsChildOf(USimpleGameplayAbility::StaticClass()))
	{
		// Get a reference to the local version of the changed ability on the server
		FAbilityState* LocalAbilityState = LocalAbilityStates.FindByPredicate([ChangedAbilityState](const FAbilityState& AbilityState)
		{
			return AbilityState.AbilityID == ChangedAbilityState.AbilityID;
		});

		// If the ability doesn't exist locally, we don't need to do anything
		if (!LocalAbilityState)
		{
			SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::OnStateChanged]: Ability with ID %s not found in LocalAbilityStates array"), *ChangedAbilityState.AbilityID.ToString()));
			return;
		}

		// Check if the status of the ability has changed
		//if (ChangedAbilityState.AbilityStatus != LocalAbilityState->AbilityStatus)
		//{
			SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::OnStateChanged]: Ability with ID %s has has status %s locally and %s on the server."),
				*ChangedAbilityState.AbilityID.ToString(),
				*UEnum::GetValueAsString(LocalAbilityState->AbilityStatus),
				*UEnum::GetValueAsString(ChangedAbilityState.AbilityStatus)));
		//}

		// Check if the latest SnapshotHistory of the ability has changed
		if (ChangedAbilityState.SnapshotHistory.Num() > 0)
		{
			const FSimpleAbilitySnapshot& LatestAuthoritySnapshot = ChangedAbilityState.SnapshotHistory.Last();

			for (int32 i = LocalAbilityState->SnapshotHistory.Num(); i >= 0; i--)
			{
				if (LocalAbilityState->SnapshotHistory[i].StateTag == LatestAuthoritySnapshot.StateTag)
				{
					if (LocalAbilityState->SnapshotHistory[i].WasClientSnapshotResolved)
					{
						break;
					}

					if (USimpleGameplayAbility* AbilityInstance = GetAbilityInstance(ChangedAbilityState.AbilityID))
					{
						AbilityInstance->ClientResolvePastState(LatestAuthoritySnapshot.StateTag, LatestAuthoritySnapshot, LocalAbilityState->SnapshotHistory[i]);
						LocalAbilityState->SnapshotHistory[i].WasClientSnapshotResolved = true;
						break;	
					}
				}
			}
		}
	}

	// Changed an attribute state
	if (ChangedAbilityState.AbilityClass->IsChildOf(USimpleAttributeModifier::StaticClass()))
	{
		
	}
}

void USimpleGameplayAbilityComponent::OnStateRemoved(const FAbilityState& RemovedAbilityState)
{
	LocalAbilityStates.RemoveAll([RemovedAbilityState](const FAbilityState& AbilityState) { return AbilityState.AbilityID == RemovedAbilityState.AbilityID; });
}

void USimpleGameplayAbilityComponent::OnRep_FloatAttributes(TArray<FFloatAttribute>& OldFloatAttributes)
{
	// Check if any new attributes were added
	FGameplayTagContainer OldAttributeTags;
	FGameplayTagContainer NewAttributeTags;
	
	for (int i = 0; i < OldFloatAttributes.Num(); i++)
	{
		OldAttributeTags.AddTag(OldFloatAttributes[i].AttributeTag);
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
	for (int i = 0; i < OldFloatAttributes.Num(); i++)
	{
		if (!NewAttributeTags.HasTagExact(OldFloatAttributes[i].AttributeTag))
		{
			USimpleAttributeFunctionLibrary::SendFloatAttributeChangedEvent(this, FDefaultTags::AttributeRemoved, OldFloatAttributes[i].AttributeTag, EAttributeValueType::BaseValue, 0.0f);
		}
		else
		{
			const int32 NewAttributeIndex = USimpleAttributeFunctionLibrary::GetFloatAttributeIndex(this, OldFloatAttributes[i].AttributeTag);

			if (NewAttributeIndex < 0)
			{
				SIMPLE_LOG(this, FString::Printf(TEXT("USimpleGameplayAttributes::OnRep_FloatAttributes: Attribute %s not found."), *OldFloatAttributes[i].AttributeTag.ToString()));
				continue;
			}

			USimpleAttributeFunctionLibrary::CompareFloatAttributesAndSendEvents(this, OldFloatAttributes[i], FloatAttributes[NewAttributeIndex]);
		}
	}
}

void USimpleGameplayAbilityComponent::OnRep_StructAttributes(TArray<FStructAttribute>& OldStructAttributes)
{
}

void USimpleGameplayAbilityComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USimpleGameplayAbilityComponent, AvatarActor);
	DOREPLIFETIME(USimpleGameplayAbilityComponent, GameplayTags);
	DOREPLIFETIME(USimpleGameplayAbilityComponent, GrantedAbilities);
	DOREPLIFETIME(USimpleGameplayAbilityComponent, FloatAttributes);
	DOREPLIFETIME(USimpleGameplayAbilityComponent, StructAttributes);
	DOREPLIFETIME(USimpleGameplayAbilityComponent, AuthorityAbilityStates);
	DOREPLIFETIME(USimpleGameplayAbilityComponent, AuthorityAttributeStates);
}


