#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"
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
    // Constants for visual layout
    static constexpr float DEFAULT_INDENT_AMOUNT = 16.0f;
    static constexpr float DEFAULT_ROW_PADDING = 2.0f;
    static constexpr float PROPERTY_NAME_WIDTH = 150.0f;
    static constexpr float SEPARATOR_THICKNESS = 1.0f;
    static constexpr float SEPARATOR_PADDING = 4.0f;
    static constexpr int32 AUTO_EXPAND_PROPERTY_THRESHOLD = 5;
    static constexpr int32 FLOAT_DISPLAY_PRECISION = 3;

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
     * Formats a float value with thousand separators and limited precision
     * @param Value The float value to format
     * @return Formatted string
     */
    FString FormatFloatValue(double Value) const;

    /**
     * Formats an integer value with thousand separators
     * @param Value The integer value to format
     * @return Formatted string
     */
    FString FormatIntValue(int64 Value) const;

    /**
     * Determines if a property should be auto-expanded based on its complexity
     * @param StructType The struct type to check
     * @return True if should auto-expand
     */
    bool ShouldAutoExpand(const UScriptStruct* StructType) const;

    /**
     * Gets background color based on indentation level
     * @param Indent The indentation level
     * @return Background color
     */
    FLinearColor GetBackgroundColorForIndent(int32 Indent, bool bIsPropertyName) const;

    /**
     * Gets the clean display name for a property (removes generated suffixes)
     * @param Property The property to get the name for
     * @return Clean property name
     */
    FString GetPropertyDisplayName(const FProperty* Property) const;
    
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