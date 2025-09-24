#pragma once

#include "CoreMinimal.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeModifier/SimpleAttributeModifier.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeModifier/SimpleAttributeModifierTypes.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeModifier/ModifierActions/ModifierActionTypes.h"
#include "SimpleGameplayAbilitySystem/DefaultTags/DefaultTags.h"
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

	void InitializeAction(FAttributeModifierActionScratchPad& NewScratchPad, USimpleAttributeModifier* NewOwningModifier)
	{
		OwningModifier = NewOwningModifier;
		ScratchPad = NewScratchPad;
	}
	
	UFUNCTION(BlueprintNativeEvent, Category="Modifier")
	bool CanApply() const;
	virtual bool CanApply_Implementation() const { return true; }
	
	UFUNCTION(BlueprintNativeEvent, Category="Modifier")
	FInstancedStruct ApplyAction();
	virtual FInstancedStruct ApplyAction_Implementation() { return FInstancedStruct(); }
	
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

	/**
	 * If the ApplicationPolicy is set to ApplyClientPredicted, this function will be called
	 * on the client when the server sends the action result and it doesn't match the client's result.
	 * @param ServerInputScratchPad
	 * @param ServerResult The value of SnapshotData from the server ApplyAction() call.
	 * @param ClientInputScratchPad
	 * @param ClientResult The value of SnapshotData from the client ApplyAction() call.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Modifier")
	void OnClientPredictedCorrection(FAttributeModifierActionScratchPad ServerInputScratchPad, FInstancedStruct ServerResult, FAttributeModifierActionScratchPad
	                                 ClientInputScratchPad, FInstancedStruct ClientResult);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Modifier")
	USimpleAttributeModifier* GetOwningModifier() const { return OwningModifier; }
	
	UFUNCTION(BlueprintCallable, Category="Modifier")
	void AddScratchPadTag(FGameplayTag ScratchPadTag);

	UFUNCTION(BlueprintCallable, Category="Modifier")
	void RemoveScratchPadTag(FGameplayTag ScratchPadTag);
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Modifier")
	float GetScratchPadValue(FGameplayTag ScratchPadTag, bool& WasFound) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Modifier")
	bool HasScratchPadValue(FGameplayTag ScratchPadTag) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Modifier")
	bool HasScratchPadTag(FGameplayTag ScratchPadTag) const;

	UFUNCTION(BlueprintCallable, Category="Modifier")
	void SetScratchPadValue(FGameplayTag ScratchPadTag, float Value);

	UFUNCTION(BlueprintCallable, Category="Modifier")
	void IncrementScratchPadValue(FGameplayTag ScratchPadTag, float IncrementAmount);

protected:
	UPROPERTY(BlueprintreadWrite, Category="Modifier")
	FAttributeModifierActionScratchPad ScratchPad;
	
	UPROPERTY()
	USimpleAttributeModifier* OwningModifier;
};
