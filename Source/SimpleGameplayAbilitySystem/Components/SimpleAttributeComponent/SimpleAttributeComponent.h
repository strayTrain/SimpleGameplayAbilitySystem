//  Copyright 2025 Ahmed Elgoni

#pragma once

#include "CoreMinimal.h"
#include "SimpleAttributeComponentTypes.h"
#include "Components/ActorComponent.h"
#include "SimpleAttributeModifier/SimpleAttributeModifierTypes.h"
#include "SimpleAttributeModifier/ModifierActions/ChangeFloatAttributeAction/FloatAttributeActionTypes.h"
#include "SimpleGameplayAbilitySystem/DataAssets/AttributeSet/SimpleAttributeSet.h"
#include "SimpleAttributeComponent.generated.h"

class USimpleTimeSynchronizer;
class USimpleAbilityOverrideSet;
class USimpleAbilitySet;
class USimpleAttributeSet;
class USimpleGameplayAbility;
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SIMPLEGAMEPLAYABILITYSYSTEM_API USimpleAttributeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USimpleAttributeComponent();

	/* Properties */

	/* Initialization Properties */
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AttributeComponent|Initial Values")
	TArray<USimpleAttributeSet*> AttributeSets;
	
	UPROPERTY(EditDefaultsOnly, Category = "AttributeComponent|Initial Values", meta = (TitleProperty = "AttributeName"))
	TArray<FFloatAttribute> DefaultFloatAttributes;
	
	UPROPERTY(EditDefaultsOnly, Category = "AttributeComponent|Initial Values", meta = (TitleProperty = "AttributeName"))
	TArray<FStructAttribute> DefaultStructAttributes;

	UPROPERTY(EditDefaultsOnly, Category = "AttributeComponent|Initial Values")
	FGameplayTagContainer DefaultGameplayTags;
	
	/* Replicated Properties */

	// Gameplay Tags
	UPROPERTY(VisibleAnywhere, Replicated, Category = "AttributeComponent|State")
	FGameplayTagCounterContainer AuthorityGameplayTags;
	UPROPERTY(VisibleAnywhere, Category = "AttributeComponent|State")
	TArray<FGameplayTagCounter> LocalGameplayTags;
	
	// Float Attributes
	UPROPERTY(VisibleAnywhere, Replicated, Category = "AttributeComponent|State", meta = (TitleProperty = "Attributes.AttributeName"))
	FFloatAttributeContainer AuthorityFloatAttributes;
	UPROPERTY(VisibleAnywhere, Category = "AttributeComponent|State", meta = (TitleProperty = "AttributeName"))
	TArray<FFloatAttribute> LocalFloatAttributes;

	// Struct Attributes
	UPROPERTY(VisibleAnywhere, Replicated, Category = "AttributeComponent|State", meta = (TitleProperty = "Attributes.AttributeName"))
	FStructAttributeContainer AuthorityStructAttributes;
	UPROPERTY(VisibleAnywhere, meta = (TitleProperty = "AttributeName"), Category = "AttributeComponent|State")
	TArray<FStructAttribute> LocalStructAttributes;

	// Attribute States
	UPROPERTY(VisibleAnywhere, Replicated, Category = "AttributeComponent|State", meta = (TitleProperty = "AbilityStates.AbilityClass"))
	FAttributeModifierStateContainer AuthorityAttributeModifierStates;
	UPROPERTY(VisibleAnywhere, Category = "AttributeComponent|State", meta = (TitleProperty = "AbilityClass"))
	TArray<FAttributeModifierState> LocalAttributeModifierStates;
	
	// Attribute Mutations
	UPROPERTY(VisibleAnywhere, Replicated, Category = "AttributeComponent|State", meta = (TitleProperty = "AbilityStates.AbilityClass"))
	FAttributeModifierMutationContainer AuthorityAttributeModifierMutations;
	UPROPERTY(VisibleAnywhere, Category = "AttributeComponent|State", meta = (TitleProperty = "AbilityClass"))
	TArray<FAttributeModifierMutation> LocalAttributeModiferMutations;
	
	/* Attribute Modifier Functions */
	
	/**
	 * Applies an attribute modifier to a target attribute component without any replication. Use this function
	 * for single player or server/client only abilities.
	 * @param ModifierID The ID of the applied modifier. This can be used to cancel the modifier later.
	 * @param ModifierClass The class of the modifier to apply
	 * @param ModifierTarget The target attribute component to apply the modifier to
	 * @param Magnitude Optional magnitude of the modifier. This can be used in the modifier's logic.
	 * @param ModifierContext Optional context struct that can be used to pass additional data to the modifier.
	 * @return True if the modifier was applied successfully, false otherwise
	 */
	UFUNCTION(BlueprintCallable, Category = "AttributeComponent|Attributes")
	bool ApplyAttributeModifierToTarget(
		FGuid& ModifierID,
		TSubclassOf<USimpleAttributeModifier> ModifierClass,
		USimpleAttributeComponent* ModifierTarget,
		float Magnitude,
		FInstancedStruct ModifierContext);

	/**
	 *	Applies an attribute modifier to a target attribute component with client prediction. If this function is
	 *	called on a client, it will attempt to apply the modifier locally and then call the server to apply the modifier.
	 *	If the server rejects the modifier, it will be cancelled on the client. If called on the server, the behaviour is
	 *	the same as ApplyAttributeModifierToTargetServerInitiated.
	 *	@param ModifierID The ID of the applied modifier. This can be used to cancel the modifier later.
	 *	@param ModifierClass The class of the modifier to apply
	 *	@param ModifierTarget The target attribute component to apply the modifier to
	 *	@param Magnitude Optional magnitude of the modifier. This can be used in the modifier's logic.
	 *	@param ModifierContext Optional context struct that can be used to pass additional data to the modifier.
	 *	@return True if the modifier was applied successfully, false otherwise
	 */
	UFUNCTION(BlueprintCallable, Category = "AttributeComponent|Attributes")
	bool ApplyAttributeModifierToTargetPredicted(
		FGuid& ModifierID,
		TSubclassOf<USimpleAttributeModifier> ModifierClass,
		USimpleAttributeComponent* ModifierTarget,
		float Magnitude,
		FInstancedStruct ModifierContext);

	/**
	 *	Applies an attribute modifier to a target attribute component. Can be invoked by the client but will only run
	 *	on the server with the results of the modifier replicated to the client. Use this function if your attribute
	 *	modifier does things which need to happen on the server (e.g. spawning actors, picking a random number, etc).
	 *	@param ModifierID The ID of the applied modifier. This can be used to cancel the modifier later.
	 *	@param ModifierClass The class of the modifier to apply
	 *	@param ModifierTarget The target attribute component to apply the modifier to
	 *	@param Magnitude Optional magnitude of the modifier. This can be used in the modifier's logic.
	 *	@param ModifierContext Optional context struct that can be used to pass additional data to the modifier.
	 *	@return True if the modifier was applied successfully, false otherwise
	 */
	UFUNCTION(BlueprintCallable, Category = "AttributeComponent|Attributes")
	void ApplyAttributeModifierToTargetServerInitiated(
		FGuid& ModifierID,
		TSubclassOf<USimpleAttributeModifier> ModifierClass,
		USimpleAttributeComponent* ModifierTarget,
		float Magnitude,
		FInstancedStruct ModifierContext);

	UFUNCTION(Server, Reliable)
	void ServerApplyAttributeModifierToTarget(
		FGuid ModifierID,
		TSubclassOf<USimpleAttributeModifier> ModifierClass,
		USimpleAttributeComponent* ModifierTarget,
		float Magnitude,
		FInstancedStruct ModifierContext);

	UFUNCTION(BlueprintCallable, Category = "AttributeComponent|Attributes")
	bool ApplyAttributeModifierToSelf(
		FGuid& ModifierID,
		TSubclassOf<USimpleAttributeModifier> ModifierClass,
		float Magnitude, FInstancedStruct ModifierContext);

	UFUNCTION(BlueprintCallable, Category = "AttributeComponent|Attributes")
	bool ApplyAttributeModifierToSelfPredicted(
		FGuid& ModifierID,
		TSubclassOf<USimpleAttributeModifier> ModifierClass,
		float Magnitude,
		FInstancedStruct ModifierContext);

	UFUNCTION(BlueprintCallable, Category = "AttributeComponent|Attributes")
	void ApplyAttributeModifierToSelfServerInitiated(
		FGuid& ModifierID,
		TSubclassOf<USimpleAttributeModifier> ModifierClass,
		float Magnitude,
		FInstancedStruct ModifierContext);

	/**
	 * Cancels a modifier.
	 * If it is an active duration modifier, it is cancelled.
	 * If it is an instant modifier or ended duration modifier, any running ability or modifier side effects it created are cancelled.
	 * @param ModifierID The ID of the modifier to cancel
	 */
	UFUNCTION(BlueprintCallable, Category = "AttributeComponent|Attributes")
	void CancelAttributeModifier(FGuid ModifierID);

	UFUNCTION(BlueprintCallable, Category = "AttributeComponent|Attributes")
	void CancelAttributeModifierPredicted(FGuid ModifierID);

	UFUNCTION(BlueprintCallable, Category = "AttributeComponent|Attributes")
	void CancelAttributeModifierServerInitiated(FGuid ModifierID);

	UFUNCTION(Server, Reliable, Category = "AttributeComponent|Attributes")
	void ServerCancelAttributeModifier(FGuid ModifierID);
	
	UFUNCTION(BlueprintCallable, Category = "AttributeComponent|Attributes")
	void CancelAttributeModifiersWithTags(FGameplayTagContainer ModifierTags);

	UFUNCTION(BlueprintCallable, Category = "AttributeComponent|Attributes")
	void CancelAttributeModifiersWithTagsPredicted(FGameplayTagContainer ModifierTags);

	UFUNCTION(BlueprintCallable, Category = "AttributeComponent|Attributes")
	void CancelAttributeModifiersWithTagsServerInitiated(FGameplayTagContainer ModifierTags);

	UFUNCTION(Server, Reliable, Category = "AttributeComponent|Attributes")
	void ServerCancelAttributeModifiersWithTags(FGameplayTagContainer ModifierTags);

	/**
	 * This function checks if there is an active attribute modifiers with ModifierTags matching the specified tags.
	 * @param ModifierTags The tags to check for in the active attribute modifiers. Only exact matches are considered.
	 * @return True if there is an active attribute modifier with the specified tags, false otherwise
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AttributeComponent|Attributes")
	bool IsModifierWithTagsActive(FGameplayTagContainer ModifierTags) const;
	
	/* Attribute Functions */
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "AttributeComponent|Attributes", meta = (AdvancedDisplay=1))
	void AddFloatAttribute(FFloatAttribute AttributeToAdd, bool OverrideValuesIfExists = true );
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "AttributeComponent|Attributes")
	void RemoveFloatAttribute(FGameplayTag AttributeTag);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "AttributeComponent|Attributes", meta = (AdvancedDisplay=1))
	void AddStructAttribute(FStructAttribute AttributeToAdd, bool OverrideValuesIfExists = true);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "AttributeComponent|Attributes")
	void RemoveStructAttribute(FGameplayTag AttributeTag);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool HasFloatAttribute(const FGameplayTag AttributeTag);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool HasStructAttribute(const FGameplayTag AttributeTag);
	
	UFUNCTION(BlueprintCallable, BlueprintPure, meta = (DeterminesOutputType = "AvatarClass", HideSelfPin))
	float GetFloatAttributeValue(EFloatAttributeValueType ValueType, FGameplayTag AttributeTag, bool& WasFound);

	UFUNCTION(BlueprintCallable, meta = (ReturnDisplayName = "WasFound"), Category = "AttributeComponent|Attributes")
	bool SetFloatAttributeValue(EFloatAttributeValueType ValueType, FGameplayTag AttributeTag, float NewValue, float& Overflow);

	UFUNCTION(BlueprintCallable, Category = "AttributeComponent|Attributes")
	bool IncrementFloatAttributeValue(EFloatAttributeValueType ValueType, FGameplayTag AttributeTag, float Increment, float& Overflow);
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool OverrideFloatAttribute(FGameplayTag AttributeTag, FFloatAttribute NewAttribute);
	
	UFUNCTION(BlueprintCallable, BlueprintPure)
	FInstancedStruct GetStructAttributeValue(FGameplayTag AttributeTag, bool& WasFound);
	
	UFUNCTION(BlueprintCallable, meta = (ReturnDisplayName = "WasFound"), Category = "AttributeComponent|Attributes")
	bool SetStructAttributeValue(FGameplayTag AttributeTag, FInstancedStruct NewValue);

	UFUNCTION()
	float ClampFloatAttributeValue(const FFloatAttribute& Attribute, EFloatAttributeValueType ValueType, float NewValue, float& Overflow);
	
	FFloatAttribute* GetFloatAttribute(FGameplayTag AttributeTag);
	FStructAttribute* GetStructAttribute(FGameplayTag AttributeTag);
	
	/* Gameplay Tag Functions */
	
	UFUNCTION(BlueprintCallable, Category = "AttributeComponent|Tags", meta = (AdvancedDisplay=1))
	void AddGameplayTag(FGameplayTag Tag);
	
	UFUNCTION(BlueprintCallable, Category = "AttributeComponent|Tags", meta = (AdvancedDisplay=1))
	void RemoveGameplayTag(FGameplayTag Tag);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AttributeComponent|Tags")
	bool HasGameplayTag(FGameplayTag Tag);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AttributeComponent|Tags")
	bool HasAllGameplayTags(FGameplayTagContainer Tags);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AttributeComponent|Tags")
	bool HasAnyGameplayTags(FGameplayTagContainer Tags);

	/**
	 * This function returns all the gameplay tags that are currently active on this component as a GameplayTagContainer.
	 * Changing the tags in the GameplayTagContainer will not affect the tags on this component.
	 * @return A GameplayTagContainer with all the gameplay tags that are currently active on this component.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AttributeComponent|Tags")
	FGameplayTagContainer GetActiveGameplayTags() const;
	
	/* Utility Functions */

	UFUNCTION(BlueprintCallable, Category = "AttributeComponent|Utility")
	bool HasAuthority() const;
	
	/**
	 * Returns the server time if called on the server.
	 * Returns the clients estimation of the server time if called on the client.
	 * By default, uses GetWorld()->GetGameState()->GetServerWorldTimeSeconds() to get the server time.
	 * Override this function to provide a custom network time synchronisation implementation.
	 * @return The current server time in seconds
	 */
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "AttributeComponent|Utility")
	double GetServerTime();

	UFUNCTION(BlueprintCallable, BlueprintPure, BlueprintCallable, Category = "AttributeComponent|Utility", meta = (DeterminesOutputType = "AttributeHandlerClass", HideSelfPin))
	USimpleAttributeHandler* GetAttributeHandler(FGameplayTag AttributeTag, TSubclassOf<USimpleAttributeHandler> AttributeHandlerClass);
	
	USimpleAttributeHandler* GetStructAttributeHandlerInstance(FGameplayTag AttributeTag, TSubclassOf<USimpleAttributeHandler> HandlerClass);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintNativeEvent, BlueprintPure, BlueprintCallable, Category = "AttributeComponent|Utility")
	USimpleTimeSynchronizer* GetTimeSynchronizerComponent();
	
	UPROPERTY()
	TArray<USimpleAttributeModifier*> InstancedAttributeModifiers;

	UPROPERTY()
	TArray<USimpleAttributeHandler*> InstancedAttributeHandlers;

private:
	USimpleAttributeModifier* GetAttributeModifierInstance(const TSubclassOf<USimpleAttributeModifier>& ModifierClass, FGuid NewModifierID, USimpleAttributeComponent* Instigator, USimpleAttributeComponent* Target, float Magnitude, const FInstancedStruct Context, const bool DoesReplicate);

	UFUNCTION()
	void OnAttributeModifierInitiallyApplied(USimpleAttributeModifier* ModifierInstance);
	
	UFUNCTION()
	void OnAttributeModifierActionStackApplied(USimpleAttributeModifier* ModifierInstance, FModifierActionStackResults ActionResult);

	UFUNCTION()
	void OnAttributeModifierEnded(USimpleAttributeModifier* ModifierInstance, FGameplayTag EndStatus, FInstancedStruct EndContext);

	UFUNCTION()
	void OnAttributeModifierCancelled(USimpleAttributeModifier* ModifierInstance, FGameplayTag EndStatus, FInstancedStruct EndContext);
	
	void ClientOnAttributeModiferStateAdded(const FAttributeModifierState& NewAttributeModiferState);
	void ClientOnAttributeModifierStateChanged(const FAttributeModifierState& ChangedAttributeModiferState);
	void ClientOnAttributeModiferStateRemoved(const FAttributeModifierState& RemovedAttributeModiferState);
	
	void ClientOnAttributeModifierMutationAdded(const FAttributeModifierMutation& NewModifierMutation);
	
	void ClientOnFloatAttributeAdded(const FFloatAttribute& NewFloatAttribute);
	void ClientOnFloatAttributeChanged(const FFloatAttribute& ChangedFloatAttribute);
	void ClientOnFloatAttributeRemoved(const FFloatAttribute& RemovedFloatAttribute);

	void ClientOnStructAttributeAdded(const FStructAttribute& NewStructAttribute);
	void ClientOnStructAttributeChanged(const FStructAttribute& ChangedStructAttribute);
	void ClientOnStructAttributeRemoved(const FStructAttribute& RemovedStructAttribute);

	void ClientOnGameplayTagAdded(const FGameplayTagCounter& NewGameplayTag);
	void ClientOnGameplayTagRemoved(const FGameplayTagCounter& RemovedGameplayTag);
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

public:
	/* Event Dispatchers */

	// Gameplay Tags
	UPROPERTY(BlueprintAssignable, Category = "AttributeComponent|Events|GameplayTags")
	FOnGameplayTagAddedSignature OnGameplayTagAdded;
	
	UPROPERTY(BlueprintAssignable, Category = "AttributeComponent|Events|GameplayTags")
	FOnGameplayTagRemovedSignature OnGameplayTagRemoved;
	
	// Float Attributes
	UPROPERTY(BlueprintAssignable, Category = "AttributeComponent|Events|Attributes|Float")
	FOnFloatAttributeAddedSignature OnFloatAttributeAdded;
	
	UPROPERTY(BlueprintAssignable, Category = "AttributeComponent|Events|Attributes|Float")
	FOnFloatAttributeRemovedSignature OnFloatAttributeRemoved;
	
	UPROPERTY(BlueprintAssignable, Category = "AttributeComponent|Events|Attributes|Float")
	FOnFloatAttributeValueChangedSignature OnFloatAttributeBaseValueChanged;

	UPROPERTY(BlueprintAssignable, Category = "AttributeComponent|Events|Attributes|Float")
	FOnFloatAttributeValueChangedSignature OnFloatAttributeCurrentValueChanged;
	
	UPROPERTY(BlueprintAssignable, Category = "AttributeComponent|Events|Attributes|Float")
	FOnFloatAttributeValueChangedSignature OnFloatAttributeMinBaseValueChanged;

	UPROPERTY(BlueprintAssignable, Category = "AttributeComponent|Events|Attributes|Float")
	FOnFloatAttributeValueChangedSignature OnFloatAttributeMinCurrentValueChanged;
	
	UPROPERTY(BlueprintAssignable, Category = "AttributeComponent|Events|Attributes|Float")
	FOnFloatAttributeValueChangedSignature OnFloatAttributeMaxBaseValueChanged;
	
	UPROPERTY(BlueprintAssignable, Category = "AttributeComponent|Events|Attributes|Float")
	FOnFloatAttributeValueChangedSignature OnFloatAttributeMaxCurrentValueChanged;

	UPROPERTY(BlueprintAssignable, Category = "AttributeComponent|Events|Attributes|Float")
	FOnFloatAttributeBoolChangedSignature OnUseMinBaseValueChanged;

	UPROPERTY(BlueprintAssignable, Category = "AttributeComponent|Events|Attributes|Float")
	FOnFloatAttributeBoolChangedSignature OnUseMinCurrentValueChanged;

	UPROPERTY(BlueprintAssignable, Category = "AttributeComponent|Events|Attributes|Float")
	FOnFloatAttributeBoolChangedSignature OnUseMaxBaseValueChanged;

	UPROPERTY(BlueprintAssignable, Category = "AttributeComponent|Events|Attributes|Float")
	FOnFloatAttributeBoolChangedSignature OnUseMaxCurrentValueChanged;

	UPROPERTY(BlueprintAssignable, Category = "AttributeComponent|Events|Attributes|Float")
	FOnFloatAttributeLimitReachedSignature OnMinBaseValueReached;

	UPROPERTY(BlueprintAssignable, Category = "AttributeComponent|Events|Attributes|Float")
	FOnFloatAttributeLimitReachedSignature OnMinCurrentValueReached;

	UPROPERTY(BlueprintAssignable, Category = "AttributeComponent|Events|Attributes|Float")
	FOnFloatAttributeLimitReachedSignature OnMaxBaseValueReached;
	
	UPROPERTY(BlueprintAssignable, Category = "AttributeComponent|Events|Attributes|Float")
	FOnFloatAttributeLimitReachedSignature OnMaxCurrentValueReached;

	// Struct Attributes
	UPROPERTY(BlueprintAssignable, Category = "AttributeComponent|Events|Attributes|Struct")
	FOnStructAttributeAddedSignature OnStructAttributeAdded;

	UPROPERTY(BlueprintAssignable, Category = "AttributeComponent|Events|Attributes|Struct")
	FOnStructAttributeRemovedSignature OnStructAttributeRemoved;

	UPROPERTY(BlueprintAssignable, Category = "AttributeComponent|Events|Attributes|Struct")
	FOnStructAttributeChangedSignature OnStructAttributeChanged;
};
