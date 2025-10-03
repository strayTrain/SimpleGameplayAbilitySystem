// Copyright 2025 Ahmed Elgoni

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "WaitForSubAbility.generated.h"

class USimpleAbilityBase;
class USimpleGameplayAbility;
class USimpleSubAbility;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FWaitForSubAbilityDelegate, FGameplayTag, ResultTag, FInstancedStruct, ResultContext);

/**
 * Activates a sub-ability locally and waits for it to complete.
 * This is a simple async action for executing sub-abilities without any replication concerns.
 *
 * Example usage: Playing an animation montage, applying a local visual effect, or any other
 * non-replicated sub-ability that needs to complete before the parent ability continues.
 */
UCLASS()
class SIMPLEGAMEPLAYABILITYSYSTEM_API UWaitForSubAbility : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	// Called when the sub-ability completes successfully
	UPROPERTY(BlueprintAssignable)
	FWaitForSubAbilityDelegate OnEnded;

	// Called when the sub-ability is cancelled or fails
	UPROPERTY(BlueprintAssignable)
	FWaitForSubAbilityDelegate OnCancelled;

	/**
	 * Activates a sub-ability and waits for it to complete.
	 * @param ParentAbility The ability that is activating the sub-ability
	 * @param SubAbilityClass The class of sub-ability to activate
	 * @param ActivationContext Context to pass to the sub-ability on activation
	 */
	UFUNCTION(BlueprintCallable, Category = "SimpleGAS|Async Functions", meta=(DefaultToSelf="ParentAbility", BlueprintInternalUseOnly=true))
	static UWaitForSubAbility* WaitForSubAbility(
		USimpleGameplayAbility* ParentAbility,
		TSubclassOf<USimpleSubAbility> SubAbilityClass,
		FInstancedStruct ActivationContext);

	virtual void Activate() override;
	virtual void SetReadyToDestroy() override;

protected:
	UFUNCTION()
	void OnSubAbilityEnded(USimpleAbilityBase* AbilityInstance, FGameplayTag StopStatus, FInstancedStruct StopContext);

	UFUNCTION()
	void OnSubAbilityCancelled(USimpleAbilityBase* AbilityInstance, FGameplayTag StopStatus, FInstancedStruct StopContext);

private:
	TWeakObjectPtr<USimpleGameplayAbility> ParentAbilityInstance;
	TSubclassOf<USimpleSubAbility> SubAbilityClassToActivate;
	FInstancedStruct SubAbilityContext;

	void CleanupAndFinish();
};
