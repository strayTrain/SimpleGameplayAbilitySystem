#pragma once

#include "CoreMinimal.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleAbilityBase/SimpleAbilityBase.h"
#include "SimpleGameplayAbility.generated.h"

UCLASS(Blueprintable)
class SIMPLEGAMEPLAYABILITYSYSTEM_API USimpleGameplayAbility : public USimpleAbilityBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Activation")
	EAbilityActivationPolicy ActivationPolicy = EAbilityActivationPolicy::LocalOnly;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Activation")
	EAbilityInstancingPolicy InstancingPolicy = EAbilityInstancingPolicy::SingleInstance;

	/* These tags must be present on the owning ability component for this ability to activate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability|Activation")
	FGameplayTagContainer ActivationRequiredTags;

	/* These tags must NOT be present on the owning ability component for this ability to activate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability|Activation")
	FGameplayTagContainer ActivationBlockingTags;

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
	bool ActivateAbility(FInstancedStruct ActivationContext);

	UFUNCTION(BlueprintCallable)
	FGuid ActivateSubAbility(
		TSubclassOf<USimpleGameplayAbility> AbilityClass,
		FInstancedStruct ActivationContext,
		bool ShouldOverrideActivationPolicy = false,
		EAbilityActivationPolicy OverridePolicy = EAbilityActivationPolicy::LocalOnly,
		bool EndIfParentEnds = true,
		bool EndIfParentCancels = true); 
	
	/**
	 * A generic function to end the ability. This function should be called by the ability itself when it's done.
	 * @param EndStatus A custom tag that describes the reason for ending the ability
	 * @param EndingContext Optional context to pass to the OnEnd event
	 */
	UFUNCTION(BlueprintCallable, meta = (AdvancedDisplay = 1))
	void EndAbility(FGameplayTag EndStatus, FInstancedStruct EndingContext);

	/**
	 * A shortcut function to end the ability with EndStatus "SimpleGAS.Events.Ability.AbilityEndedSuccessfully".
	 * @param EndingContext Optional context to pass to the OnEnd event
	 */
	UFUNCTION(BlueprintCallable, meta = (AdvancedDisplay = 1))
	void EndSuccess(FInstancedStruct EndingContext);

	/**
	 * A shortcut function to end the ability with EndStatus "SimpleGAS.Events.Ability.AbilityCancelled".
	 * @param EndingContext Optional context to pass to the OnEnd event
	 */
	UFUNCTION(BlueprintCallable, meta = (AdvancedDisplay = 1))
	void EndCancel(FInstancedStruct EndingContext);

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
	void OnEnd(FGameplayTag EndingStatus, FInstancedStruct EndingContext);
	
	UFUNCTION(BlueprintImplementableEvent)
	void OnClientReceivedAuthorityState(FGameplayTag StateTag, FSimpleAbilitySnapshot AuthorityState, FSimpleAbilitySnapshot PredictedState);
	virtual void ClientResolvePastState(FGameplayTag StateTag, FSimpleAbilitySnapshot AuthorityState, FSimpleAbilitySnapshot PredictedState) override;

	/* Utility functions */
	UFUNCTION(BlueprintCallable, meta = (DeterminesOutputType = "AvatarClass", HideSelfPin))
	AActor* GetAvatarActorAs(TSubclassOf<AActor> AvatarClass) const;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool IsAbilityActive() const;
	
	UFUNCTION(BlueprintCallable, BlueprintPure)
	double GetActivationTime() const;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FInstancedStruct GetActivationContext() const;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool WasActivatedOnServer() const;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool WasActivatedOnClient() const;
protected:
	virtual UWorld* GetWorld() const override;
	
private:
	// Used to keep track of sub abilities which this ability has created which need to be ended when this ability ends
	TArray<FGuid> EndOnEndedSubAbilities;
	// Used to keep track of sub abilities which this ability has created which need to be ended when this ability cancels
	TArray<FGuid> EndOnCancelledSubAbilities;
	
	bool MeetsTagRequirements() const;
	bool bIsAbilityActive = false;
	FInstancedStruct CachedActivationContext;
};
