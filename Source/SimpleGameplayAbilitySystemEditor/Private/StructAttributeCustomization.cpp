#include "StructAttributeCustomization.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeComponentTypes.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Views/SExpanderArrow.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Images/SImage.h"
#include "Styling/AppStyle.h"

#if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5)
    #include "StructUtils/InstancedStruct.h"
#else
    #include "InstancedStruct.h"
#endif

TSharedRef<IPropertyTypeCustomization> FStructAttributeCustomization::MakeInstance()
{
    return MakeShareable(new FStructAttributeCustomization());
}

FStructAttributeCustomization::~FStructAttributeCustomization()
{
    // Clean up any registered delegates
    if (ParentPropertyHandle.IsValid())
    {
        void* RawStructAttributeData = nullptr;
        if (ParentPropertyHandle->GetValueData(RawStructAttributeData) == FPropertyAccess::Success)
        {
            FStructAttribute* StructAttributePtr = static_cast<FStructAttribute*>(RawStructAttributeData);
            if (StructAttributePtr)
            {
                // Unbind from the OnValueChanged delegate
                StructAttributePtr->OnValueChanged.Unbind();
            }
        }
        
        ParentPropertyHandle->SetOnPropertyValueChanged(FSimpleDelegate());
    }

    if (AttributeValueHandle.IsValid())
    {
        AttributeValueHandle->SetOnPropertyValueChanged(FSimpleDelegate());
    }

    if (StructTypeHandle.IsValid())
    {
        StructTypeHandle->SetOnPropertyValueChanged(FSimpleDelegate());
    }
}

void FStructAttributeCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
    // Store parent handle for cleanup
    ParentPropertyHandle = PropertyHandle;

    // Find the handles to the properties we care about
    AttributeValueHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FStructAttribute, AttributeValue));
    StructTypeHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FStructAttribute, StructType));

    TSharedPtr<IPropertyHandle> AttributeNameHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FStructAttribute, AttributeName));
    TSharedPtr<IPropertyHandle> AttributeTagHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FStructAttribute, AttributeTag));

    // Also register if we have a child array
    TSharedPtr<IPropertyHandleArray> ArrayHandle = PropertyHandle->GetParentHandle() != nullptr ? PropertyHandle->GetParentHandle()->AsArray() : nullptr;

    // Create the header row
    HeaderRow
    .NameContent()
    [
        PropertyHandle->CreatePropertyNameWidget()
    ]
    .ValueContent()
    .MinDesiredWidth(250.0f)
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot()
        .FillWidth(1.0f)
        .VAlign(VAlign_Center)
        [
            AttributeNameHandle->CreatePropertyValueWidget()
        ]
        + SHorizontalBox::Slot()
        .FillWidth(1.0f)
        .VAlign(VAlign_Center)
        [
            AttributeTagHandle->CreatePropertyValueWidget()
        ]
    ];
}

void FStructAttributeCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
    // Add all the default properties
    uint32 NumChildren;
    PropertyHandle->GetNumChildren(NumChildren);

    for (uint32 ChildIndex = 0; ChildIndex < NumChildren; ++ChildIndex)
    {
        TSharedPtr<IPropertyHandle> ChildHandle = PropertyHandle->GetChildHandle(ChildIndex);
        if (ChildHandle->GetProperty()->GetName() != TEXT("AttributeValue"))
        {
            ChildBuilder.AddProperty(ChildHandle.ToSharedRef());
        }
    }

    // Try direct access to struct data
    void* RawStructAttributeData = nullptr;
    if (PropertyHandle->GetValueData(RawStructAttributeData) == FPropertyAccess::Success)
    {
        // Cast to FStructAttribute
        FStructAttribute* StructAttributePtr = static_cast<FStructAttribute*>(RawStructAttributeData);

        if (StructAttributePtr)
        {
            // Now we have direct access to the struct data
            UScriptStruct* StructType = StructAttributePtr->StructType;
            FInstancedStruct& AttributeValue = StructAttributePtr->AttributeValue;

            if (StructType && AttributeValue.IsValid())
            {
                // Listen for changes in the struct data
                StructAttributePtr->OnValueChanged.BindSP(this, &FStructAttributeCustomization::OnStructAttributeValueChanged);
                
                const void* StructData = AttributeValue.GetMemory();

                if (StructData)
                {
                    // Create a key for the main struct data section
                    FName StructDataKey = TEXT("StructData_Main");

                    // Auto-expand if struct is small
                    if (!ExpandedState.Contains(StructDataKey))
                    {
                        ExpandedState.Add(StructDataKey, ShouldAutoExpand(StructType));
                    }

                    // Create a container for the struct data section
                    FDetailWidgetRow& StructRow = ChildBuilder.AddCustomRow(FText::FromString(TEXT("Struct Data")));

                    // Remove the default left indent to align with other properties
                    StructRow.WholeRowContent()
                    [
                        SNew(SVerticalBox)
                        // Collapsible header
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(FMargin(0, 5, 0, 5))
                        [
                            SNew(SHorizontalBox)
                            // Expander button
                            + SHorizontalBox::Slot()
                            .AutoWidth()
                            .VAlign(VAlign_Center)
                            .Padding(FMargin(0, 0, 4, 0))
                            [
                                SNew(SButton)
                                .ButtonStyle(FAppStyle::Get(), "NoBorder")
                                .ContentPadding(0)
                                .OnClicked_Lambda([this, StructDataKey]() {
                                    ToggleExpanded(StructDataKey, !IsExpanded(StructDataKey));
                                    return FReply::Handled();
                                })
                                [
                                    SNew(SImage)
                                    .Image_Lambda([this, StructDataKey]() {
                                        return IsExpanded(StructDataKey) ?
                                            FAppStyle::GetBrush("TreeArrow_Expanded") :
                                            FAppStyle::GetBrush("TreeArrow_Collapsed");
                                    })
                                ]
                            ]
                            // Header text
                            + SHorizontalBox::Slot()
                            .FillWidth(1.0f)
                            .VAlign(VAlign_Center)
                            [
                                SNew(STextBlock)
                                .Text(FText::FromString(FString::Printf(TEXT("Struct Data (%s)"), *StructType->GetName())))
                                .Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
                                .ColorAndOpacity(FAppStyle::GetSlateColor("Colors.Foreground"))
                            ]
                        ]
                        // Collapsible content
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0)
                        [
                            SNew(SBox)
                            .Visibility_Lambda([this, StructDataKey]() {
                                return IsExpanded(StructDataKey) ? EVisibility::Visible : EVisibility::Collapsed;
                            })
                            [
                                SAssignNew(StructContentWidget, SVerticalBox)
                            ]
                        ]
                    ];

                    // Display the struct contents if expanded
                    if (IsExpanded(StructDataKey))
                    {
                        DisplayStructContents(StructType, StructData, StructContentWidget.ToSharedRef(), 0);
                    }
                    return;
                }
            }
        }
    }

    // Add an improved message if we couldn't display the struct data
    void* RawStructData = nullptr;
    FStructAttribute* StructAttrPtr = nullptr;
    if (PropertyHandle->GetValueData(RawStructData) == FPropertyAccess::Success)
    {
        StructAttrPtr = static_cast<FStructAttribute*>(RawStructData);
    }

    FString ErrorMessage;
    FString ErrorDetail;
    const FSlateBrush* ErrorIcon = FAppStyle::GetBrush("Icons.Info");

    if (!StructAttrPtr)
    {
        ErrorMessage = TEXT("No Struct Attribute Data");
        ErrorDetail = TEXT("Unable to access the struct attribute. This may be a multi-selection.");
        ErrorIcon = FAppStyle::GetBrush("Icons.Warning");
    }
    else if (!StructAttrPtr->StructType)
    {
        ErrorMessage = TEXT("No Struct Type Selected");
        ErrorDetail = TEXT("Please select a struct type from the 'Struct Type' property above.");
        ErrorIcon = FAppStyle::GetBrush("Icons.Info");
    }
    else if (!StructAttrPtr->AttributeValue.IsValid())
    {
        ErrorMessage = TEXT("Struct Data Not Available");
        ErrorDetail = TEXT("You can see the underlying struct data at runtime when the game is running.");
        ErrorIcon = FAppStyle::GetBrush("Icons.Warning");
    }
    else
    {
        ErrorMessage = TEXT("Cannot Access Struct Data");
        ErrorDetail = TEXT("An unknown error occurred while trying to display the struct data.");
        ErrorIcon = FAppStyle::GetBrush("Icons.Error");
    }

    ChildBuilder.AddCustomRow(FText::FromString(TEXT("Struct Data")))
    [
        SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
        .Padding(FMargin(8.0f, 12.0f))
        [
            SNew(SHorizontalBox)
            // Icon
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Top)
            .Padding(FMargin(0, 0, 8, 0))
            [
                SNew(SImage)
                .Image(ErrorIcon)
                .DesiredSizeOverride(FVector2D(16, 16))
            ]
            // Text content
            + SHorizontalBox::Slot()
            .FillWidth(1.0)
            .VAlign(VAlign_Center)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(FMargin(0, 0, 0, 4))
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(ErrorMessage))
                    .Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
                    .ColorAndOpacity(FAppStyle::GetSlateColor("Colors.Foreground"))
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(ErrorDetail))
                    .Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
                    .ColorAndOpacity(FAppStyle::GetSlateColor("Colors.ForegroundHover"))
                    .AutoWrapText(true)
                ]
            ]
        ]
    ];
}

bool FStructAttributeCustomization::IsExpanded(FName ExpanderKey) const
{
    const bool* FoundValue = ExpandedState.Find(ExpanderKey);
    return FoundValue ? *FoundValue : false; // Default to collapsed if not found
}

void FStructAttributeCustomization::OnStructAttributeValueChanged()
{
    // Access the struct data again and refresh the display
    void* RawStructAttributeData = nullptr;
    if (ParentPropertyHandle.IsValid() && 
        ParentPropertyHandle->GetValueData(RawStructAttributeData) == FPropertyAccess::Success)
    {
        FStructAttribute* StructAttributePtr = static_cast<FStructAttribute*>(RawStructAttributeData);
        if (StructAttributePtr && StructAttributePtr->StructType && StructAttributePtr->AttributeValue.IsValid())
        {
            const void* StructData = StructAttributePtr->AttributeValue.GetMemory();
            if (StructData && StructContentWidget.IsValid())
            {
                // Clear existing content
                StructContentWidget->ClearChildren();
                
                // Rebuild the display
                DisplayStructContents(StructAttributePtr->StructType, StructData, StructContentWidget.ToSharedRef(), 0);
            }
        }
    }
}

void FStructAttributeCustomization::DisplayStructContents(const UScriptStruct* StructType, const void* StructData, TSharedRef<SVerticalBox> TargetWidget, int32 Indent)
{
    // Get background colors based on indent level for visual hierarchy
    const FLinearColor PropertyNameBgColor = GetBackgroundColorForIndent(Indent, true);
    const FLinearColor PropertyValueBgColor = GetBackgroundColorForIndent(Indent, false);

    for (TFieldIterator<FProperty> PropIt(StructType); PropIt; ++PropIt)
    {
        FProperty* Property = *PropIt;
        const void* ValuePtr = Property->ContainerPtrToValuePtr<void>(StructData);

        // Create an expander key for this property
        FName PropertyKey = *FString::Printf(TEXT("Prop_%s_%p"), *Property->GetName(), Property);

        // Handle different property types
        if (const FStructProperty* StructProp = CastField<FStructProperty>(Property))
        {
            // Auto-expand small nested structs
            if (!ExpandedState.Contains(PropertyKey))
            {
                ExpandedState.Add(PropertyKey, ShouldAutoExpand(StructProp->Struct));
            }

            // Create expandable row for nested struct
            TSharedRef<SHorizontalBox> PropertyRow = SNew(SHorizontalBox);

            // Add the expander button
            PropertyRow->AddSlot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                .HAlign(HAlign_Left)
                .Padding(FMargin(Indent * DEFAULT_INDENT_AMOUNT, 0, 0, 0))
                [
                    SNew(SButton)
                    .ButtonStyle(FAppStyle::Get(), "NoBorder")
                    .ContentPadding(0)
                    .OnClicked_Lambda([this, PropertyKey]() {
                        ToggleExpanded(PropertyKey, !IsExpanded(PropertyKey));
                        return FReply::Handled();
                    })
                    [
                        SNew(SImage)
                        .Image_Lambda([this, PropertyKey]() {
                            return IsExpanded(PropertyKey) ?
                                FAppStyle::GetBrush("TreeArrow_Expanded") :
                                FAppStyle::GetBrush("TreeArrow_Collapsed");
                        })
                    ]
                ];

            // Add the property name with background
            PropertyRow->AddSlot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                .Padding(FMargin(2.0f, 0, 0, 0))
                [
                    SNew(SBorder)
                    .BorderImage(FAppStyle::GetBrush("DetailsView.CategoryMiddle"))
                    .BorderBackgroundColor(PropertyNameBgColor)
                    .Padding(FMargin(4.0f, 2.0f))
                    [
                        SNew(SBox)
                        .WidthOverride(PROPERTY_NAME_WIDTH)
                        [
                            SNew(STextBlock)
                            .Text(FText::FromString(GetPropertyDisplayName(Property)))
                            .ToolTipText(FText::FromString(FString::Printf(TEXT("%s\n\nInternal Name: %s"), *Property->GetToolTipText().ToString(), *Property->GetName())))
                            .Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
                            .ColorAndOpacity(FAppStyle::GetSlateColor("Colors.Foreground"))
                        ]
                    ]
                ];

            // Add the struct type with background
            PropertyRow->AddSlot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                .Padding(FMargin(0, 0, 0, 0))
                [
                    SNew(SBorder)
                    .BorderImage(FAppStyle::GetBrush("DetailsView.CategoryMiddle"))
                    .BorderBackgroundColor(PropertyValueBgColor)
                    .Padding(FMargin(4.0f, 2.0f))
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(FString::Printf(TEXT("(%s)"), *StructProp->Struct->GetName())))
                        .Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
                        .ColorAndOpacity(FSlateColor::UseSubduedForeground())
                    ]
                ];

            // Add the row to the target widget
            TargetWidget->AddSlot()
                .AutoHeight()
                .Padding(FMargin(0, DEFAULT_ROW_PADDING))
                [
                    PropertyRow
                ];

            // Create a content box for the nested properties
            TSharedRef<SVerticalBox> StructContentBox = SNew(SVerticalBox);

            // Add the content box, with visibility controlled by our expansion state
            TSharedRef<SWidget> StructContentWrapper =
                SNew(SBox)
                .Visibility_Lambda([this, PropertyKey]() {
                    return IsExpanded(PropertyKey) ? EVisibility::Visible : EVisibility::Collapsed;
                })
                [
                    StructContentBox
                ];

            TargetWidget->AddSlot()
                .AutoHeight()
                .Padding(0)
                [
                    StructContentWrapper
                ];

            // Recursively display the nested struct's contents
            DisplayStructContents(StructProp->Struct, ValuePtr, StructContentBox, Indent + 1);
        }
        else if (const FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property))
        {
            // For array properties
            FScriptArrayHelper ArrayHelper(ArrayProp, ValuePtr);
            int32 NumElements = ArrayHelper.Num();

            // Create a row for the array
            TSharedRef<SHorizontalBox> PropertyRow = SNew(SHorizontalBox);

            // Add the expander button
            PropertyRow->AddSlot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                .HAlign(HAlign_Left)
                .Padding(FMargin(Indent * DEFAULT_INDENT_AMOUNT, 0, 0, 0))
                [
                    SNew(SButton)
                    .ButtonStyle(FAppStyle::Get(), "NoBorder")
                    .ContentPadding(0)
                    .OnClicked_Lambda([this, PropertyKey]() {
                        ToggleExpanded(PropertyKey, !IsExpanded(PropertyKey));
                        return FReply::Handled();
                    })
                    [
                        SNew(SImage)
                        .Image_Lambda([this, PropertyKey]() {
                            return IsExpanded(PropertyKey) ?
                                FAppStyle::GetBrush("TreeArrow_Expanded") :
                                FAppStyle::GetBrush("TreeArrow_Collapsed");
                        })
                    ]
                ];

            // Add the property name with background
            PropertyRow->AddSlot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                .Padding(FMargin(2.0f, 0, 0, 0))
                [
                    SNew(SBorder)
                    .BorderImage(FAppStyle::GetBrush("DetailsView.CategoryMiddle"))
                    .BorderBackgroundColor(PropertyNameBgColor)
                    .Padding(FMargin(4.0f, 2.0f))
                    [
                        SNew(SBox)
                        .WidthOverride(PROPERTY_NAME_WIDTH)
                        [
                            SNew(STextBlock)
                            .Text(FText::FromString(GetPropertyDisplayName(Property)))
                            .ToolTipText(FText::FromString(FString::Printf(TEXT("%s\n\nInternal Name: %s"), *Property->GetToolTipText().ToString(), *Property->GetName())))
                            .Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
                            .ColorAndOpacity(FAppStyle::GetSlateColor("Colors.Foreground"))
                        ]
                    ]
                ];

            // Add the element count with background
            PropertyRow->AddSlot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                .Padding(FMargin(0, 0, 0, 0))
                [
                    SNew(SBorder)
                    .BorderImage(FAppStyle::GetBrush("DetailsView.CategoryMiddle"))
                    .BorderBackgroundColor(PropertyValueBgColor)
                    .Padding(FMargin(4.0f, 2.0f))
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(FString::Printf(TEXT("(%d elements)"), NumElements)))
                        .Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
                        .ColorAndOpacity(FSlateColor::UseSubduedForeground())
                    ]
                ];

            // Add the row to the target widget
            TargetWidget->AddSlot()
                .AutoHeight()
                .Padding(FMargin(0, DEFAULT_ROW_PADDING))
                [
                    PropertyRow
                ];

            // Create a content box for the array elements
            TSharedRef<SVerticalBox> ArrayContentBox = SNew(SVerticalBox);

            // Add the content box, with visibility controlled by our expansion state
            TSharedRef<SWidget> ArrayContentWrapper =
                SNew(SBox)
                .Visibility_Lambda([this, PropertyKey]() {
                    return IsExpanded(PropertyKey) ? EVisibility::Visible : EVisibility::Collapsed;
                })
                [
                    ArrayContentBox
                ];

            TargetWidget->AddSlot()
                .AutoHeight()
                .Padding(0)
                [
                    ArrayContentWrapper
                ];

            // Add each array element
            for (int32 ArrayIndex = 0; ArrayIndex < NumElements; ++ArrayIndex)
            {
                void* ElementPtr = ArrayHelper.GetRawPtr(ArrayIndex);

                // Create an element key
                FName ElementKey = *FString::Printf(TEXT("Elem_%s_%d"), *Property->GetName(), ArrayIndex);

                // Handle elements differently based on type
                if (const FStructProperty* InnerStructProp = CastField<FStructProperty>(ArrayProp->Inner))
                {
                    // For arrays of structs - create an element row with expandable content

                    // Create a wrapper for the element row
                    TSharedRef<SHorizontalBox> ElementRow = SNew(SHorizontalBox);

                    // Add the expander button with extra indent
                    ElementRow->AddSlot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        .HAlign(HAlign_Left)
                        .Padding(FMargin((Indent + 1) * DEFAULT_INDENT_AMOUNT, 0, 0, 0))
                        [
                            SNew(SButton)
                            .ButtonStyle(FAppStyle::Get(), "NoBorder")
                            .ContentPadding(0)
                            .OnClicked_Lambda([this, ElementKey]() {
                                ToggleExpanded(ElementKey, !IsExpanded(ElementKey));
                                return FReply::Handled();
                            })
                            [
                                SNew(SImage)
                                .Image_Lambda([this, ElementKey]() {
                                    return IsExpanded(ElementKey) ?
                                        FAppStyle::GetBrush("TreeArrow_Expanded") :
                                        FAppStyle::GetBrush("TreeArrow_Collapsed");
                                })
                            ]
                        ];

                    // Add the element index with background
                    ElementRow->AddSlot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        .Padding(FMargin(2.0f, 0, 0, 0))
                        [
                            SNew(SBorder)
                            .BorderImage(FAppStyle::GetBrush("DetailsView.CategoryMiddle"))
                            .BorderBackgroundColor(PropertyNameBgColor)
                            .Padding(FMargin(4.0f, 2.0f))
                            [
                                SNew(SBox)
                                .WidthOverride(PROPERTY_NAME_WIDTH)
                                [
                                    SNew(STextBlock)
                                    .Text(FText::FromString(FString::Printf(TEXT("[%d]"), ArrayIndex)))
                                    .Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
                                    .ColorAndOpacity(FAppStyle::GetSlateColor("Colors.Foreground"))
                                ]
                            ]
                        ];

                    // Add the struct type with background
                    ElementRow->AddSlot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        .Padding(FMargin(0, 0, 0, 0))
                        [
                            SNew(SBorder)
                            .BorderImage(FAppStyle::GetBrush("DetailsView.CategoryMiddle"))
                            .BorderBackgroundColor(PropertyValueBgColor)
                            .Padding(FMargin(4.0f, 2.0f))
                            [
                                SNew(STextBlock)
                                .Text(FText::FromString(FString::Printf(TEXT("(%s)"), *InnerStructProp->Struct->GetName())))
                                .Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
                                .ColorAndOpacity(FSlateColor::UseSubduedForeground())
                            ]
                        ];

                    // Add the element row to the array content
                    ArrayContentBox->AddSlot()
                        .AutoHeight()
                        .Padding(FMargin(0, DEFAULT_ROW_PADDING))
                        [
                            ElementRow
                        ];

                    // Create a content box for the struct properties
                    TSharedRef<SVerticalBox> ElementStructBox = SNew(SVerticalBox);

                    // Add the struct content, with visibility controlled by our expansion state
                    TSharedRef<SWidget> ElementStructWrapper =
                        SNew(SBox)
                        .Visibility_Lambda([this, ElementKey]() {
                            return IsExpanded(ElementKey) ? EVisibility::Visible : EVisibility::Collapsed;
                        })
                        [
                            ElementStructBox
                        ];

                    ArrayContentBox->AddSlot()
                        .AutoHeight()
                        .Padding(0)
                        [
                            ElementStructWrapper
                        ];

                    // Recursively display the struct's properties
                    DisplayStructContents(InnerStructProp->Struct, ElementPtr, ElementStructBox, Indent + 2);
                }
                else
                {
                    // For arrays of simple types - just show the value directly
                    FString ValueString = GetPropertyValueString(ArrayProp->Inner, ElementPtr);

                    // Create the array element row
                    ArrayContentBox->AddSlot()
                        .AutoHeight()
                        .Padding(FMargin(0, DEFAULT_ROW_PADDING))
                        [
                            SNew(SHorizontalBox)
                            // Left indent
                            + SHorizontalBox::Slot()
                            .AutoWidth()
                            .VAlign(VAlign_Center)
                            .Padding(FMargin((Indent + 1) * DEFAULT_INDENT_AMOUNT + DEFAULT_INDENT_AMOUNT, 0, 0, 0))
                            // Element index with background
                            + SHorizontalBox::Slot()
                            .AutoWidth()
                            .VAlign(VAlign_Center)
                            [
                                SNew(SBorder)
                                .BorderImage(FAppStyle::GetBrush("DetailsView.CategoryMiddle"))
                                .BorderBackgroundColor(PropertyNameBgColor)
                                .Padding(FMargin(4.0f, 2.0f))
                                [
                                    SNew(SBox)
                                    .WidthOverride(PROPERTY_NAME_WIDTH)
                                    [
                                        SNew(STextBlock)
                                        .Text(FText::FromString(FString::Printf(TEXT("[%d]"), ArrayIndex)))
                                        .Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
                                        .ColorAndOpacity(FAppStyle::GetSlateColor("Colors.Foreground"))
                                    ]
                                ]
                            ]
                            // Value with background
                            + SHorizontalBox::Slot()
                            .FillWidth(1.0f)
                            .VAlign(VAlign_Center)
                            .Padding(FMargin(0, 0, 0, 0))
                            [
                                SNew(SBorder)
                                .BorderImage(FAppStyle::GetBrush("DetailsView.CategoryMiddle"))
                                .BorderBackgroundColor(PropertyValueBgColor)
                                .Padding(FMargin(4.0f, 2.0f))
                                [
                                    SNew(STextBlock)
                                    .Text(FText::FromString(ValueString))
                                    .Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
                                    .ColorAndOpacity(FAppStyle::GetSlateColor("Colors.Foreground"))
                                ]
                            ]
                        ];
                }
            }
        }
        else
        {
            // For simple property types - display in a two-column layout with copy button
            FString ValueString = GetPropertyValueString(Property, ValuePtr);

            // Create the property row
            TargetWidget->AddSlot()
                .AutoHeight()
                .Padding(FMargin(0, DEFAULT_ROW_PADDING))
                [
                    SNew(SHorizontalBox)
                    // Left indent
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .VAlign(VAlign_Center)
                    .Padding(FMargin(Indent * DEFAULT_INDENT_AMOUNT + DEFAULT_INDENT_AMOUNT, 0, 0, 0))
                    // Property name with lighter background
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .VAlign(VAlign_Center)
                    .HAlign(HAlign_Left)
                    [
                        SNew(SBorder)
                        .BorderImage(FAppStyle::GetBrush("DetailsView.CategoryMiddle"))
                        .BorderBackgroundColor(PropertyNameBgColor)
                        .Padding(FMargin(4.0f, 2.0f))
                        [
                            SNew(SBox)
                            .WidthOverride(PROPERTY_NAME_WIDTH)
                            [
                                SNew(STextBlock)
                                .Text(FText::FromString(GetPropertyDisplayName(Property)))
                                .ToolTipText(FText::FromString(FString::Printf(TEXT("%s\n\nInternal Name: %s"), *Property->GetToolTipText().ToString(), *Property->GetName())))
                                .Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
                                .ColorAndOpacity(FAppStyle::GetSlateColor("Colors.Foreground"))
                            ]
                        ]
                    ]
                    // Property value with slightly darker background
                    + SHorizontalBox::Slot()
                    .FillWidth(1.0f)
                    .VAlign(VAlign_Center)
                    .Padding(FMargin(0, 0, 0, 0))
                    [
                        SNew(SBorder)
                        .BorderImage(FAppStyle::GetBrush("DetailsView.CategoryMiddle"))
                        .BorderBackgroundColor(PropertyValueBgColor)
                        .Padding(FMargin(4.0f, 2.0f))
                        [
                            SNew(SEditableText)
                            .Text(FText::FromString(ValueString))
                            .Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
                            .ColorAndOpacity(FAppStyle::GetSlateColor("Colors.Foreground"))
                            .IsReadOnly(true)
                            .SelectAllTextWhenFocused(true)
                        ]
                    ]
                ];
        }
    }

    // Add a subtle separator at the end if there's content
    if (Indent == 0)
    {
        TargetWidget->AddSlot()
            .AutoHeight()
            .Padding(FMargin(0, SEPARATOR_PADDING, 0, SEPARATOR_PADDING))
            [
                SNew(SBorder)
                .BorderImage(FAppStyle::GetBrush("WhiteBrush"))
                .BorderBackgroundColor(FLinearColor(0.5f, 0.5f, 0.5f, 0.1f))
                .Padding(0)
                [
                    SNew(SBox)
                    .HeightOverride(SEPARATOR_THICKNESS)
                ]
            ];
    }
}

FReply FStructAttributeCustomization::ToggleExpanded(FName ExpanderKey, bool bIsExpanded)
{
    ExpandedState.FindOrAdd(ExpanderKey) = bIsExpanded;
    return FReply::Handled();
}

FString FStructAttributeCustomization::GetPropertyValueString(const FProperty* Property, const void* ValuePtr)
{
    if (!Property || !ValuePtr)
    {
        return TEXT("N/A");
    }

    // Handle different property types
    if (const FNumericProperty* NumericProp = CastField<FNumericProperty>(Property))
    {
        if (NumericProp->IsFloatingPoint())
        {
            return FormatFloatValue(NumericProp->GetFloatingPointPropertyValue(ValuePtr));
        }
        else
        {
            return FormatIntValue(NumericProp->GetSignedIntPropertyValue(ValuePtr));
        }
    }
    else if (const FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
    {
        return BoolProp->GetPropertyValue(ValuePtr) ? TEXT("True") : TEXT("False");
    }
    else if (const FStrProperty* StrProp = CastField<FStrProperty>(Property))
    {
        return StrProp->GetPropertyValue(ValuePtr);
    }
    else if (const FNameProperty* NameProp = CastField<FNameProperty>(Property))
    {
        return NameProp->GetPropertyValue(ValuePtr).ToString();
    }
    else if (const FTextProperty* TextProp = CastField<FTextProperty>(Property))
    {
        return TextProp->GetPropertyValue(ValuePtr).ToString();
    }
    else if (const FEnumProperty* EnumProp = CastField<FEnumProperty>(Property))
    {
        int64 Value = EnumProp->GetUnderlyingProperty()->GetSignedIntPropertyValue(ValuePtr);
        return EnumProp->GetEnum()->GetNameStringByValue(Value);
    }
    else if (const FObjectPropertyBase* ObjProp = CastField<FObjectPropertyBase>(Property))
    {
        UObject* Obj = ObjProp->GetObjectPropertyValue(ValuePtr);
        return Obj ? Obj->GetName() : TEXT("None");
    }

    return FString::Printf(TEXT("[%s]"), *Property->GetCPPType());
}

FString FStructAttributeCustomization::FormatFloatValue(double Value) const
{
    // Handle special cases
    if (FMath::IsNaN(Value))
    {
        return TEXT("NaN");
    }
    if (!FMath::IsFinite(Value))
    {
        return Value > 0 ? TEXT("Infinity") : TEXT("-Infinity");
    }

    // Use scientific notation for very large or very small numbers
    if (FMath::Abs(Value) >= 1000000.0 || (FMath::Abs(Value) < 0.001 && Value != 0.0))
    {
        return FString::Printf(TEXT("%.3e"), Value);
    }

    // Format with limited precision and thousand separators
    FString BaseString = FString::Printf(TEXT("%.3f"), Value);

    // Remove trailing zeros after decimal point
    if (BaseString.Contains(TEXT(".")))
    {
        while (BaseString.EndsWith(TEXT("0")) && !BaseString.EndsWith(TEXT(".0")))
        {
            BaseString.LeftChopInline(1);
        }
        if (BaseString.EndsWith(TEXT(".")))
        {
            BaseString.LeftChopInline(1);
        }
    }

    // Add thousand separators for the integer part
    int32 DecimalPos = BaseString.Find(TEXT("."));
    if (DecimalPos == INDEX_NONE)
    {
        DecimalPos = BaseString.Len();
    }

    FString IntPart = BaseString.Left(DecimalPos);
    FString DecPart = DecimalPos < BaseString.Len() ? BaseString.Mid(DecimalPos) : TEXT("");

    // Handle negative numbers
    bool bNegative = IntPart.StartsWith(TEXT("-"));
    if (bNegative)
    {
        IntPart.RemoveAt(0);
    }

    // Add commas
    FString FormattedInt;
    int32 Count = 0;
    for (int32 i = IntPart.Len() - 1; i >= 0; --i)
    {
        if (Count > 0 && Count % 3 == 0)
        {
            FormattedInt.InsertAt(0, TEXT(","));
        }
        FormattedInt.InsertAt(0, FString::Chr(IntPart[i]));
        Count++;
    }

    if (bNegative)
    {
        FormattedInt.InsertAt(0, TEXT("-"));
    }

    return FormattedInt + DecPart;
}

FString FStructAttributeCustomization::FormatIntValue(int64 Value) const
{
    FString ValueString = FString::Printf(TEXT("%lld"), Value);

    // Handle negative numbers
    bool bNegative = Value < 0;
    if (bNegative)
    {
        ValueString.RemoveAt(0);
    }

    // Add thousand separators
    FString FormattedString;
    int32 Count = 0;
    for (int32 i = ValueString.Len() - 1; i >= 0; --i)
    {
        if (Count > 0 && Count % 3 == 0)
        {
            FormattedString.InsertAt(0, TEXT(","));
        }
        FormattedString.InsertAt(0, FString::Chr(ValueString[i]));
        Count++;
    }

    if (bNegative)
    {
        FormattedString.InsertAt(0, TEXT("-"));
    }

    return FormattedString;
}

bool FStructAttributeCustomization::ShouldAutoExpand(const UScriptStruct* StructType) const
{
    if (!StructType)
    {
        return false;
    }

    // Count the number of properties
    int32 PropertyCount = 0;
    for (TFieldIterator<FProperty> PropIt(StructType); PropIt; ++PropIt)
    {
        PropertyCount++;
        if (PropertyCount > AUTO_EXPAND_PROPERTY_THRESHOLD)
        {
            return false;
        }
    }

    return true;
}

FLinearColor FStructAttributeCustomization::GetBackgroundColorForIndent(int32 Indent, bool bIsPropertyName) const
{
    // Create progressively lighter backgrounds for deeper nesting
    const float BaseAlpha = bIsPropertyName ? 0.08f : 0.05f;
    const float AlphaIncrement = 0.01f;

    float Alpha = BaseAlpha + (Indent * AlphaIncrement);
    Alpha = FMath::Clamp(Alpha, 0.0f, 0.15f);

    return FLinearColor(bIsPropertyName ? 0.6f : 0.4f, bIsPropertyName ? 0.6f : 0.4f, bIsPropertyName ? 0.6f : 0.4f, Alpha);
}

FString FStructAttributeCustomization::GetPropertyDisplayName(const FProperty* Property) const
{
    if (!Property)
    {
        return TEXT("Unknown");
    }

    // Try GetAuthoredName (returns the original name before Blueprint compilation)
    FString PropertyName = Property->GetAuthoredName();

    // If GetAuthoredName() returns empty or same as GetName(), it didn't help
    if (PropertyName.IsEmpty() || PropertyName == Property->GetName())
    {
        PropertyName = Property->GetName();

        // Remove the Blueprint-generated suffix (e.g., "_8_976737C747C308F4F0341099104196CB")
        // Pattern: underscore, digit(s), underscore, 32-character hex hash
        int32 LastUnderscore = INDEX_NONE;
        int32 SecondLastUnderscore = INDEX_NONE;

        // Find the last underscore
        if (PropertyName.FindLastChar(TEXT('_'), LastUnderscore) && LastUnderscore > 0)
        {
            // Find the second to last underscore
            FString BeforeLast = PropertyName.Left(LastUnderscore);
            if (BeforeLast.FindLastChar(TEXT('_'), SecondLastUnderscore) && SecondLastUnderscore > 0)
            {
                // Check if the pattern matches: _[digits]_[32 char hash]
                FString MiddlePart = PropertyName.Mid(SecondLastUnderscore + 1, LastUnderscore - SecondLastUnderscore - 1);
                FString HashPart = PropertyName.Mid(LastUnderscore + 1);

                // Verify the pattern
                if (MiddlePart.IsNumeric() && HashPart.Len() == 32)
                {
                    bool bIsHexHash = true;
                    for (TCHAR Ch : HashPart)
                    {
                        if (!FChar::IsAlnum(Ch))
                        {
                            bIsHexHash = false;
                            break;
                        }
                    }

                    if (bIsHexHash)
                    {
                        // Strip the suffix
                        PropertyName = PropertyName.Left(SecondLastUnderscore);
                    }
                }
            }
        }
    }

    return PropertyName;
}