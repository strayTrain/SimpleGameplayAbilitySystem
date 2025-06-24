#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleAbilityTypes.h"
#include "SimpleAbilityBase.generated.h"

class USimpleGameplayAbilityComponent;

UCLASS(NotBlueprintable)
class SIMPLEGAMEPLAYABILITYSYSTEM_API  USimpleAbilityBase : public UObject, public FTickableGameObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "State")
	FGuid AbilityID;
	
	UPROPERTY(BlueprintReadOnly, Category = "State")
	USimpleGameplayAbilityComponent* OwningAbilityComponent;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	FAbilityContextCollection AbilityContexts;
	
	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool IsActive;
	
	/* Call these delegates in child classes to let the OwningAbilityComponent know when the ability instance's state changes */
	FAbilityActivationDelegate OnActivationSuccess;
	FAbilityActivationDelegate OnActivationFailed;
	FAbilityStoppedDelegate OnAbilityEnded;
	FAbilityStoppedDelegate OnAbilityCancelled;
	
	/* Implement these functions in derived classes */
	virtual bool CanActivate(USimpleGameplayAbilityComponent* ActivatingAbilityComponent, const FAbilityContextCollection ActivationContext) { return true; }
	virtual bool Activate(USimpleGameplayAbilityComponent* ActivatingAbilityComponent, const FGuid NewAbilityID, const FAbilityContextCollection ActivationContext);
	virtual void Tick(float DeltaTime) override {}
	virtual void Cancel(FGameplayTag CancelStatus, FInstancedStruct CancelContext) { }
	virtual void End(FGameplayTag EndStatus, FInstancedStruct EndContext) {}
	virtual void CleanUpAbility();
	
	/* Utility functions */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	FInstancedStruct GetAbilityContext(FGameplayTag ContextTag, bool& WasFound) const;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	EAbilityNetworkRole GetNetworkRole(bool& IsListenServer) const;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool IsRunningOnClient() const;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool IsRunningOnServer() const;
	
	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool HasAuthority() const;

	UFUNCTION(BlueprintCallable, Category = "Ability|Snapshot")
	void TakeStateSnapshot(FInstancedStruct SnapshotData, const FOnSnapshotResolved& OnResolved) { TakeSnapshotInternal(SnapshotData, OnResolved); }
	// Override in child classes to decide where to save the snapshot data.
	virtual void TakeSnapshotInternal(const FInstancedStruct SnapshotData, const FOnSnapshotResolved& OnResolved) {}
	// Called by the OwningAbilityComponent on the client version of this ability when a server snapshot is replicated
	virtual void OnServerSnapshotReceived(const int32 SnapshotCounter, const FInstancedStruct& AuthoritySnapshotData, const FInstancedStruct& LocalSnapshotData);
	
protected:
	// Set every time we activate this ability
	UPROPERTY(BlueprintReadOnly, Category = "State")
	double ActivationTime;
	
	virtual UWorld* GetWorld() const override;

	// Used to keep track of which snapshots need to be resolved
	TMap<int32, FOnSnapshotResolved> PendingSnapshots;
	
	/* FTickableGameObject overrides */
	virtual bool IsTickable() const override { return IsActive && GetWorld(); }
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(USimpleGameplayAbility, STATGROUP_Tickables); }
	virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Conditional; }
};
