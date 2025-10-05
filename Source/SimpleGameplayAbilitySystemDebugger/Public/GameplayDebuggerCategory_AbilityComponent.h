#pragma once

#include "CoreMinimal.h"

#if WITH_GAMEPLAY_DEBUGGER
#include "GameplayDebuggerCategory.h"

class AActor;
class APlayerController;
class USimpleGameplayAbilityComponent;

class FGameplayDebuggerCategory_AbilityComponent : public FGameplayDebuggerCategory
{
public:
	FGameplayDebuggerCategory_AbilityComponent();

	virtual void CollectData(APlayerController* OwnerPC, AActor* DebugActor) override;
	virtual void DrawData(APlayerController* OwnerPC, FGameplayDebuggerCanvasContext& CanvasContext) override;

	static TSharedRef<FGameplayDebuggerCategory> MakeInstance();

protected:
	struct FAbilityStateData
	{
		FString AbilityID;
		FString AbilityClass;
		FString AbilityStatus;
		FString NetworkRole;
		double ActivationTimestamp;
		double EndingTimestamp;
		FString ActivationContext;
		FString EndingContext;

		FAbilityStateData()
			: ActivationTimestamp(0.0)
			, EndingTimestamp(0.0)
		{}
	};

	// Replicated data pack (server-authoritative data)
	struct FRepData
	{
		TArray<FAbilityStateData> AbilityStates;
		int32 DeferredSnapshotCount;
		int32 PendingSnapshotCount;

		FRepData()
			: DeferredSnapshotCount(0)
			, PendingSnapshotCount(0)
		{}

		void Serialize(FArchive& Ar);
	};

	FRepData DataPack;

	// Pagination
	int32 CurrentPage;
	int32 ItemsPerPage;

	void NextPage();
	void PrevPage();

	FString GetAbilityStatusString(uint8 Status) const;
	FString GetNetworkRoleString(uint8 Role) const;
};

#endif // WITH_GAMEPLAY_DEBUGGER
