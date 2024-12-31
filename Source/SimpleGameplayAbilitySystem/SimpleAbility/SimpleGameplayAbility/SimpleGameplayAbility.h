#pragma once

#include "CoreMinimal.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleAbilityBase/SimpleAbilityBase.h"
#include "SimpleGameplayAbility.generated.h"

UCLASS(Blueprintable)
class SIMPLEGAMEPLAYABILITYSYSTEM_API USimpleGameplayAbility : public USimpleAbilityBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	EAbilityActivationPolicy ActivationPolicy = EAbilityActivationPolicy::LocalOnly;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	EAbilityInstancingPolicy InstancingPolicy = EAbilityInstancingPolicy::SingleInstanceCancellable;
	
	UFUNCTION(BlueprintNativeEvent)
	bool CanActivate(FInstancedStruct AbilityContext);

	UFUNCTION(BlueprintCallable)
	bool Activate(FInstancedStruct ActivationContext);
	
	UFUNCTION(BlueprintCallable, meta = (AdvancedDisplay = 1))
	void EndAbility(FGameplayTag EndStatus, FInstancedStruct EndingContext);

	/* Override these functions in your ability blueprint */
	
	UFUNCTION(BlueprintImplementableEvent)
	void OnActivate(FInstancedStruct ActivationContext);

	UFUNCTION(BlueprintImplementableEvent)
	void OnEnd(FGameplayTag EndStatus, FInstancedStruct EndingContext);

	/* Utility functions */
	UFUNCTION(BlueprintCallable, meta = (DeterminesOutputType = "AvatarClass", HideSelfPin))
	AActor* GetAvatarActorAs(TSubclassOf<AActor> AvatarClass) const;
	
	UFUNCTION(BlueprintCallable)
	bool IsAbilityActive() const { return bIsAbilityActive; }
	
	UFUNCTION(BlueprintCallable, BlueprintPure)
	double GetActivationTime() const { return ActivationTime; }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FInstancedStruct GetActivationContext() const { return CurrentActivationContext; }

protected:
	FInstancedStruct CurrentActivationContext;
	virtual UWorld* GetWorld() const override;
	
private:
	bool bIsAbilityActive = false;
	double ActivationTime = 0.0;
};
