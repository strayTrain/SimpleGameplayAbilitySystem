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
#include "SimpleGameplayAbilitySystem/DataAssets/AbilityOverrideSet/AbilityOverrideSet.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleAttributeModifier/SimpleAttributeModifier.h"
#include "StructAttributeHandler/SimpleStructAttributeHandler.h"
#include "StructUtils/InstancedStruct.h"
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

	LocalFloatAttributes = AuthorityFloatAttributes.Attributes;
	LocalStructAttributes = AuthorityStructAttributes.Attributes;
}

/* Ability Functions */

bool USimpleGameplayAbilityComponent::ActivateAbility(
	const TSubclassOf<USimpleGameplayAbility> AbilityClass,
	const FInstancedStruct AbilityContext,
	const bool OverrideActivationPolicy, const EAbilityActivationPolicy ActivationPolicyOverride)
{
	const FGuid NewAbilityID = FGuid::NewGuid();
	const EAbilityActivationPolicy ActivationPolicy = OverrideActivationPolicy ? ActivationPolicyOverride : AbilityClass.GetDefaultObject()->ActivationPolicy;
	
	const bool IsClient = GetNetMode() == NM_Client;
	const float ActivationTime = GetServerTime();

	switch (ActivationPolicy)
	{
		case EAbilityActivationPolicy::LocalOnly:
			return ActivateAbilityInternal(NewAbilityID, AbilityClass, AbilityContext, ActivationPolicy, false, ActivationTime);
		
		case EAbilityActivationPolicy::ClientOnly:
			if (!IsClient)
			{
				SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::ActivateAbility]: Can't activate ability %s on server with ClientOnly policy."), *AbilityClass->GetName()));
			}
		
			return ActivateAbilityInternal(NewAbilityID, AbilityClass, AbilityContext, ActivationPolicy, false, ActivationTime);
		
		case EAbilityActivationPolicy::ServerInitiatedFromClient:
			if (IsClient)
			{
				ServerActivateAbility(NewAbilityID, AbilityClass, AbilityContext, ActivationPolicy, ActivationTime);
				return true;
			}

			return ActivateAbilityInternal(NewAbilityID, AbilityClass, AbilityContext, ActivationPolicy, true, ActivationTime);
		
		case EAbilityActivationPolicy::ClientPredicted:
			if (IsClient)
			{
				ServerActivateAbility(NewAbilityID, AbilityClass, AbilityContext, ActivationPolicy, ActivationTime);
			}
		
			return ActivateAbilityInternal(NewAbilityID, AbilityClass, AbilityContext, ActivationPolicy, true, ActivationTime);

		case EAbilityActivationPolicy::ServerAuthority:
			if (IsClient)
			{
				SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::ActivateAbility]: Can't activate ability %s on client with ServerAuthority policy."), *AbilityClass->GetName()));
				return false;
			}
		
			return ActivateAbilityInternal(NewAbilityID, AbilityClass, AbilityContext, ActivationPolicy, true, ActivationTime);

		case EAbilityActivationPolicy::ServerOnly:
			if (IsClient)
			{
				SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::ActivateAbility]: Can't activate ability %s on client with ServerOnly policy."), *AbilityClass->GetName()));
				return false;
			}
		
			return ActivateAbilityInternal(NewAbilityID, AbilityClass, AbilityContext, ActivationPolicy, false, ActivationTime);
	}
	
	return false;
}

bool USimpleGameplayAbilityComponent::ActivateAbilityWithID(
	const FGuid AbilityID,
	TSubclassOf<USimpleGameplayAbility> AbilityClass,
	const FInstancedStruct& AbilityContext,
	bool TrackState,
	bool OverrideActivationPolicy,
	EAbilityActivationPolicy ActivationPolicyOverride)
{
	EAbilityActivationPolicy ActivationPolicy = OverrideActivationPolicy ? ActivationPolicyOverride : AbilityClass.GetDefaultObject()->ActivationPolicy;
	return ActivateAbilityInternal(AbilityID, AbilityClass, AbilityContext, ActivationPolicy, TrackState, GetServerTime());
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

	// For single player or local-only games, skip state tracking and replication if not needed
	if (TrackState)
	{
		CreateAbilityState(AbilityID, ActivationPolicy, AbilityClass, AbilityContext, HasAuthority(), ActivationTime);
	}
	
	USimpleGameplayAbility* AbilityInstance = GetAbilityInstance(AbilityClass);
	
	if (AbilityInstance->InstancingPolicy == EAbilityInstancingPolicy::SingleInstance)
	{
		if (AbilityInstance->IsAbilityActive() && !AbilityInstance->CanCancel())
		{
			SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::ActivateAbilityWithID]: Failed to activate ability %s because it is already running and cannot be cancelled."), *AbilityClass->GetName()));
			return false;
		}
	}
	
	AbilityInstance->InitializeAbility(this, AbilityID, false);
	return AbilityInstance->ActivateAbility(AbilityID, AbilityContext);
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
		
		AbilityInstance->CancelAbility(FDefaultTags::AbilityCancelled, CancellationContext);
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
			
			AbilityInstance->CancelAbility(FDefaultTags::AbilityCancelled, CancellationContext);
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

void USimpleGameplayAbilityComponent::ChangeAbilityStatus(FGuid AbilityInstanceID, EAbilityStatus NewStatus)
{
	if (HasAuthority())
	{
		for (FAbilityState& AuthorityAbilityState : AuthorityAbilityStates.AbilityStates)
		{
			if (AuthorityAbilityState.AbilityID == AbilityInstanceID)
			{
				AuthorityAbilityState.AbilityStatus = NewStatus;
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

void USimpleGameplayAbilityComponent::SetAbilityStateEndingContext(FGuid AbilityInstanceID, FGameplayTag EndTag, FInstancedStruct EndContext)
{
	FSimpleAbilityEndedEvent EndEvent;
	EndEvent.AbilityID = AbilityInstanceID;
	EndEvent.EndStatus = EndTag;
	EndEvent.EndingContext = EndContext;
	
	if (HasAuthority())
	{
		for (FAbilityState& AuthorityAbilityState : AuthorityAbilityStates.AbilityStates)
		{
			if (AuthorityAbilityState.AbilityID == AbilityInstanceID)
			{
				AuthorityAbilityState.EndingContext = EndContext;
				AuthorityAbilityStates.MarkItemDirty(AuthorityAbilityState);
				SendEvent(FDefaultTags::AbilityEnded, EndTag, FInstancedStruct::Make(EndEvent), GetAvatarActor(), {}, ESimpleEventReplicationPolicy::NoReplication);

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
				ActiveAbility.EndingContext = EndContext;
				SendEvent(FDefaultTags::AbilityEnded, EndTag, FInstancedStruct::Make(EndEvent), GetAvatarActor(), {}, ESimpleEventReplicationPolicy::NoReplication);

				return;
			}
		}

		SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::ChangeAbilityStatus]: Ability with ID %s not found in LocalAbilityStates array"), *AbilityInstanceID.ToString()));
	}
}

/* Attribute Functions */

void USimpleGameplayAbilityComponent::AddFloatAttribute(FFloatAttribute AttributeToAdd, bool OverrideValuesIfExists)
{
	for (FFloatAttribute& AuthorityAttribute : AuthorityFloatAttributes.Attributes)
	{
		if (AuthorityAttribute.AttributeTag.MatchesTagExact(AttributeToAdd.AttributeTag))
		{
			// Attribute exists but we don't want to override it
			if (!OverrideValuesIfExists)
			{
				return;
			}

			// Attribute exists and we want to override it
			AuthorityAttribute = AttributeToAdd;
			AuthorityFloatAttributes.MarkItemDirty(AuthorityAttribute);
			return;
		}
	}
	
	AuthorityFloatAttributes.Attributes.Add(AttributeToAdd);
	AuthorityFloatAttributes.MarkArrayDirty();
}

void USimpleGameplayAbilityComponent::RemoveFloatAttribute(FGameplayTag AttributeTag)
{
	AuthorityFloatAttributes.Attributes.RemoveAll([AttributeTag](const FFloatAttribute& Attribute) { return Attribute.AttributeTag == AttributeTag; });
	AuthorityFloatAttributes.MarkArrayDirty();
}

void USimpleGameplayAbilityComponent::AddStructAttribute(FStructAttribute AttributeToAdd, bool OverrideValuesIfExists)
{
	if (!AttributeToAdd.AttributeHandler)
	{
		SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::AddStructAttribute]: AttributeHandler is null for attribute %s! Can't add new attribute"), *AttributeToAdd.AttributeTag.ToString()));
		return;
	}
	
	const int32 AttributeIndex = AuthorityStructAttributes.Attributes.Find(AttributeToAdd);
	USimpleStructAttributeHandler* AttributeHandler = GetStructAttributeHandler(AttributeToAdd.AttributeHandler);

	if (!AttributeHandler->StructType)
	{
		SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::AddStructAttribute]: StructType is null for attribute %s! Can't add new attribute"), *AttributeToAdd.AttributeTag.ToString()));
		return;
	}
	
	// This is a new attribute
	if (AttributeIndex == INDEX_NONE)
	{
		AuthorityStructAttributes.Attributes.AddUnique(AttributeToAdd);
		AuthorityStructAttributes.MarkArrayDirty();
		
		return;
	}

	// Attribute exists but we don't want to override it
	if (!OverrideValuesIfExists)
	{
		return;
	}

	// Attribute exists and we want to override it
	AuthorityStructAttributes.Attributes[AttributeIndex] = AttributeToAdd;
	AuthorityStructAttributes.MarkItemDirty(AuthorityStructAttributes.Attributes[AttributeIndex]);
}

void USimpleGameplayAbilityComponent::RemoveStructAttribute(FGameplayTag AttributeTag)
{
	AuthorityStructAttributes.Attributes.RemoveAll([AttributeTag](const FStructAttribute& Attribute) { return Attribute.AttributeTag == AttributeTag; });
	AuthorityStructAttributes.MarkArrayDirty();
}

bool USimpleGameplayAbilityComponent::ApplyAttributeModifierToTarget(USimpleGameplayAbilityComponent* ModifierTarget,
	TSubclassOf<USimpleAttributeModifier> ModifierClass, FInstancedStruct ModifierContext, FGuid& ModifierID)
{
	if (!ModifierClass)
	{
		SIMPLE_LOG(this, TEXT("[USimpleGameplayAbilityComponent::ApplyAttributeModifierToTarget]: ModifierClass is null!"));
		return false;
	}
	
	const FGuid ModifierInstanceID = FGuid::NewGuid();
	USimpleAttributeModifier* Modifier = nullptr;
	ModifierID = ModifierInstanceID;

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
	
	Modifier->InitializeAbility(this, ModifierInstanceID, false);
	CreateAttributeState(ModifierClass, ModifierContext, ModifierInstanceID);
	
	return Modifier->ApplyModifier(this, ModifierTarget, ModifierContext);
}

bool USimpleGameplayAbilityComponent::ApplyAttributeModifierToSelf(TSubclassOf<USimpleAttributeModifier> ModifierClass,
	FInstancedStruct ModifierContext, FGuid& ModifierID)
{
	return ApplyAttributeModifierToTarget(this, ModifierClass, ModifierContext, ModifierID);
}

void USimpleGameplayAbilityComponent::AddAttributeStateSnapshot(FGuid AbilityInstanceID, FSimpleAbilitySnapshot State)
{
	if (HasAuthority())
	{
		for (FAbilityState& AuthorityAttributeState : AuthorityAttributeStates.AbilityStates)
		{
			if (AuthorityAttributeState.AbilityID == AbilityInstanceID)
			{
				AuthorityAttributeState.SnapshotHistory.Add(State);
				AuthorityAttributeStates.MarkItemDirty(AuthorityAttributeState);
				return;
			}
		}
	}
	else
	{
		for (FAbilityState& ActiveAttribute : LocalAttributeStates)
		{
			if (ActiveAttribute.AbilityID == AbilityInstanceID)
			{
				ActiveAttribute.SnapshotHistory.Add(State);
				return;
			}
		}
	}

	SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::AddAttributeStateSnapshot]: Attribute with ID %s not found in InstancedAttributes array"), *AbilityInstanceID.ToString()));
}

void USimpleGameplayAbilityComponent::CancelAttributeModifier(FGuid ModifierID)
{
	// If this is an active duration modifier, we end it
	if (USimpleAttributeModifier* ModifierInstance = GetAttributeModifierInstance(ModifierID))
	{
		if (ModifierInstance->ModifierType == EAttributeModifierType::Duration && ModifierInstance->IsModifierActive())
		{
			ModifierInstance->EndModifier(FDefaultTags::AbilityCancelled, FInstancedStruct());
			return;
		}
	}

	// If it's not an active duration modifier we go through all activated ability states and cancel any active ability side effects
	TArray<FSimpleAbilitySnapshot>* Snapshots = GetLocalAttributeStateSnapshots(ModifierID);

	if (Snapshots)
	{
		for (FSimpleAbilitySnapshot& Snapshot : *Snapshots)
		{
			// Cancel any active abilities that were activated by this modifier
			if (const FAttributeModifierResult* ModifierResult = Snapshot.StateData.GetPtr<FAttributeModifierResult>())
			{
				for (const FAbilitySideEffect& AbilitySideEffect : ModifierResult->AppliedAbilitySideEffects)
				{
					if (USimpleGameplayAbility* AbilityInstance = GetGameplayAbilityInstance(AbilitySideEffect.AbilityInstanceID))
					{
						AbilityInstance->CancelAbility(FDefaultTags::AbilityCancelled, FInstancedStruct());
					}
				}
			}

			// Cancel anu duration modifiers that were activated by this modifier
			if (const FAttributeModifierResult* ModifierResult = Snapshot.StateData.GetPtr<FAttributeModifierResult>())
			{
				for (const FAttributeModifierSideEffect& AttributeModifier : ModifierResult->AppliedAttributeModifierSideEffects)
				{
					if (USimpleAttributeModifier* AttributeModifierInstance = GetAttributeModifierInstance(AttributeModifier.AttributeID))
					{
						AttributeModifierInstance->EndModifier(FDefaultTags::AbilityCancelled, FInstancedStruct());
					}
				}
			}
		}
	}
}

void USimpleGameplayAbilityComponent::CancelAttributeModifiersWithTags(FGameplayTagContainer Tags)
{
	// We go through all active modifiers and cancel them if any of their tags match the provided tags
	for (USimpleAttributeModifier* ModifierInstance : InstancedAttributes)
	{
		if (ModifierInstance->IsModifierActive() && ModifierInstance->ModifierTags.HasAnyExact(Tags))
		{
			CancelAttributeModifier(ModifierInstance->AbilityInstanceID);
		}
	}
}

const USimpleStructAttributeHandler* USimpleGameplayAbilityComponent::GetStructAttributeHandlerAs(FGameplayTag AttributeTag,
	TSubclassOf<USimpleStructAttributeHandler> HandlerClass, bool& WasFound)
{
	const FStructAttribute* Attribute = nullptr;

	if (HasAuthority())
	{
		for (const FStructAttribute& AuthorityAttribute : AuthorityStructAttributes.Attributes)
		{
			if (AuthorityAttribute.AttributeTag.MatchesTagExact(AttributeTag))
			{
				Attribute = &AuthorityAttribute;
				break;
			}
		}
	}
	else
	{
		for (const FStructAttribute& LocalAttribute : LocalStructAttributes)
		{
			if (LocalAttribute.AttributeTag.MatchesTagExact(AttributeTag))
			{
				Attribute = &LocalAttribute;
				break;
			}
		}
	}

	if (Attribute && Attribute->AttributeHandler)
	{
		WasFound = true;
		return GetStructAttributeHandler(HandlerClass);
	}

	WasFound = false;
	return nullptr;
}

void USimpleGameplayAbilityComponent::CreateAttributeState(const TSubclassOf<USimpleAttributeModifier>& AttributeClass,
                                                           const FInstancedStruct& AttributeContext, FGuid AttributeInstanceID)
{
	FAbilityState NewAttributeState;
	
	NewAttributeState.AbilityID = AttributeInstanceID;
	NewAttributeState.AbilityClass = AttributeClass;
	NewAttributeState.ActivationTimeStamp = GetServerTime();
	NewAttributeState.ActivationContext = AttributeContext;
	NewAttributeState.AbilityStatus = ActivationSuccess;
	
	if (HasAuthority())
	{
		for (FAbilityState& AuthorityAttributeState : AuthorityAttributeStates.AbilityStates)
		{
			if (AuthorityAttributeState.AbilityID == AttributeInstanceID)
			{
				SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::CreateAttributeState]: Attribute with ID %s already exists in AuthorityAttributeStates array."), *AttributeInstanceID.ToString()));
				return;
			}
		}
		
		FAbilityState NewAttributeStateItem;
		NewAttributeStateItem = NewAttributeState;

		AuthorityAttributeStates.AbilityStates.Add(NewAttributeStateItem);
		AuthorityAttributeStates.MarkArrayDirty();
	}
	else
	{
		for (FAbilityState& AbilityState : LocalAbilityStates)
		{
			if (AbilityState.AbilityID == AttributeInstanceID)
			{
				SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::CreateAttributeState]: Attribute with ID %s already exists in LocalAttributeStates array."), *AttributeInstanceID.ToString()));
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
	SendEvent(FDefaultTags::GameplayTagAdded, Tag, Payload, GetOwner(), {}, ESimpleEventReplicationPolicy::ServerAndOwningClientPredicted);
}

void USimpleGameplayAbilityComponent::RemoveGameplayTag(FGameplayTag Tag, FInstancedStruct Payload)
{
	GameplayTags.RemoveTag(Tag);
	SendEvent(FDefaultTags::GameplayTagRemoved, Tag, Payload, GetOwner(), {}, ESimpleEventReplicationPolicy::ServerAndOwningClientPredicted);
}

/* Event Functions */

void USimpleGameplayAbilityComponent::SendEvent(FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload,
                                                AActor* Sender, TArray<UObject*> ListenerFilter, ESimpleEventReplicationPolicy ReplicationPolicy)
{
	const FGuid EventID = FGuid::NewGuid();

	switch (ReplicationPolicy)
	{
		case ESimpleEventReplicationPolicy::NoReplication:
			SendEventInternal(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy, {});
			return;
			
		case ESimpleEventReplicationPolicy::ServerAndOwningClient:
			if (!HasAuthority() && GetOwner()->HasLocalNetOwner())
			{
				ServerSendEvent(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy);
				return;
			}
		
			SendEventInternal(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy, {});
			ClientSendEvent(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy);
			break;

		case ESimpleEventReplicationPolicy::ServerAndOwningClientPredicted:
			SendEventInternal(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy, {});

			if (HasAuthority())
			{
				ClientSendEvent(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy);
			}
		
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

			if (HasAuthority())
			{
				MulticastSendEvent(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy);
				break;
			}

			SendEventInternal(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy, {});
		
			if (!HasAuthority() && GetOwner()->HasLocalNetOwner())
			{
				ServerSendEvent(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy);
			}
			break;
	}
}

void USimpleGameplayAbilityComponent::SendEventInternal(FGuid EventID, FGameplayTag EventTag, FGameplayTag DomainTag,
                                                        const FInstancedStruct& Payload, AActor* Sender, ESimpleEventReplicationPolicy ReplicationPolicy, TArray<UObject*>
                                                        ListenerFilter)
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
	
	EventSubsystem->SendEvent(EventTag, DomainTag, Payload, Sender, {});

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
			SendEventInternal(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy, {});
			ClientSendEvent(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy);
			break;
		
		case ESimpleEventReplicationPolicy::ServerAndOwningClientPredicted:
			SendEventInternal(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy, {});
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
	SendEventInternal(EventID, EventTag, DomainTag, Payload, Sender, ReplicationPolicy, {});
}

void USimpleGameplayAbilityComponent::MulticastSendEvent_Implementation(FGuid EventID,
	FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload, AActor*Sender, ESimpleEventReplicationPolicy ReplicationPolicy)
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

FAbilityState USimpleGameplayAbilityComponent::FindAbilityState(const FGuid AbilityInstanceID, bool& WasFound) const
{
	if (HasAuthority())
	{
		for (const FAbilityState& AbilityStateItem : AuthorityAbilityStates.AbilityStates)
		{
			if (AbilityStateItem.AbilityID == AbilityInstanceID)
			{
				WasFound = true;
				return AbilityStateItem;
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

USimpleStructAttributeHandler* USimpleGameplayAbilityComponent::GetStructAttributeHandler(const TSubclassOf<USimpleStructAttributeHandler>& HandlerClass)
{
	for (USimpleStructAttributeHandler* Handler : StructAttributeHandlers)
	{
		if (Handler->GetClass() == HandlerClass)
		{
			return Handler;
		}
	}

	USimpleStructAttributeHandler* NewHandler = NewObject<USimpleStructAttributeHandler>(this, HandlerClass);
	NewHandler->SetOwningAbilityComponent(this);
	StructAttributeHandlers.Add(NewHandler);
	return NewHandler;
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
	SendEvent(FDefaultTags::FloatAttributeAdded, NewFloatAttribute.AttributeTag, FInstancedStruct(), GetOwner(), {}, ESimpleEventReplicationPolicy::NoReplication);
}

void USimpleGameplayAbilityComponent::OnFloatAttributeChanged(const FFloatAttribute& ChangedFloatAttribute)
{
	for (FFloatAttribute& LocalFloatAttribute : LocalFloatAttributes)
	{
		if (LocalFloatAttribute.AttributeTag.MatchesTagExact(ChangedFloatAttribute.AttributeTag))
		{
			USimpleAttributeFunctionLibrary::CompareFloatAttributesAndSendEvents(this, LocalFloatAttribute, ChangedFloatAttribute);
			LocalFloatAttribute = ChangedFloatAttribute;
			return;
		}
	}

	LocalFloatAttributes.AddUnique(ChangedFloatAttribute);
	SendEvent(FDefaultTags::FloatAttributeAdded, ChangedFloatAttribute.AttributeTag, FInstancedStruct(), GetOwner(), {}, ESimpleEventReplicationPolicy::NoReplication);
}

void USimpleGameplayAbilityComponent::OnFloatAttributeRemoved(const FFloatAttribute& RemovedFloatAttribute)
{
	LocalFloatAttributes.Remove(RemovedFloatAttribute);
	SendEvent(FDefaultTags::FloatAttributeRemoved, RemovedFloatAttribute.AttributeTag, FInstancedStruct(), GetOwner(), {}, ESimpleEventReplicationPolicy::NoReplication);
}

void USimpleGameplayAbilityComponent::OnStructAttributeAdded(const FStructAttribute& NewStructAttribute)
{
	if (!LocalStructAttributes.Contains(NewStructAttribute))
	{
		LocalStructAttributes.Add(NewStructAttribute);
		SendEvent(FDefaultTags::StructAttributeAdded, NewStructAttribute.AttributeTag, NewStructAttribute.AttributeValue, GetOwner(), {}, ESimpleEventReplicationPolicy::NoReplication);
	}
}

void USimpleGameplayAbilityComponent::OnStructAttributeChanged(const FStructAttribute& ChangedStructAttribute)
{
	for (FStructAttribute& LocalStructAttribute : LocalStructAttributes)
	{
		if (LocalStructAttribute.AttributeTag.MatchesTagExact(ChangedStructAttribute.AttributeTag))
		{
			if (USimpleStructAttributeHandler* Handler = GetStructAttributeHandler(ChangedStructAttribute.AttributeHandler))
			{
				Handler->OnStructChanged(LocalStructAttribute.AttributeTag, LocalStructAttribute.AttributeValue, ChangedStructAttribute.AttributeValue);
			}
			
			LocalStructAttribute = ChangedStructAttribute;
			return;
		}
	}

	LocalStructAttributes.Add(ChangedStructAttribute);
	SendEvent(FDefaultTags::StructAttributeAdded, ChangedStructAttribute.AttributeTag, ChangedStructAttribute.AttributeValue, GetOwner(), {}, ESimpleEventReplicationPolicy::NoReplication);
}

void USimpleGameplayAbilityComponent::OnStructAttributeRemoved(const FStructAttribute& RemovedStructAttribute)
{
	LocalStructAttributes.Remove(RemovedStructAttribute);
	SendEvent(FDefaultTags::StructAttributeRemoved, RemovedStructAttribute.AttributeTag, FInstancedStruct(), GetOwner(), {}, ESimpleEventReplicationPolicy::NoReplication);
}

void USimpleGameplayAbilityComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USimpleGameplayAbilityComponent, AvatarActor);
	DOREPLIFETIME(USimpleGameplayAbilityComponent, GameplayTags);
	DOREPLIFETIME(USimpleGameplayAbilityComponent, GrantedAbilities);
	DOREPLIFETIME(USimpleGameplayAbilityComponent, ActiveAbilityOverrides);
	DOREPLIFETIME(USimpleGameplayAbilityComponent, AuthorityFloatAttributes);
	DOREPLIFETIME(USimpleGameplayAbilityComponent, AuthorityStructAttributes);
	DOREPLIFETIME(USimpleGameplayAbilityComponent, AuthorityAbilityStates);
	DOREPLIFETIME(USimpleGameplayAbilityComponent, AuthorityAttributeStates);
}


