#pragma once

#include "CoreMinimal.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeModifier/SimpleAttributeModifierTypes.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeModifier/ModifierActions/ModifierActionTypes.h"
#include "UObject/Object.h"
#include "ModifierAction.generated.h"

class USimpleAttributeModifier;

UCLASS(Blueprintable, Abstract, BlueprintType, EditInlineNew, DefaultToInstanced, CollapseCategories)
class SIMPLEGAMEPLAYABILITYSYSTEM_API UModifierAction : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Config", meta = (DisplayPriority = 0))
	FString Description = "New Modifier Action";

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Config", meta = (DisplayPriority = 0))
	EAttributeModifierActionPolicy ApplicationPolicy = EAttributeModifierActionPolicy::ApplyClientPredicted;
	
	/**
	 * This modifier will trigger when it receives any of these event tags from the OwningModifier
	 */
	UPROPERTY(EditDefaultsOnly, Category="Config", meta = (DisplayPriority = 0))
	FGameplayTagContainer EventTriggers;	
	
	UFUNCTION(BlueprintNativeEvent, Category="Modifier")
	bool CanApply(USimpleAttributeModifier* NewOwningModifier) const;
	virtual bool CanApply_Implementation(USimpleAttributeModifier* NewOwningModifier) const { return true; }
	
	UFUNCTION(BlueprintNativeEvent, Category="Modifier")
	FInstancedStruct ApplyAction(USimpleAttributeModifier* AttributeModifier);
	virtual FInstancedStruct ApplyAction_Implementation(USimpleAttributeModifier* AttributeModifier) { return FInstancedStruct(); }

	/**
	 * If the ApplicationPolicy is set to ApplyClientPredicted, this function will be called
	 * on the client when the server sends the action result and it doesn't match the client's result.
	 * @param ServerResult The value of SnapshotData from the server ApplyAction() call.
	 * @param ClientResult The value of SnapshotData from the client ApplyAction() call.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Modifier")
	void OnClientPredictedCorrection(FInstancedStruct ServerResult, FInstancedStruct ClientResult);
	virtual void OnClientPredictedCorrection_Implementation(FInstancedStruct ServerResult, FInstancedStruct ClientResult) { }

	/**
	 * If the ApplicationPolicy is set to ServerInitiated, this function will be called
	 * on the client when the server sends the action result snapshot.
	 * @param ServerResult The value of SnapshotData from the server ApplyAction() call.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Modifier")
	void OnServerInitiatedResultReceived(FInstancedStruct ServerResult);
	virtual void OnServerInitiatedResultReceived_Implementation(FInstancedStruct ServerResult) { }

	/**
	 * Called when the modifier that is applying this action is cancelled or when the client mispredicts and action applying.
	 * Use this to clean up any resources if required.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Modifier")
	void OnCancelAction();
	virtual void OnCancelAction_Implementation() { }

	/**
	 * Called when the modifier that is applying this action ends normally. Use this to clean up any resources if required.
	 * @param Modifier A reference to the owning modifier that has ended.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Modifier")
	void OnOwningModifierEnded(USimpleAttributeModifier* Modifier);
	virtual void OnOwningModifierEnded_Implementation(USimpleAttributeModifier* Modifier) { }
};
