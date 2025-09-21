#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SimpleAbilityComponentTypes.h"
#include "Components/ActorComponent.h"
#include "SimpleAbility/SimpleAbilityTypes.h"
#include "SimpleGameplayAbilityComponent.generated.h"

struct FAbilitySideEffect;
struct FAbilityOverride;
class USimpleAbilityOverrideSet;
class USimpleAbilitySet;
class USimpleAttributeSet;
class USimpleGameplayAbility;

UCLASS(Blueprintable, ClassGroup=(AbilityComponent), meta=(BlueprintSpawnableComponent))
class SIMPLEGAMEPLAYABILITYSYSTEM_API USimpleGameplayAbilityComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USimpleGameplayAbilityComponent();

	/* Properties */
	
	UPROPERTY(BlueprintReadOnly, Replicated)
	AActor* AvatarActor;

	/* Initialization Properties */
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AbilityComponent|Abilities")
	TArray<USimpleAbilitySet*> AbilitySets;
	
	UPROPERTY(EditAnywhere, Replicated, BlueprintReadOnly, Category = "AbilityComponent|Abilities")
	TArray<TSubclassOf<USimpleGameplayAbility>> GrantedAbilities;

	/* Replicated Properties */
	
	// Ability States
	UPROPERTY(VisibleAnywhere, Replicated, Category = "AbilityComponent|State", meta = (TitleProperty = "AbilityStates.AbilityClass"))
	FAbilityStateContainer AuthorityAbilityStates;
	UPROPERTY(VisibleAnywhere, Category = "AbilityComponent|State", meta = (TitleProperty = "AbilityClass"))
	TArray<FAbilityState> LocalAbilityStates;
	
	// Ability Snapshots
	UPROPERTY(VisibleAnywhere, Replicated, Category = "AbilityComponent|State", meta = (TitleProperty = "AbilityStates.AbilityClass"))
	FAbilitySnapshotContainer AuthorityAbilitySnapshots;
	UPROPERTY(VisibleAnywhere, Category = "AbilityComponent|State", meta = (TitleProperty = "AbilityClass"))
	TArray<FAbilitySnapshot> LocalPendingAbilitySnapshots;

	/* Event Dispatchers */
	
	
	/* Avatar Actor Functions */
	
	UFUNCTION(BlueprintCallable, Category = "AbilityComponent|AvatarActor")
	AActor* GetAvatarActor() const { return AvatarActor; }
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "AbilityComponent|AvatarActor")
	void SetAvatarActor(AActor* NewAvatarActor) { AvatarActor = NewAvatarActor; }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool IsAvatarActorOfType(TSubclassOf<AActor> AvatarClass) const;
	
	/* Ability Functions */
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "AbilityComponent|Abilities")
	void GrantAbility(TSubclassOf<USimpleGameplayAbility> AbilityClass);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "AbilityComponent|Abilities")
	void RevokeAbility(TSubclassOf<USimpleGameplayAbility> AbilityClass);
	
	/**
	 * Activates an ability without any replication. Can be called from both server and client.
	 * @param AbilityClass The class of the ability to activate
	 * @param AbilityContext Extra context to pass to the ability on activation
	 * @param AbilityID The unique ID for this ability activation instance. This will be generated inside the function and returned by reference.
	 * @return 
	 */
	UFUNCTION(BlueprintCallable, meta=(ReturnDisplayName="WasActivated"), Category = "AbilityComponent|AbilityActivation")
	bool ActivateAbilityLocal(
		TSubclassOf<USimpleGameplayAbility> AbilityClass,
		FInstancedStruct AbilityContext,
		FGuid& AbilityID);

	/**
	 * Activates an ability with client-side prediction. Normally called on the client.
	 * If called on the server, it will still replicate the activation to clients.
	 * @param AbilityClass The class of the ability to activate
	 * @param AbilityContext Extra context to pass to the ability on activation
	 * @param AbilityID The unique ID for this ability activation instance. This will be generated inside the function and returned by reference.
	 * @return 
	 */
	UFUNCTION(BlueprintCallable, meta=(ReturnDisplayName="WasActivated"), Category = "AbilityComponent|AbilityActivation")
	bool ActivateAbilityPredicted(
		TSubclassOf<USimpleGameplayAbility> AbilityClass,
		FInstancedStruct AbilityContext,
		FGuid& AbilityID);

	/**
	 * Activates an ability on the server. If called from a client, it will send an RPC requesting to activate the ability on the server.
	 * The ability is still replicated to clients
	 * @param AbilityClass The class of the ability to activate
	 * @param AbilityContext Extra context to pass to the ability on activation
	 * @param AbilityID The unique ID for this ability activation instance. This will be generated inside the function and returned by reference.
	 */
	UFUNCTION(BlueprintCallable, meta=(ReturnDisplayName="WasActivated"), Category = "AbilityComponent|AbilityActivation")
	void ActivateAbilityServerInitiated(
		TSubclassOf<USimpleGameplayAbility> AbilityClass,
		FInstancedStruct AbilityContext,
		FGuid& AbilityID);
	
	UFUNCTION(Server, Reliable)
	void ServerActivateAbility(
		const FGuid AbilityID,
		TSubclassOf<USimpleGameplayAbility> AbilityClass,
		const FInstancedStruct& AbilityContexts,
		float ActivationTime);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "AbilityComponent|AbilityActivation")
	void CancelAbility(FGuid AbilityInstanceID, FInstancedStruct CancellationContext);

	UFUNCTION(BlueprintCallable, meta=(ReturnDisplayName="WasActivated"), Category = "AbilityComponent|AbilityActivation")
	void CancelAbilityPredicted(FGuid AbilityInstanceID, FInstancedStruct CancellationContext);
	
	UFUNCTION(Server, Reliable)
	void ServerCancelAbility(FGuid AbilityInstanceID, FInstancedStruct CancellationContext);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "AbilityComponent|AbilityActivation")
	TArray<FGuid> CancelAbilitiesWithTags(FGameplayTagContainer Tags, FInstancedStruct CancellationContext);

	UFUNCTION(BlueprintCallable, Category = "AbilityComponent|AbilityActivation")
	TArray<FGuid> CancelAbilitiesWithTagsPredicted(FGameplayTagContainer Tags, FInstancedStruct CancellationContext);

	UFUNCTION(Server, Reliable, Category = "AbilityComponent|AbilityActivation")
	void ServerCancelAbilitiesWithTags(FGameplayTagContainer Tags, FInstancedStruct CancellationContext);

	int32 AddGameplayAbilitySnapshot(FGuid AbilityID, FInstancedStruct SnapshotData);
	
	/* Utility Functions */

	UFUNCTION(BlueprintCallable, Category = "AbilityComponent|Utility")
	bool HasAuthority() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AbilityComponent|Utility")
	bool IsAnyAbilityActive() const;
	
	/**
	 * Returns the server time if called on the server.
	 * Returns the clients estimation of the server time if called on the client.
	 * By default, uses GetWorld()->GetGameState()->GetServerWorldTimeSeconds() to get the server time.
	 * Override this function to provide a custom network time synchronisation implementation.
	 * @return The current server time in seconds
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, BlueprintCallable, Category = "AbilityComponent|Utility")
	double GetServerTime();
	virtual double GetServerTime_Implementation();
	
	USimpleGameplayAbility* GetAbilityInstanceByID(FGuid AbilityInstanceID);
	USimpleGameplayAbility* GetAbilityInstanceByClass(TSubclassOf<USimpleGameplayAbility> AbilityClass);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	bool ActivateAbilityInternal(
		const FGuid AbilityID,
		const TSubclassOf<USimpleGameplayAbility>& AbilityClass,
		const FInstancedStruct& AbilityContext,
		bool ShouldTrackState,
		double ActivationTime = -1);
	
	UPROPERTY()
	TArray<USimpleGameplayAbility*> InstancedAbilities;

private:
	UFUNCTION()
	void OnAbilityActivationSuccess(USimpleAbilityBase* AbilityInstance);
	UFUNCTION()
	void OnAbilityActivationFailed(USimpleAbilityBase* AbilityInstance);
	UFUNCTION()
	void OnAbilityEnded(USimpleAbilityBase* AbilityInstance, FGameplayTag EndStatus, FInstancedStruct EndingContext);
	UFUNCTION()
	void OnAbilityCancelled(USimpleAbilityBase* AbilityInstance, FGameplayTag CancelStatus, FInstancedStruct CancellationContext);

	void ResolveLocalAbilityState(const FAbilityState& UpdatedAbilityState);
	
	// These functions are called on the client when the authoritative version of the variable changes on the server
	void ClientOnAbilityStateAdded(const FAbilityState& NewAbilityState);
	void ClientOnAbilityStateChanged(const FAbilityState& ChangedAbilityState);
	void ClientOnAbilityStateRemoved(const FAbilityState& RemovedAbilityState);
	void ClientOnAbilitySnapshotAdded(const FAbilitySnapshot& NewAbilitySnapshot);
	void ClientOnAbilitySnapshotRemoved(const FAbilitySnapshot& NewAbilitySnapshot);
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
};
