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

	/* Properties */
	
	UPROPERTY(Replicated)
	AActor* AvatarActor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "AbilityComponent|Tags")
	FGameplayTagContainer GameplayTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AbilityComponent|Attributes")
	TArray<UAttributeSet*> AttributeSets;

	UPROPERTY(EditDefaultsOnly, Category = "AbilityComponent|Attributes")
	TArray<FFloatAttribute> DirectlyGrantedFloatAttributes;
	UPROPERTY(EditDefaultsOnly, Category = "AbilityComponent|Attributes")
	TArray<FStructAttribute> DirectlyGrantedStructAttributes;
	
	UPROPERTY(VisibleAnywhere, Replicated, Category = "AbilityComponent|State")
	FFloatAttributeContainer AuthorityFloatAttributes;
	TArray<FFloatAttribute> PreviousFloatAttributes;
	
	UPROPERTY(VisibleAnywhere, Replicated, Category = "AbilityComponent|State")
	FStructAttributeContainer AuthorityStructAttributes;
	TArray<FStructAttribute> PreviousStructAttributes;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AbilityComponent|Abilities")
	TArray<UAbilitySet*> AbilitySets;
	
	UPROPERTY(EditAnywhere, Replicated, BlueprintReadOnly, Category = "AbilityComponent|Abilities")
	TArray<TSubclassOf<USimpleGameplayAbility>> GrantedAbilities;
	
	UPROPERTY(Replicated, VisibleAnywhere, Category = "AbilityComponent|State")
	FAbilityStateContainer AuthorityAbilityStates;
	UPROPERTY(VisibleAnywhere, Category = "AbilityComponent|State")
	TArray<FAbilityState> LocalAbilityStates;
	
	UPROPERTY(Replicated, VisibleAnywhere, Category = "AbilityComponent|State")
	FAbilityStateContainer AuthorityAttributeStates;
	UPROPERTY(VisibleAnywhere, Category = "AbilityComponent|State")
	TArray<FAbilityState> LocalAttributeStates;

	/* Avatar Actor Functions */
	
	UFUNCTION(BlueprintCallable, Category = "AbilityComponent|AvatarActor")
	AActor* GetAvatarActor() const { return AvatarActor; }
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "AbilityComponent|AvatarActor")
	void SetAvatarActor(AActor* NewAvatarActor) { AvatarActor = NewAvatarActor; }

	/* Ability Functions */
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "AbilityComponent|Abilities")
	void GrantAbility(TSubclassOf<USimpleGameplayAbility> AbilityClass);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "AbilityComponent|Abilities")
	void RevokeAbility(TSubclassOf<USimpleGameplayAbility> AbilityClass);

	UFUNCTION(BlueprintCallable, meta=(AdvancedDisplay=2), Category = "AbilityComponent|AbilityActivation")
	bool ActivateAbility(TSubclassOf<USimpleGameplayAbility> AbilityClass, FInstancedStruct AbilityContext, bool OverrideActivationPolicy, EAbilityActivationPolicy ActivationPolicy);

	UFUNCTION(Server, Reliable)
	void ServerActivateAbility(TSubclassOf<USimpleGameplayAbility> AbilityClass, FInstancedStruct AbilityContext, FGuid AbilityInstanceID);

	UFUNCTION(BlueprintCallable, Category = "AbilityComponent|AbilityActivation")
	bool CancelAbility(FGuid AbilityInstanceID, FInstancedStruct CancellationContext);

	UFUNCTION(BlueprintCallable, Category = "AbilityComponent|AbilityActivation")
	TArray<FGuid> CancelAbilitiesWithTags(FGameplayTagContainer Tags, FInstancedStruct CancellationContext);
	
	void AddAbilityStateSnapshot(FGuid AbilityInstanceID, FSimpleAbilitySnapshot State);
	void ChangeAbilityStatus(FGuid AbilityInstanceID, EAbilityStatus NewStatus);
	
	/* Attribute Functions */
	
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

	UFUNCTION(BlueprintCallable, Category = "AbilityComponent|Attributes")
	void AddAttributeStateSnapshot(FGuid AbilityInstanceID, FSimpleAbilitySnapshot State);
	
	/* Gameplay Tag Functions */
	
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
	
	/* Replicated Event Functions */
	
	UFUNCTION(BlueprintCallable, Category = "AbilityComponent|Events")
	void SendEvent(FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload, AActor* Sender, ESimpleEventReplicationPolicy ReplicationPolicy);
	void SendEventInternal(FGuid EventID, FGameplayTag EventTag, FGameplayTag DomainTag, const FInstancedStruct& Payload, AActor* Sender, ESimpleEventReplicationPolicy ReplicationPolicy);

	UFUNCTION(Server, Reliable)
	void ServerSendEvent(FGuid EventID, FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload, AActor* Sender, ESimpleEventReplicationPolicy ReplicationPolicy);

	UFUNCTION(Client, Reliable)
	void ClientSendEvent(FGuid EventID, FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload, AActor* Sender, ESimpleEventReplicationPolicy ReplicationPolicy);
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastSendEvent(FGuid EventID, FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload, AActor* Sender, ESimpleEventReplicationPolicy ReplicationPolicy);
	
	/* Utility Functions */
	
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, BlueprintCallable, Category = "AbilityComponent|Utility")
	double GetServerTime();
	virtual double GetServerTime_Implementation();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AbilityComponent|Utility")
	FAbilityState GetAbilityState(FGuid AbilityInstanceID, bool& WasFound) const;
	
	UFUNCTION(BlueprintCallable, Category = "AbilityComponent|Utility")
	bool HasAuthority() const { return GetOwner()->HasAuthority(); }

	/* Called by multiple instance abilities to set themselves up for deletion once the ability is over */
	void RemoveInstancedAbility(USimpleGameplayAbility* AbilityToRemove);

protected:
	UPROPERTY()
	TArray<USimpleGameplayAbility*> InstancedAbilities;
	UPROPERTY()
	TArray<USimpleAttributeModifier*> InstancedAttributes;
	
	// Used to keep track of which events have been handled locally to avoid double event sending with multicast
	TArray<FGuid> HandledEventIDs;
	
	virtual void BeginPlay() override;
	bool ActivateAbilityInternal(const TSubclassOf<USimpleGameplayAbility>& AbilityClass, const FInstancedStruct& AbilityContext, FGuid AbilityInstanceID);
	void AddNewAbilityState(const TSubclassOf<USimpleGameplayAbility>& AbilityClass, const FInstancedStruct& AbilityContext, FGuid AbilityInstanceID);
	void AddNewAttributeState(const TSubclassOf<USimpleAttributeModifier>& AttributeClass, const FInstancedStruct& AttributeContext, FGuid AttributeInstanceID);
	USimpleGameplayAbility* GetGameplayAbilityInstance(FGuid AbilityInstanceID);
	USimpleAttributeModifier* GetAttributeModifierInstance(FGuid AttributeInstanceID);
	TArray<FSimpleAbilitySnapshot>* GetLocalAttributeStateSnapshots(FGuid AttributeInstanceID);

	// Called on the client after an ability or attribute state has been added, changed or removed
	void OnStateAdded(const FAbilityState& NewAbilityState);
	void OnStateChanged(const FAbilityState& ChangedAbilityState);
	void OnStateRemoved(const FAbilityState& RemovedAbilityState);
	
	void OnFloatAttributeAdded(const FFloatAttribute& NewFloatAttribute);
	void OnFloatAttributeChanged(const FFloatAttribute& ChangedFloatAttribute);
	void OnFloatAttributeRemoved(const FFloatAttribute& RemovedFloatAttribute);

	void OnStructAttributeAdded(const FStructAttribute& NewStructAttribute);
	void OnStructAttributeChanged(const FStructAttribute& ChangedStructAttribute);
	void OnStructAttributeRemoved(const FStructAttribute& RemovedStructAttribute);
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
};
