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
	
	UFUNCTION(BlueprintCallable, meta=(AdvancedDisplay=3, ReturnDisplayName="WasActivated"), Category = "AbilityComponent|AbilityActivation")
	bool ActivateAbility(
		TSubclassOf<USimpleGameplayAbility> AbilityClass,
		const FInstancedStruct& AbilityContext,
		FGuid& AbilityID,
		EAbilityActivationPolicyOverride ActivationPolicyOverride = EAbilityActivationPolicyOverride::DontOverride);

	bool ActivateAbilityWithID(
		const FGuid AbilityID,
		const TSubclassOf<USimpleGameplayAbility>& AbilityClass,
		const FInstancedStruct& AbilityContext,
		EAbilityActivationPolicyOverride ActivationPolicyOverride = EAbilityActivationPolicyOverride::DontOverride);
	
	UFUNCTION(Server, Reliable)
	void ServerActivateAbility(
		const FGuid AbilityID,
		TSubclassOf<USimpleGameplayAbility> AbilityClass,
		const FInstancedStruct& AbilityContexts,
		EAbilityActivationPolicy ActivationPolicy,
		float ActivationTime);

	UFUNCTION(BlueprintCallable, meta=(AdvancedDisplay=2), Category = "AbilityComponent|AbilityActivation")
	void CancelAbility(FGuid AbilityInstanceID, FInstancedStruct CancellationContext);

	UFUNCTION(BlueprintCallable, Category = "AbilityComponent|AbilityActivation")
	TArray<FGuid> CancelAbilitiesWithTags(FGameplayTagContainer Tags, FInstancedStruct CancellationContext);

	int32 AddGameplayAbilitySnapshot(FGuid AbilityID, FInstancedStruct SnapshotData);
	
	/* Replicated Event Functions */
	
	UFUNCTION(BlueprintCallable, Category = "AbilityComponent|Events", meta=(AutoCreateRefTerm = "ListenerFilter"))
	void SendEvent(
		FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload,
		UObject* Sender, TArray<UObject*> ListenerFilter, ESimpleEventReplicationPolicy ReplicationPolicy);
	
	void SendEventInternal(
		FGuid EventID, FGameplayTag EventTag, FGameplayTag DomainTag, const FInstancedStruct& Payload,
		UObject* Sender, ESimpleEventReplicationPolicy ReplicationPolicy, const TArray<UObject*>& ListenerFilter);

	UFUNCTION(Server, Reliable)
	void ServerSendEvent(
		FGuid EventID, FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload,
		UObject* Sender, ESimpleEventReplicationPolicy ReplicationPolicy, const TArray<UObject*>& ListenerFilter);

	UFUNCTION(Client, Reliable)
	void ClientSendEvent(
		FGuid EventID, FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload,
		UObject* Sender, ESimpleEventReplicationPolicy ReplicationPolicy, const TArray<UObject*>& ListenerFilter);
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastSendEvent(
		FGuid EventID, FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload,
		UObject* Sender, ESimpleEventReplicationPolicy ReplicationPolicy, const TArray<UObject*>& ListenerFilter);
	
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
		EAbilityActivationPolicy ActivationPolicy,
		bool TrackState,
		double ActivationTime = -1);
	
	UPROPERTY()
	TArray<USimpleGameplayAbility*> InstancedAbilities;
	
	// Used to keep track of which events have been handled locally to avoid double event sending with multicast
	TArray<FGuid> HandledEventIDs;

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
	void ClientOnAttributeModifierSnapshotAdded(const FAbilitySnapshot& NewAttributeModifierSnapshot);
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
};
