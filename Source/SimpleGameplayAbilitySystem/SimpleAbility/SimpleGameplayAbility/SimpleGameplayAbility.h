#pragma once

#include "CoreMinimal.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleAbilityBase/SimpleAbilityBase.h"
#include "Tickable.h"
#include "SimpleGameplayAbilitySystem/SimpleGameplayAbilityComponent/SimpleAbilityComponentTypes.h"
#include "SimpleGameplayAbility.generated.h"

UCLASS(Blueprintable, BlueprintType)
class SIMPLEGAMEPLAYABILITYSYSTEM_API USimpleGameplayAbility : public USimpleAbilityBase
{
	GENERATED_BODY()

public:
	/* Properties */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Policy")
	EAbilityActivationPolicy ActivationPolicy = EAbilityActivationPolicy::LocalOnly;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Policy")
	EAbilityInstancingPolicy InstancingPolicy = EAbilityInstancingPolicy::SingleInstance;
	
	/* These tags must be present on the owning ability component for this ability to activate. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Requirements")
	FGameplayTagContainer ActivationRequiredTags;

	/* These tags must NOT be present on the owning ability component for this ability to activate. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Requirements")
	FGameplayTagContainer ActivationBlockingTags;
	
	/* If set, this ability will only activate if these struct types are present in the activation context */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Requirements")
	TArray<UScriptStruct*> RequiredContextTypes;

	/**
	 * This ability will fail to activate if the avatar actor of the ability component is not one of these types.
	 * If left empty any (or null) avatar actor will be allowed.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Requirements")
	TArray<TSubclassOf<AActor>> AvatarTypeFilter;

	/* If true, the owning ability component must have this ability granted to it for this ability to activate. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Requirements")
	bool RequireGrantToActivate = true;

	/* Tags that can be used to classify this ability. e.g. "Melee", "Ranged", "AOE", etc. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability|Tags")
	FGameplayTagContainer AbilityTags;

	/* These tags are applied when this ability is activated and removed when it ends. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability|Tags")
	FGameplayTagContainer TemporarilyAppliedTags;

	/* These tags are applied when this ability is activated and not automatically removed when it ends. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability|Tags")
	FGameplayTagContainer PermanentlyAppliedTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Tick")
	bool CanAbilityTick = false;
	
	/* SimpleAbilityBase overrides */
	virtual bool CanActivate(USimpleGameplayAbilityComponent* ActivatingAbilityComponent, const FAbilityContextCollection ActivationContext) override;
	virtual bool Activate(USimpleGameplayAbilityComponent* ActivatingAbilityComponent, const FGuid NewAbilityID, const FAbilityContextCollection ActivationContext) override;
	virtual void Tick(float DeltaTime) override { OnTick(DeltaTime); }

	virtual void Cancel(FGameplayTag CancelStatus, FInstancedStruct CancelContext) override;
	virtual void End(FGameplayTag EndStatus, FInstancedStruct EndContext) override;

	virtual void TakeSnapshotInternal(const FInstancedStruct SnapshotData, const FOnSnapshotResolved& OnResolved) override;
	
	/* Implementable functions for child classes */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Ability")
	bool CanAbilityActivate(const USimpleGameplayAbilityComponent* ActivatingAbilityComponent, const FAbilityContextCollection ActivationContext);
	virtual bool CanAbilityActivate_Implementation(const USimpleGameplayAbilityComponent* ActivatingAbilityComponent, const FAbilityContextCollection ActivationContext) { return true; }

	UFUNCTION(BlueprintNativeEvent, Category = "Ability")
	void OnPreActivate();
	virtual void OnPreActivate_Implementation() {}
	
	UFUNCTION(BlueprintNativeEvent, Category = "Ability")
	void OnActivate();
	virtual void OnActivate_Implementation() {}

	UFUNCTION(BlueprintNativeEvent, Category = "Ability")
	void OnTick(float DeltaTime);
	virtual void OnTick_Implementation(float DeltaTime) {}
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Ability")
	void OnEnd(FGameplayTag EndStatus, FInstancedStruct EndingContext);
	virtual void OnEnd_Implementation(FGameplayTag EndStatus, FInstancedStruct EndingContext) {}
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Ability")
	void OnCancel(FGameplayTag CancelStatus, FInstancedStruct CancellationContext);
	virtual void OnCancel_Implementation(FGameplayTag CancelStatus, FInstancedStruct CancellationContext) {}

	UFUNCTION(BlueprintCallable, Category = "Ability", meta=(AdvancedDisplay = 1))
	void EndAbility(FGameplayTag EndStatus, FInstancedStruct EndingContext)
	{
		if (!IsActive)
		{
			return;
		}
		
		End(EndStatus, EndingContext);
	}

	UFUNCTION(BlueprintCallable, Category = "Ability", meta=(AdvancedDisplay = 1))
	void CancelAbility(const FGameplayTag CancelStatus, const FInstancedStruct CancelContext)
	{
		if (!IsActive)
		{
			return;
		}
		
		Cancel(CancelStatus, CancelContext);
	}

	UFUNCTION(BlueprintNativeEvent, Category = "Abilities")
	void OnGranted(USimpleGameplayAbilityComponent* GrantedAbilityComponent);
	virtual void OnGranted_Implementation(USimpleGameplayAbilityComponent* GrantedAbilityComponent) {}
    
	// Static wrapper to call the function on the class default object
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	static void OnGrantedStatic(TSubclassOf<USimpleGameplayAbility> AbilityClass, USimpleGameplayAbilityComponent* GrantedAbilityComponent);
	
	/* Utility functions */
	
	/**
	 * Use this function to activate abilities within this ability. SubAbilities don't support replication and you can
	 * think of them as the smallest unit of work an ability can do. For example, a good candidate for a sub ability is
	 * a montage playing ability. The input context would be the montage to play + any other data needed to play the montage.
	 * The output would be if the montage completed successfully or not. In the parent ability you could then use the end result
	 * for a StateSnapshot comparison between Server and Client.
	 * @param AbilityClass The class of the ability to activate
	 * @param ActivationContext Context to pass to the ability
	 * @param CancellationPolicy If the parent ability ends, should the sub ability be cancelled?
	 * @return The AbilityID of the sub ability
	 */
	UFUNCTION(BlueprintCallable, meta = (AdvancedDisplay = 2))
	FGuid ActivateSubAbility(
		TSubclassOf<USimpleGameplayAbility> AbilityClass,
		FAbilityContextCollection ActivationContext,
		ESubAbilityCancellationPolicy CancellationPolicy = ESubAbilityCancellationPolicy::CancelOnParentAbilityEndedOrCancelled);

	UFUNCTION(BlueprintCallable)
	void SendEvent(FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct EventContext, ESimpleEventReplicationPolicy ReplicationPolicy);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	AActor* GetAvatarActor() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, meta = (DeterminesOutputType = "AvatarClass", HideSelfPin))
	AActor* GetAvatarActorAs(TSubclassOf<AActor> AvatarClass, bool& IsValid) const;

	UFUNCTION(BlueprintCallable)
	void ApplyAttributeModifierToTarget(USimpleGameplayAbilityComponent* TargetComponent,
	                                    TSubclassOf<USimpleAttributeModifier> ModifierClass, float Magnitude,
	                                    FAbilityContextCollection ModifierContext, FGuid& ModifierID);
	
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
	virtual bool IsTickable() const override { return CanAbilityTick && IsActive && GetWorld(); }
	
private:
	// Called when the ability is ended or cancelled. Removes temporary tags and cancels any sub abilities configured to cancel 
	void OnAbilityStopped(FInstancedStruct& StopContext, bool WasCancelled);
	
	// Used to keep track of sub abilities which this ability has created which need to be ended when this ability ends/cancels
	TArray<FActivatedSubAbility> ActivatedSubAbilities;
};
