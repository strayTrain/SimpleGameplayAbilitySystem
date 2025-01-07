#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SimpleAbilityComponentTypes.h"
#include "Components/ActorComponent.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleAbilityTypes.h"
#include "SimpleGameplayAbilityComponent.generated.h"

class UAbilitySet;
class UAttributeSet;
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AbilityComponent|Attributes")
	TArray<UAttributeSet*> AttributeSets;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_FloatAttributes, Category = "AbilityComponent|Attributes", meta = (TitleProperty = "AttributeName"))
	TArray<FFloatAttribute> FloatAttributes;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_StructAttributes, Category = "AbilityComponent|Attributes", meta = (TitleProperty = "AttributeName"))
	TArray<FStructAttribute> StructAttributes;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AbilityComponent|Abilities")
	TArray<UAbilitySet*> AbilitySets;
	
	UPROPERTY(EditAnywhere, Replicated, BlueprintReadOnly, Category = "AbilityComponent|Abilities")
	TArray<TSubclassOf<USimpleGameplayAbility>> GrantedAbilities;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_AuthorityAbilityStates, Category = "AbilityComponent|State")
	TArray<FAbilityState> AuthorityAbilityStates;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilityComponent|State")
	TArray<FAbilityState> LocalAbilityStates;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_AuthorityAttributeStates, Category = "AbilityComponent|State")
	TArray<FAbilityState> AuthorityAttributeStates;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilityComponent|State")
	TArray<FAbilityState> LocalAttributeStates;
	
	UFUNCTION(BlueprintCallable, Category = "AbilityComponent|AvatarActor")
	AActor* GetAvatarActor() const { return AvatarActor; }
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "AbilityComponent|AvatarActor")
	void SetAvatarActor(AActor* NewAvatarActor) { AvatarActor = NewAvatarActor; }

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "AbilityComponent|Abilities")
	void GrantAbility(TSubclassOf<USimpleGameplayAbility> AbilityClass);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "AbilityComponent|Abilities")
	void RevokeAbility(TSubclassOf<USimpleGameplayAbility> AbilityClass);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "AbilityComponent|Attributes", meta = (AdvancedDisplay=1))
	void AddFloatAttribute(FFloatAttribute AttributeToAdd, bool OverrideValuesIfExists = true );

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "AbilityComponent|Attributes")
	void RemoveFloatAttribute(FGameplayTag AttributeTag);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "AbilityComponent|Attributes", meta = (AdvancedDisplay=1))
	void AddStructAttribute(FStructAttribute AttributeToAdd, bool OverrideValuesIfExists = true);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "AbilityComponent|Attributes")
	void RemoveStructAttribute(FGameplayTag AttributeTag);

	UFUNCTION(BlueprintCallable, Category = "AbilityComponent|Attributes")
	bool ApplyAttributeModifierToTarget(USimpleGameplayAbilityComponent* ModifierTarget, TSubclassOf<USimpleAttributeModifier> ModifierClass, FInstancedStruct ModifierContext);

	UFUNCTION(BlueprintCallable, Category = "AbilityComponent|Attributes")
	bool ApplyAttributeModifierToSelf(TSubclassOf<USimpleAttributeModifier> ModifierClass, FInstancedStruct ModifierContext);
	
	/**
	 * Adds a gameplay tag to this component. Upon being added a tag added event is sent to the event system.
	 * If Payload is not null, it gets sent with the event.
	 * @param Tag The tag to add to this component 
	 * @param Payload Optional payload that gets sent with the tag added event
	 */
	UFUNCTION(BlueprintCallable, Category = "AbilityComponent|Tags", meta = (AdvancedDisplay=1))
	void AddGameplayTag(FGameplayTag Tag, FInstancedStruct Payload);

	/**
	 * Removes a gameplay tag from this component. Upon being removed a tag removed event is sent to the event system.
	 * If Payload is not null, it gets sent with the event.
	 * @param Tag The tag to remove from this component 
	 * @param Payload Optional payload that gets sent with the tag removed event
	 */
	UFUNCTION(BlueprintCallable, Category = "AbilityComponent|Tags", meta = (AdvancedDisplay=1))
	void RemoveGameplayTag(FGameplayTag Tag, FInstancedStruct Payload);
	
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
	void SendEvent(FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload, AActor* Sender, ESimpleEventReplicationPolicy ReplicationPolicy);
	
	UFUNCTION(Server, Reliable)
	void ServerSendEvent(FGuid EventID, FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload, AActor* Sender, ESimpleEventReplicationPolicy ReplicationPolicy);

	UFUNCTION(Client, Reliable)
	void ClientSendEvent(FGuid EventID, FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload, AActor* Sender, ESimpleEventReplicationPolicy ReplicationPolicy);
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastSendEvent(FGuid EventID, FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload, AActor* Sender, ESimpleEventReplicationPolicy ReplicationPolicy);

	void SendEventInternal(FGuid EventID, FGameplayTag EventTag, FGameplayTag DomainTag, const FInstancedStruct& Payload, AActor* Sender, ESimpleEventReplicationPolicy ReplicationPolicy);

	/* Utility Functions */
	
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, BlueprintCallable, Category = "AbilityComponent|Utility")
	double GetServerTime() const;
	virtual double GetServerTime_Implementation() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AbilityComponent|Utility")
	FAbilityState GetAbilityState(FGuid AbilityInstanceID, bool& WasFound) const;
	
	UFUNCTION(BlueprintCallable, Category = "AbilityComponent|Utility")
	bool HasAuthority() const { return GetOwner()->HasAuthority(); }

	/* Called by multiple instance abilities to set themselves up for deletion once the ability is over */
	void RemoveInstancedAbility(USimpleGameplayAbility* AbilityToRemove);
	/* Called by abilities to signal if they activated or ended */
	void UpdateAbilityStatus(FGuid AbilityInstanceID, EAbilityStatus NewStatus);

protected:
	virtual void BeginPlay() override;
	bool ActivateAbilityInternal(TSubclassOf<USimpleGameplayAbility>& AbilityClass, const FInstancedStruct& AbilityContext, FGuid AbilityInstanceID);
	void AddNewAbilityState(const TSubclassOf<USimpleGameplayAbility>& AbilityClass, const FInstancedStruct& AbilityContext, FGuid AbilityInstanceID, bool DidActivateSuccessfully);
	void AddNewAttributeState(const TSubclassOf<USimpleAttributeModifier>& AttributeClass, const FInstancedStruct& AttributeContext, FGuid AttributeInstanceID);
	USimpleGameplayAbility* GetAbilityInstance(FGuid AbilityInstanceID);
	
	UFUNCTION()
	void OnRep_AuthorityAbilityStates();
	UFUNCTION()
	void OnRep_AuthorityAttributeStates();
	UFUNCTION()
	void OnRep_FloatAttributes(TArray<FFloatAttribute>& OldFloatAttributes);
	UFUNCTION()
	void OnRep_StructAttributes(TArray<FStructAttribute>& OldStructAttributes);
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

private:
	// The actual instances of the abilities. Visible for debugging purposes.
	UPROPERTY(VisibleAnywhere, Category = "AbilityComponent|State")
	TArray<USimpleGameplayAbility*> InstancedAbilities;
	
	// The actual instances of the attributes. Visible for debugging purposes.
	UPROPERTY(VisibleAnywhere, Category = "AbilityComponent|State")
	TArray<USimpleAttributeModifier*> InstancedAttributes;
	
	// Used to keep track of which events have been handled locally to void double event sending with multicast
	TArray<FGuid> HandledEventIDs;
};
