#include "GameplayDebuggerCategory_AttributeModifier.h"

#if WITH_GAMEPLAY_DEBUGGER

#include "GameplayDebuggerCategoryReplicator.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeComponent.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeModifier/SimpleAttributeModifierTypes.h"
#include "SimpleGameplayAbilitySystem/Components/Interfaces/SimpleAbilitySystemInterfaces.h"
#include "Engine/Canvas.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeModifier/SimpleAttributeModifier.h"

FGameplayDebuggerCategory_AttributeModifier::FGameplayDebuggerCategory_AttributeModifier()
	: CurrentPage(0)
	, ItemsPerPage(3)
	, CurrentViewMode(0)
{
	bShowOnlyWithDebugActor = false;

	// Enable data pack replication from server to client
	SetDataPackReplication(&DataPack);

	BindKeyPress(EKeys::PageUp.GetFName(), FGameplayDebuggerInputModifier::None, this, &FGameplayDebuggerCategory_AttributeModifier::PrevPage, EGameplayDebuggerInputMode::Local);
	BindKeyPress(EKeys::PageDown.GetFName(), FGameplayDebuggerInputModifier::None, this, &FGameplayDebuggerCategory_AttributeModifier::NextPage, EGameplayDebuggerInputMode::Local);
	BindKeyPress(EKeys::SpaceBar.GetFName(), FGameplayDebuggerInputModifier::None, this, &FGameplayDebuggerCategory_AttributeModifier::ToggleViewMode, EGameplayDebuggerInputMode::Local);
}

TSharedRef<FGameplayDebuggerCategory> FGameplayDebuggerCategory_AttributeModifier::MakeInstance()
{
	return MakeShareable(new FGameplayDebuggerCategory_AttributeModifier());
}

void FGameplayDebuggerCategory_AttributeModifier::FRepData::Serialize(FArchive& Ar)
{
	int32 NumModifiers = ModifierStates.Num();
	int32 NumMutations = Mutations.Num();

	Ar << NumModifiers;
	Ar << NumMutations;
	Ar << PredictedSnapshotCount;
	Ar << PendingMutationQueueCount;

	if (Ar.IsLoading())
	{
		ModifierStates.SetNum(NumModifiers);
		Mutations.SetNum(NumMutations);
	}

	for (FModifierData& Data : ModifierStates)
	{
		Ar << Data.ModifierID;
		Ar << Data.ModifierClass;
		Ar << Data.Status;
		Ar << Data.Magnitude;
		Ar << Data.InstigatorName;
		Ar << Data.TargetName;
		Ar << Data.ApplicationTimestamp;
		Ar << Data.EndedTimestamp;
		Ar << Data.ContextType;
	}

	for (FMutationData& Data : Mutations)
	{
		Ar << Data.ModifierID;
		Ar << Data.ModifierClass;
		Ar << Data.MutationTimestamp;
		Ar << Data.ActionCount;
	}
}

void FGameplayDebuggerCategory_AttributeModifier::CollectData(APlayerController* OwnerPC, AActor* DebugActor)
{
	// This runs on the SERVER and collects authoritative data to replicate to clients
	DataPack.ModifierStates.Empty();
	DataPack.Mutations.Empty();
	DataPack.PredictedSnapshotCount = 0;
	DataPack.PendingMutationQueueCount = 0;

	if (!DebugActor)
	{
		return;
	}

	// Check if the actor implements the interface
	if (!DebugActor->Implements<UAttributeComponentInterface>())
	{
		return;
	}

	const USimpleAttributeComponent* AttributeComp = IAttributeComponentInterface::Execute_GetSimpleAttributeComponent(DebugActor);
	if (!AttributeComp || !IsValid(AttributeComp))
	{
		return;
	}

	// Make copies to avoid crashes if arrays are modified during iteration
	TArray<FAttributeModifierState> ModifierStatesCopy = AttributeComp->AuthorityAttributeModifierStates.ModifierStates;
	TArray<FAttributeModifierMutation> MutationsCopy = AttributeComp->AuthorityAttributeModifierMutations.Mutations;

	// Collect server-authoritative Modifier States
	for (const FAttributeModifierState& State : ModifierStatesCopy)
	{
		// Skip invalid states
		if (!State.ModifierClass || !IsValid(State.ModifierClass))
		{
			continue;
		}

		FModifierData Data;
		Data.ModifierID = State.ModifierID.ToString();
		Data.ModifierClass = State.ModifierClass->GetName();
		Data.Status = GetModifierStatusString(static_cast<uint8>(State.ModifierStatus));
		Data.Magnitude = State.ModifierMagnitude;
		Data.InstigatorName = (State.InstigatorAttributeComponent && State.InstigatorAttributeComponent->GetOwner()) ? State.InstigatorAttributeComponent->GetOwner()->GetName() : TEXT("None");
		Data.TargetName = (State.TargetAttributeComponent && State.TargetAttributeComponent->GetOwner()) ? State.TargetAttributeComponent->GetOwner()->GetName() : TEXT("None");
		Data.ApplicationTimestamp = State.ApplicationTimestamp;
		Data.EndedTimestamp = State.EndedTimestamp;

		if (State.ModifierContext.IsValid() && State.ModifierContext.GetScriptStruct())
		{
			Data.ContextType = State.ModifierContext.GetScriptStruct()->GetName();
		}

		DataPack.ModifierStates.Add(Data);
	}

	// Collect server-authoritative Mutations
	for (const FAttributeModifierMutation& Mutation : MutationsCopy)
	{
		// Skip invalid mutations
		if (!Mutation.ModifierClass || !IsValid(Mutation.ModifierClass))
		{
			continue;
		}

		FMutationData Data;
		Data.ModifierID = Mutation.ModifierID.ToString();
		Data.ModifierClass = Mutation.ModifierClass->GetName();
		Data.MutationTimestamp = Mutation.MutationTimestamp;
		Data.ActionCount = Mutation.ActionStackResult.ActionsResults.Num();

		DataPack.Mutations.Add(Data);
	}

	// Collect snapshot and queue info
	DataPack.PredictedSnapshotCount = AttributeComp->PredictedModifierSnapshots.Num();
	DataPack.PendingMutationQueueCount = AttributeComp->PendingMutationQueue.Num();
}

void FGameplayDebuggerCategory_AttributeModifier::DrawData(APlayerController* OwnerPC, FGameplayDebuggerCanvasContext& CanvasContext)
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
		CanvasContext.Printf(TEXT("{yellow}=== ATTRIBUTE MODIFIER ==="));
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

	CanvasContext.Printf(TEXT("{yellow}=== ATTRIBUTE MODIFIER ==="));
	if (bShowLocalPrediction)
	{
		CanvasContext.Printf(TEXT("{green}[LOCAL PREDICTED VALUES]"));
	}
	else
	{
		CanvasContext.Printf(TEXT("{grey}[SERVER REPLICATED VALUES]"));
	}

	FString ViewModeText = CurrentViewMode == 0 ? TEXT("MODIFIERS") : TEXT("MUTATIONS");
	CanvasContext.Printf(TEXT("View: {cyan}%s {grey}(Space to toggle)"), *ViewModeText);
	CanvasContext.Printf(TEXT(""));

	// Get local data if needed
	TArray<FModifierData> LocalModifierStates;
	TArray<FMutationData> LocalMutations;
	int32 LocalPredictedCount = 0;
	int32 LocalPendingCount = 0;

	if (bShowLocalPrediction)
	{
		IAttributeComponentInterface* Interface = Cast<IAttributeComponentInterface>(DebugActor);
		USimpleAttributeComponent* AttributeComp = Interface ? Interface->GetSimpleAttributeComponent() : nullptr;
		if (AttributeComp)
		{
			bool bIsServer = (NetMode == NM_ListenServer || NetMode == NM_DedicatedServer);

			// Make copies to avoid crashes if arrays are modified during iteration
			TArray<FAttributeModifierState> ModifierStatesCopy = bIsServer
				? AttributeComp->AuthorityAttributeModifierStates.ModifierStates
				: AttributeComp->LocalAttributeModifierStates;

			for (const FAttributeModifierState& State : ModifierStatesCopy)
			{
				// Skip invalid states
				if (!State.ModifierClass || !IsValid(State.ModifierClass))
				{
					continue;
				}

				FModifierData Data;
				Data.ModifierID = State.ModifierID.ToString();
				Data.ModifierClass = State.ModifierClass->GetName();
				Data.Status = GetModifierStatusString(static_cast<uint8>(State.ModifierStatus));
				Data.Magnitude = State.ModifierMagnitude;
				Data.InstigatorName = (State.InstigatorAttributeComponent && State.InstigatorAttributeComponent->GetOwner()) ? State.InstigatorAttributeComponent->GetOwner()->GetName() : TEXT("None");
				Data.TargetName = (State.TargetAttributeComponent && State.TargetAttributeComponent->GetOwner()) ? State.TargetAttributeComponent->GetOwner()->GetName() : TEXT("None");
				Data.ApplicationTimestamp = State.ApplicationTimestamp;
				Data.EndedTimestamp = State.EndedTimestamp;

				if (State.ModifierContext.IsValid() && State.ModifierContext.GetScriptStruct())
				{
					Data.ContextType = State.ModifierContext.GetScriptStruct()->GetName();
				}

				LocalModifierStates.Add(Data);
			}

			// Make copies to avoid crashes if arrays are modified during iteration
			TArray<FAttributeModifierMutation> MutationsCopy = bIsServer
				? AttributeComp->AuthorityAttributeModifierMutations.Mutations
				: AttributeComp->LocalAttributeModiferMutations;

			for (const FAttributeModifierMutation& Mutation : MutationsCopy)
			{
				// Skip invalid mutations
				if (!Mutation.ModifierClass || !IsValid(Mutation.ModifierClass))
				{
					continue;
				}

				FMutationData Data;
				Data.ModifierID = Mutation.ModifierID.ToString();
				Data.ModifierClass = Mutation.ModifierClass->GetName();
				Data.MutationTimestamp = Mutation.MutationTimestamp;
				Data.ActionCount = Mutation.ActionStackResult.ActionsResults.Num();

				LocalMutations.Add(Data);
			}

			LocalPredictedCount = AttributeComp->PredictedModifierSnapshots.Num();
			LocalPendingCount = AttributeComp->PendingMutationQueue.Num();
		}
	}

	const TArray<FModifierData>& ModifierStatesToShow = bShowLocalPrediction ? LocalModifierStates : DataPack.ModifierStates;
	const TArray<FMutationData>& MutationsToShow = bShowLocalPrediction ? LocalMutations : DataPack.Mutations;
	int32 PredictedCount = bShowLocalPrediction ? LocalPredictedCount : DataPack.PredictedSnapshotCount;
	int32 PendingCount = bShowLocalPrediction ? LocalPendingCount : DataPack.PendingMutationQueueCount;

	// Modifier States View
	if (CurrentViewMode == 0)
	{
		if (ModifierStatesToShow.Num() == 0)
		{
			CanvasContext.Printf(TEXT("{grey}No active modifiers"));
		}
		else
		{
			// Show in reverse order (most recent first)
			int32 TotalPages = FMath::CeilToInt(static_cast<float>(ModifierStatesToShow.Num()) / ItemsPerPage);
			int32 StartIdx = ModifierStatesToShow.Num() - 1 - (CurrentPage * ItemsPerPage);
			int32 EndIdx = FMath::Max(StartIdx - ItemsPerPage + 1, 0);

			CanvasContext.Printf(TEXT("Page {yellow}%d{white}/{yellow}%d {grey}(PageUp/PageDown)"),
				CurrentPage + 1, TotalPages);
			CanvasContext.Printf(TEXT(""));

			for (int32 i = StartIdx; i >= EndIdx; i--)
			{
				const FModifierData& Data = ModifierStatesToShow[i];
				CanvasContext.Printf(TEXT("{cyan}[%d] %s"), i + 1, *Data.ModifierClass);
				CanvasContext.Printf(TEXT("  ID: {white}%s"), *Data.ModifierID);
				CanvasContext.Printf(TEXT("  Status: {yellow}%s {grey}| Magnitude: {white}%.2f"), *Data.Status, Data.Magnitude);
				CanvasContext.Printf(TEXT("  {grey}%s -> %s"), *Data.InstigatorName, *Data.TargetName);
				CanvasContext.Printf(TEXT("  Applied: {grey}%.2f"), Data.ApplicationTimestamp);

				if (Data.EndedTimestamp > 0.0)
				{
					CanvasContext.Printf(TEXT("  Ended: {grey}%.2f"), Data.EndedTimestamp);
				}

				CanvasContext.Printf(TEXT(""));
			}
		}

		CanvasContext.Printf(TEXT("Predicted: {white}%d {grey}| Pending: {white}%d"),
			PredictedCount, PendingCount);
	}
	// Mutations View
	else if (CurrentViewMode == 1)
	{
		if (MutationsToShow.Num() == 0)
		{
			CanvasContext.Printf(TEXT("{grey}No mutations"));
		}
		else
		{
			// Show in reverse order (most recent first)
			int32 TotalPages = FMath::CeilToInt(static_cast<float>(MutationsToShow.Num()) / ItemsPerPage);
			int32 StartIdx = MutationsToShow.Num() - 1 - (CurrentPage * ItemsPerPage);
			int32 EndIdx = FMath::Max(StartIdx - ItemsPerPage + 1, 0);

			CanvasContext.Printf(TEXT("Page {yellow}%d{white}/{yellow}%d {grey}(PageUp/PageDown)"),
				CurrentPage + 1, TotalPages);
			CanvasContext.Printf(TEXT(""));

			for (int32 i = StartIdx; i >= EndIdx; i--)
			{
				const FMutationData& Data = MutationsToShow[i];
				CanvasContext.Printf(TEXT("{cyan}[%d] %s"), i + 1, *Data.ModifierClass);
				CanvasContext.Printf(TEXT("  ID: {white}%s"), *Data.ModifierID);
				CanvasContext.Printf(TEXT("  Actions: {white}%d"), Data.ActionCount);
				CanvasContext.Printf(TEXT("  Timestamp: {grey}%.2f"), Data.MutationTimestamp);
				CanvasContext.Printf(TEXT(""));
			}
		}
	}
}

void FGameplayDebuggerCategory_AttributeModifier::NextPage()
{
	int32 TotalItems = 0;

	if (CurrentViewMode == 0)
	{
		TotalItems = DataPack.ModifierStates.Num();
	}
	else if (CurrentViewMode == 1)
	{
		TotalItems = DataPack.Mutations.Num();
	}

	int32 TotalPages = FMath::CeilToInt(static_cast<float>(TotalItems) / ItemsPerPage);

	if (CurrentPage < TotalPages - 1)
	{
		CurrentPage++;
	}
}

void FGameplayDebuggerCategory_AttributeModifier::PrevPage()
{
	if (CurrentPage > 0)
	{
		CurrentPage--;
	}
}

void FGameplayDebuggerCategory_AttributeModifier::ToggleViewMode()
{
	CurrentViewMode = (CurrentViewMode + 1) % 2;
	CurrentPage = 0; // Reset page when changing view mode
}

FString FGameplayDebuggerCategory_AttributeModifier::GetModifierStatusString(uint8 Status) const
{
	switch (static_cast<EModifierStatus>(Status))
	{
		case EModifierStatus::Applied: return TEXT("Applied");
		case EModifierStatus::Cancelled: return TEXT("Cancelled");
		case EModifierStatus::Ended: return TEXT("Ended");
		default: return TEXT("Unknown");
	}
}

#endif // WITH_GAMEPLAY_DEBUGGER
