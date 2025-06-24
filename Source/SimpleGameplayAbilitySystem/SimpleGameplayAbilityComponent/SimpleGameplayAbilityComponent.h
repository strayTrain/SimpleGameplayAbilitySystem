#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SimpleAbilityComponentTypes.h"
#include "Components/ActorComponent.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleAbilityTypes.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleGameplayAbility/SimpleGameplayAbility.h"
#include "SimpleGameplayAbilityComponent.generated.h"

struct FAbilitySideEffect;
struct FAbilityOverride;
class UAbilityOverrideSet;
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
	
	UPROPERTY(BlueprintReadOnly, Replicated)
	AActor* AvatarActor;

	/* Initialization Properties */
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AbilityComponent|Abilities")
	TArray<UAbilitySet*> AbilitySets;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AbilityComponent|Abilities")
	TArray<UAbilityOverrideSet*> AbilityOverrideSets;

	UPROPERTY(Replicated)
	TArray<FAbilityOverride> ActiveAbilityOverrides;
	
	UPROPERTY(EditAnywhere, Replicated, BlueprintReadOnly, Category = "AbilityComponent|Abilities")
	TArray<TSubclassOf<USimpleGameplayAbility>> GrantedAbilities;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AbilityComponent|Attributes")
	TArray<UAttributeSet*> AttributeSets;
	
	UPROPERTY(EditDefaultsOnly, Category = "AbilityComponent|Attributes", meta = (TitleProperty = "AttributeName"))
	TArray<FFloatAttribute> FloatAttributes;
	UPROPERTY(EditDefaultsOnly, Category = "AbilityComponent|Attributes", meta = (TitleProperty = "AttributeName"))
	TArray<FStructAttribute> StructAttributes;

	/* Replicated Properties */

	// Gameplay Tags
	UPROPERTY(VisibleAnywhere, Replicated, Category = "AbilityComponent|State")
	FGameplayTagCounterContainer AuthorityGameplayTags;
	UPROPERTY(VisibleAnywhere, Category = "AbilityComponent|State")
	TArray<FGameplayTagCounter> LocalGameplayTags;

	/** Called when a gameplay tag is added */
	UPROPERTY(BlueprintAssignable, Category = "AbilityComponent|Tags")
	FOnGameplayTagAddedSignature OnGameplayTagAdded;
	
	/** Called when a gameplay tag is removed */
	UPROPERTY(BlueprintAssignable, Category = "AbilityComponent|Tags")
	FOnGameplayTagRemovedSignature OnGameplayTagRemoved;
	
	// Float Attributes
	UPROPERTY(VisibleAnywhere, Replicated, Category = "AbilityComponent|State", meta = (TitleProperty = "Attributes.AttributeName"))
	FFloatAttributeContainer AuthorityFloatAttributes;
	UPROPERTY(VisibleAnywhere, Category = "AbilityComponent|State", meta = (TitleProperty = "AttributeName"))
	TArray<FFloatAttribute> LocalFloatAttributes;

	// Struct Attributes
	UPROPERTY(VisibleAnywhere, Replicated, Category = "AbilityComponent|State", meta = (TitleProperty = "Attributes.AttributeName"))
	FStructAttributeContainer AuthorityStructAttributes;
	UPROPERTY(VisibleAnywhere, meta = (TitleProperty = "AttributeName"), Category = "AbilityComponent|State")
	TArray<FStructAttribute> LocalStructAttributes;

	// Ability States
	UPROPERTY(VisibleAnywhere, Replicated, Category = "AbilityComponent|State", meta = (TitleProperty = "AbilityStates.AbilityClass"))
	FAbilityStateContainer AuthorityAbilityStates;
	UPROPERTY(VisibleAnywhere, Category = "AbilityComponent|State", meta = (TitleProperty = "AbilityClass"))
	TArray<FAbilityState> LocalAbilityStates;
	// Ability Snapshots
	UPROPERTY(VisibleAnywhere, Replicated, Category = "AbilityComponent|State", meta = (TitleProperty = "AbilityStates.AbilityClass"))
	FAbilitySnapshotContainer AuthorityAbilitySnapshots;
	UPROPERTY(VisibleAnywhere, Category = "AbilityComponent|State", meta = (TitleProperty = "AbilityClass"))
	TArray<FAbilitySnapshot> LocalPendingAbilitySnapshots;

	// Attribute States
	UPROPERTY(VisibleAnywhere, Replicated, Category = "AbilityComponent|State", meta = (TitleProperty = "AbilityStates.AbilityClass"))
	FAbilityStateContainer AuthorityAttributeModifierStates;
	UPROPERTY(VisibleAnywhere, Category = "AbilityComponent|State", meta = (TitleProperty = "AbilityClass"))
	TArray<FAbilityState> LocalAttributeModifierStates;
	// Attribute Snapshots
	UPROPERTY(VisibleAnywhere, Replicated, Category = "AbilityComponent|State", meta = (TitleProperty = "AbilityStates.AbilityClass"))
	FAbilitySnapshotContainer AuthorityAttributeModifierSnapshots;
	UPROPERTY(VisibleAnywhere, Category = "AbilityComponent|State", meta = (TitleProperty = "AbilityClass"))
	TArray<FAbilitySnapshot> LocalPendingAttributeModiferSnapshots;
	
	/* Avatar Actor Functions */
	
	UFUNCTION(BlueprintCallable, Category = "AbilityComponent|AvatarActor")
	AActor* GetAvatarActor() const { return AvatarActor; }
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "AbilityComponent|AvatarActor")
	void SetAvatarActor(AActor* NewAvatarActor) { AvatarActor = NewAvatarActor; }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool IsAvatarActorOfType(TSubclassOf<AActor> AvatarClass) const;
	
	/* Ability Functions */
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "AbilityComponent|Abilities")
	void GrantAbility(TSubclassOf<USimpleGameplayAbility> AbilityClass);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "AbilityComponent|Abilities")
	void RevokeAbility(TSubclassOf<USimpleGameplayAbility> AbilityClass);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "AbilityComponent|Abilities")
	void AddAbilityOverride(TSubclassOf<USimpleGameplayAbility> Ability, TSubclassOf<USimpleGameplayAbility> OverrideAbility);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "AbilityComponent|Abilities")
	void RemoveAbilityOverride(TSubclassOf<USimpleGameplayAbility> Ability);

	UFUNCTION(BlueprintCallable, meta=(AdvancedDisplay=3, ReturnDisplayName="WasActivated"), Category = "AbilityComponent|AbilityActivation")
	bool ActivateAbility(
		TSubclassOf<USimpleGameplayAbility> AbilityClass,
		FAbilityContextCollection AbilityContexts,
		FGuid& AbilityID,
		EAbilityActivationPolicyOverride ActivationPolicyOverride = EAbilityActivationPolicyOverride::DontOverride);

	bool ActivateAbilityWithID(
		const FGuid AbilityID,
		const TSubclassOf<USimpleGameplayAbility>& AbilityClass,
		const FAbilityContextCollection& AbilityContexts,
		EAbilityActivationPolicyOverride ActivationPolicyOverride = EAbilityActivationPolicyOverride::DontOverride);
	
	UFUNCTION(Server, Reliable)
	void ServerActivateAbility(
		const FGuid AbilityID,
		TSubclassOf<USimpleGameplayAbility> AbilityClass,
		const FAbilityContextCollection& AbilityContexts,
		EAbilityActivationPolicy ActivationPolicy,
		float ActivationTime);

	UFUNCTION(BlueprintCallable, meta=(AdvancedDisplay=2), Category = "AbilityComponent|AbilityActivation")
	void CancelAbility(FGuid AbilityInstanceID, FInstancedStruct CancellationContext);

	UFUNCTION(BlueprintCallable, Category = "AbilityComponent|AbilityActivation")
	TArray<FGuid> CancelAbilitiesWithTags(FGameplayTagContainer Tags, FInstancedStruct CancellationContext);

	int32 AddGameplayAbilitySnapshot(FGuid AbilityID, FInstancedStruct SnapshotData);
	int32 AddAttributeModifierSnapshot(FGuid AbilityID, TSubclassOf<USimpleAttributeModifier> ModifierClass, FInstancedStruct SnapshotData);

	/* Attribute Modifier Functions */
	
	UFUNCTION(BlueprintCallable, Category = "AbilityComponent|Attributes")
	bool ApplyAttributeModifierToTarget(
		USimpleGameplayAbilityComponent* ModifierTarget,
		TSubclassOf<USimpleAttributeModifier> ModifierClass,
		float Magnitude,
		FAbilityContextCollection ModifierContexts,
		FGuid& ModifierID);

	UFUNCTION(BlueprintCallable, Category = "AbilityComponent|Attributes")
	bool ApplyAttributeModifierToSelf(
		TSubclassOf<USimpleAttributeModifier> ModifierClass,
		float Magnitude,
		FAbilityContextCollection ModifierContexts,
		FGuid& ModifierID);

	/**
	 * Cancels a modifier.
	 * If it is an active duration modifier, it is cancelled.
	 * If it is an instant modifier or ended duration modifier, any running ability or modifier side effects it created are cancelled.
	 * @param ModifierID The ID of the modifier to cancel
	 */
	UFUNCTION(BlueprintCallable, Category = "AbilityComponent|Attributes")
	void CancelAttributeModifier(FGuid ModifierID);

	UFUNCTION(BlueprintCallable, Category = "AbilityComponent|Attributes")
	void CancelAttributeModifiersWithTags(FGameplayTagContainer Tags);

	/**
	 * This function checks if there is an active attribute modifiers with ModifierTags matching the specified tags.
	 * @param Tags The tags to check for in the active attribute modifiers. Only exact matches are considered.
	 * @return True if there is an active attribute modifier with the specified tags, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AbilityComponent|Attributes")
	bool HasAttributeModifierWithTags(FGameplayTagContainer Tags) const;
	
	/* Attribute Functions */
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "AbilityComponent|Attributes", meta = (AdvancedDisplay=1))
	void AddFloatAttribute(FFloatAttribute AttributeToAdd, bool OverrideValuesIfExists = true );
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "AbilityComponent|Attributes")
	void RemoveFloatAttribute(FGameplayTag AttributeTag);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "AbilityComponent|Attributes", meta = (AdvancedDisplay=1))
	void AddStructAttribute(FStructAttribute AttributeToAdd, bool OverrideValuesIfExists = true);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "AbilityComponent|Attributes")
	void RemoveStructAttribute(FGameplayTag AttributeTag);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool HasFloatAttribute(const FGameplayTag AttributeTag);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool HasStructAttribute(const FGameplayTag AttributeTag);
	
	UFUNCTION(BlueprintCallable, BlueprintPure, meta = (DeterminesOutputType = "AvatarClass", HideSelfPin))
	float GetFloatAttributeValue(EFloatAttributeValueType ValueType, FGameplayTag AttributeTag, bool& WasFound);

	UFUNCTION(BlueprintCallable, meta = (ReturnDisplayName = "WasFound"), Category = "AbilityComponent|Attributes")
	bool SetFloatAttributeValue(EFloatAttributeValueType ValueType, FGameplayTag AttributeTag, float NewValue, float& Overflow);

	UFUNCTION(BlueprintCallable, Category = "AbilityComponent|Attributes")
	bool IncrementFloatAttributeValue(EFloatAttributeValueType ValueType, FGameplayTag AttributeTag, float Increment, float& Overflow);
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool OverrideFloatAttribute(FGameplayTag AttributeTag, FFloatAttribute NewAttribute);
	
	UFUNCTION(BlueprintCallable, BlueprintPure)
	FInstancedStruct GetStructAttributeValue(FGameplayTag AttributeTag, bool& WasFound);
	
	UFUNCTION(BlueprintCallable, meta = (ReturnDisplayName = "WasFound"), Category = "AbilityComponent|Attributes")
	bool SetStructAttributeValue(FGameplayTag AttributeTag, FInstancedStruct NewValue);

	UFUNCTION()
	float ClampFloatAttributeValue(const FFloatAttribute& Attribute, EFloatAttributeValueType ValueType, float NewValue, float& Overflow);

	UFUNCTION()
	void CompareFloatAttributesAndSendEvents(const FFloatAttribute& OldAttribute, const FFloatAttribute& NewAttribute);

	UFUNCTION()
	void SendFloatAttributeChangedEvent(FGameplayTag EventTag, FGameplayTag AttributeTag, EFloatAttributeValueType ValueType, float NewValue);
	
	FFloatAttribute* GetFloatAttribute(FGameplayTag AttributeTag);
	FStructAttribute* GetStructAttribute(FGameplayTag AttributeTag);
	
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

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AbilityComponent|Tags")
	bool HasGameplayTag(FGameplayTag Tag);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AbilityComponent|Tags")
	bool HasAllGameplayTags(FGameplayTagContainer Tags);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AbilityComponent|Tags")
	bool HasAnyGameplayTags(FGameplayTagContainer Tags);

	/**
	 * This function returns all the gameplay tags that are currently active on this component as a GameplayTagContainer.
	 * Changing the tags in the GameplayTagContainer will not affect the tags on this component.
	 * @return A GameplayTagContainer with all the gameplay tags that are currently active on this component.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AbilityComponent|Tags")
	FGameplayTagContainer GetActiveGameplayTags() const;
	
	/* Replicated Event Functions */
	
	UFUNCTION(BlueprintCallable, Category = "AbilityComponent|Events", meta=(AutoCreateRefTerm = "ListenerFilter"))
	void SendEvent(
		FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload,
		UObject* Sender, TArray<UObject*> ListenerFilter, ESimpleEventReplicationPolicy ReplicationPolicy);
	
	void SendEventInternal(
		FGuid EventID, FGameplayTag EventTag, FGameplayTag DomainTag, const FInstancedStruct& Payload,
		UObject* Sender, ESimpleEventReplicationPolicy ReplicationPolicy, const TArray<UObject*>& ListenerFilter);

	UFUNCTION(Server, Reliable)
	void ServerSendEvent(
		FGuid EventID, FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload,
		UObject* Sender, ESimpleEventReplicationPolicy ReplicationPolicy, const TArray<UObject*>& ListenerFilter);

	UFUNCTION(Client, Reliable)
	void ClientSendEvent(
		FGuid EventID, FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload,
		UObject* Sender, ESimpleEventReplicationPolicy ReplicationPolicy, const TArray<UObject*>& ListenerFilter);
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastSendEvent(
		FGuid EventID, FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload,
		UObject* Sender, ESimpleEventReplicationPolicy ReplicationPolicy, const TArray<UObject*>& ListenerFilter);
	
	/* Utility Functions */

	UFUNCTION(BlueprintCallable, Category = "AbilityComponent|Utility")
	bool HasAuthority() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AbilityComponent|Utility")
	bool IsAnyAbilityActive() const;
	
	/**
	 * This function checks if an ability has an override set for it on this ability component.
	 * @param AbilityClass The class of the ability to check for overrides
	 * @return True if the ability has an override, false otherwise
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AbilityComponent|Utility")
	bool DoesAbilityHaveOverride(TSubclassOf<USimpleGameplayAbility> AbilityClass) const;
	
	/**
	 * Returns the server time if called on the server.
	 * Returns the clients estimation of the server time if called on the client.
	 * By default, uses GetWorld()->GetGameState()->GetServerWorldTimeSeconds() to get the server time.
	 * Override this function to provide a custom network time synchronisation implementation.
	 * @return The current server time in seconds
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, BlueprintCallable, Category = "AbilityComponent|Utility")
	double GetServerTime();
	virtual double GetServerTime_Implementation();
	
	UFUNCTION(BlueprintCallable, BlueprintPure, BlueprintCallable, Category = "AbilityComponent|Utility")
	USimpleAttributeHandler* GetAttributeHandler(FGameplayTag AttributeTag);

	UFUNCTION(BlueprintCallable, BlueprintPure, BlueprintCallable, Category = "AbilityComponent|Utility", meta = (DeterminesOutputType = "AttributeHandlerClass", HideSelfPin))
	USimpleAttributeHandler* GetAttributeHandlerAs(FGameplayTag AttributeTag, TSubclassOf<USimpleAttributeHandler> AttributeHandlerClass);
	
	USimpleAttributeHandler* GetStructAttributeHandlerInstance(TSubclassOf<USimpleAttributeHandler> HandlerClass);
	USimpleGameplayAbility* GetGameplayAbilityInstance(FGuid AbilityInstanceID);
	USimpleGameplayAbility* GetAbilityInstance(const TSubclassOf<USimpleGameplayAbility>& AbilityClass);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	bool ActivateAbilityInternal(
		const FGuid AbilityID,
		const TSubclassOf<USimpleGameplayAbility>& AbilityClass,
		const FAbilityContextCollection& AbilityContexts,
		EAbilityActivationPolicy ActivationPolicy,
		bool TrackState,
		double ActivationTime = -1);
	
	UPROPERTY()
	TArray<USimpleGameplayAbility*> InstancedAbilities;
	
	UPROPERTY()
	TArray<USimpleAttributeModifier*> InstancedAttributeModifiers;

	UPROPERTY()
	TArray<USimpleAttributeHandler*> InstancedAttributeHandlers;
	
	// Used to keep track of which events have been handled locally to avoid double event sending with multicast
	TArray<FGuid> HandledEventIDs;

private:
	UFUNCTION()
	void OnAbilityActivationSuccess(FGuid AbilityID);
	UFUNCTION()
	void OnAbilityActivationFailed(FGuid AbilityID);
	UFUNCTION()
	void OnAbilityEnded(FGuid AbilityID, FGameplayTag EndStatus, FInstancedStruct EndingContext);
	UFUNCTION()
	void OnAbilityCancelled(FGuid AbilityID, FGameplayTag CancelStatus, FInstancedStruct CancellationContext);

	USimpleAttributeModifier* GetAttributeModifierInstance(TSubclassOf<USimpleAttributeModifier> ModifierClass);
	
	// These functions are called on the client when the authoritative version of the variable changes on the server
	void OnAbilityStateAdded(const FAbilityState& NewAbilityState);
	void OnAbilityStateChanged(const FAbilityState& ChangedAbilityState);
	void ResolveLocalAbilityState(const FAbilityState& UpdatedAbilityState);
	void OnAbilityStateRemoved(const FAbilityState& RemovedAbilityState);
	
	void OnAttributeModiferStateAdded(const FAbilityState& NewAttributeModiferState);
	void OnAttributeModifierStateChanged(const FAbilityState& ChangedAttributeModiferState);
	void OnAttributeModiferStateRemoved(const FAbilityState& RemovedAttributeModiferState);
	
	void OnAbilitySnapshotAdded(const FAbilitySnapshot& NewAbilitySnapshot);
	void OnAttributeModifierSnapshotAdded(const FAbilitySnapshot& NewAttributeModifierSnapshot);
	
	void OnFloatAttributeAdded(const FFloatAttribute& NewFloatAttribute);
	void OnFloatAttributeChanged(const FFloatAttribute& ChangedFloatAttribute);
	void OnFloatAttributeRemoved(const FFloatAttribute& RemovedFloatAttribute);

	void OnStructAttributeAdded(const FStructAttribute& NewStructAttribute);
	void OnStructAttributeChanged(const FStructAttribute& ChangedStructAttribute);
	void OnStructAttributeRemoved(const FStructAttribute& RemovedStructAttribute);

	void OnAuthorityGameplayTagAdded(const FGameplayTagCounter& NewGameplayTag);
	void OnAuthorityGameplayTagChanged(const FGameplayTagCounter& ChangedGameplayTag);
	void OnAuthorityGameplayTagRemoved(const FGameplayTagCounter& RemovedGameplayTag);
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
};
