#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "InstancedStruct.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleAbilityTypes.h"
#include "SimpleGameplayAbilitySystem/DefaultTags/DefaultTags.h"
#include "SimpleGameplayAbilitySystem/SimpleAbilityComponent/SimpleAbilityComponentTypes.h"
#include "UObject/Object.h"
#include "SimpleAbility.generated.h"

class USimpleGameplayAttributes;
class UAbilityStateResolver;
class USimpleGameplayAbilityState;
class USimpleAbilityComponent;

UCLASS(Blueprintable, ClassGroup = (SimpleGAS))
class SIMPLEGAMEPLAYABILITYSYSTEM_API USimpleAbility : public UObject, public FTickableGameObject
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly)
	FSimpleGameplayAbilityConfig AbilityConfig = FSimpleGameplayAbilityConfig();

	UPROPERTY(BlueprintReadOnly, Replicated)
	FGuid AbilityInstanceID;
	
	UPROPERTY(BlueprintReadOnly)
	USimpleAbilityComponent* OwningAbilityComponent;
	
	UFUNCTION(BlueprintCallable)
	void InitializeAbility(USimpleAbilityComponent* InOwningAbilityComponent, FGuid InAbilityInstanceID, double InActivationTime);

	UFUNCTION(BlueprintNativeEvent)
	bool CanActivate(FInstancedStruct AbilityContext);

	UFUNCTION(BlueprintCallable)
	bool Activate(FInstancedStruct AbilityContext);
	
	UFUNCTION(BlueprintCallable, Category = "SimpleGAS")
	void EndAbility(FGameplayTag EndStatus);
	
	UFUNCTION(BlueprintCallable, Category = "SimpleGAS")
	void EndAbilitySuccess() { EndAbility(FDefaultTags::AbilityEndedSuccessfully); }

	UFUNCTION(BlueprintCallable, Category = "SimpleGAS")
	void EndAbilityCancel() { EndAbility(FDefaultTags::AbilityCancelled); }

	/* Override these functions in your ability blueprint */
	
	UFUNCTION(BlueprintImplementableEvent, Category = "SimpleGAS|Ability")
	void OnActivate(FInstancedStruct AbilityContext);
	
	/**
	 * OnTick is called every frame while the ability is active. Override this function in your ability blueprint to add custom logic.
	 * @param DeltaTime The time since the last tick.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "SimpleGAS|Ability")
	void OnTick(float DeltaTime);
	
	UFUNCTION(BlueprintImplementableEvent, Category = "SimpleGAS|Ability")
	void OnEnd(FGameplayTag EndStatus);

	/* State snapshotting */
	
	UFUNCTION(BlueprintCallable, Category = "SimpleGAS|Ability", meta=(AdvancedDisplay=3))
	void TakeStateSnapshot(
		FGameplayTag SnapshotTag, FInstancedStruct SnapshotData,
		FResolveStateMispredictionDelegate OnResolveState,
		bool UseCustomStateResolver, TSubclassOf<UAbilityStateResolver> CustomStateResolverClass);

	UFUNCTION(BlueprintInternalUseOnly)
	void OnAuthorityStateSnapshotEventReceived(FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload);

	/* Utility functions */

	UFUNCTION(BlueprintCallable, Category = "SimpleGAS|Ability")
	void SendEvent(FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload, ESimpleEventReplicationPolicy ReplicationPolicy) const;
	
	/**
	 * A convenience function to get the avatar actor as a specific class. Internally does a cast, so I recommend caching
	 * this value if you are going call this node frequently (e.g. OnTick)
	 * @param AvatarClass The class to set the output pin to. If the avatar actor is not of this class, the output will be nullptr.
	 * @return The avatar actor as the specified class.
	 */
	UFUNCTION(BlueprintCallable, Category = "SimpleGAS|Ability", meta = (DeterminesOutputType = "AvatarClass", HideSelfPin))
	AActor* GetAvatarActorAs(TSubclassOf<AActor> AvatarClass) const;
	
	UFUNCTION(BlueprintCallable, Category = "SimpleGAS|Ability")
	bool IsAbilityActive() const { return bIsAbilityActive; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SimpleGAS|Ability")
	bool HasAuthority();

	/**
	 * This function returns the server time that this ability was activated (this is set in InitializeAbility).
	 * The activation time comes from SimpleGameplayAbilityComponent::GetServerTime.
	 * @return The time on the server that this ability was activated.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SimpleGAS|Ability")
	double GetActivationTime() const { return ActivationTime; }

protected:
	UPROPERTY(BlueprintReadOnly, Replicated)
	TArray<FAbilityState> AuthorityStateHistory;
	UPROPERTY(BlueprintReadOnly)
	TArray<FAbilityState> PredictedStateHistory;

	virtual UWorld* GetWorld() const override;
	virtual bool IsSupportedForNetworking() const override { return true; }
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

private:
	bool bIsInitialized = false;
	bool bIsAbilityActive = false;
	double ActivationTime = 0.0;

	/* Tick Support */
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(USimpleGameplayAbility, STATGROUP_Tickables); }
	virtual ETickableTickType GetTickableTickType() const override;
	// Track The last frame number we were ticked to avoid double ticking
	uint32 LastTickFrame = INDEX_NONE;
	
	/* Utility functions */
	FString GetAbilityName() const;
	void SendActivationStateChangeEvent(FGameplayTag EventTag, FGameplayTag DomainTag) const;
	int32 FindStateIndexByTag(FGameplayTag StateTag) const;
	int32 FindPredictedStateIndexByTag(FGameplayTag StateTag) const;
};
