// Copyright 2025 Ahmed Elgoni

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "WaitForClientSubAbility.generated.h"

class USimpleAbilityBase;
class USimpleGameplayAbility;
class USimpleSubAbility;
class USimpleGameplayAbilityComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSubAbilityResultDelegate, FGameplayTag, ResultTag, FInstancedStruct, ResultContext);

/**
 * Activates a sub-ability on the client and waits for the result to be sent back to the server.
 * This is useful for abilities that need client input/confirmation before continuing on the server.
 *
 * Example usage: A DisplayConfirmation sub-ability shows a UI on the client and sends the user's
 * choice (confirm/cancel) back to the server so the server ability can continue with the result.
 */
UCLASS()
class SIMPLEGAMEPLAYABILITYSYSTEM_API UWaitForClientSubAbility : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	// Called when the sub-ability completes successfully and sends a result
	UPROPERTY(BlueprintAssignable)
	FSubAbilityResultDelegate OnEnded;

	// Called when the sub-ability is cancelled or fails
	UPROPERTY(BlueprintAssignable)
	FSubAbilityResultDelegate OnCancelled;

	// Called when the async node times out waiting for a result
	UPROPERTY(BlueprintAssignable)
	FSubAbilityResultDelegate OnTimeout;

	/**
	 * Activates a sub-ability on the client only and waits for it to send a result back to the server.
	 * @param ParentAbility The ability that is activating the sub-ability
	 * @param SubAbilityClass The class of sub-ability to activate
	 * @param ActivationContext Context to pass to the sub-ability on activation
	 * @param TimeoutDuration How long to wait for a result before timing out (0 = no timeout)
	 */
	UFUNCTION(BlueprintCallable, Category = "SimpleGAS|Async Functions", meta=(DefaultToSelf="ParentAbility", BlueprintInternalUseOnly=true))
	static UWaitForClientSubAbility* WaitForClientSubAbility(
		USimpleGameplayAbility* ParentAbility,
		TSubclassOf<USimpleSubAbility> SubAbilityClass,
		FInstancedStruct ActivationContext,
		float TimeoutDuration = 0.0f);

	virtual void Activate() override;
	virtual void SetReadyToDestroy() override;

protected:
	UFUNCTION()
	void OnEventReceived(FGameplayTag EventTag, FGuid AbilityID, FInstancedStruct EventContext);

	UFUNCTION()
	void OnSubAbilityEnded(USimpleAbilityBase* AbilityInstance, FGameplayTag StopStatus, FInstancedStruct StopContext);

	UFUNCTION()
	void OnSubAbilityCancelled(USimpleAbilityBase* AbilityInstance, FGameplayTag StopStatus, FInstancedStruct StopContext);

	UFUNCTION()
	void OnTimeoutExpired();

private:
	TWeakObjectPtr<USimpleGameplayAbility> ParentAbilityInstance;
	TSubclassOf<USimpleSubAbility> SubAbilityClassToActivate;
	FInstancedStruct SubAbilityContext;
	float Timeout;
	FGuid ExpectedAbilityID;

	FTimerHandle TimeoutTimerHandle;

	void CleanupAndFinish();
};