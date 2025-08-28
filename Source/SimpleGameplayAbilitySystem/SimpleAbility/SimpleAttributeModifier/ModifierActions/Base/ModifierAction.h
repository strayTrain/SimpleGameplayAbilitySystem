#pragma once

#include "CoreMinimal.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleAttributeModifier/SimpleAttributeModifier.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleAttributeModifier/SimpleAttributeModifierTypes.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleAttributeModifier/ModifierActions/ModifierActionTypes.h"
#include "UObject/Object.h"
#include "ModifierAction.generated.h"

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
	 * Used to filter which phases this action will trigger. If left empty, the action will apply in default order as defined in the Actions stack.
	 */
	UPROPERTY(EditDefaultsOnly, Category="Config", meta = (DisplayPriority = 0))
	TArray<EAttributeModifierPhase> ApplicationPhaseFilter;	
	
	UFUNCTION(BlueprintNativeEvent, Category="Modifier")
	bool ShouldApply(USimpleAttributeModifier* NewOwningModifier) const;
	virtual bool ShouldApply_Implementation(USimpleAttributeModifier* NewOwningModifier) const { return true; }

	void InitializeAction(USimpleAttributeModifier* NewOwningModifier)
	{
		OwningModifier = NewOwningModifier;
	}
	
	UFUNCTION(BlueprintNativeEvent, Category="Modifier")
	bool ApplyAction(FInstancedStruct& SnapshotData);
	virtual bool ApplyAction_Implementation(FInstancedStruct& SnapshotData) { return true; }

	/**
	 * If the ApplicationPolicy is set to ApplyClientPredicted, this function will be called
	 * on the client when the server sends the action result and it doesn't match the client's result.
	 * @param ServerSnapshot The value of SnapshotData from the server ApplyAction() call.
	 * @param ClientSnapshot The value of SnapshotData from the client ApplyAction() call.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Modifier")
	void OnClientPredictedCorrection(FInstancedStruct ServerSnapshot, FInstancedStruct ClientSnapshot);
	virtual void OnClientPredictedCorrection_Implementation(FInstancedStruct ServerSnapshot, FInstancedStruct ClientSnapshot) { }

	/**
	 * If the ApplicationPolicy is set to ServerInitiated, this function will be called
	 * on the client when the server sends the action result snapshot.
	 * @param ServerSnapshot The value of SnapshotData from the server ApplyAction() call.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Modifier")
	void OnServerInitiatedResultReceived(FInstancedStruct ServerSnapshot);
	virtual void OnServerInitiatedResultReceived_Implementation(FInstancedStruct ServerSnapshot) { }

	/**
	 * Called when the modifier that is applying this action is cancelled or when the client mispredicts and action applying.
	 * Use this to clean up any resources if required.
	 * @param Modifier A reference to the owning modifier that is being cancelled.
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

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Modifier")
	FAttributeModifierActionScratchPad& GetScratchPad() const
	{
		return OwningModifier->GetModifierActionScratchPad();
	}
protected:
	UPROPERTY(BlueprintReadOnly)
	USimpleAttributeModifier* OwningModifier;
};
