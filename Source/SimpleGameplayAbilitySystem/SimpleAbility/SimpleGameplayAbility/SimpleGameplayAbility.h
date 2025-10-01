#pragma once

#include "CoreMinimal.h"
#include "Tickable.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleAbilityBase/SimpleAbilityBase.h"
#include "SimpleGameplayAbility.generated.h"

class USimpleSubAbility;
class USimpleAttributeComponent;
class USimpleGameplayAbilityComponent;

UCLASS(Blueprintable, Abstract, BlueprintType, Abstract)
class SIMPLEGAMEPLAYABILITYSYSTEM_API USimpleGameplayAbility : public USimpleAbilityBase
{
	GENERATED_BODY()

public:
	/* Properties */

	UPROPERTY(BlueprintReadOnly, Category = "SimpleAbility|State")
	FGuid AbilityID;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SimpleAbility")
	EAbilityInstancingPolicy InstancingPolicy = EAbilityInstancingPolicy::SingleInstance;

	/* If true, the owning ability component must have this ability granted to it for this ability to activate. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SimpleAbility|Requirements")
	bool RequireGrantToActivate = true;
	
	/* These tags must be present on the AttributeComponent for this ability to activate. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SimpleAbility|Requirements")
	FGameplayTagContainer ActivationRequiredTags;

	/* These tags must NOT be present on the AttributeComponent for this ability to activate. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SimpleAbility|Requirements")
	FGameplayTagContainer ActivationBlockingTags;
	
	/*
	 * If set, this ability will only activate if ActivationContext contains this struct type.
	 * If left null, the ability will activate with any payload.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SimpleAbility|Requirements")
	UScriptStruct* RequiredContextType;

	/**
	 * This ability will fail to activate if the avatar actor of the ability component is not one of these types.
	 * If left empty any (or no) avatar actor will be allowed.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SimpleAbility|Requirements")
	TArray<TSubclassOf<AActor>> AvatarTypeFilter;

	/*
	 * Tags that can be used to classify this ability. e.g. "Melee", "Ranged", "AOE", etc.
	 * This is used in USimpleAbilityComponent::CancelAbilitiesWithTags
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleAbility|Tags")
	FGameplayTagContainer AbilityTags;

	/* These tags are added to the AttributeComponent when this ability is activated and removed when it ends. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleAbility|Tags")
	FGameplayTagContainer TemporarilyAppliedTags;

	/* These tags are added to the AttributeComponent when this ability is activated and not automatically removed when it ends. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleAbility|Tags")
	FGameplayTagContainer PermanentlyAppliedTags;

	/* Callable Functions */

	// Called by the AbilityComponent immediately after creating the ability instance
	void Initialize(USimpleGameplayAbilityComponent* ActivatingAbilityComponent, FGuid NewAbilityID);

	/**
	 * Use this function to activate abilities within this ability. SubAbilities don't support replication, you can
	 * think of them as the smallest unit of work an ability can do. For example, a good candidate for a sub ability is
	 * a montage playing ability. The input context would be the montage to play + any other data needed to play the montage.
	 * The output would be if the montage completed successfully or not. In the parent ability you could then use the end result
	 * for a StateSnapshot comparison between Server and Client.
	 * @param AbilityClass The class of the ability to activate
	 * @param ActivationContext Context to pass to the ability
	 * @return The activated sub ability instance
	 */
	UFUNCTION(BlueprintCallable, Meta = (ExpandEnumAsExecs = "ActivationResult"))
	USimpleSubAbility* ActivateSubAbility(TSubclassOf<USimpleSubAbility> AbilityClass, FInstancedStruct ActivationContext, EAbilityActivationResult& ActivationResult);

	USimpleSubAbility* GetSubAbilityInstance(TSubclassOf<USimpleSubAbility> AbilityClass);
	
	/* Overridable Functions */

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Ability")
	bool CanActivate(const FInstancedStruct& ActivationContext);
	virtual bool CanActivate_Implementation(const FInstancedStruct& ActivationContext) { return true; }
	
	/*
	 * Override this if your AttributeComponent and AbilityComponent don't exist on the same actor.
	 * e.g. You can have your AbilityComponent on the Pawn but the AttributeComponent on the PlayerState
	 * so that when you destroy/respawn the pawn, the "Stats" from the AttributeComponent remain intact.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, BlueprintPure, Category = "Ability")
	USimpleAttributeComponent* GetAttributeComponent();
	virtual USimpleAttributeComponent* GetAttributeComponent_Implementation();
	
	UFUNCTION(BlueprintNativeEvent, Category = "Abilities")
	void OnGranted(USimpleGameplayAbilityComponent* GrantedAbilityComponent);
	virtual void OnGranted_Implementation(USimpleGameplayAbilityComponent* GrantedAbilityComponent) {}
    
	// Static wrapper to call the function on the class default object
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	static void OnGrantedStatic(TSubclassOf<USimpleGameplayAbility> AbilityClass, USimpleGameplayAbilityComponent* GrantedAbilityComponent);

	virtual bool CanActivateInternal() override;
	virtual void PreActivateInternal() override;
	virtual void AbilityEndedInternal(FInstancedStruct EndingContext, bool WasCancelled) override;
	
	/* Utility functions */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	USimpleGameplayAbilityComponent* GetAbilityComponent() const { return AbilityComponent; }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	AActor* GetAvatarActor() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, meta = (DeterminesOutputType = "AvatarClass", HideSelfPin))
	AActor* GetAvatarActorAs(TSubclassOf<AActor> AvatarClass, bool& IsValid) const;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	EAbilityNetworkRole GetNetworkRole() const;
	
	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool HasAuthority() const;

	UFUNCTION(BlueprintCallable, Category = "SimpleAbility|Snapshot")
	void TakeStateSnapshot(FInstancedStruct SnapshotData, const FOnSnapshotResolved& OnResolved);
	// Called by the AbilityComponent on the client version of this ability when a server snapshot is replicated
	void OnServerSnapshotReceived(const int32 SnapshotCounter, const FInstancedStruct& AuthoritySnapshotData, const FInstancedStruct& LocalSnapshotData);
	
	/**
	 * Returns the server time this ability was activated at.
	 * If called from the Server Initiated ability it returns the authoritative time.
	 * If called from a Client Predicted ability it returns the clients estimation of the server time.
	 * @return The server time this ability was activated.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	double GetActivationTime() const;

	/**
	 * Returns the difference in time between when this ability was activated and now (in server time)
	 * You can use this value to help "fast-forward" predicted/replicated abilities.
	 * e.g. A client predicted ability starts by playing an anim montage. When the client calls this function it returns
	 * 0 at the start of the ability. When the server activates this ability it will be > 0 because the timestamp when the
	 * ability was activated will be different from the time on the server currently. You then use this time delay amount to
	 * offset your animation montage on the server, bringing it closer in sync with the client.
	 * @return The activation delay of the ability
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	double GetActivationDelay() const;

protected:
	// Used to keep track of which snapshots need to be resolved
	TMap<int32, FOnSnapshotResolved> PendingSnapshots;
	
	virtual bool IsTickable() const override { return CanTick && IsActive && GetWorld(); }
	
	// Delegate handlers for sub-ability lifecycle
	UFUNCTION()
	void OnSubAbilityEnded(USimpleAbilityBase* AbilityInstance, FGameplayTag StopStatus, FInstancedStruct StopContext);

	UFUNCTION()
	void OnSubAbilityCancelled(USimpleAbilityBase* AbilityInstance, FGameplayTag StopStatus, FInstancedStruct StopContext);

	// Helper to remove a tracked sub-ability by its instance pointer
	void RemoveTrackedSubAbilityByInstance(USimpleAbilityBase* AbilityInstance);
	
private:
	UPROPERTY()
	USimpleGameplayAbilityComponent* AbilityComponent;

	UPROPERTY()
	USimpleAttributeComponent* AttributeComponent;

	// Set every time we activate this ability
	UPROPERTY()
	double ActivationTime = 0;
	
	// Used to keep track of sub abilities which this ability has created which need to be ended when this ability ends/cancels
	TArray<FActivatedSubAbility> SubAbilityInstances;
};
