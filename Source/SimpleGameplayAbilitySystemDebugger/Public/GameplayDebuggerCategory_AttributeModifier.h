#pragma once

#include "CoreMinimal.h"

#if WITH_GAMEPLAY_DEBUGGER
#include "GameplayDebuggerCategory.h"

class AActor;
class APlayerController;
class USimpleAttributeComponent;

class FGameplayDebuggerCategory_AttributeModifier : public FGameplayDebuggerCategory
{
public:
	FGameplayDebuggerCategory_AttributeModifier();

	virtual void CollectData(APlayerController* OwnerPC, AActor* DebugActor) override;
	virtual void DrawData(APlayerController* OwnerPC, FGameplayDebuggerCanvasContext& CanvasContext) override;

	static TSharedRef<FGameplayDebuggerCategory> MakeInstance();

protected:
	struct FModifierData
	{
		FString ModifierID;
		FString ModifierClass;
		FString Status;
		float Magnitude;
		FString InstigatorName;
		FString TargetName;
		double ApplicationTimestamp;
		double EndedTimestamp;
		FString ContextType;

		FModifierData()
			: Magnitude(0.0f)
			, ApplicationTimestamp(0.0)
			, EndedTimestamp(0.0)
		{}
	};

	struct FMutationData
	{
		FString ModifierID;
		FString ModifierClass;
		double MutationTimestamp;
		int32 ActionCount;

		FMutationData()
			: MutationTimestamp(0.0)
			, ActionCount(0)
		{}
	};

	// Replicated data pack (server-authoritative data)
	struct FRepData
	{
		TArray<FModifierData> ModifierStates;
		TArray<FMutationData> Mutations;
		int32 PredictedSnapshotCount;
		int32 PendingMutationQueueCount;

		FRepData()
			: PredictedSnapshotCount(0)
			, PendingMutationQueueCount(0)
		{}

		void Serialize(FArchive& Ar);
	};

	FRepData DataPack;

	// Pagination and view mode
	int32 CurrentPage;
	int32 ItemsPerPage;
	int32 CurrentViewMode; // 0 = Modifiers, 1 = Mutations

	void NextPage();
	void PrevPage();
	void ToggleViewMode();

	FString GetModifierStatusString(uint8 Status) const;
};

#endif // WITH_GAMEPLAY_DEBUGGER
