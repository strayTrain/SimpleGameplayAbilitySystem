#include "GameplayDebuggerCategory_AbilityComponent.h"

#if WITH_GAMEPLAY_DEBUGGER

#include "GameplayDebuggerCategoryReplicator.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleGameplayAbilityComponent/SimpleGameplayAbilityComponent.h"
#include "SimpleGameplayAbilitySystem/Components/Interfaces/SimpleAbilitySystemInterfaces.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleAbilityTypes.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleAbilityBase/SimpleAbilityBase.h"

FGameplayDebuggerCategory_AbilityComponent::FGameplayDebuggerCategory_AbilityComponent()
	: CurrentPage(0)
	, ItemsPerPage(3)
{
	bShowOnlyWithDebugActor = false;

	// Enable data pack replication from server to client
	SetDataPackReplication(&DataPack);

	BindKeyPress(EKeys::PageUp.GetFName(), FGameplayDebuggerInputModifier::None, this, &FGameplayDebuggerCategory_AbilityComponent::PrevPage, EGameplayDebuggerInputMode::Local);
	BindKeyPress(EKeys::PageDown.GetFName(), FGameplayDebuggerInputModifier::None, this, &FGameplayDebuggerCategory_AbilityComponent::NextPage, EGameplayDebuggerInputMode::Local);
}

TSharedRef<FGameplayDebuggerCategory> FGameplayDebuggerCategory_AbilityComponent::MakeInstance()
{
	return MakeShareable(new FGameplayDebuggerCategory_AbilityComponent());
}

void FGameplayDebuggerCategory_AbilityComponent::FRepData::Serialize(FArchive& Ar)
{
	int32 NumStates = AbilityStates.Num();

	Ar << NumStates;
	Ar << DeferredSnapshotCount;
	Ar << PendingSnapshotCount;

	if (Ar.IsLoading())
	{
		AbilityStates.SetNum(NumStates);
	}

	for (FAbilityStateData& Data : AbilityStates)
	{
		Ar << Data.AbilityID;
		Ar << Data.AbilityClass;
		Ar << Data.AbilityStatus;
		Ar << Data.NetworkRole;
		Ar << Data.ActivationTimestamp;
		Ar << Data.EndingTimestamp;
		Ar << Data.ActivationContext;
		Ar << Data.EndingContext;
	}
}

void FGameplayDebuggerCategory_AbilityComponent::CollectData(APlayerController* OwnerPC, AActor* DebugActor)
{
	// This runs on the server and collects authoritative data to replicate to clients
	DataPack.AbilityStates.Empty();
	DataPack.DeferredSnapshotCount = 0;
	DataPack.PendingSnapshotCount = 0;

	if (!DebugActor)
	{
		return;
	}

	// Check if the actor implements the interface
	if (!DebugActor->Implements<UAbilityComponentInterface>())
	{
		return;
	}

	const USimpleGameplayAbilityComponent* AbilityComp = IAbilityComponentInterface::Execute_GetSimpleAbilityComponent(DebugActor);
	if (!AbilityComp || !IsValid(AbilityComp))
	{
		return;
	}

	// Make a copy to avoid crashes if the array is modified during iteration
	TArray<FAbilityState> AbilityStatesCopy = AbilityComp->AuthorityAbilityStates.AbilityStates;

	// Collect server-authoritative Ability States
	for (const FAbilityState& State : AbilityStatesCopy)
	{
		// Skip invalid states
		if (!State.AbilityClass || !IsValid(State.AbilityClass))
		{
			continue;
		}

		FAbilityStateData Data;
		Data.AbilityID = State.AbilityID.ToString();
		Data.AbilityClass = State.AbilityClass->GetName();
		Data.AbilityStatus = GetAbilityStatusString(static_cast<uint8>(State.AbilityStatus));
		Data.NetworkRole = GetNetworkRoleString(static_cast<uint8>(State.ActivatedOn));
		Data.ActivationTimestamp = State.ActivationTimeStamp;
		Data.EndingTimestamp = State.EndingTimeStamp;

		if (State.ActivationContext.IsValid() && State.ActivationContext.GetScriptStruct())
		{
			Data.ActivationContext = State.ActivationContext.GetScriptStruct()->GetName();
		}
		if (State.EndingContext.IsValid() && State.EndingContext.GetScriptStruct())
		{
			Data.EndingContext = State.EndingContext.GetScriptStruct()->GetName();
		}

		DataPack.AbilityStates.Add(Data);
	}

	// Collect snapshot counts
	DataPack.DeferredSnapshotCount = AbilityComp->DeferredSnapshots.Num();
	DataPack.PendingSnapshotCount = AbilityComp->LocalPendingAbilitySnapshots.Num();
}

void FGameplayDebuggerCategory_AbilityComponent::DrawData(APlayerController* OwnerPC, FGameplayDebuggerCanvasContext& CanvasContext)
{
	// This runs on both server and client
	// Get the currently selected debug actor
	AActor* DebugActor = nullptr;
	if (AGameplayDebuggerCategoryReplicator* Replicator = GetReplicator())
	{
		DebugActor = Replicator->GetDebugActor();
	}

	if (!DebugActor)
	{
		CanvasContext.Printf(TEXT("{yellow}=== ABILITY COMPONENT ==="));
		CanvasContext.Printf(TEXT("{grey}No debug actor selected"));
		return;
	}

	// Determine if we should show local predicted data
	bool bShowLocalPrediction = false;

	UWorld* World = DebugActor->GetWorld();
	ENetMode NetMode = World ? World->GetNetMode() : NM_Standalone;

	if (OwnerPC && OwnerPC->IsLocalController())
	{
		APawn* ViewerPawn = OwnerPC->GetPawn();
		bShowLocalPrediction = (DebugActor == ViewerPawn) || (DebugActor->GetOwner() == ViewerPawn);
	}

	// On server, always show local data (which is the authoritative data)
	if (NetMode == NM_ListenServer || NetMode == NM_DedicatedServer || NetMode == NM_Standalone)
	{
		bShowLocalPrediction = true;
	}

	CanvasContext.Printf(TEXT("{yellow}=== ABILITY COMPONENT ==="));
	if (bShowLocalPrediction)
	{
		CanvasContext.Printf(TEXT("{green}[LOCAL PREDICTED VALUES]"));
	}
	else
	{
		CanvasContext.Printf(TEXT("{grey}[SERVER REPLICATED VALUES]"));
	}
	CanvasContext.Printf(TEXT(""));

	// Get local data if needed
	TArray<FAbilityStateData> LocalStates;
	int32 LocalDeferredCount = 0;
	int32 LocalPendingCount = 0;

	// Track which local states have been confirmed by server (for clients)
	TSet<FString> ConfirmedAbilityIDs;

	if (bShowLocalPrediction)
	{
		USimpleGameplayAbilityComponent* AbilityComp = nullptr;
		if (DebugActor->Implements<UAbilityComponentInterface>())
		{
			AbilityComp = IAbilityComponentInterface::Execute_GetSimpleAbilityComponent(DebugActor);
		}
		if (AbilityComp && IsValid(AbilityComp))
		{
			bool bIsServer = (NetMode == NM_ListenServer || NetMode == NM_DedicatedServer);

			// Make a copy to avoid crashes if the array is modified during iteration
			TArray<FAbilityState> AbilityStatesCopy = bIsServer
				? AbilityComp->AuthorityAbilityStates.AbilityStates
				: AbilityComp->LocalAbilityStates;

			for (const FAbilityState& State : AbilityStatesCopy)
			{
				// Skip invalid states
				if (!State.AbilityClass || !IsValid(State.AbilityClass))
				{
					continue;
				}

				FAbilityStateData Data;
				Data.AbilityID = State.AbilityID.ToString();
				Data.AbilityClass = State.AbilityClass->GetName();
				Data.AbilityStatus = GetAbilityStatusString(static_cast<uint8>(State.AbilityStatus));
				Data.NetworkRole = GetNetworkRoleString(static_cast<uint8>(State.ActivatedOn));
				Data.ActivationTimestamp = State.ActivationTimeStamp;
				Data.EndingTimestamp = State.EndingTimeStamp;

				if (State.ActivationContext.IsValid() && State.ActivationContext.GetScriptStruct())
				{
					Data.ActivationContext = State.ActivationContext.GetScriptStruct()->GetName();
				}
				if (State.EndingContext.IsValid() && State.EndingContext.GetScriptStruct())
				{
					Data.EndingContext = State.EndingContext.GetScriptStruct()->GetName();
				}

				// On client, check if this state exists in the replicated authority states
				if (!bIsServer)
				{
					// Make a copy of authority states too for thread safety
					TArray<FAbilityState> AuthorityStatesCopy = AbilityComp->AuthorityAbilityStates.AbilityStates;
					const FAbilityState* AuthState = AuthorityStatesCopy.FindByPredicate(
						[&](const FAbilityState& A) { return A.AbilityID == State.AbilityID; });

					if (AuthState)
					{
						ConfirmedAbilityIDs.Add(Data.AbilityID);
					}
				}

				LocalStates.Add(Data);
			}

			LocalDeferredCount = AbilityComp->DeferredSnapshots.Num();
			LocalPendingCount = AbilityComp->LocalPendingAbilitySnapshots.Num();
		}
	}

	// Sort states by activation timestamp (most recent first)
	TArray<FAbilityStateData> SortedStates = bShowLocalPrediction ? LocalStates : DataPack.AbilityStates;
	SortedStates.Sort([](const FAbilityStateData& A, const FAbilityStateData& B) {
		return A.ActivationTimestamp > B.ActivationTimestamp;
	});

	int32 DeferredCount = bShowLocalPrediction ? LocalDeferredCount : DataPack.DeferredSnapshotCount;
	int32 PendingCount = bShowLocalPrediction ? LocalPendingCount : DataPack.PendingSnapshotCount;

	// On server, all states are confirmed
	bool bIsServer = (NetMode == NM_ListenServer || NetMode == NM_DedicatedServer || NetMode == NM_Standalone);

	if (SortedStates.Num() == 0)
	{
		CanvasContext.Printf(TEXT("{grey}No active abilities"));
	}
	else
	{
		// Already sorted by timestamp (most recent first)
		int32 TotalPages = FMath::CeilToInt(static_cast<float>(SortedStates.Num()) / ItemsPerPage);
		int32 StartIdx = CurrentPage * ItemsPerPage;
		int32 EndIdx = FMath::Min(StartIdx + ItemsPerPage, SortedStates.Num());

		CanvasContext.Printf(TEXT("Page {yellow}%d{white}/{yellow}%d {grey}(PageUp/PageDown)"),
			CurrentPage + 1, TotalPages);
		CanvasContext.Printf(TEXT(""));

		for (int32 i = StartIdx; i < EndIdx; i++)
		{
			const FAbilityStateData& Data = SortedStates[i];

			// Determine color based on whether this is confirmed by server
			// Server: always green (authoritative)
			// Client confirmed: green
			// Client unconfirmed: yellow (predicted, waiting for server)
			bool bIsConfirmed = bIsServer || ConfirmedAbilityIDs.Contains(Data.AbilityID);
			FString StatusColor = bIsConfirmed ? TEXT("green") : TEXT("yellow");

			CanvasContext.Printf(TEXT("{cyan}[%d] %s"), i + 1, *Data.AbilityClass);
			CanvasContext.Printf(TEXT("  ID: {white}%s"), *Data.AbilityID);
			CanvasContext.Printf(TEXT("  Status: {%s}%s"), *StatusColor, *Data.AbilityStatus);
			CanvasContext.Printf(TEXT("  Activated On: {white}%s"), *Data.NetworkRole);
			CanvasContext.Printf(TEXT("  Activation: {grey}%.2f"), Data.ActivationTimestamp);

			if (Data.EndingTimestamp > 0.0)
			{
				CanvasContext.Printf(TEXT("  Ended: {grey}%.2f"), Data.EndingTimestamp);
			}

			if (!Data.ActivationContext.IsEmpty())
			{
				CanvasContext.Printf(TEXT("  Context: {grey}%s"), *Data.ActivationContext);
			}

			CanvasContext.Printf(TEXT(""));
		}
	}

	CanvasContext.Printf(TEXT("Deferred: {white}%d {grey}| Pending: {white}%d"),
		DeferredCount, PendingCount);
}

void FGameplayDebuggerCategory_AbilityComponent::NextPage()
{
	int32 TotalPages = FMath::CeilToInt(static_cast<float>(DataPack.AbilityStates.Num()) / ItemsPerPage);

	if (CurrentPage < TotalPages - 1)
	{
		CurrentPage++;
	}
}

void FGameplayDebuggerCategory_AbilityComponent::PrevPage()
{
	if (CurrentPage > 0)
	{
		CurrentPage--;
	}
}

FString FGameplayDebuggerCategory_AbilityComponent::GetAbilityStatusString(uint8 Status) const
{
	switch (static_cast<EAbilityStatus>(Status))
	{
		case EAbilityStatus::PreActivation: return TEXT("PreActivation");
		case EAbilityStatus::ActivationSuccess: return TEXT("Active");
		case EAbilityStatus::ActivationFailed: return TEXT("Failed");
		case EAbilityStatus::Ended: return TEXT("Ended");
		case EAbilityStatus::Cancelled: return TEXT("Cancelled");
		default: return TEXT("Unknown");
	}
}

FString FGameplayDebuggerCategory_AbilityComponent::GetNetworkRoleString(uint8 Role) const
{
	switch (static_cast<EAbilityNetworkRole>(Role))
	{
		case EAbilityNetworkRole::DedicatedServer: return TEXT("DedicatedServer");
		case EAbilityNetworkRole::ListenServer: return TEXT("ListenServer");
		case EAbilityNetworkRole::Client: return TEXT("Client");
		default: return TEXT("Unknown");
	}
}

#endif // WITH_GAMEPLAY_DEBUGGER
