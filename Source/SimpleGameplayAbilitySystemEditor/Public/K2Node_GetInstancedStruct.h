#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "K2Node_GetInstancedStruct.generated.h"

/**
 * Custom K2Node that extracts a typed struct from an FInstancedStruct with validation.
 *
 * Features:
 * - Takes FInstancedStruct and UScriptStruct* (type selector) as inputs
 * - Outputs the actual struct type (dynamically changes based on UScriptStruct selection)
 * - Provides Valid/Invalid execution paths based on type matching
 * - The output struct pin type automatically updates when the UScriptStruct pin is connected or changed
 */
UCLASS()
class SIMPLEGAMEPLAYABILITYSYSTEMEDITOR_API UK2Node_GetInstancedStruct : public UK2Node
{
	GENERATED_BODY()

public:
	UK2Node_GetInstancedStruct();

	// Pin names
	static const FName InstancedStructPinName;
	static const FName StructTypePinName;
	static const FName ValidPinName;
	static const FName InvalidPinName;
	static const FName OutStructPinName;

	//~ Begin UObject Interface
	virtual void PostLoad() override;
	//~ End UObject Interface

	//~ Begin UEdGraphNode Interface
	virtual void AllocateDefaultPins() override;
	virtual void PostReconstructNode() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
	//~ End UEdGraphNode Interface

	//~ Begin UK2Node Interface
	virtual void PreloadRequiredAssets() override;
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual FText GetMenuCategory() const override;
	virtual bool IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const override;
	virtual void NotifyPinConnectionListChanged(UEdGraphPin* Pin) override;
	virtual void PinDefaultValueChanged(UEdGraphPin* Pin) override;
	virtual bool IsNodePure() const override { return false; }
	//~ End UK2Node Interface

private:
	/** Helper to get all pins by name */
	UEdGraphPin* GetInstancedStructPin() const;
	UEdGraphPin* GetStructTypePin() const;
	UEdGraphPin* GetValidPin() const;
	UEdGraphPin* GetInvalidPin() const;
	UEdGraphPin* GetOutStructPin() const;

	/** Updates the OutStruct pin type based on the StructType pin value */
	void RefreshOutputStructType();

	/** Gets the currently selected struct type from the StructType pin */
	UScriptStruct* GetSelectedStructType() const;

	/** Cached struct type for proper serialization - synchronized with StructType pin */
	UPROPERTY()
	TObjectPtr<UScriptStruct> CachedStructType;
};