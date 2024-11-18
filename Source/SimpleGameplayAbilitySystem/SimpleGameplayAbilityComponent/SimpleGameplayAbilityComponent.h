#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SimpleGameplayAbilitySystem/SimpleGASTypes/SimpleGasTypes.h"
#include "SimpleGameplayAbilityComponent.generated.h"

class USimpleGameplayAttributes;
class UAbilitySet;
struct FSimpleGameplayAbilityConfig;
struct FInstancedStruct;
struct FGameplayTag;
class USimpleGameplayAbility;

UCLASS(Blueprintable, ClassGroup=(AbilitySystemComponent), meta=(BlueprintSpawnableComponent))
class SIMPLEGAMEPLAYABILITYSYSTEM_API USimpleGameplayAbilityComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USimpleGameplayAbilityComponent();

	virtual void BeginPlay() override;

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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated)
	FGameplayTagContainer GameplayTags;

	// Gameplay tag functions
	UFUNCTION(BlueprintCallable)
	void AddGameplayTags(FGameplayTagContainer Tags);

	UFUNCTION(BlueprintCallable)
	void RemoveGameplayTags(FGameplayTagContainer Tags);
	
	/**
	 * The abilities that this component can activate. Only modify this on the server.
	 */
	UPROPERTY(EditDefaultsOnly, Replicated)
	TArray<TSubclassOf<USimpleGameplayAbility>> GrantedAbilities;

	UPROPERTY(EditDefaultsOnly)
	TArray<UAbilitySet*> GrantedAbilitySets;

	UPROPERTY(Replicated, EditAnywhere, Instanced, BlueprintReadWrite)
	USimpleGameplayAttributes* Attributes;
	
	/**
	 * @return The avatar actor of this component. Usually the current player pawn.
	 */
	UFUNCTION(BlueprintCallable, Category = "SimpleGAS|GASComponent")
	AActor* GetAvatarActor() const;
	
	/**
	 * @param NewAvatarActor The new avatar actor of this component.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "SimpleGAS|GASComponent")
	void SetAvatarActor(AActor* NewAvatarActor);
	
	/**
	 * Adds an ability that this component can activate. Only call this on the server.
	 * @param Ability The ability to grant to this component.
	 * @param AutoActivateGrantedAbility Whether the ability should be immediately activated when granted.
	 * @param AutoActivatedAbilityContext The context to pass to the auto activating ability.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "SimpleGAS|GASComponent", meta=(AdvancedDisplay=1))
	void GrantAbility(TSubclassOf<USimpleGameplayAbility> Ability, bool AutoActivateGrantedAbility, FInstancedStruct AutoActivatedAbilityContext);

	/** 
	 * Removes this components ability to activate an ability. Only call this on the server.
	 * @param Ability The ability to revoke from this component.
	 * @param CancelRunningInstances If true, all running instances of this ability will be canceled immediately, otherwise they will be allowed to finish.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "SimpleGAS|GASComponent")
	void RevokeAbility(TSubclassOf<USimpleGameplayAbility> Ability, bool CancelRunningInstances);

	UFUNCTION(BlueprintCallable, Category = "SimpleGAS|GASComponent")
	bool ActivateAbility(TSubclassOf<USimpleGameplayAbility> AbilityClass, FInstancedStruct AbilityContext, bool OnlyActivateIfGranted = true);

	/**
	 * Sends an event through the SimpleEventSubsystem.
	   Replication settings:
	 * ActivationPolicy == EAbilityActivationPolicy::LocalOnly: Same result as using the SimpleEventSubsystem directly. i.e. no replication
	 * ActivationPolicy == EAbilityActivationPolicy::LocalPredicted: Event is sent on the client client first and then sent on the server.
	   If called from the server using this policy it behaves the same as EAbilityActivationPolicy::LocalOnly. i.e Client will replicate to server but server won't replicate to client.
	 * ActivationPolicy == EAbilityActivationPolicy::ServerInitiated: Sends the event to all connected clients (including server in a listen server scenario)
	   If called from the client this will send a server RPC which then Multicasts to all connected clients.
	 * ActivationPolicy == EAbilityActivationPolicy::ServerOnly: Same as using the SimpleEventSubsystem directly but only works on the server.
	   If called from the client using this policy it will not send the event.
	 * @param ActivationPolicy How to replicate the event.
	 */
	UFUNCTION(BlueprintCallable, Category = "SimpleGAS|GASComponent")
	void SendEvent(FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload, ESimpleEventReplicationPolicy ReplicationPolicy);

	/* Utility Functions */

	UFUNCTION(BlueprintCallable)
	bool HasAuthority() const { return GetOwner()->HasAuthority(); }
	
	UFUNCTION(BlueprintCallable, BlueprintPure,Category = "SimpleGAS|GASComponent")
	TSubclassOf<USimpleGameplayAbility> FindGrantedAbilityByTag(FGameplayTag AbilityTag);

	UFUNCTION(BlueprintCallable, BlueprintPure,Category = "SimpleGAS|GASComponent")
	FSimpleGameplayAbilityConfig GetAbilityConfig(TSubclassOf<USimpleGameplayAbility> Ability);

	/**
	 * Looks in both the RunningAbilities and LocalRunningAbilities arrays for an ability with the specified ID.
	 * @param AbilityInstanceID The ability instance ID to search for.
	 * @return A reference to the ability with the specified ID, or nullptr if no ability was found.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure,Category = "SimpleGAS|GASComponent")
	USimpleGameplayAbility* FindAnyAbilityInstanceByID(FGuid AbilityInstanceID);

	/**
	 * Looks in the RunningAbilities array for an ability with the specified ID.
	 * @param AbilityInstanceID The ability instance ID to search for.
	 * @return A reference to the ability with the specified ID, or nullptr if no ability was found.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure,Category = "SimpleGAS|GASComponent")
	USimpleGameplayAbility* FindAuthorityAbilityInstanceByID(FGuid AbilityInstanceID);

	/**
	 * Looks in the LocalRunningAbilities array for an ability with the specified ID.
	 * @param AbilityInstanceID The ability instance ID to search for.
	 * @return A reference to the ability with the specified ID, or nullptr if no ability was found.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure,Category = "SimpleGAS|GASComponent")
	USimpleGameplayAbility* FindPredictedAbilityInstanceByID(FGuid AbilityInstanceID);
	
	/* Override these functions to add your own functionality */
	
	/**
	 * Called on both the server and client after an ability's OnActivate function is called.
	 * @param Ability The ability that was started.
	 */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "SimpleGAS|GASComponent")
	void OnAbilityStarted(USimpleGameplayAbility* Ability); 

	/**
	 * Called on both the server and client after an ability's OnEnd function is called.
	 * This is called before the ability is removed from the running abilities list, don't modify the running abilities list in this function!
	 * @param Ability The ability that was ended.
	 */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "SimpleGAS|GASComponent")
	void OnAbilityEnded(USimpleGameplayAbility* Ability); 
	
	/**
	 * This function should return the server time if called from the server and
	 * the client estimation of the server time if called from the client.
	 * By default, we return the server time reported by the GameState (GameState->GetServerWorldTimeSeconds()).
	 * If you have your own method of synchronizing time between the server and client, override this function.
	 * @return The current time on the server.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "SimpleGAS|GASComponent")
	double GetServerTime() const;
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_RunningAbilities)
	TArray<USimpleGameplayAbility*> RunningAbilities;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<USimpleGameplayAbility*> LocalRunningAbilities;

	UFUNCTION(BlueprintInternalUseOnly)
	void OnAbilityStartedInternal(FGameplayTag AbilityTag, FGameplayTag DomainTag, FInstancedStruct Payload);
	
	UFUNCTION(BlueprintInternalUseOnly)
	void OnAbilityEndedInternal(FGameplayTag AbilityTag, FGameplayTag DomainTag, FInstancedStruct Payload);
	
	bool ActivateAbilityInternal(const TSubclassOf<USimpleGameplayAbility>& AbilityClass, const FInstancedStruct& AbilityContext, FGuid AbilityInstanceID);

	UFUNCTION(Server, Reliable, BlueprintInternalUseOnly)
	void ServerActivateAbility(TSubclassOf<USimpleGameplayAbility> AbilityClass, FInstancedStruct AbilityContext, FGuid AbilityInstanceID);
	
	UFUNCTION(NetMulticast, Reliable, BlueprintInternalUseOnly)
	void MulticastActivateAbility(TSubclassOf<USimpleGameplayAbility> AbilityClass, FInstancedStruct AbilityContext, FGuid AbilityInstanceID);

	UFUNCTION(NetMulticast, Unreliable, BlueprintInternalUseOnly)
	void MulticastActivateAbilityUnreliable(TSubclassOf<USimpleGameplayAbility> AbilityClass, FInstancedStruct AbilityContext, FGuid AbilityInstanceID);

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
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

private:
	TArray<FGuid> LocalPredictedEventIDs;
};
