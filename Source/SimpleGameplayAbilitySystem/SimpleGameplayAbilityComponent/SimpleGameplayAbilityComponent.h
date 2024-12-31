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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "AbilityComponent|Tags")
	FGameplayTagContainer GameplayTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_FloatAttributes, Category = "AbilityComponent|Attributes", meta = (TitleProperty = "AttributeName"))
	TArray<FFloatAttribute> FloatAttributes;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_StructAttributes, Category = "AbilityComponent|Attributes", meta = (TitleProperty = "AttributeName"))
	TArray<FStructAttribute> StructAttributes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_AuthorityAbilityStates, Category = "AbilityComponent|State")
	TArray<FAbilityState> AuthorityAbilityStates;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilityComponent|State")
	TArray<FAbilityState> LocalAbilityStates;
	
	UFUNCTION(BlueprintCallable, Category = "AbilityComponent|AvatarActor")
	AActor* GetAvatarActor() const { return AvatarActor; }
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "AbilityComponent|AvatarActor")
	void SetAvatarActor(AActor* NewAvatarActor) { AvatarActor = NewAvatarActor; }

	UFUNCTION(BlueprintCallable, Category = "AbilityComponent|Tags")
	void AddGameplayTags(FGameplayTagContainer Tags);

	UFUNCTION(BlueprintCallable, Category = "AbilityComponent|Tags")
	void RemoveGameplayTags(FGameplayTagContainer Tags);
	
	UFUNCTION(BlueprintCallable, meta=(AdvancedDisplay=2), Category = "AbilityComponent|AbilityActivation")
	bool ActivateAbility(TSubclassOf<USimpleGameplayAbility> AbilityClass, FInstancedStruct AbilityContext, bool OverrideActivationPolicy, EAbilityActivationPolicy ActivationPolicy);

	UFUNCTION(Server, Reliable)
	void ServerActivateAbility(TSubclassOf<USimpleGameplayAbility> AbilityClass, FInstancedStruct AbilityContext, FGuid AbilityInstanceID);

	UFUNCTION(BlueprintCallable, Category = "AbilityComponent|AbilityActivation")
	bool CancelAbility(FGuid AbilityInstanceID, FInstancedStruct CancellationContext);

	UFUNCTION(BlueprintCallable, Category = "AbilityComponent|AbilityActivation")
	TArray<FGuid> CancelAbilitiesWithTags(FGameplayTagContainer Tags, FInstancedStruct CancellationContext);
	
	UFUNCTION()
	void AddAbilityStateSnapshot(FGuid AbilityInstanceID, FSimpleAbilitySnapshot State);
	
	/* Replicated Event Functions */
	
	UFUNCTION(BlueprintCallable, Category = "AbilityComponent|Events")
	void SendEvent(FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload, ESimpleEventReplicationPolicy ReplicationPolicy);

	/* Utility Functions */
	
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, BlueprintCallable, Category = "AbilityComponent|Utility")
	double GetServerTime() const;
	virtual double GetServerTime_Implementation() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AbilityComponent|Utility")
	FAbilityState GetAbilityState(FGuid AbilityInstanceID, bool& WasFound) const;
	
	UFUNCTION(BlueprintCallable, Category = "AbilityComponent|Utility")
	bool HasAuthority() const { return GetOwner()->HasAuthority(); }

protected:
	bool ActivateAbilityInternal(TSubclassOf<USimpleGameplayAbility>& AbilityClass, const FInstancedStruct& AbilityContext, FGuid AbilityInstanceID);
	void AddNewAbilityState(const TSubclassOf<USimpleGameplayAbility>& AbilityClass, const FInstancedStruct& AbilityContext, FGuid AbilityInstanceID, bool DidActivateSuccessfully);
	USimpleGameplayAbility* GetAbilityInstance(FGuid AbilityInstanceID);
	
	UFUNCTION()
	void OnRep_AuthorityAbilityStates();
	UFUNCTION()
	void OnRep_FloatAttributes();
	UFUNCTION()
	void OnRep_StructAttributes();
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

private:
	TArray<USimpleGameplayAbility*> InstancedAbilities;
};
