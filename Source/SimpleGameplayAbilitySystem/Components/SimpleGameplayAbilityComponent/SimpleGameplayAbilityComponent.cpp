#include "SimpleGameplayAbilityComponent.h"

#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "SimpleAbility/SimpleGameplayAbility/SimpleGameplayAbility.h"
#include "SimpleGameplayAbilitySystem/DataAssets/AbilitySet/SimpleAbilitySet.h"
#include "SimpleGameplayAbilitySystem/DefaultTags/DefaultTags.h"
#include "SimpleGameplayAbilitySystem/Module/SimpleGameplayAbilitySystem.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleGameplayAbilityComponent/SimpleAbilityComponentTypes.h"
#include "SimpleGameplayAbilitySystem/SimpleEventSubsystem/SimpleEventSubSystem.h"

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
		
		// Grant abilities from ability sets
		for (USimpleAbilitySet* AbilitySet : AbilitySets)
		{
			for (const TSubclassOf<USimpleGameplayAbility> AbilityClass : AbilitySet->AbilitiesToGrant)
			{
				GrantAbility(AbilityClass);
			}
		}
		
		return;
	}

	// Delegates called on the client to handle replicated data
	AuthorityAbilityStates.OnStateAdded.BindUObject(this, &USimpleGameplayAbilityComponent::ClientOnAbilityStateAdded);
	AuthorityAbilityStates.OnStateChanged.BindUObject(this, &USimpleGameplayAbilityComponent::ClientOnAbilityStateChanged);
	AuthorityAbilityStates.OnStateRemoved.BindUObject(this, &USimpleGameplayAbilityComponent::ClientOnAbilityStateRemoved);
	AuthorityAbilitySnapshots.OnSnapshotAdded.BindUObject(this, &USimpleGameplayAbilityComponent::ClientOnAbilitySnapshotAdded);
	AuthorityAbilitySnapshots.OnSnapshotRemoved.BindUObject(this, &USimpleGameplayAbilityComponent::ClientOnAbilitySnapshotRemoved);
}

void USimpleGameplayAbilityComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Clean up abilities
	for (USimpleGameplayAbility* Ability : InstancedAbilities)
	{
		Ability->CancelAbility(FGameplayTag::EmptyTag, FInstancedStruct());
	}
    
	// Clear collections
	InstancedAbilities.Empty();
    
	// Unsubscribe from events
	if (USimpleEventSubsystem* EventSubsystem = GetWorld() ? GetWorld()->GetGameInstance()->GetSubsystem<USimpleEventSubsystem>() : nullptr)
	{
		EventSubsystem->StopListeningForAllEvents(this);
	}
	
	Super::EndPlay(EndPlayReason);
}

/* Ability Functions */

bool USimpleGameplayAbilityComponent::ActivateAbilityLocal(
	const TSubclassOf<USimpleGameplayAbility> AbilityClass,
	FInstancedStruct AbilityContext,
	FGuid& AbilityID)
{
	AbilityID = FGuid::NewGuid();
	return ActivateAbilityInternal(AbilityID, AbilityClass, AbilityContext, false, GetServerTime());
}

bool USimpleGameplayAbilityComponent::ActivateAbilityPredicted(TSubclassOf<USimpleGameplayAbility> AbilityClass,
	FInstancedStruct AbilityContext, FGuid& AbilityID)
{
	AbilityID = FGuid::NewGuid();
	const bool WasActivated = ActivateAbilityInternal(AbilityID, AbilityClass, AbilityContext, true, GetServerTime());

	if (WasActivated && !HasAuthority())
	{
		ServerActivateAbility(AbilityID, AbilityClass, AbilityContext, GetServerTime());
	}
	
	return WasActivated;
}

void USimpleGameplayAbilityComponent::ActivateAbilityServerInitiated(TSubclassOf<USimpleGameplayAbility> AbilityClass, FInstancedStruct AbilityContext, FGuid& AbilityID)
{
	AbilityID = FGuid::NewGuid();

	if (HasAuthority())
	{
		ActivateAbilityInternal(AbilityID, AbilityClass, AbilityContext, true, GetServerTime());
		return;
	}
	
	ServerActivateAbility(AbilityID, AbilityClass, AbilityContext, GetServerTime());
}

bool USimpleGameplayAbilityComponent::ActivateAbilityInternal(
	const FGuid AbilityID,
	const TSubclassOf<USimpleGameplayAbility>& AbilityClass,
	const FInstancedStruct& AbilityContext,
	const bool ShouldTrackState,
	const double ActivationTime)
{
	if (!AbilityClass)
	{
		SIMPLE_LOG(this, TEXT("[USimpleGameplayAbilityComponent::ActivateAbilityInternal]: AbilityClass is null!"));
		return false;
	}

	if (!GrantedAbilities.Contains(AbilityClass) && AbilityClass->GetDefaultObject<USimpleGameplayAbility>()->RequireGrantToActivate)
	{
		SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::ActivateAbility]: Ability %s is not granted!"), *AbilityClass->GetName()));	
		return false;
	}
	
	USimpleGameplayAbility* AbilityInstance = GetAbilityInstanceByClass(AbilityClass);
	
	if (AbilityInstance->InstancingPolicy == EAbilityInstancingPolicy::SingleInstance)
	{
		if (AbilityInstance->IsActive)
		{
			AbilityInstance->CancelAbility(FGameplayTag::EmptyTag, FInstancedStruct());
		}
	}
	
	if (ShouldTrackState)
	{
		FAbilityState NewState;
		NewState.AbilityID = AbilityID;
		NewState.AbilityClass = AbilityClass;
		NewState.ActivationTimeStamp = ActivationTime;
		NewState.ActivationContext = AbilityContext;
		
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

	AbilityInstance->Initialize(this, AbilityID);
	return AbilityInstance->ActivateAbility(AbilityContext);;
}

void USimpleGameplayAbilityComponent::ServerActivateAbility_Implementation(
	const FGuid AbilityID,
	TSubclassOf<USimpleGameplayAbility> AbilityClass,
	const FInstancedStruct& AbilityContexts,
	const float ActivationTime)
{
	if (!AbilityClass)
	{
		SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::ServerActivateAbility]: AbilityClass is null!")));
		return;
	}
	
	ActivateAbilityInternal(AbilityID, AbilityClass, AbilityContexts, true, ActivationTime);
}

USimpleGameplayAbility* USimpleGameplayAbilityComponent::GetAbilityInstanceByClass(TSubclassOf<USimpleGameplayAbility> AbilityClass)
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
	NewAbilityInstance->OnActivationSuccess.AddDynamic(this, &USimpleGameplayAbilityComponent::OnAbilityActivationSuccess);
	NewAbilityInstance->OnActivationFailed.AddDynamic(this, &USimpleGameplayAbilityComponent::OnAbilityActivationFailed);
	NewAbilityInstance->OnAbilityEnded.AddDynamic(this, &USimpleGameplayAbilityComponent::OnAbilityEnded);
	NewAbilityInstance->OnAbilityCancelled.AddDynamic(this, &USimpleGameplayAbilityComponent::OnAbilityCancelled);
	
	InstancedAbilities.Add(NewAbilityInstance);
	
	return NewAbilityInstance;	
}

void USimpleGameplayAbilityComponent::CancelAbility(const FGuid AbilityInstanceID, const FInstancedStruct CancellationContext)
{
	if (USimpleGameplayAbility* AbilityInstance = GetAbilityInstanceByID(AbilityInstanceID))
	{
		if (!AbilityInstance->IsActive)
		{
			return;
		}
		
		AbilityInstance->CancelAbility(FGameplayTag::EmptyTag, CancellationContext);
	}
}

void USimpleGameplayAbilityComponent::CancelAbilityPredicted(FGuid AbilityInstanceID, FInstancedStruct CancellationContext)
{
	if (HasAuthority())
	{
		CancelAbility(AbilityInstanceID, CancellationContext);	
		return;
	}
	
	CancelAbility(AbilityInstanceID, CancellationContext);
	ServerCancelAbility(AbilityInstanceID, CancellationContext);
}

void USimpleGameplayAbilityComponent::ServerCancelAbility_Implementation(FGuid AbilityInstanceID, FInstancedStruct CancellationContext)
{
	CancelAbility(AbilityInstanceID, CancellationContext);
}

TArray<FGuid> USimpleGameplayAbilityComponent::CancelAbilitiesWithTags(const FGameplayTagContainer Tags, FInstancedStruct CancellationContext)
{
	TArray<FGuid> CancelledAbilities;
	
	for (USimpleGameplayAbility* AbilityInstance : InstancedAbilities)
	{
		if (AbilityInstance->AbilityTags.HasAnyExact(Tags))
		{
			AbilityInstance->CancelAbility(FGameplayTag::EmptyTag, CancellationContext);
			CancelledAbilities.Add(AbilityInstance->AbilityID);
		}
	}
	
	return CancelledAbilities;
}

TArray<FGuid> USimpleGameplayAbilityComponent::CancelAbilitiesWithTagsPredicted(FGameplayTagContainer Tags, FInstancedStruct CancellationContext)
{
	if (HasAuthority())
	{
		return CancelAbilitiesWithTags(Tags, CancellationContext);	
	}
	
	TArray<FGuid> CancelledAbilities = CancelAbilitiesWithTags(Tags, CancellationContext);
	ServerCancelAbilitiesWithTags(Tags, CancellationContext);
	return CancelledAbilities;
}

void USimpleGameplayAbilityComponent::ServerCancelAbilitiesWithTags_Implementation(FGameplayTagContainer Tags, FInstancedStruct CancellationContext)
{
	CancelAbilitiesWithTags(Tags, CancellationContext);
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

/* Utility Functions */

USimpleGameplayAbility* USimpleGameplayAbilityComponent::GetAbilityInstanceByID(FGuid AbilityInstanceID)
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

/* Ability Lifecycle */

void USimpleGameplayAbilityComponent::OnAbilityActivationSuccess(USimpleAbilityBase* AbilityInstance)
{
	USimpleGameplayAbility* Ability = Cast<USimpleGameplayAbility>(AbilityInstance);
	const FGuid AbilityID = Ability ? Ability->AbilityID : FGuid();
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

void USimpleGameplayAbilityComponent::OnAbilityActivationFailed(USimpleAbilityBase* AbilityInstance)
{
	USimpleGameplayAbility* Ability = Cast<USimpleGameplayAbility>(AbilityInstance);
	const FGuid AbilityID = Ability ? Ability->AbilityID : FGuid();
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

void USimpleGameplayAbilityComponent::OnAbilityEnded(USimpleAbilityBase* AbilityInstance, FGameplayTag EndStatus, FInstancedStruct EndingContext)
{
	USimpleGameplayAbility* Ability = Cast<USimpleGameplayAbility>(AbilityInstance);
	const FGuid AbilityID = Ability ? Ability->AbilityID : FGuid();
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

void USimpleGameplayAbilityComponent::OnAbilityCancelled(USimpleAbilityBase* AbilityInstance, FGameplayTag CancelStatus, FInstancedStruct CancellationContext)
{
	USimpleGameplayAbility* Ability = Cast<USimpleGameplayAbility>(AbilityInstance);
	const FGuid AbilityID = Ability ? Ability->AbilityID : FGuid();
	
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

void USimpleGameplayAbilityComponent::ClientOnAbilityStateAdded(const FAbilityState& NewAbilityState)
{
	ResolveLocalAbilityState(NewAbilityState);
}

void USimpleGameplayAbilityComponent::ClientOnAbilityStateChanged(const FAbilityState& ChangedAbilityState)
{
	ResolveLocalAbilityState(ChangedAbilityState);
}

void USimpleGameplayAbilityComponent::ResolveLocalAbilityState(const FAbilityState& UpdatedAbilityState)
{
	FAbilityState* LocalAbilityState = LocalAbilityStates.FindByPredicate([UpdatedAbilityState](const FAbilityState& AbilityState) { return AbilityState.AbilityID == UpdatedAbilityState.AbilityID; });
	
	const TSubclassOf<USimpleGameplayAbility> AbilityClass = static_cast<TSubclassOf<USimpleGameplayAbility>>(UpdatedAbilityState.AbilityClass);
	USimpleGameplayAbility* AbilityInstance = GetAbilityInstanceByClass(AbilityClass);
	const bool IsSingleInstanceAbility = AbilityInstance->InstancingPolicy == EAbilityInstancingPolicy::SingleInstance;
	const bool IsInstancedAbilityActive = AbilityInstance->IsActive;
	
	switch (UpdatedAbilityState.AbilityStatus)
	{
		// If an ability ends on the server but never existed locally it probably ended very quickly and we should activate it locally
		// i.e. the ability activated and ended on the server in the same frame and got replicated to the client with the Ended status
		case Ended:
			if (!LocalAbilityState)
			{
				AbilityInstance->Initialize(this, UpdatedAbilityState.AbilityID);
				AbilityInstance->ActivateAbility(UpdatedAbilityState.ActivationContext);
				return;
			}
			break;
		
		case ActivationSuccess:
			if (IsInstancedAbilityActive)
			{
				// If the ability is already running on the client using the UpdatedAbility state ID we don't need to do anything
				if (AbilityInstance->AbilityID == UpdatedAbilityState.AbilityID)
				{
					return;
				}

				// Otherwise an ability with a different ID is already running and needs to be cancelled first
				AbilityInstance->CancelAbility(FGameplayTag::EmptyTag, FInstancedStruct());
			}
		
			AbilityInstance->Initialize(this, UpdatedAbilityState.AbilityID);
			AbilityInstance->ActivateAbility(UpdatedAbilityState.ActivationContext);

			break;
		
		// In these cases the ability is not running on the server and so if it is running locally we need to cancel it
		case PreActivation:
		case ActivationFailed:
		case Cancelled:
			if (IsSingleInstanceAbility && AbilityInstance->AbilityID == UpdatedAbilityState.AbilityID)
			{
				AbilityInstance->CancelAbility(FGameplayTag::EmptyTag, FInstancedStruct());
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

void USimpleGameplayAbilityComponent::ClientOnAbilityStateRemoved(const FAbilityState& RemovedAbilityState)
{
	LocalAbilityStates.RemoveAll([RemovedAbilityState](const FAbilityState& AbilityState) { return AbilityState.AbilityID == RemovedAbilityState.AbilityID; });
}

void USimpleGameplayAbilityComponent::ClientOnAbilitySnapshotAdded(const FAbilitySnapshot& NewAbilitySnapshot)
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

void USimpleGameplayAbilityComponent::ClientOnAbilitySnapshotRemoved(const FAbilitySnapshot& NewAbilitySnapshot)
{
	LocalPendingAbilitySnapshots.RemoveAll([NewAbilitySnapshot](const FAbilitySnapshot& Snapshot)
	{
		return Snapshot.AbilityID == NewAbilitySnapshot.AbilityID && Snapshot.SnapshotCounter == NewAbilitySnapshot.SnapshotCounter;
	});
}

void USimpleGameplayAbilityComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USimpleGameplayAbilityComponent, AvatarActor);
	DOREPLIFETIME(USimpleGameplayAbilityComponent, GrantedAbilities);
	DOREPLIFETIME(USimpleGameplayAbilityComponent, AuthorityAbilityStates);
	DOREPLIFETIME(USimpleGameplayAbilityComponent, AuthorityAbilitySnapshots);
}


