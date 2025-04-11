#include "StructAttributeCustomization.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Views/SExpanderArrow.h"
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
                    // Create a container for the struct data section
                    FDetailWidgetRow& StructRow = ChildBuilder.AddCustomRow(FText::FromString(TEXT("Struct Data")));

                    // Create a heading for the struct data section that matches UE style
                    StructRow
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(FMargin(0, 5, 0, 5))
                        [
                            SNew(STextBlock)
                            .Text(FText::FromString(FString::Printf(TEXT("Struct Data (%s)"), *StructType->GetName())))
                            .Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
                            .ColorAndOpacity(FAppStyle::GetSlateColor("Colors.Foreground"))
                        ]
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0)
                        [
                            SAssignNew(StructContentWidget, SVerticalBox)
                        ]
                    ];

                    // Display the struct contents
                    DisplayStructContents(StructType, StructData, StructContentWidget.ToSharedRef(), 0);
                    return;
                }
            }
        }
    }

    // Add a message if we couldn't display the struct data
    ChildBuilder.AddCustomRow(FText::FromString(TEXT("Struct Data")))
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot()
        .FillWidth(1.0)
        [
            SNew(STextBlock)
            .Text(FText::FromString(TEXT("Cannot access struct data")))
            .Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
            .ColorAndOpacity(FAppStyle::GetSlateColor("Colors.Foreground"))
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
    // Use standard property styling
    const float IndentAmount = 16.0f;
    const float RowPadding = 2.0f;
    const float PropertyNameWidth = 150.0f;

    // Define background colors for property names and values
    const FLinearColor PropertyNameBgColor(0.6f, 0.6f, 0.6f, 0.05f);  // Lighter background
    const FLinearColor PropertyValueBgColor(0.4f, 0.4f, 0.4f, 0.05f); // Slightly darker background

    for (TFieldIterator<FProperty> PropIt(StructType); PropIt; ++PropIt)
    {
        FProperty* Property = *PropIt;
        const void* ValuePtr = Property->ContainerPtrToValuePtr<void>(StructData);

        // Create an expander key for this property
        FName PropertyKey = *FString::Printf(TEXT("Prop_%s_%p"), *Property->GetName(), Property);

        // Handle different property types
        if (const FStructProperty* StructProp = CastField<FStructProperty>(Property))
        {
            // Create expandable row for nested struct
            TSharedRef<SHorizontalBox> PropertyRow = SNew(SHorizontalBox);

            // Add the expander button
            PropertyRow->AddSlot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                .HAlign(HAlign_Left)
                .Padding(FMargin(Indent * IndentAmount, 0, 0, 0))
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
                        .WidthOverride(PropertyNameWidth)
                        [
                            SNew(STextBlock)
                            .Text(FText::FromString(Property->GetName()))
                            .ToolTipText(Property->GetToolTipText())
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
                        .ColorAndOpacity(FAppStyle::GetSlateColor("Colors.DimForeground"))
                    ]
                ];

            // Add the row to the target widget
            TargetWidget->AddSlot()
                .AutoHeight()
                .Padding(FMargin(0, RowPadding))
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
                .Padding(FMargin(Indent * IndentAmount, 0, 0, 0))
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
                        .WidthOverride(PropertyNameWidth)
                        [
                            SNew(STextBlock)
                            .Text(FText::FromString(Property->GetName()))
                            .ToolTipText(Property->GetToolTipText())
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
                        .ColorAndOpacity(FAppStyle::GetSlateColor("Colors.DimForeground"))
                    ]
                ];

            // Add the row to the target widget
            TargetWidget->AddSlot()
                .AutoHeight()
                .Padding(FMargin(0, RowPadding))
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
                        .Padding(FMargin((Indent + 1) * IndentAmount, 0, 0, 0))
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
                                .WidthOverride(PropertyNameWidth)
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
                                .ColorAndOpacity(FAppStyle::GetSlateColor("Colors.DimForeground"))
                            ]
                        ];

                    // Add the element row to the array content
                    ArrayContentBox->AddSlot()
                        .AutoHeight()
                        .Padding(FMargin(0, RowPadding))
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
                        .Padding(FMargin(0, RowPadding))
                        [
                            SNew(SHorizontalBox)
                            // Left indent
                            + SHorizontalBox::Slot()
                            .AutoWidth()
                            .VAlign(VAlign_Center)
                            .Padding(FMargin((Indent + 1) * IndentAmount + 16.0f, 0, 0, 0))
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
                                    .WidthOverride(PropertyNameWidth)
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
            // For simple property types - display in a two-column layout
            FString ValueString = GetPropertyValueString(Property, ValuePtr);

            // Create the property row
            TargetWidget->AddSlot()
                .AutoHeight()
                .Padding(FMargin(0, RowPadding))
                [
                    SNew(SHorizontalBox)
                    // Left indent
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .VAlign(VAlign_Center)
                    .Padding(FMargin(Indent * IndentAmount + 16.0f, 0, 0, 0))
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
                            .WidthOverride(PropertyNameWidth)
                            [
                                SNew(STextBlock)
                                .Text(FText::FromString(Property->GetName()))
                                .ToolTipText(Property->GetToolTipText())
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
            return FString::SanitizeFloat(NumericProp->GetFloatingPointPropertyValue(ValuePtr));
        }
        else
        {
            return FString::FromInt(NumericProp->GetSignedIntPropertyValue(ValuePtr));
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