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
	EAbilityInstancingPolicy InstancingPolicy = EAbilityInstancingPolicy::SingleInstanceCancellable;

	/* These tags must be present on the owning ability component for this ability to activate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability|Activation")
	FGameplayTagContainer ActivationRequiredTags;

	/* These tags must NOT be present on the owning ability component for this ability to activate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability|Activation")
	FGameplayTagContainer ActivationBlockingTags;

	/* If true, the owning ability component must have this ability granted to it for it to activate. */
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
	 * A list of events that can trigger this ability to activate.
	 * The ability must be granted to the owning ability component for it to activate.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Activation Events")
	TArray<FAbilityEventActivationConfig> ActivationEvents;
	
	/**
	 * Use this function to add custom rules for when this ability can activate.
	 * E.g. Check for activation cost like mana, stamina, etc.
	 * @param ActivationContext The context passed to this ability when it was activated. Can be empty.
	 * @return True if the ability can activate, false otherwise
	 */
	UFUNCTION(BlueprintNativeEvent)
	bool CanActivate(FInstancedStruct ActivationContext);

	UFUNCTION()
	bool ActivateAbility(FInstancedStruct ActivationContext);
	
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
	UFUNCTION(BlueprintImplementableEvent)
	void PreActivate(FInstancedStruct ActivationContext);
	
	/**
	 * Called after CanActivate returns true and PreActivate has been called.
	 * This is where the ability should do its main work.
	 * @param ActivationContext The context passed to this ability when it was activated. Can be empty.
	 */
	UFUNCTION(BlueprintImplementableEvent)
	void OnActivate(FInstancedStruct ActivationContext);

	UFUNCTION(BlueprintImplementableEvent)
	void OnEnd(FGameplayTag EndStatus, FInstancedStruct EndingContext);

	/* Utility functions */
	UFUNCTION(BlueprintCallable, meta = (DeterminesOutputType = "AvatarClass", HideSelfPin))
	AActor* GetAvatarActorAs(TSubclassOf<AActor> AvatarClass) const;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool IsAbilityActive() const;
	
	UFUNCTION(BlueprintCallable, BlueprintPure)
	double GetActivationTime() const;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FInstancedStruct GetActivationContext() const;

protected:
	virtual UWorld* GetWorld() const override;
	
private:
	bool MeetsTagRequirements() const;
};
