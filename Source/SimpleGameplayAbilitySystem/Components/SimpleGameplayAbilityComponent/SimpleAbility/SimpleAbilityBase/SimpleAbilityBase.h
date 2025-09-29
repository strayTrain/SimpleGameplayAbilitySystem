#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleGameplayAbilityComponent/SimpleAbility/SimpleAbilityTypes.h"
#if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5)
	#include "StructUtils/InstancedStruct.h"
#else
	#include "InstancedStruct.h"
#endif
#include "UObject/Object.h"
#include "SimpleAbilityBase.generated.h"

UCLASS(Blueprintable, BlueprintType, Abstract)
class SIMPLEGAMEPLAYABILITYSYSTEM_API  USimpleAbilityBase : public UObject, public FTickableGameObject
{
	GENERATED_BODY()

public:
	/* Properties */
	
	UPROPERTY(BlueprintReadOnly, Category = "SimpleAbility|State")
	FInstancedStruct Context;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SimpleAbility")
	bool CanTick = false;

	// This is true in the time between OnActivate and OnEnd/Cancel
	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool IsActive;

	/* Event Dispatchers */

	UPROPERTY(BlueprintAssignable, Category = "SimpleAbility|Events")
	FAbilityActivationDelegate OnActivationSuccess;

	UPROPERTY(BlueprintAssignable, Category = "SimpleAbility|Events")
	FAbilityActivationDelegate OnActivationFailed;

	UPROPERTY(BlueprintAssignable, Category = "SimpleAbility|Events")
	FAbilityStoppedDelegate OnAbilityEnded;

	UPROPERTY(BlueprintAssignable, Category = "SimpleAbility|Events")
	FAbilityStoppedDelegate OnAbilityCancelled;

	/* Callable Functions */
	
	UFUNCTION(BlueprintCallable, Category = "Ability")
	bool ActivateAbility(FInstancedStruct ActivationContext);

	UFUNCTION(BlueprintCallable, Category = "Ability", meta=(AdvancedDisplay = "EndStatus,EndContext"))
	void EndAbility(FGameplayTag EndStatus, FInstancedStruct EndContext);

	UFUNCTION(BlueprintCallable, Category = "Ability", meta=(AdvancedDisplay = "EndStatus,EndContext"))
	void CancelAbility(FGameplayTag EndStatus, FInstancedStruct EndContext);
	
	/* Overridable functions */

	// Called after passing the CanActivate check but before calling OnActivate
	UFUNCTION(BlueprintNativeEvent, Category = "Ability")
	void OnPreActivate();
	virtual void OnPreActivate_Implementation() {}
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Ability")
	void OnActivate(const FInstancedStruct& ActivationContext);
	virtual void OnActivate_Implementation(const FInstancedStruct& ActivationContext) {}

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Ability")
	void OnTick(float DeltaTime);
	virtual void OnTick_Implementation(float DeltaTime) {}
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Ability")
	void OnCancel(FGameplayTag CancelStatus, FInstancedStruct CancelContext);
	virtual void OnCancel_Implementation(FGameplayTag CancelStatus, FInstancedStruct CancelContext) {}

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Ability")
	void OnEnd(FGameplayTag EndStatus, FInstancedStruct EndContext);
	virtual void OnEnd_Implementation(FGameplayTag EndStatus, FInstancedStruct EndContext) {}

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Ability")
	void OnCleanUpAbility();
	virtual void OnCleanUpAbility_Implementation() {};

protected:
	// Used by child C++ classes like SimpleGameplayAbility to add additional functionality to the overridable functions
	// because adding a blueprint node for CallParentFunction for every ability gets annoying
	virtual bool CanActivateInternal() { return true; }
	virtual void PreActivateInternal() {}
	virtual void AbilityEndedInternal(FInstancedStruct EndingContext, bool WasCancelled) {}
	
	virtual UWorld* GetWorld() const override;
	
	/* FTickableGameObject overrides */
	virtual bool IsTickable() const override { return CanTick && GetWorld() != nullptr; }
	virtual void Tick(float DeltaTime) override { OnTick(DeltaTime); }
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(USimpleGameplayAbility, STATGROUP_Tickables); }
	virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Conditional; }
};
