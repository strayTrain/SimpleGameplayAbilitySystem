//  Copyright 2025 Ahmed Elgoni

#pragma once

#include "CoreMinimal.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleAbilityBase/SimpleAbilityBase.h"
#include "SimpleSubAbility.generated.h"

class USimpleGameplayAbilityComponent;

UCLASS()
class SIMPLEGAMEPLAYABILITYSYSTEM_API USimpleSubAbility : public USimpleAbilityBase
{
	GENERATED_BODY()

public:
	/* Properties */

	UPROPERTY(BlueprintReadOnly, Category = "SimpleAbility|State")
	FGuid ParentAbilityID;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SimpleAbility")
	ESubAbilityCancellationPolicy CancellationPolicy = ESubAbilityCancellationPolicy::CancelOnParentAbilityEndedOrCancelled;

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
	
	/* Callable Functions */

	// Called by the AbilityComponent immediately after creating the ability instance
	void Initialize(USimpleGameplayAbility* ParentAbility, FGuid NewParentAbilityID);

	virtual bool CanActivateInternal() override;
	
	/* Utility functions */

	UFUNCTION(BlueprintCallable, BlueprintPure)
	AActor* GetAvatarActor() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, meta = (DeterminesOutputType = "AvatarClass", HideSelfPin))
	AActor* GetAvatarActorAs(TSubclassOf<AActor> AvatarClass, bool& IsValid) const;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	USimpleGameplayAbility* GetParentAbilityInstance() const { return ParentAbilityInstance; }
	
	UFUNCTION(BlueprintCallable, BlueprintPure)
	USimpleGameplayAbilityComponent* GetParentAbilityComponent() const;
	
	/* Event functions */

	// Sends an event to the server using the parent ability's ID
	UFUNCTION(BlueprintCallable, Category = "SimpleSubAbility|Events")
	void SendEventToServer(FGameplayTag EventTag, FInstancedStruct EventContext);

	// Sends an event to the client using the parent ability's ID
	UFUNCTION(BlueprintCallable, Category = "SimpleSubAbility|Events")
	void SendEventToClient(FGameplayTag EventTag, FInstancedStruct EventContext);

	// Sends an event locally using the parent ability's ID
	UFUNCTION(BlueprintCallable, Category = "SimpleSubAbility|Events")
	void SendEvent(FGameplayTag EventTag, FInstancedStruct EventContext);

protected:
	virtual bool IsTickable() const override { return CanTick && IsActive && GetWorld(); }
	
private:
	UPROPERTY()
	USimpleGameplayAbility* ParentAbilityInstance;
};
