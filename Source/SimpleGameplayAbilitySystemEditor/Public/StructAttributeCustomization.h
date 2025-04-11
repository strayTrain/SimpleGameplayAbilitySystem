#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"
#include "SimpleGameplayAbilitySystem/SimpleGameplayAbilityComponent/SimpleAbilityComponentTypes.h"
#include "Widgets/SBoxPanel.h"

class SIMPLEGAMEPLAYABILITYSYSTEMEDITOR_API FStructAttributeCustomization : public IPropertyTypeCustomization
{
public:
    static TSharedRef<IPropertyTypeCustomization> MakeInstance();
    
    // Constructor and destructor
    FStructAttributeCustomization() = default;
    virtual ~FStructAttributeCustomization();

    // IPropertyTypeCustomization interface
    virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
    virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
    // End of IPropertyTypeCustomization interface

private:
    TSharedPtr<IPropertyHandle> AttributeValueHandle;
    TSharedPtr<IPropertyHandle> StructTypeHandle;
    TSharedPtr<SVerticalBox> StructContentWidget;
    TSharedPtr<IPropertyHandle> ParentPropertyHandle;
    
    // Map to track expansion state of properties
    TMap<FName, bool> ExpandedState;
    
    /**
     * Recursively displays the contents of a struct in the provided target widget
     * @param StructType The UScriptStruct type definition
     * @param StructData Pointer to the actual struct data
     * @param TargetWidget The widget where the struct content should be displayed
     * @param Indent Indentation level for nested display
     */
    void DisplayStructContents(const UScriptStruct* StructType, const void* StructData, TSharedRef<SVerticalBox> TargetWidget, int32 Indent);
    
    /**
     * Gets a string representation of a property value
     * @param Property The property definition
     * @param ValuePtr Pointer to the property value data
     * @return String representation of the property value
     */
    FString GetPropertyValueString(const FProperty* Property, const void* ValuePtr);
    
    /**
     * Toggles the expanded state for the given key
     * @param ExpanderKey The key to toggle
     * @param bIsExpanded The new expansion state
     * @return Handled reply
     */
    FReply ToggleExpanded(FName ExpanderKey, bool bIsExpanded);
    
    /**
     * Gets whether the section for the given key is expanded
     * @param ExpanderKey The key to check
     * @return True if expanded, false otherwise
     */
    bool IsExpanded(FName ExpanderKey) const;

    /** Called whenever the struct attribute value is changed through USimpleGameplayAbilityComponent::SetStructAttributeValue */
    void OnStructAttributeValueChanged();
};