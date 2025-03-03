#pragma once

#include "CoreMinimal.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleAbilityTypes.h"
#include "UObject/Object.h"
#include "SimpleAbilityBase.generated.h"

class USimpleGameplayAbilityComponent;
class USimpleAbilityComponent;

UCLASS(NotBlueprintable)
class SIMPLEGAMEPLAYABILITYSYSTEM_API  USimpleAbilityBase : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
	FGuid AbilityInstanceID;
	
	UPROPERTY(BlueprintReadOnly)
	USimpleGameplayAbilityComponent* OwningAbilityComponent;

	/**
	 * True if this ability was replicated to a client, false if it was activated on the server or was client predicted
	 */
	UPROPERTY(BlueprintReadOnly)
	bool IsProxyAbility = false;

	UFUNCTION(BlueprintCallable)
	void InitializeAbility(USimpleGameplayAbilityComponent* InOwningAbilityComponent, FGuid InAbilityInstanceID, bool IsProxyActivation);

	UFUNCTION(BlueprintCallable, Category = "Ability|Snapshot")
	void TakeStateSnapshot(FGameplayTag SnapshotTag, FInstancedStruct SnapshotData, const FOnSnapshotResolved& OnResolved);

	// Called when the snapshot history locally is ahead of the server (usually in the case of a local predicted ability)
	virtual void ClientResolvePastState(FGameplayTag StateTag, FSimpleAbilitySnapshot AuthorityState, FSimpleAbilitySnapshot PredictedState);

	// Called when the snapshot history locally is behind the server (usually in the case of a server initiated ability)
	virtual void ClientFastForwardState(FGameplayTag StateTag, FSimpleAbilitySnapshot LatestAuthorityState);

protected:
	/**
	 * Used to keep track of how many snapshots we have taken so we can assign sequence numbers. The sequence number is used to
	 * determine which snapshot to use when resolving mispredictions.
	 */
	UPROPERTY()
	int32 SnapshotSequenceCounter = 0;

	// Map to store snapshot resolution callbacks, keyed by SnapshotSequenceCounter
	TMap<int32, FOnSnapshotResolved> SnapshotResolveCallbacks;
};
