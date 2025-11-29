#include "SimpleGameplayAbilityComponent.h"

#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleTimeSynchronizerComponent/ExtendedTimeSynchronizer/SimpleTimeSynchronizerExtended.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleGameplayAbility/SimpleGameplayAbility.h"
#include "SimpleGameplayAbilitySystem/DataAssets/AbilitySet/SimpleAbilitySet.h"
#include "SimpleGameplayAbilitySystem/DefaultTags/DefaultTags.h"
#include "SimpleGameplayAbilitySystem/Module/SimpleGameplayAbilitySystem.h"
#include "SimpleGameplayAbilitySystem/SimpleEventSubsystem/SimpleEventSubsystem.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeComponent.h"
#include "TimerManager.h"

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

	// Start periodic EventID cleanup timer (every 30 seconds)
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			EventIDCleanupTimerHandle,
			this,
			&USimpleGameplayAbilityComponent::CleanupOldEventIDs,
			30.0f,
			true
		);
	}

	if (HasAuthority())
	{
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
	
	Super::EndPlay(EndPlayReason);
}

/* ISimpleEventReplicator Interface Implementation */

void USimpleGameplayAbilityComponent::SendEvent(
	FGameplayTag EventTag,
	FGameplayTag DomainTag,
	FInstancedStruct Payload,
	UObject* Sender,
	const TArray<UObject*>& ListenerFilter)
{
	ProcessIncomingEvent(FGuid(), EventTag, DomainTag, Payload, Sender, ListenerFilter);
}

void USimpleGameplayAbilityComponent::SendEventToServer(
	FGameplayTag EventTag,
	FGameplayTag DomainTag,
	FInstancedStruct Payload,
	UObject* Sender,
	const TArray<UObject*>& ListenerFilter)
{
	if (HasAuthority())
	{
		SendEvent(EventTag, DomainTag, Payload, Sender, ListenerFilter);
		return;
	}

	const FGuid EventID = FGuid::NewGuid();
	ServerSendEvent(EventID, EventTag, DomainTag, Payload, Sender, ListenerFilter);
}

void USimpleGameplayAbilityComponent::SendEventToClient(
	FGameplayTag EventTag,
	FGameplayTag DomainTag,
	FInstancedStruct Payload,
	UObject* Sender,
	const TArray<UObject*>& ListenerFilter)
{
	if (!HasAuthority())
	{
		SIMPLE_LOG(this, TEXT("[USimpleGameplayAbilityComponent::SendEventToClient]: Called on client, ignoring!"));
		return;
	}

	const FGuid EventID = FGuid::NewGuid();
	ClientSendEvent(EventID, EventTag, DomainTag, Payload, Sender, ListenerFilter);
}

void USimpleGameplayAbilityComponent::SendEventToAllClients(
	FGameplayTag EventTag,
	FGameplayTag DomainTag,
	FInstancedStruct Payload,
	UObject* Sender,
	const TArray<UObject*>& ListenerFilter)
{
	// Generate unique EventID for deduplication
	const FGuid EventID = FGuid::NewGuid();

	// Send locally first
	ProcessIncomingEvent(EventID, EventTag, DomainTag, Payload, Sender, ListenerFilter);

	if (!HasAuthority())
	{
		// Request server to multicast
		ServerSendEvent(EventID, EventTag, DomainTag, Payload, Sender, ListenerFilter);
		return;
	}

	// Server multicasts to all clients
	MulticastSendEvent(EventID, EventTag, DomainTag, Payload, Sender, ListenerFilter);
}

void USimpleGameplayAbilityComponent::ServerSendEvent_Implementation(
	const FGuid& EventID,
	FGameplayTag EventTag,
	FGameplayTag DomainTag,
	const FInstancedStruct& Payload,
	UObject* Sender,
	const TArray<UObject*>& ListenerFilter)
{
	// Broadcast locally on server
	ProcessIncomingEvent(EventID, EventTag, DomainTag, Payload, Sender, ListenerFilter);

	// Also multicast to all clients
	MulticastSendEvent(EventID, EventTag, DomainTag, Payload, Sender, ListenerFilter);
}

void USimpleGameplayAbilityComponent::ClientSendEvent_Implementation(
	const FGuid& EventID,
	FGameplayTag EventTag,
	FGameplayTag DomainTag,
	const FInstancedStruct& Payload,
	UObject* Sender,
	const TArray<UObject*>& ListenerFilter)
{
	ProcessIncomingEvent(EventID, EventTag, DomainTag, Payload, Sender, ListenerFilter);
}

void USimpleGameplayAbilityComponent::MulticastSendEvent_Implementation(
	const FGuid& EventID,
	FGameplayTag EventTag,
	FGameplayTag DomainTag,
	const FInstancedStruct& Payload,
	UObject* Sender,
	const TArray<UObject*>& ListenerFilter)
{
	ProcessIncomingEvent(EventID, EventTag, DomainTag, Payload, Sender, ListenerFilter);
}

void USimpleGameplayAbilityComponent::ProcessIncomingEvent(
	const FGuid& EventID,
	FGameplayTag EventTag,
	FGameplayTag DomainTag,
	const FInstancedStruct& Payload,
	UObject* Sender,
	const TArray<UObject*>& ListenerFilter)
{
	// Check if we've already processed this event
	if (LocallySentEventIDs.Contains(EventID))
	{
		// Skip duplicate event (we already sent it locally)
		return;
	}

	// Add EventID to our set and track timestamp
	LocallySentEventIDs.Add(EventID);
	if (UWorld* World = GetWorld())
	{
		EventIDTimestamps.Add(EventID, World->GetTimeSeconds());
	}

	// Get the SimpleEventSubsystem and dispatch the event
	if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
	{
		if (USimpleEventSubsystem* EventSubsystem = GameInstance->GetSubsystem<USimpleEventSubsystem>())
		{
			EventSubsystem->SendEvent(EventTag, DomainTag, Payload, Sender, ListenerFilter);
		}
	}
}

void USimpleGameplayAbilityComponent::CleanupOldEventIDs()
{
	if (!GetWorld())
	{
		return;
	}

	const double CurrentTime = GetWorld()->GetTimeSeconds();
	const double MaxRetentionTime = 30.0;

	TArray<FGuid> EventIDsToRemove;

	// Find EventIDs older than 30 seconds
	for (const TPair<FGuid, double>& Pair : EventIDTimestamps)
	{
		if (CurrentTime - Pair.Value > MaxRetentionTime)
		{
			EventIDsToRemove.Add(Pair.Key);
		}
	}

	// Remove old EventIDs
	for (const FGuid& EventIDToRemove : EventIDsToRemove)
	{
		LocallySentEventIDs.Remove(EventIDToRemove);
		EventIDTimestamps.Remove(EventIDToRemove);
	}
}

/* Ability Functions */

bool USimpleGameplayAbilityComponent::ActivateAbility(const TSubclassOf<USimpleGameplayAbility> AbilityClass, FGuid& AbilityID)
{
	AbilityID = FGuid::NewGuid();
	return ActivateAbilityInternal(AbilityID, AbilityClass, FInstancedStruct(), false, GetServerTime(), EAbilityNetworkRole::Client); // We're not tracking state so ActivatedOn doesn't matter
}

DEFINE_FUNCTION(USimpleGameplayAbilityComponent::execActivateAbilityWithContext)
{
	P_GET_OBJECT(UClass, AbilityClass);

	// Get the wildcard struct parameter
	Stack.MostRecentProperty = nullptr;
	Stack.StepCompiledIn<FStructProperty>(nullptr);
	const void* StructPtr = Stack.MostRecentPropertyAddress;
	const FStructProperty* StructProperty = CastField<FStructProperty>(Stack.MostRecentProperty);

	P_GET_STRUCT_REF(FGuid, AbilityID);

	P_FINISH;

	// Convert the struct to FInstancedStruct
	FInstancedStruct AbilityContext;
	if (StructProperty && StructPtr)
	{
		AbilityContext.InitializeAs(StructProperty->Struct);
		StructProperty->Struct->CopyScriptStruct(AbilityContext.GetMutableMemory(), StructPtr);
	}

	// Call the internal function
	AbilityID = FGuid::NewGuid();
	const bool Result = P_THIS->ActivateAbilityInternal(AbilityID, TSubclassOf<USimpleGameplayAbility>(AbilityClass), AbilityContext, false, P_THIS->GetServerTime(), EAbilityNetworkRole::Client);
	*static_cast<bool*>(RESULT_PARAM) = Result;
}

// C++ only version that accepts FInstancedStruct directly
bool USimpleGameplayAbilityComponent::ActivateAbilityWithContext(
	TSubclassOf<USimpleGameplayAbility> AbilityClass,
	FInstancedStruct AbilityContext,
	FGuid& AbilityID)
{
	AbilityID = FGuid::NewGuid();
	return ActivateAbilityInternal(AbilityID, AbilityClass, AbilityContext, false, GetServerTime(), EAbilityNetworkRole::Client);
}

bool USimpleGameplayAbilityComponent::ActivateAbilityPredicted(TSubclassOf<USimpleGameplayAbility> AbilityClass, FGuid& AbilityID)
{
	AbilityID = FGuid::NewGuid();
	const bool WasActivated = ActivateAbilityInternal(
		AbilityID,
		AbilityClass,
		FInstancedStruct(),
		true,
		GetServerTime(),
		HasAuthority() ? EAbilityNetworkRole::ListenServer : EAbilityNetworkRole::Client);

	if (WasActivated && !HasAuthority())
	{
		ServerActivateAbility(AbilityID, AbilityClass, FInstancedStruct(), GetServerTime());
	}

	return WasActivated;
}

DEFINE_FUNCTION(USimpleGameplayAbilityComponent::execActivateAbilityPredictedWithContext)
{
	P_GET_OBJECT(UClass, AbilityClass);

	// Get the wildcard struct parameter
	Stack.MostRecentProperty = nullptr;
	Stack.StepCompiledIn<FStructProperty>(nullptr);
	const void* StructPtr = Stack.MostRecentPropertyAddress;
	const FStructProperty* StructProperty = CastField<FStructProperty>(Stack.MostRecentProperty);

	P_GET_STRUCT_REF(FGuid, AbilityID);

	P_FINISH;

	// Convert the struct to FInstancedStruct
	FInstancedStruct AbilityContext;
	if (StructProperty && StructPtr)
	{
		AbilityContext.InitializeAs(StructProperty->Struct);
		StructProperty->Struct->CopyScriptStruct(AbilityContext.GetMutableMemory(), StructPtr);
	}

	// Call the internal function
	AbilityID = FGuid::NewGuid();
	const bool WasActivated = P_THIS->ActivateAbilityInternal(
		AbilityID,
		TSubclassOf<USimpleGameplayAbility>(AbilityClass),
		AbilityContext,
		true,
		P_THIS->GetServerTime(),
		P_THIS->HasAuthority() ? EAbilityNetworkRole::ListenServer : EAbilityNetworkRole::Client);

	if (WasActivated && !P_THIS->HasAuthority())
	{
		P_THIS->ServerActivateAbility(AbilityID, TSubclassOf<USimpleGameplayAbility>(AbilityClass), AbilityContext, P_THIS->GetServerTime());
	}

	*static_cast<bool*>(RESULT_PARAM) = WasActivated;
}

// C++ only version that accepts FInstancedStruct directly
bool USimpleGameplayAbilityComponent::ActivateAbilityPredictedWithContext(
	TSubclassOf<USimpleGameplayAbility> AbilityClass,
	FInstancedStruct AbilityContext,
	FGuid& AbilityID)
{
	AbilityID = FGuid::NewGuid();
	const bool WasActivated = ActivateAbilityInternal(
		AbilityID,
		AbilityClass,
		AbilityContext,
		true,
		GetServerTime(),
		HasAuthority() ? EAbilityNetworkRole::ListenServer : EAbilityNetworkRole::Client);

	if (WasActivated && !HasAuthority())
	{
		ServerActivateAbility(AbilityID, AbilityClass, AbilityContext, GetServerTime());
	}

	return WasActivated;
}

void USimpleGameplayAbilityComponent::ActivateAbilityServerInitiated(TSubclassOf<USimpleGameplayAbility> AbilityClass, FGuid& AbilityID)
{
	AbilityID = FGuid::NewGuid();

	if (HasAuthority())
	{
		ActivateAbilityInternal(AbilityID, AbilityClass, FInstancedStruct(), true, GetServerTime(), EAbilityNetworkRole::ListenServer);
		return;
	}

	ServerActivateAbility(AbilityID, AbilityClass, FInstancedStruct(), GetServerTime());
}

DEFINE_FUNCTION(USimpleGameplayAbilityComponent::execActivateAbilityServerInitiatedWithContext)
{
	P_GET_OBJECT(UClass, AbilityClass);

	// Get the wildcard struct parameter
	Stack.MostRecentProperty = nullptr;
	Stack.StepCompiledIn<FStructProperty>(nullptr);
	const void* StructPtr = Stack.MostRecentPropertyAddress;
	const FStructProperty* StructProperty = CastField<FStructProperty>(Stack.MostRecentProperty);

	P_GET_STRUCT_REF(FGuid, AbilityID);

	P_FINISH;

	// Convert the struct to FInstancedStruct
	FInstancedStruct AbilityContext;
	if (StructProperty && StructPtr)
	{
		AbilityContext.InitializeAs(StructProperty->Struct);
		StructProperty->Struct->CopyScriptStruct(AbilityContext.GetMutableMemory(), StructPtr);
	}

	// Call the internal function
	AbilityID = FGuid::NewGuid();

	if (P_THIS->HasAuthority())
	{
		P_THIS->ActivateAbilityInternal(AbilityID, TSubclassOf<USimpleGameplayAbility>(AbilityClass), AbilityContext, true, P_THIS->GetServerTime(), EAbilityNetworkRole::ListenServer);
		return;
	}

	P_THIS->ServerActivateAbility(AbilityID, TSubclassOf<USimpleGameplayAbility>(AbilityClass), AbilityContext, P_THIS->GetServerTime());
}

// C++ only version that accepts FInstancedStruct directly
void USimpleGameplayAbilityComponent::ActivateAbilityServerInitiatedWithContext(
	TSubclassOf<USimpleGameplayAbility> AbilityClass,
	FInstancedStruct AbilityContext,
	FGuid& AbilityID)
{
	AbilityID = FGuid::NewGuid();

	if (HasAuthority())
	{
		ActivateAbilityInternal(AbilityID, AbilityClass, AbilityContext, true, GetServerTime(), EAbilityNetworkRole::ListenServer);
		return;
	}

	ServerActivateAbility(AbilityID, AbilityClass, AbilityContext, GetServerTime());
}

bool USimpleGameplayAbilityComponent::ActivateAbilityInternal(
	const FGuid AbilityID,
	const TSubclassOf<USimpleGameplayAbility>& AbilityClass,
	const FInstancedStruct& AbilityContext,
	const bool ShouldTrackState,
	const double ActivationTime, const EAbilityNetworkRole ActivatedOn)
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
		NewState.AbilityStatus = PreActivation;
		NewState.ActivatedOn = ActivatedOn;
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

	AbilityInstance->Initialize(this, GetSimpleAttributeComponent_Implementation(), AbilityID);
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

	const double ServerTime = GetServerTime();
	const double ClientTime = ActivationTime;
	const double TimeDifference = FMath::Abs(ServerTime - ClientTime);
	const double MaxAllowedTimeDrift = 2.0; // Allow up to 2 seconds of drift for high latency

	double ValidatedTime = ActivationTime;
	if (TimeDifference > MaxAllowedTimeDrift)
	{
		// Client time is too far off, use server time instead
		SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbilityComponent::ServerActivateAbility]: Client timestamp %.2f differs from server %.2f by %.2f seconds (max allowed: %.2f). Using server time."),
			ClientTime, ServerTime, TimeDifference, MaxAllowedTimeDrift));
		ValidatedTime = ServerTime;
	}

	ActivateAbilityInternal(AbilityID, AbilityClass, AbilityContexts, true, ValidatedTime, EAbilityNetworkRole::Client);
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

FAbilityState* USimpleGameplayAbilityComponent::GetAbilityStateByID(FGuid AbilityInstanceID)
{
	TArray<FAbilityState>& AbilityStates = HasAuthority() ? AuthorityAbilityStates.AbilityStates : LocalAbilityStates;

	for (FAbilityState& AbilityState : AbilityStates)
	{
		if (AbilityState.AbilityID == AbilityInstanceID)
		{
			return &AbilityState;
		}
	}

	return nullptr;
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
		if (AbilityInstance->AbilityTags.HasAnyExact(Tags) && AbilityInstance->IsActive)
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

TArray<FGuid> USimpleGameplayAbilityComponent::CancelAbilitiesWithClass(TSubclassOf<USimpleGameplayAbility> AbilityClass, FInstancedStruct CancellationContext)
{
	TArray<FGuid> CancelledAbilities;
	
	for (USimpleGameplayAbility* AbilityInstance : InstancedAbilities)
	{
		if (AbilityInstance->GetClass() == AbilityClass && AbilityInstance->IsActive)
		{
			AbilityInstance->CancelAbility(FGameplayTag::EmptyTag, CancellationContext);
			CancelledAbilities.Add(AbilityInstance->AbilityID);
		}
	}
	
	return CancelledAbilities;
}

TArray<FGuid> USimpleGameplayAbilityComponent::CancelAbilitiesWithClassPredicted(TSubclassOf<USimpleGameplayAbility> AbilityClass, FInstancedStruct CancellationContext)
{
	if (HasAuthority())
	{
		return CancelAbilitiesWithClass(AbilityClass, CancellationContext);	
	}
	
	TArray<FGuid> CancelledAbilities = CancelAbilitiesWithClass(AbilityClass, CancellationContext);
	ServerCancelAbilitiesWithClass(AbilityClass, CancellationContext);
	return CancelledAbilities;
}

void USimpleGameplayAbilityComponent::ServerCancelAbilitiesWithClass_Implementation(TSubclassOf<USimpleGameplayAbility> AbilityClass, FInstancedStruct CancellationContext)
{
	CancelAbilitiesWithClass(AbilityClass, CancellationContext);
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

/* Implementation of ISimpleAbilitySystemComponent interface */

USimpleAttributeComponent* USimpleGameplayAbilityComponent::GetSimpleAttributeComponent_Implementation()
{
	if (CachedAttributeComponent)
	{
		return CachedAttributeComponent;
	}
	
	// Start by checking if the attribute component is on the owner actor
	if (USimpleAttributeComponent* AttributeComp = GetOwner()->GetComponentByClass<USimpleAttributeComponent>())
	{
		CachedAttributeComponent = AttributeComp;
		return AttributeComp;
	}
	
	// Check if the attribute component is on the player state (assuming the owner is a pawn)
	if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		if (APlayerState* PS = OwnerPawn->GetPlayerState())
		{
			if (USimpleAttributeComponent* AttributeComp = PS->GetComponentByClass<USimpleAttributeComponent>())
			{
				CachedAttributeComponent = AttributeComp;
				return AttributeComp;
			}
		}
	}

	SIMPLE_LOG(GetOwner(), TEXT("[USimpleGameplayAbilityComponent::GetSimpleAttributeComponent_Implementation]: No SimpleAttributeComponent found on owner or player state! If you placed it on another actor, please override this function to return it correctly."));
	return nullptr;
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

USimpleTimeSynchronizer* USimpleGameplayAbilityComponent::GetTimeSynchronizerComponent_Implementation()
{
	// Default to assuming the owner actor has a time synchronizer component
	return GetOwner()->GetComponentByClass<USimpleTimeSynchronizerExtended>();
}

double USimpleGameplayAbilityComponent::GetServerTime()
{
	if (GetTimeSynchronizerComponent())
	{
		return GetTimeSynchronizerComponent()->GetServerTime();
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

bool USimpleGameplayAbilityComponent::IsOwnedByLocalPlayer() const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return false;
	}

	// Case 1: Component is on a Pawn
	if (APawn* OwnerPawn = Cast<APawn>(Owner))
	{
		return OwnerPawn->IsLocallyControlled();
	}

	// Case 2: Component is on a PlayerState
	if (APlayerState* OwnerPlayerState = Cast<APlayerState>(Owner))
	{
		if (APlayerController* PC = Cast<APlayerController>(OwnerPlayerState->GetOwner()))
		{
			return PC->IsLocalController();
		}
	}

	// Fallback: Use role-based check
	return Owner->GetLocalRole() == ROLE_AutonomousProxy;
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
		if (AbilityState.AbilityID == AbilityID && AbilityState.AbilityStatus == PreActivation)
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

			break;
		}
	}

	if (Ability && Ability->InstancingPolicy == EAbilityInstancingPolicy::MultipleInstances)
	{
		InstancedAbilities.Remove(Ability);
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
				// Clean up old states periodically on server
				CleanupOldAbilityStates();
			}

			break;
		}
	}

	if (Ability && Ability->InstancingPolicy == EAbilityInstancingPolicy::MultipleInstances)
	{
		InstancedAbilities.Remove(Ability);
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
				// Clean up old states periodically on server
				CleanupOldAbilityStates();
			}

			break;
		}
	}

	if (Ability && Ability->InstancingPolicy == EAbilityInstancingPolicy::MultipleInstances)
	{
		InstancedAbilities.Remove(Ability);
	}
}

void USimpleGameplayAbilityComponent::CleanupOldAbilityStates()
{
	if (!HasAuthority())
	{
		return;
	}

	const double CurrentTime = GetServerTime();
	const double CleanupThreshold = 5.0; // Remove states older than 5 seconds

	// Remove ended/cancelled states that are old enough
	AuthorityAbilityStates.AbilityStates.RemoveAll([CurrentTime, CleanupThreshold](const FAbilityState& State)
	{
		const bool bIsFinished = (State.AbilityStatus == Ended || State.AbilityStatus == Cancelled);
		const bool bIsOldEnough = (CurrentTime - State.EndingTimeStamp) > CleanupThreshold;
		return bIsFinished && bIsOldEnough;
	});
	AuthorityAbilityStates.MarkArrayDirty();

	// Also cleanup old snapshots
	const double SnapshotCleanupThreshold = 10.0; // Keep snapshots a bit longer for late clients
	AuthorityAbilitySnapshots.Snapshots.RemoveAll([CurrentTime, SnapshotCleanupThreshold](const FAbilitySnapshot& Snapshot)
	{
		return (CurrentTime - Snapshot.TimeStamp) > SnapshotCleanupThreshold;
	});
	AuthorityAbilitySnapshots.MarkArrayDirty();
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

	bool bAbilityJustActivated = false;

	switch (UpdatedAbilityState.AbilityStatus)
	{
		// If an ability ends on the server but never existed locally it probably ended very quickly and we should activate it locally
		// i.e. the ability activated and ended on the server in the same frame and got replicated to the client with the Ended status
		case Ended:
			if (!LocalAbilityState)
			{
				// Don't re-activate if this was a client-predicted ability that was originally activated on this client
				// This prevents instant abilities from running twice (once predicted, once on state replication)
				if (UpdatedAbilityState.ActivatedOn == EAbilityNetworkRole::Client && !HasAuthority())
				{
					LocalAbilityStates.Add(UpdatedAbilityState);
					return;
				}

				AbilityInstance->Initialize(this, GetSimpleAttributeComponent(), UpdatedAbilityState.AbilityID);
				AbilityInstance->ActivateAbility(UpdatedAbilityState.ActivationContext);
				LocalAbilityStates.Add(UpdatedAbilityState);
			}
			// Update existing state if it exists
			else
			{
				ProcessDeferredSnapshots(UpdatedAbilityState.AbilityID);
				*LocalAbilityState = UpdatedAbilityState;
			}
			return;

		case ActivationSuccess:
			if (IsInstancedAbilityActive)
			{
				// If the ability is already running on the client using the UpdatedAbility state ID we don't need to do anything
				if (AbilityInstance->AbilityID == UpdatedAbilityState.AbilityID)
				{
					// Check if there is a local ability state with the same ID and update it
					if (LocalAbilityState)
					{
						*LocalAbilityState = UpdatedAbilityState;
					}
					else
					{
						LocalAbilityStates.Add(UpdatedAbilityState);
					}
					return;
				}

				// Otherwise an ability with a different ID is already running and needs to be cancelled first
				AbilityInstance->CancelAbility(FGameplayTag::EmptyTag, FInstancedStruct());
			}

			// Don't re-activate if this was a client-predicted ability that was originally activated on this client
			// This handles the case where an instant ability completed before the ActivationSuccess state replicated
			// We check if we have a local state for this ability ID - if we do, this client predicted it
			if (UpdatedAbilityState.ActivatedOn == EAbilityNetworkRole::Client && !HasAuthority() && LocalAbilityState)
			{
				// Update the existing local state
				*LocalAbilityState = UpdatedAbilityState;
				break;
			}

			AbilityInstance->Initialize(this, GetSimpleAttributeComponent(), UpdatedAbilityState.AbilityID);
			AbilityInstance->ActivateAbility(UpdatedAbilityState.ActivationContext);
			bAbilityJustActivated = true;

			break;

		// In these cases the ability is not running on the server and so if it is running locally we need to cancel it
		case PreActivation:
		case ActivationFailed:
		case Cancelled:
			// The server rejected this ability, so cancel whatever is running
			if (IsSingleInstanceAbility && IsInstancedAbilityActive)
			{
				AbilityInstance->CancelAbility(FGameplayTag::EmptyTag, FInstancedStruct());
			}
			break;
	}

	// Check if there is a local ability state with the same ID and update it
	if (LocalAbilityState)
	{
		*LocalAbilityState = UpdatedAbilityState;
	}
	else
	{
		// Otherwise add the new ability state to the local array
		LocalAbilityStates.Add(UpdatedAbilityState);
	}

	if (bAbilityJustActivated)
	{
		ProcessDeferredSnapshots(UpdatedAbilityState.AbilityID);
	}
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

	// Try to resolve the snapshot immediately
	TryResolveSnapshot(NewAbilitySnapshot);
}

void USimpleGameplayAbilityComponent::TryResolveSnapshot(const FAbilitySnapshot& NewAbilitySnapshot)
{
	// Get the local version of this snapshot
	const FAbilitySnapshot* LocalSnapshot = LocalPendingAbilitySnapshots.FindByPredicate(
		[NewAbilitySnapshot](const FAbilitySnapshot& Snapshot)
		{
			return Snapshot.AbilityID == NewAbilitySnapshot.AbilityID && Snapshot.SnapshotCounter == NewAbilitySnapshot.SnapshotCounter;
		});

	if (!LocalSnapshot)
	{
		return;
	}

	// Find the ability instance (it can be active or already ended)
	USimpleGameplayAbility* LocalAbilityInstance = nullptr;
	for (USimpleGameplayAbility* InstancedAbility : InstancedAbilities)
	{
		if (InstancedAbility->AbilityID == LocalSnapshot->AbilityID)
		{
			LocalAbilityInstance = InstancedAbility;
			break;
		}
	}

	// If ability instance doesn't exist yet, defer this snapshot for later processing
	if (!LocalAbilityInstance)
	{
		// Add to deferred queue if not already there
		const bool bAlreadyDeferred = DeferredSnapshots.ContainsByPredicate(
			[NewAbilitySnapshot](const FAbilitySnapshot& Snapshot)
			{
				return Snapshot.AbilityID == NewAbilitySnapshot.AbilityID && Snapshot.SnapshotCounter == NewAbilitySnapshot.SnapshotCounter;
			});

		if (!bAlreadyDeferred)
		{
			DeferredSnapshots.Add(NewAbilitySnapshot);
		}
		return;
	}

	// Call the resolve function on the local ability instance
	LocalAbilityInstance->OnServerSnapshotReceived(NewAbilitySnapshot.SnapshotCounter, NewAbilitySnapshot.SnapshotData, LocalSnapshot->SnapshotData);

	// Remove the local snapshot from the pending snapshots array now that we've resolved the differences
	LocalPendingAbilitySnapshots.RemoveAll([LocalSnapshot](const FAbilitySnapshot& Snapshot)
	{
		return Snapshot.AbilityID == LocalSnapshot->AbilityID && Snapshot.SnapshotCounter == LocalSnapshot->SnapshotCounter;
	});

	// Also remove from deferred queue if it was there
	DeferredSnapshots.RemoveAll([NewAbilitySnapshot](const FAbilitySnapshot& Snapshot)
	{
		return Snapshot.AbilityID == NewAbilitySnapshot.AbilityID && Snapshot.SnapshotCounter == NewAbilitySnapshot.SnapshotCounter;
	});
}

void USimpleGameplayAbilityComponent::ProcessDeferredSnapshots(FGuid AbilityID)
{
	// Process any deferred snapshots for this ability now that it's active
	TArray<FAbilitySnapshot> SnapshotsToProcess;

	for (const FAbilitySnapshot& DeferredSnapshot : DeferredSnapshots)
	{
		if (DeferredSnapshot.AbilityID == AbilityID)
		{
			SnapshotsToProcess.Add(DeferredSnapshot);
		}
	}

	for (const FAbilitySnapshot& Snapshot : SnapshotsToProcess)
	{
		TryResolveSnapshot(Snapshot);
	}
}

void USimpleGameplayAbilityComponent::ClientOnAbilitySnapshotRemoved(const FAbilitySnapshot& NewAbilitySnapshot)
{
	LocalPendingAbilitySnapshots.RemoveAll([NewAbilitySnapshot](const FAbilitySnapshot& Snapshot)
	{
		return Snapshot.AbilityID == NewAbilitySnapshot.AbilityID && Snapshot.SnapshotCounter == NewAbilitySnapshot.SnapshotCounter;
	});

	// Also remove from deferred queue
	DeferredSnapshots.RemoveAll([NewAbilitySnapshot](const FAbilitySnapshot& Snapshot)
	{
		return Snapshot.AbilityID == NewAbilitySnapshot.AbilityID && Snapshot.SnapshotCounter == NewAbilitySnapshot.SnapshotCounter;
	});
}

void USimpleGameplayAbilityComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(USimpleGameplayAbilityComponent, GrantedAbilities, COND_OwnerOnly);
	
	DOREPLIFETIME(USimpleGameplayAbilityComponent, AvatarActor);
	DOREPLIFETIME(USimpleGameplayAbilityComponent, AuthorityAbilityStates);
	DOREPLIFETIME(USimpleGameplayAbilityComponent, AuthorityAbilitySnapshots);
}


