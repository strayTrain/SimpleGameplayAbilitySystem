#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SimpleAbilityComponentTypes.h"
#include "Components/ActorComponent.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleAbilityTypes.h"
#include "SimpleGameplayAbilityComponent.generated.h"

class USimpleGameplayAbility;

UCLASS(Blueprintable, ClassGroup=(AbilityComponent), meta=(BlueprintSpawnableComponent))
class SIMPLEGAMEPLAYABILITYSYSTEM_API USimpleGameplayAbilityComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USimpleGameplayAbilityComponent();
	
	UPROPERTY(Replicated)
	AActor* AvatarActor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated)
	FGameplayTagContainer GameplayTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_FloatAttributes, Category = "Gameplay Attributes", meta = (TitleProperty = "AttributeName"))
	TArray<FFloatAttribute> FloatAttributes;

	UFUNCTION()
	void OnRep_FloatAttributes();
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_StructAttributes, Category = "Gameplay Attributes", meta = (TitleProperty = "AttributeName"))
	TArray<FStructAttribute> StructAttributes;

	UFUNCTION()
	void OnRep_StructAttributes();
	
	UFUNCTION(BlueprintCallable)
	AActor* GetAvatarActor() const { return AvatarActor; }
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void SetAvatarActor(AActor* NewAvatarActor) { AvatarActor = NewAvatarActor; }

	UFUNCTION(BlueprintCallable)
	void AddGameplayTags(FGameplayTagContainer Tags);

	UFUNCTION(BlueprintCallable)
	void RemoveGameplayTags(FGameplayTagContainer Tags);
	
	UFUNCTION(BlueprintCallable, meta=(AdvancedDisplay=2))
	bool ActivateAbility(TSubclassOf<USimpleGameplayAbility> AbilityClass, FInstancedStruct AbilityContext, bool OverrideActivationPolicy, EAbilityActivationPolicy ActivationPolicy);

	UFUNCTION(Server, Reliable)
	void ServerActivateAbility(TSubclassOf<USimpleGameplayAbility> AbilityClass, FInstancedStruct AbilityContext, FGuid AbilityInstanceID);
	
	/* Replicated Event Functions */
	
	/**
	 * Sends an event through the SimpleEventSubsystem and optionally replicates it.
	 * @param ReplicationPolicy How to replicate the event.
	 */
	UFUNCTION(BlueprintCallable)
	void SendEvent(FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload, ESimpleEventReplicationPolicy ReplicationPolicy);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, BlueprintCallable)
	double GetServerTime() const;
	virtual double GetServerTime_Implementation() const;

	UFUNCTION(BlueprintCallable)
	bool HasAuthority() const { return GetOwner()->HasAuthority(); }

	UFUNCTION()
	void PushAbilityState(FGuid AbilityInstanceID, FSimpleAbilitySnapshot State);
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_AuthorityAbilityStates)
	TArray<FAbilityState> AuthorityAbilityStates;
	UPROPERTY(BlueprintReadOnly)
	TArray<FAbilityState> LocalAbilityStates;
	
	UFUNCTION()
	void OnRep_AuthorityAbilityStates();

protected:
	bool ActivateAbilityInternal(TSubclassOf<USimpleGameplayAbility>& AbilityClass, const FInstancedStruct& AbilityContext, FGuid AbilityInstanceID);
	void AddNewAbilityState(const TSubclassOf<USimpleGameplayAbility>& AbilityClass, const FInstancedStruct& AbilityContext, FGuid AbilityInstanceID, bool DidActivateSuccessfully);
	USimpleGameplayAbility* GetAbilityInstance(FGuid AbilityInstanceID);
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

private:
	TArray<USimpleGameplayAbility*> InstancedAbilities;
};
