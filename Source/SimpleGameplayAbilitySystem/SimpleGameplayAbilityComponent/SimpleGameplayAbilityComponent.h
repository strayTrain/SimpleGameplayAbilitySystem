#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SimpleAbilityComponentTypes.h"
#include "Components/ActorComponent.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleAbilityTypes.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleGameplayAbility/SimpleGameplayAbility.h"
#include "SimpleGameplayAbilityComponent.generated.h"

struct FAbilityOverride;
class UAbilityOverrideSet;
class UAbilitySet;
class UAttributeSet;
class USimpleGameplayAbility;
class USimpleStructAttributeHandler;

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
	
	UPROPERTY(VisibleAnywhere, Replicated)
	FFloatAttributeContainer AuthorityFloatAttributes;
	UPROPERTY(VisibleAnywhere, meta = (TitleProperty = "AttributeName"))
	TArray<FFloatAttribute> LocalFloatAttributes;
	
	UPROPERTY(VisibleAnywhere, Replicated)
	FStructAttributeContainer AuthorityStructAttributes;
	UPROPERTY(VisibleAnywhere, meta = (TitleProperty = "AttributeName"))
	TArray<FStructAttribute> LocalStructAttributes;
	
	UPROPERTY(Replicated)
	FAbilityStateContainer AuthorityAbilityStates;
	UPROPERTY()
	TArray<FAbilityState> LocalAbilityStates;
	
	UPROPERTY(Replicated)
	FAbilityStateContainer AuthorityAttributeStates;
	UPROPERTY()
	TArray<FAbilityState> LocalAttributeStates;

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

	UFUNCTION(BlueprintCallable, meta=(AdvancedDisplay=2, ReturnDisplayName="WasActivated"), Category = "AbilityComponent|AbilityActivation")
	bool ActivateAbility(
		TSubclassOf<USimpleGameplayAbility> AbilityClass,
		FInstancedStruct AbilityContext,
		bool OverrideActivationPolicy,
		EAbilityActivationPolicy ActivationPolicyOverride);

	bool ActivateAbilityWithID(
		const FGuid AbilityID,
		TSubclassOf<USimpleGameplayAbility> AbilityClass,
		const FInstancedStruct& AbilityContext,
		bool TrackState = true,
		bool OverrideActivationPolicy = false,
		EAbilityActivationPolicy ActivationPolicyOverride = EAbilityActivationPolicy::LocalOnly);
	
	UFUNCTION(Server, Reliable)
	void ServerActivateAbility(const FGuid AbilityID, TSubclassOf<USimpleGameplayAbility> AbilityClass,
	                           const FInstancedStruct& AbilityContext, EAbilityActivationPolicy ActivationPolicy, float ActivationTime);

	UFUNCTION(BlueprintCallable, Category = "AbilityComponent|AbilityActivation")
	bool CancelAbility(FGuid AbilityInstanceID, FInstancedStruct CancellationContext);

	UFUNCTION(BlueprintCallable, Category = "AbilityComponent|AbilityActivation")
	TArray<FGuid> CancelAbilitiesWithTags(FGameplayTagContainer Tags, FInstancedStruct CancellationContext);
	
	void AddAbilityStateSnapshot(FGuid AbilityInstanceID, FSimpleAbilitySnapshot State);
	void ChangeAbilityStatus(FGuid AbilityInstanceID, EAbilityStatus NewStatus);
	void SetAbilityStateEndingContext(FGuid AbilityInstanceID, FGameplayTag EndTag, FInstancedStruct EndContext);
	
	/* Attribute Functions */
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "AbilityComponent|Attributes", meta = (AdvancedDisplay=1))
	void AddFloatAttribute(FFloatAttribute AttributeToAdd, bool OverrideValuesIfExists = true );
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "AbilityComponent|Attributes")
	void RemoveFloatAttribute(FGameplayTag AttributeTag);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "AbilityComponent|Attributes", meta = (AdvancedDisplay=1))
	void AddStructAttribute(FStructAttribute AttributeToAdd, bool OverrideValuesIfExists = true);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "AbilityComponent|Attributes")
	void RemoveStructAttribute(FGameplayTag AttributeTag);

	UFUNCTION(BlueprintCallable, meta = (DeterminesOutputType = "HandlerClass", HideSelfPin))
	USimpleStructAttributeHandler* GetStructAttributeHandler(const TSubclassOf<USimpleStructAttributeHandler>& HandlerClass);
	
	/* Attribute Modifier Functions */
	
	UFUNCTION(BlueprintCallable, Category = "AbilityComponent|Attributes")
	bool ApplyAttributeModifierToTarget(USimpleGameplayAbilityComponent* ModifierTarget, TSubclassOf<USimpleAttributeModifier> ModifierClass, FInstancedStruct ModifierContext, FGuid& ModifierID);

	UFUNCTION(BlueprintCallable, Category = "AbilityComponent|Attributes")
	bool ApplyAttributeModifierToSelf(TSubclassOf<USimpleAttributeModifier> ModifierClass, FInstancedStruct ModifierContext, FGuid& ModifierID);

	UFUNCTION(BlueprintCallable, Category = "AbilityComponent|Attributes")
	void AddAttributeStateSnapshot(FGuid AbilityInstanceID, FSimpleAbilitySnapshot State);

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

	UFUNCTION(BlueprintCallable, BlueprintPure, meta = (DeterminesOutputType = "HandlerClass", HideSelfPin))
	const USimpleStructAttributeHandler* GetStructAttributeHandlerAs(FGameplayTag AttributeTag, TSubclassOf<USimpleStructAttributeHandler> HandlerClass, bool& WasFound);
	
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
	
	UFUNCTION(BlueprintCallable, Category = "AbilityComponent|Events", meta=(AutoCreateRefTerm = "ListenerFilter"))
	void SendEvent(FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload, AActor* Sender,
		TArray<UObject*> ListenerFilter, ESimpleEventReplicationPolicy ReplicationPolicy);
	void SendEventInternal(FGuid EventID, FGameplayTag EventTag, FGameplayTag DomainTag, const FInstancedStruct& Payload, AActor* Sender, ESimpleEventReplicationPolicy ReplicationPolicy, TArray
	                       <UObject*> ListenerFilter);

	UFUNCTION(Server, Reliable)
	void ServerSendEvent(FGuid EventID, FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload, AActor* Sender, ESimpleEventReplicationPolicy ReplicationPolicy);

	UFUNCTION(Client, Reliable)
	void ClientSendEvent(FGuid EventID, FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload, AActor* Sender, ESimpleEventReplicationPolicy ReplicationPolicy);
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastSendEvent(FGuid EventID, FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload, AActor* Sender, ESimpleEventReplicationPolicy ReplicationPolicy);
	
	/* Utility Functions */

	UFUNCTION(BlueprintCallable, Category = "AbilityComponent|Utility")
	bool HasAuthority() const { return GetOwner()->HasAuthority(); }
	
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

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AbilityComponent|Utility")
	FAbilityState FindAbilityState(FGuid AbilityInstanceID, bool& WasFound) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AbilityComponent|Utility")
	bool IsAnyAbilityActive() const;

	/**
	 * This function checks if an ability has an override set for it on this ability component.
	 * @param AbilityClass The class of the ability to check for overrides
	 * @return True if the ability has an override, false otherwise
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AbilityComponent|Utility")
	bool DoesAbilityHaveOverride(TSubclassOf<USimpleGameplayAbility> AbilityClass) const;

	/* Called by multiple instance abilities to set themselves up for deletion once the ability is over */
	void RemoveInstancedAbility(USimpleGameplayAbility* AbilityToRemove);
	
	template <typename T>
	T* GetStructAttributeHandlerAs()
	{
		return Cast<T>(GetStructAttributeHandler(T::StaticClass()));
	}
	
protected:
	
	UPROPERTY()
	TArray<USimpleGameplayAbility*> InstancedAbilities;
	
	UPROPERTY()
	TArray<USimpleAttributeModifier*> InstancedAttributes;

	TArray<USimpleStructAttributeHandler*> StructAttributeHandlers;
	
	// Used to keep track of which events have been handled locally to avoid double event sending with multicast
	TArray<FGuid> HandledEventIDs;
	
	virtual void BeginPlay() override;
	
	void CreateAttributeState(const TSubclassOf<USimpleAttributeModifier>& AttributeClass, const FInstancedStruct& AttributeContext, FGuid AttributeInstanceID);
	USimpleGameplayAbility* GetGameplayAbilityInstance(FGuid AbilityInstanceID);
	USimpleAttributeModifier* GetAttributeModifierInstance(FGuid AttributeInstanceID);
	TArray<FSimpleAbilitySnapshot>* GetLocalAttributeStateSnapshots(FGuid AttributeInstanceID);

	bool ActivateAbilityInternal(const FGuid AbilityID, TSubclassOf<USimpleGameplayAbility> AbilityClass,
		const FInstancedStruct& AbilityContext, EAbilityActivationPolicy ActivationPolicy,
		bool TrackState, float ActivationTime = -1);

	FAbilityState& CreateAbilityState(FGuid AbilityID,
		EAbilityActivationPolicy ActivationPolicy, const TSubclassOf<USimpleGameplayAbility>& AbilityClass,
		const FInstancedStruct& ActivationContext, bool IsAuthorityState, float ActivationTime = -1);
	
	FAbilityState* GetAbilityState(FGuid AbilityID, bool IsAuthorityState);
	USimpleGameplayAbility* GetAbilityInstance(TSubclassOf<USimpleGameplayAbility> AbilityClass);
	
	// Called on the client after an ability or attribute state has been added, changed or removed
	void OnStateAdded(const FAbilityState& NewAbilityState);
	void OnStateChanged(const FAbilityState& AuthorityAbilityState);
	void OnStateRemoved(const FAbilityState& RemovedAbilityState);
	void CompareSnapshots(const FAbilityState& AuthorityAbilityState, FAbilityState& LocalAbilityState);
	
	void OnFloatAttributeAdded(const FFloatAttribute& NewFloatAttribute);
	void OnFloatAttributeChanged(const FFloatAttribute& ChangedFloatAttribute);
	void OnFloatAttributeRemoved(const FFloatAttribute& RemovedFloatAttribute);

	void OnStructAttributeAdded(const FStructAttribute& NewStructAttribute);
	void OnStructAttributeChanged(const FStructAttribute& ChangedStructAttribute);
	void OnStructAttributeRemoved(const FStructAttribute& RemovedStructAttribute);
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
};
