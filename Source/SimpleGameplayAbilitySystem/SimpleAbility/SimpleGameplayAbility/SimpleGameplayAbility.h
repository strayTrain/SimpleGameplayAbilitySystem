#pragma once

#include "CoreMinimal.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleAbilityBase/SimpleAbilityBase.h"
#include "Tickable.h"
#include "SimpleGameplayAbilitySystem/SimpleGameplayAbilityComponent/SimpleAbilityComponentTypes.h"
#include "SimpleGameplayAbility.generated.h"

UCLASS(Blueprintable)
class SIMPLEGAMEPLAYABILITYSYSTEM_API USimpleGameplayAbility : public USimpleAbilityBase, public FTickableGameObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Tick")
	bool CanTick = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Activation")
	EAbilityActivationPolicy ActivationPolicy = EAbilityActivationPolicy::LocalOnly;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Activation")
	EAbilityInstancingPolicy InstancingPolicy = EAbilityInstancingPolicy::SingleInstance;

	/* These tags must be present on the owning ability component for this ability to activate. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Activation")
	FGameplayTagContainer ActivationRequiredTags;

	/* These tags must NOT be present on the owning ability component for this ability to activate. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Activation")
	FGameplayTagContainer ActivationBlockingTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability|Activation")
	float Cooldown = 0.0f;

	/* If set, this ability will only activate if it receives an ActivationContext of this struct type. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Activation")
	UScriptStruct* RequiredContextType;

	/**
	 * This ability will fail to activate if the avatar actor of the ability component is not one of these types.
	 * If left empty any (or null) avatar actor will be allowed.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Activation")
	TArray<TSubclassOf<AActor>> AvatarTypeFilter;

	/* If true, the owning ability component must have this ability granted to it for this ability to activate. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Activation")
	bool bRequireGrantToActivate = true;

	/* Tags that can be used to classify this ability. e.g. "Melee", "Ranged", "AOE", etc. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability|Tags")
	FGameplayTagContainer AbilityTags;

	/* These tags are applied when this ability is activated and removed when it ends. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability|Tags")
	FGameplayTagContainer TemporarilyAppliedTags;

	/* These tags are applied when this ability is activated and not automatically removed when it ends. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability|Tags")
	FGameplayTagContainer PermanentlyAppliedTags;

	/**
	 * Use this function to add custom rules for when this ability can activate.
	 * E.g. Check for activation cost like mana, stamina, etc.
	 * @param ActivationContext The context passed to this ability when it was activated. Can be empty.
	 * @return True if the ability can activate, false otherwise
	 */
	UFUNCTION(BlueprintNativeEvent)
	bool CanActivate(FInstancedStruct ActivationContext);
	virtual bool CanActivate_Implementation(FInstancedStruct ActivationContext);

	/**
	 * If we try to cancel the ability, this function is called to check if it can be cancelled.
	 * @return True if the ability can be cancelled, false otherwise
	 */
	UFUNCTION(BlueprintNativeEvent)
	bool CanCancel();
	virtual bool CanCancel_Implementation();

	UFUNCTION()
	bool ActivateAbility(FGuid AbilityID, FInstancedStruct ActivationContext);

	/**
	 * Use this function to activate abilities within this ability. SubAbilities don't support replication and you can
	 * think of them as the smallest unit of work an ability can do. For example, a good candidate for a sub ability is
	 * a montage playing ability. The input context would be the montage to play + any other data needed to play the montage.
	 * The output would be if the montage completed successfully or not. In the parent ability you could then use the end result
	 * for a StateSnapshot comparison between Server and Client.
	 * @param AbilityClass The class of the ability to activate
	 * @param ActivationContext Context to pass to the ability
	 * @param CancelIfParentEnds If this ability ends, should the SubAbility be cancelled?
	 * @param CancelIfParentCancels If this ability is cancelled, should the SubAbility be cancelled?
	 * @param SubAbilityActivationPolicy
	 * @return The ID of the sub ability
	 */
	UFUNCTION(BlueprintCallable, meta = (AdvancedDisplay = 2))
	FGuid ActivateSubAbility(
		TSubclassOf<USimpleGameplayAbility> AbilityClass,
		FInstancedStruct ActivationContext,
		bool CancelIfParentEnds = true,
		bool CancelIfParentCancels = true,
		ESubAbilityActivationPolicy SubAbilityActivationPolicy = ESubAbilityActivationPolicy::NoReplication);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "AbilityComponent|Utility")
	void OnTick(float DeltaTime);
	virtual void OnTick_Implementation(float DeltaTime);

	/**
	 * A generic function to end the ability. This function should be called by the ability itself when it's done.
	 * @param EndStatus A custom tag that describes the reason for ending the ability
	 * @param EndingContext Optional context to pass to the OnEnd event
	 */
	UFUNCTION(BlueprintCallable, meta = (AdvancedDisplay = 1))
	void EndAbility(FGameplayTag EndStatus, FInstancedStruct EndingContext);

	UFUNCTION(BlueprintCallable, meta = (AdvancedDisplay = 1))
	void CancelAbility(FGameplayTag CancelStatus, FInstancedStruct CancelContext, bool ForceCancel = false);

	/* Override these functions in your ability blueprint */

	/**
	 * This function is called if CanActivate returns true and runs before OnActivate is called.
	 * Use this function to prepare the ability for activation, consume resources, etc.
	 * @param ActivationContext The context passed to this ability when it was activated. Can be empty.
	 */
	UFUNCTION(BlueprintNativeEvent)
	void PreActivate(FInstancedStruct ActivationContext);
	virtual void PreActivate_Implementation(FInstancedStruct ActivationContext);

	/**
	 * Called after CanActivate returns true and PreActivate has been called.
	 * This is where the ability should do its main work.
	 * @param ActivationContext The context passed to this ability when it was activated. Can be empty.
	 */
	UFUNCTION(BlueprintImplementableEvent)
	void OnActivate(FInstancedStruct ActivationContext);

	UFUNCTION(BlueprintImplementableEvent)
	void OnEnd(FGameplayTag EndingStatus, FInstancedStruct EndingContext, bool WasCancelled);

	virtual void ClientResolvePastState(FGameplayTag StateTag, FSimpleAbilitySnapshot AuthorityState,
	                                    FSimpleAbilitySnapshot PredictedState) override;

	/* Utility functions */
	UFUNCTION(BlueprintCallable)
	void SendEvent(FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct EventContext, ESimpleEventReplicationPolicy
	                      ReplicationPolicy) const;

	UFUNCTION(BlueprintCallable)
	AActor* GetAvatarActor() const;

	UFUNCTION(BlueprintCallable, meta = (DeterminesOutputType = "AvatarClass", HideSelfPin))
	AActor* GetAvatarActorAs(TSubclassOf<AActor> AvatarClass, bool& IsValid) const;

	UFUNCTION(BlueprintCallable)
	void ApplyAttributeModifierToTarget(USimpleGameplayAbilityComponent* TargetComponent,
	                                    TSubclassOf<USimpleAttributeModifier> ModifierClass, FInstancedStruct Context,
	                                    FGuid& ModifierID);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool IsAbilityActive() const;

	// FTickableGameObject overrides
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;
	virtual ETickableTickType GetTickableTickType() const override;

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

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FInstancedStruct GetActivationContext() const;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool WasActivatedOnServer() const;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool WasActivatedOnClient() const;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	EAbilityServerRole GetServerRole() const;

protected:
	virtual UWorld* GetWorld() const override;

private:
	void EndAbilityInternal(FGameplayTag Status, FInstancedStruct Context, bool WasCancelled);
	// Used to keep track of sub abilities which this ability has created which need to be ended when this ability ends
	TArray<FGuid> EndOnEndedSubAbilities;
	// Used to keep track of sub abilities which this ability has created which need to be ended when this ability cancels
	TArray<FGuid> EndOnCancelledSubAbilities;

	bool MeetsActivationRequirements(FInstancedStruct& ActivationContext);
	bool bIsAbilityActive = false;
	FInstancedStruct CachedActivationContext;
};
