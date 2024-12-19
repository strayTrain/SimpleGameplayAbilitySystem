#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleAbilityTypes.h"
#include "SimpleGameplayAbilitySystem/SimpleAttributeModifier/SimpleAttributeModifierTypes.h"
#include "SimpleAbilityComponent.generated.h"

class UAttributeSet;
class USimpleAttributeModifier;
class USimpleGameplayAttributes;
class UAbilitySet;
struct FSimpleGameplayAbilityConfig;
struct FInstancedStruct;
struct FGameplayTag;
class USimpleAbility;

UCLASS(Blueprintable, ClassGroup=(AbilityComponent), meta=(BlueprintSpawnableComponent))
class SIMPLEGAMEPLAYABILITYSYSTEM_API USimpleAbilityComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USimpleAbilityComponent();
	
	/**
	 * The actor that is controlled by the player.
	 * This actor is meant to be frequently accessed by abilities (usually AvatarActor is the player pawn).
	 * You can set this variable in BP by calling SetAvatarActor (server only).
	 */
	UPROPERTY(Replicated)
	AActor* AvatarActor;

	/**
	* The gameplay tags that this component has.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Gameplay Tags")
	FGameplayTagContainer GameplayTags;

	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Abilities")
	TArray<UAbilitySet*> GrantedAbilitySets;
	
	/**
	 * The abilities that this component can activate. Only modify this on the server.
	 */
	UPROPERTY(EditDefaultsOnly, Replicated, Category = "Gameplay Abilities")
	TArray<TSubclassOf<USimpleAbility>> GrantedAbilities;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Gameplay Attributes")
	TArray<UAttributeSet*> AttributeSets;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_FloatAttributes, Category = "Gameplay Attributes", meta = (TitleProperty = "AttributeName"))
	TArray<FFloatAttribute> FloatAttributes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_StructAttributes, Category = "Gameplay Attributes", meta = (TitleProperty = "AttributeName"))
	TArray<FStructAttribute> StructAttributes;

	/* Avatar Actor Functions */
	
	/**
	 * @return The avatar actor of this component. Usually the current player pawn.
	 */
	UFUNCTION(BlueprintCallable)
	AActor* GetAvatarActor() const;
	
	/**
	 * @param NewAvatarActor The new avatar actor of this component.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void SetAvatarActor(AActor* NewAvatarActor);

	/* Gameplay Tag Functions */
	
	UFUNCTION(BlueprintCallable)
	void AddGameplayTags(FGameplayTagContainer Tags);

	UFUNCTION(BlueprintCallable)
	void RemoveGameplayTags(FGameplayTagContainer Tags);

	/* Ability Functions */
	
	/**
	 * Adds an ability that this component can activate. Only call this on the server.
	 * @param Ability The ability to grant to this component.
	 * @param AutoActivateGrantedAbility Whether the ability should be immediately activated when granted.
	 * @param AutoActivatedAbilityContext The context to pass to the auto activating ability.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, meta=(AdvancedDisplay=1))
	void GrantAbility(TSubclassOf<USimpleAbility> Ability, bool AutoActivateGrantedAbility, FInstancedStruct AutoActivatedAbilityContext);

	/** 
	 * Removes this components ability to activate an ability. Only call this on the server.
	 * @param Ability The ability to revoke from this component.
	 * @param CancelRunningInstances If true, all running instances of this ability will be canceled immediately, otherwise they will be allowed to finish.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void RevokeAbility(TSubclassOf<USimpleAbility> Ability, bool CancelRunningInstances);

	UFUNCTION(BlueprintCallable, meta=(AdvancedDisplay=2))
	bool ActivateAbility(TSubclassOf<USimpleAbility> AbilityClass, FInstancedStruct AbilityContext, bool OnlyActivateIfGranted = true);

	/* Attribute Functions */
	
	UFUNCTION(BlueprintCallable)
	bool ApplyAttributeModifier(USimpleAbilityComponent* Instigator, const TSubclassOf<USimpleAttributeModifier> AttributeModifier, FInstancedStruct EffectContext);

	UFUNCTION(BlueprintCallable)
	bool ApplyPendingAttributeModifier(const FPendingAttributeModifier PendingAttributeModifier);

	UFUNCTION(BlueprintCallable)
	bool HasModifierWithTags(const FGameplayTagContainer& Tags) const;
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void AddFloatAttribute(FFloatAttribute Attribute);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void RemoveFloatAttributes(FGameplayTagContainer AttributeTags);
	
	UFUNCTION(BlueprintCallable)
	bool HasFloatAttribute(FGameplayTag AttributeTag) const;
	
	UFUNCTION(BlueprintCallable)
	FFloatAttribute GetFloatAttribute(FGameplayTag AttributeTag, bool& WasFound) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, meta = (DeterminesOutputType = "AvatarClass", HideSelfPin))
	float GetFloatAttributeValue(EAttributeValueType ValueType, FGameplayTag AttributeTag, bool& WasFound) const;

	UFUNCTION(BlueprintCallable)
	bool SetFloatAttributeValue(EAttributeValueType ValueType, FGameplayTag AttributeTag, float NewValue, float& Overflow);

	UFUNCTION(BlueprintCallable)
	bool OverrideFloatAttribute(FGameplayTag AttributeTag, FFloatAttribute NewAttribute);
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void AddStructAttribute(FStructAttribute Attribute);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void RemoveStructAttributes(FGameplayTagContainer AttributeTags);

	UFUNCTION(BlueprintCallable)
	bool HasStructAttribute(FGameplayTag AttributeTag) const;
	
	UFUNCTION(BlueprintCallable)
	FStructAttribute GetStructAttribute(FGameplayTag AttributeTag, bool& WasFound) const;
	
	UFUNCTION(BlueprintCallable)
	FInstancedStruct GetStructAttributeValue(FGameplayTag AttributeTag, bool& WasFound) const;

	UFUNCTION(BlueprintCallable)
	bool SetStructAttributeValue(FGameplayTag AttributeTag, FInstancedStruct NewValue);
	
	/* Replicated Event Functions */
	
	/**
	 * Sends an event through the SimpleEventSubsystem and optionally replicates it.
	 * @param ReplicationPolicy How to replicate the event.
	 */
	UFUNCTION(BlueprintCallable)
	void SendEvent(FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload, ESimpleEventReplicationPolicy ReplicationPolicy);
	
	/* Overridable functions */
	
	/**
	 * Called on both the server and client after an ability's OnActivate function is called.
	 * @param Ability The ability that was started.
	 */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void OnAbilityStarted(USimpleAbility* Ability); 

	/**
	 * Called on both the server and client after an ability's OnEnd function is called.
	 * This is called before the ability is removed from the running abilities list, don't modify the running abilities list in this function!
	 * @param Ability The ability that was ended.
	 */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void OnAbilityEnded(USimpleAbility* Ability); 
	
	/**
	 * This function should return the server time if called from the server and
	 * the client estimation of the server time if called from the client.
	 * By default, we return the server time reported by the GameState (GameState->GetServerWorldTimeSeconds()).
	 * If you have your own method of synchronizing time between the server and client, override this function.
	 * @return The current time on the server.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, BlueprintCallable)
	double GetServerTime() const;
	virtual double GetServerTime_Implementation() const;

	/* Utility Functions */

	UFUNCTION(BlueprintCallable)
	bool HasAuthority() const { return GetOwner()->HasAuthority(); }
	
	UFUNCTION(BlueprintCallable, BlueprintPure)
	TSubclassOf<USimpleAbility> FindGrantedAbilityByTag(FGameplayTag AbilityTag);

	/**
	 * Looks in both the RunningAbilities and LocalRunningAbilities arrays for an ability with the specified ID.
	 * @param AbilityInstanceID The ability instance ID to search for.
	 * @return A reference to the ability with the specified ID, or nullptr if no ability was found.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	USimpleAbility* FindAnyAbilityInstanceByID(FGuid AbilityInstanceID);

	/**
	 * Looks in the RunningAbilities array for an ability with the specified ID.
	 * @param AbilityInstanceID The ability instance ID to search for.
	 * @return A reference to the ability with the specified ID, or nullptr if no ability was found.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	USimpleAbility* FindAuthorityAbilityInstanceByID(FGuid AbilityInstanceID);

	/**
	 * Looks in the LocalRunningAbilities array for an ability with the specified ID.
	 * @param AbilityInstanceID The ability instance ID to search for.
	 * @return A reference to the ability with the specified ID, or nullptr if no ability was found.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	USimpleAbility* FindPredictedAbilityInstanceByID(FGuid AbilityInstanceID);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FSimpleGameplayAbilityConfig GetAbilityConfig(TSubclassOf<USimpleAbility> Ability);

	static float ClampFloatAttributeValue(const FFloatAttribute& Attribute, EAttributeValueType ValueType, float NewValue, float& Overflow);
protected:
	virtual void BeginPlay() override;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_RunningAbilities)
	TArray<USimpleAbility*> RunningAbilities;

	UPROPERTY(BlueprintReadOnly)
	TArray<USimpleAbility*> LocalRunningAbilities;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_RunningAttributeModifiers)
	TArray<USimpleAttributeModifier*> RunningAttributeModifiers;

	UPROPERTY(BlueprintReadOnly)
	TArray<USimpleAttributeModifier*> LocalRunningAttributeModifiers;

	UFUNCTION(BlueprintInternalUseOnly)
	void OnAbilityStartedInternal(FGameplayTag AbilityTag, FGameplayTag DomainTag, FInstancedStruct Payload);
	
	UFUNCTION(BlueprintInternalUseOnly)
	void OnAbilityEndedInternal(FGameplayTag AbilityTag, FGameplayTag DomainTag, FInstancedStruct Payload);
	
	bool ActivateAbilityInternal(const TSubclassOf<USimpleAbility>& AbilityClass, const FInstancedStruct& AbilityContext, FGuid AbilityInstanceID);

	UFUNCTION(Server, Reliable, BlueprintInternalUseOnly)
	void ServerActivateAbility(TSubclassOf<USimpleAbility> AbilityClass, FInstancedStruct AbilityContext, FGuid AbilityInstanceID);
	
	UFUNCTION(NetMulticast, Reliable, BlueprintInternalUseOnly)
	void MulticastActivateAbility(TSubclassOf<USimpleAbility> AbilityClass, FInstancedStruct AbilityContext, FGuid AbilityInstanceID);

	UFUNCTION(NetMulticast, Unreliable, BlueprintInternalUseOnly)
	void MulticastActivateAbilityUnreliable(TSubclassOf<USimpleAbility> AbilityClass, FInstancedStruct AbilityContext, FGuid AbilityInstanceID);

	void SendEventInternal(FGameplayTag EventTag, FGameplayTag DomainTag, const FInstancedStruct& Payload) const;

	UFUNCTION(Client, Reliable, BlueprintInternalUseOnly)
	void ClientSendEvent(FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload);
	
	UFUNCTION(Server, Reliable, BlueprintInternalUseOnly)
	void ServerSendEvent(FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload);

	UFUNCTION(Server, Reliable, BlueprintInternalUseOnly)
	void ServerSendMulticastEvent(FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload, FGuid EventID);

	UFUNCTION(Server, Reliable, BlueprintInternalUseOnly)
	void ServerSendClientEvent(FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload);
	
	UFUNCTION(NetMulticast, Reliable, BlueprintInternalUseOnly)
	void MulticastSendEvent(FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload, FGuid EventID);
	
	UFUNCTION()
	void OnRep_RunningAbilities();

	UFUNCTION()
	void OnRep_RunningAttributeModifiers();

	UFUNCTION()
	void OnRep_FloatAttributes(TArray<FFloatAttribute> OldAttributes);
	
	UFUNCTION()
	void OnRep_StructAttributes(TArray<FStructAttribute> OldAttributes);
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

private:
	TArray<FGuid> LocalPredictedEventIDs;
	int32 GetFloatAttributeIndex(FGameplayTag AttributeTag) const;
	void CompareFloatAttributesAndSendEvents(const FFloatAttribute& OldAttribute, const FFloatAttribute& NewAttribute) const;
	void SendFloatAttributeChangedEvent(FGameplayTag EventTag, FGameplayTag AttributeTag, EAttributeValueType ValueType, float NewValue) const;
};
