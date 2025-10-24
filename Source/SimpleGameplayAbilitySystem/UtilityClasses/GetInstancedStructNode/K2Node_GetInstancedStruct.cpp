#include "K2Node_GetInstancedStruct.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_CallFunction.h"
#include "K2Node_SwitchEnum.h"
#include "KismetCompiler.h"
#include "Kismet/KismetSystemLibrary.h"
#include "SimpleGameplayAbilitySystem/BlueprintFunctionLibraries/NodeHelpers/NodeHelpers.h"
#include "UObject/SoftObjectPath.h"

#define LOCTEXT_NAMESPACE "K2Node_GetInstancedStruct"

// Pin name constants
const FName UK2Node_GetInstancedStruct::InstancedStructPinName(TEXT("InstancedStruct"));
const FName UK2Node_GetInstancedStruct::StructTypePinName(TEXT("StructType"));
const FName UK2Node_GetInstancedStruct::ValidPinName(TEXT("Valid"));
const FName UK2Node_GetInstancedStruct::InvalidPinName(TEXT("Invalid"));
const FName UK2Node_GetInstancedStruct::OutStructPinName(TEXT("OutStruct"));

UK2Node_GetInstancedStruct::UK2Node_GetInstancedStruct()
{
}

void UK2Node_GetInstancedStruct::PostLoad()
{
	Super::PostLoad();

	// Load struct from DefaultValue if needed, then clear it
	UEdGraphPin* StructTypePin = GetStructTypePin();
	if (StructTypePin)
	{
		// If there's a DefaultValue but no DefaultObject, try to load it
		if (!StructTypePin->DefaultValue.IsEmpty() && !StructTypePin->DefaultObject)
		{
			const FSoftObjectPath ObjectPath(StructTypePin->DefaultValue);
			UObject* LoadedObject = ObjectPath.ResolveObject();
			if (!LoadedObject)
			{
				LoadedObject = ObjectPath.TryLoad();
			}

			if (LoadedObject)
			{
				StructTypePin->DefaultObject = LoadedObject;
			}
		}

		// Clear DefaultValue (object pins should only use DefaultObject)
		if (!StructTypePin->DefaultValue.IsEmpty())
		{
			StructTypePin->DefaultValue.Empty();
		}

		// Sync the StructType pin with our cached struct type
		if (CachedStructType)
		{
			StructTypePin->DefaultObject = CachedStructType;
		}
		// If we don't have a cache but the pin has a DefaultObject, cache it
		else if (StructTypePin->DefaultObject)
		{
			CachedStructType = Cast<UScriptStruct>(StructTypePin->DefaultObject);
		}
	}

	// CRITICAL FIX: If the OutStruct pin was loaded with an incorrect type (wildcard when it should be concrete),
	// fix it NOW before compilation happens. This handles the case where the node is loaded without
	// AllocateDefaultPins being called (which happens during dependency compilation).
	UEdGraphPin* OutStructPin = GetOutStructPin();
	if (OutStructPin && CachedStructType)
	{
		if (OutStructPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard ||
			OutStructPin->PinType.PinSubCategoryObject != CachedStructType)
		{
			// Fix the pin type immediately
			OutStructPin->PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
			OutStructPin->PinType.PinSubCategoryObject = CachedStructType;
		}
	}

	// Refresh the output pin type
	RefreshOutputStructType();
}

void UK2Node_GetInstancedStruct::AllocateDefaultPins()
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	// Create execution input pin
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);

	// Create input pin for FInstancedStruct
	UEdGraphPin* InstancedStructPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Struct,
		FInstancedStruct::StaticStruct(), InstancedStructPinName);

	// Create input pin for UScriptStruct* (the type selector)
	UEdGraphPin* StructTypePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Object,
		UScriptStruct::StaticClass(), StructTypePinName);

	// Create execution output pins for Valid and Invalid paths
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, ValidPinName);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, InvalidPinName);

	// Create output pin - if we have a cached struct type, use it immediately
	// This prevents the pin from being a wildcard during early compilation
	UEdGraphPin* OutStructPin = nullptr;
	if (CachedStructType)
	{
		OutStructPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Struct,
			CachedStructType, OutStructPinName);
	}
	else
	{
		OutStructPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Wildcard,
			OutStructPinName);
	}

	Super::AllocateDefaultPins();

	// Try to refresh the output type if we have a default value
	RefreshOutputStructType();
}

void UK2Node_GetInstancedStruct::PostReconstructNode()
{
	Super::PostReconstructNode();

	// Load struct from DefaultValue if needed, then clear it
	UEdGraphPin* StructTypePin = GetStructTypePin();
	if (StructTypePin)
	{
		// If there's a DefaultValue but no DefaultObject, try to load it
		if (!StructTypePin->DefaultValue.IsEmpty() && !StructTypePin->DefaultObject)
		{
			const FSoftObjectPath ObjectPath(StructTypePin->DefaultValue);
			UObject* LoadedObject = ObjectPath.ResolveObject();
			if (!LoadedObject)
			{
				LoadedObject = ObjectPath.TryLoad();
			}

			if (LoadedObject)
			{
				StructTypePin->DefaultObject = LoadedObject;
				// Also update cache
				CachedStructType = Cast<UScriptStruct>(LoadedObject);
			}
		}

		// Clear DefaultValue (object pins should only use DefaultObject)
		if (!StructTypePin->DefaultValue.IsEmpty())
		{
			StructTypePin->DefaultValue.Empty();
		}
	}

	// Refresh the output struct type when the node is reconstructed
	// This ensures the type is correct when opening a Blueprint
	RefreshOutputStructType();
}

FText UK2Node_GetInstancedStruct::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (TitleType == ENodeTitleType::MenuTitle)
	{
		return LOCTEXT("GetInstancedStructMenuTitle", "Get Instanced Struct");
	}

	UScriptStruct* SelectedStruct = GetSelectedStructType();

	if (SelectedStruct)
	{
		return FText::Format(LOCTEXT("GetInstancedStructTitle", "Get {0} from Instanced Struct"),
			FText::FromString(SelectedStruct->GetName()));
	}

	return LOCTEXT("GetInstancedStructTitleDefault", "Get Instanced Struct");
}

FText UK2Node_GetInstancedStruct::GetTooltipText() const
{
	return LOCTEXT("GetInstancedStructTooltip",
		"Extracts a typed struct from an FInstancedStruct with type validation.\n"
		"Outputs through 'Valid' if the types match, or 'Invalid' if they don't.");
}

FLinearColor UK2Node_GetInstancedStruct::GetNodeTitleColor() const
{
	return FLinearColor(0.8f, 0.4f, 0.2f);
}

FSlateIcon UK2Node_GetInstancedStruct::GetIconAndTint(FLinearColor& OutColor) const
{
	OutColor = GetNodeTitleColor();
	static FSlateIcon Icon("EditorStyle", "Kismet.AllClasses.FunctionIcon");
	return Icon;
}

void UK2Node_GetInstancedStruct::PreloadRequiredAssets()
{
	Super::PreloadRequiredAssets();

	// Ensure struct type is fully loaded and cached before compilation
	UEdGraphPin* StructTypePin = GetStructTypePin();
	if (StructTypePin)
	{
		// If we have a DefaultValue but no DefaultObject, load it NOW
		if (!StructTypePin->DefaultValue.IsEmpty() && !StructTypePin->DefaultObject)
		{
			const FSoftObjectPath ObjectPath(StructTypePin->DefaultValue);
			UObject* LoadedObject = ObjectPath.TryLoad();  // Force synchronous load

			if (LoadedObject)
			{
				StructTypePin->DefaultObject = LoadedObject;
				CachedStructType = Cast<UScriptStruct>(LoadedObject);
			}

			StructTypePin->DefaultValue.Empty();
		}

		if (!CachedStructType && StructTypePin->DefaultObject)
		{
			CachedStructType = Cast<UScriptStruct>(StructTypePin->DefaultObject);
		}
	}

	UScriptStruct* SelectedStruct = GetSelectedStructType();

	if (SelectedStruct)
	{
		PreloadObject(SelectedStruct);
		RefreshOutputStructType();
	}
}

void UK2Node_GetInstancedStruct::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();

	// Refresh the output pin type FIRST before any validation
	// This ensures the pin type is set correctly at compile time
	RefreshOutputStructType();

	// Get all our pins
	UEdGraphPin* ExecPin = GetExecPin();
	UEdGraphPin* InstancedStructPin = GetInstancedStructPin();
	UEdGraphPin* StructTypePin = GetStructTypePin();
	UEdGraphPin* ValidPin = GetValidPin();
	UEdGraphPin* InvalidPin = GetInvalidPin();
	UEdGraphPin* OutStructPin = GetOutStructPin();

	// Validate that we have all required pins
	if (!ExecPin || !InstancedStructPin || !StructTypePin || !ValidPin || !InvalidPin || !OutStructPin)
	{
		CompilerContext.MessageLog.Error(*LOCTEXT("MissingPins", "Missing required pins in @@").ToString(), this);
		return;
	}

	// Validate that the output struct pin has a concrete type
	if (OutStructPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard)
	{
		CompilerContext.MessageLog.Error(*LOCTEXT("WildcardNotResolved", "Output struct type not resolved in @@. Please connect or select a struct type.").ToString(), this);
		return;
	}

	// Create a call to UNodeHelpers::GetInstancedStruct
	UK2Node_CallFunction* CallGetInstancedStruct = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	CallGetInstancedStruct->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UNodeHelpers, GetInstancedStruct), UNodeHelpers::StaticClass());
	CallGetInstancedStruct->AllocateDefaultPins();

	// Connect exec input to the function call
	CompilerContext.MovePinLinksToIntermediate(*ExecPin, *CallGetInstancedStruct->GetExecPin());

	// Connect InstancedStruct input
	UEdGraphPin* CallInstancedStructPin = CallGetInstancedStruct->FindPinChecked(TEXT("InstancedStruct"));
	CompilerContext.MovePinLinksToIntermediate(*InstancedStructPin, *CallInstancedStructPin);

	// Connect StructType input
	UEdGraphPin* CallStructTypePin = CallGetInstancedStruct->FindPinChecked(TEXT("StructType"));
	CompilerContext.MovePinLinksToIntermediate(*StructTypePin, *CallStructTypePin);

	// Connect OutStruct output
	UEdGraphPin* CallOutStructPin = CallGetInstancedStruct->FindPinChecked(TEXT("OutStruct"));

	// CRITICAL FIX: Set the wildcard OutStruct pin type to our concrete type BEFORE moving connections
	// The intermediate function call's OutStruct pin starts as a wildcard and needs to be typed
	if (CachedStructType)
	{
		CallOutStructPin->PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
		CallOutStructPin->PinType.PinSubCategoryObject = CachedStructType;
	}
	else
	{
		CompilerContext.MessageLog.Error(*LOCTEXT("NoCachedType", "Cannot determine struct type for intermediate node in @@").ToString(), this);
		return;
	}

	CompilerContext.MovePinLinksToIntermediate(*OutStructPin, *CallOutStructPin);

	// Get the return value pin (EGetInstancedStructResult enum)
	UEdGraphPin* CallReturnPin = CallGetInstancedStruct->GetReturnValuePin();
	if (!CallReturnPin)
	{
		CompilerContext.MessageLog.Error(*LOCTEXT("MissingReturnPin", "Failed to find return pin in @@").ToString(), this);
		return;
	}

	// Create a switch enum node to branch on the result
	UK2Node_SwitchEnum* SwitchNode = CompilerContext.SpawnIntermediateNode<UK2Node_SwitchEnum>(this, SourceGraph);
	SwitchNode->Enum = StaticEnum<EGetInstancedStructResult>();
	SwitchNode->AllocateDefaultPins();

	// Connect the function's exec then pin to the switch's exec pin
	UEdGraphPin* CallThenPin = CallGetInstancedStruct->GetThenPin();
	UEdGraphPin* SwitchExecPin = SwitchNode->GetExecPin();
	if (CallThenPin && SwitchExecPin)
	{
		CallThenPin->MakeLinkTo(SwitchExecPin);
	}

	// Connect the return value to the switch selection pin
	UEdGraphPin* SwitchSelectionPin = SwitchNode->GetSelectionPin();
	if (SwitchSelectionPin)
	{
		CallReturnPin->MakeLinkTo(SwitchSelectionPin);
	}

	// Find the Valid and Invalid pins on the switch node
	UEdGraphPin* SwitchValidPin = SwitchNode->FindPin(TEXT("EGetInstancedStructResult::Valid"));
	UEdGraphPin* SwitchInvalidPin = SwitchNode->FindPin(TEXT("EGetInstancedStructResult::Invalid"));

	if (!SwitchValidPin || !SwitchInvalidPin)
	{
		// Try alternate naming convention
		SwitchValidPin = SwitchNode->FindPin(TEXT("Valid"));
		SwitchInvalidPin = SwitchNode->FindPin(TEXT("Invalid"));
	}

	if (SwitchValidPin && SwitchInvalidPin)
	{
		// Move the exec links from our Valid/Invalid pins to the switch node outputs
		CompilerContext.MovePinLinksToIntermediate(*ValidPin, *SwitchValidPin);
		CompilerContext.MovePinLinksToIntermediate(*InvalidPin, *SwitchInvalidPin);
	}
	else
	{
		CompilerContext.MessageLog.Error(*LOCTEXT("SwitchPinsNotFound", "Failed to find Valid/Invalid pins on switch node in @@").ToString(), this);
	}

	// Break all links to this node since we've moved them to intermediate nodes
	BreakAllNodeLinks();
}

void UK2Node_GetInstancedStruct::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	UClass* ActionKey = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);

		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

FText UK2Node_GetInstancedStruct::GetMenuCategory() const
{
	return LOCTEXT("GetInstancedStructCategory", "SimpleGAS|Utilities");
}

bool UK2Node_GetInstancedStruct::IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const
{
	// The OutStruct pin can only connect to pins that match its current type
	if (MyPin == GetOutStructPin() && MyPin->PinType.PinCategory != UEdGraphSchema_K2::PC_Wildcard)
	{
		const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
		if (!K2Schema->ArePinTypesCompatible(MyPin->PinType, OtherPin->PinType))
		{
			OutReason = TEXT("Struct types must match");
			return true;
		}
	}

	return Super::IsConnectionDisallowed(MyPin, OtherPin, OutReason);
}

void UK2Node_GetInstancedStruct::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::NotifyPinConnectionListChanged(Pin);

	// If the StructType pin changed, update the output struct type
	if (Pin == GetStructTypePin())
	{
		RefreshOutputStructType();
	}
}

void UK2Node_GetInstancedStruct::PinDefaultValueChanged(UEdGraphPin* Pin)
{
	Super::PinDefaultValueChanged(Pin);

	// If the StructType pin default value changed, update the output struct type
	if (Pin == GetStructTypePin())
	{
		// When user selects from dropdown, Unreal sets DefaultValue to the path
		// We need to load the object from that path and set it to DefaultObject
		if (!Pin->DefaultValue.IsEmpty() && !Pin->DefaultObject)
		{
			// Load the struct from the path
			const FSoftObjectPath ObjectPath(Pin->DefaultValue);
			UObject* LoadedObject = ObjectPath.ResolveObject();
			if (!LoadedObject)
			{
				LoadedObject = ObjectPath.TryLoad();
			}

			if (LoadedObject)
			{
				Pin->DefaultObject = LoadedObject;
			}
		}

		// Clear DefaultValue (object pins should only use DefaultObject)
		if (!Pin->DefaultValue.IsEmpty())
		{
			Pin->DefaultValue.Empty();
		}

		// Update our cached struct type from DefaultObject
		if (Pin->DefaultObject)
		{
			CachedStructType = Cast<UScriptStruct>(Pin->DefaultObject);
		}
		else
		{
			CachedStructType = nullptr;
		}

		RefreshOutputStructType();
	}
}

UEdGraphPin* UK2Node_GetInstancedStruct::GetInstancedStructPin() const
{
	return FindPinChecked(InstancedStructPinName);
}

UEdGraphPin* UK2Node_GetInstancedStruct::GetStructTypePin() const
{
	return FindPinChecked(StructTypePinName);
}

UEdGraphPin* UK2Node_GetInstancedStruct::GetValidPin() const
{
	return FindPinChecked(ValidPinName);
}

UEdGraphPin* UK2Node_GetInstancedStruct::GetInvalidPin() const
{
	return FindPinChecked(InvalidPinName);
}

UEdGraphPin* UK2Node_GetInstancedStruct::GetOutStructPin() const
{
	return FindPinChecked(OutStructPinName);
}

void UK2Node_GetInstancedStruct::RefreshOutputStructType()
{
	UScriptStruct* SelectedStruct = GetSelectedStructType();
	UEdGraphPin* OutStructPin = GetOutStructPin();

	if (!OutStructPin)
	{
		return;
	}

	// Cache the selected struct for serialization
	if (SelectedStruct != CachedStructType)
	{
		CachedStructType = SelectedStruct;
	}

	// If we have a selected struct, update the output pin type
	if (SelectedStruct)
	{
		// Only update if the type actually changed
		if (OutStructPin->PinType.PinSubCategoryObject != SelectedStruct)
		{
			// Break any invalid connections
			for (int32 i = OutStructPin->LinkedTo.Num() - 1; i >= 0; --i)
			{
				UEdGraphPin* OtherPin = OutStructPin->LinkedTo[i];
				if (OtherPin->PinType.PinSubCategoryObject != SelectedStruct)
				{
					OutStructPin->BreakLinkTo(OtherPin);
				}
			}

			// Update the pin type
			OutStructPin->PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
			OutStructPin->PinType.PinSubCategoryObject = SelectedStruct;

			// Notify the graph that the node has changed
			if (GetGraph())
			{
				GetGraph()->NotifyGraphChanged();
			}
		}
	}
	else
	{
		// Reset to wildcard if no struct is selected
		if (OutStructPin->PinType.PinCategory != UEdGraphSchema_K2::PC_Wildcard)
		{
			OutStructPin->PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;
			OutStructPin->PinType.PinSubCategoryObject = nullptr;
			OutStructPin->BreakAllPinLinks();

			if (GetGraph())
			{
				GetGraph()->NotifyGraphChanged();
			}
		}
	}
}

UScriptStruct* UK2Node_GetInstancedStruct::GetSelectedStructType() const
{
	// First check the cached struct type (most reliable after serialization)
	if (CachedStructType)
	{
		return CachedStructType;
	}

	auto ResolveStructFromDefaultString = [](const FString& DefaultValue) -> UScriptStruct*
	{
		if (DefaultValue.IsEmpty())
		{
			return nullptr;
		}

		const FSoftObjectPath ObjectPath(DefaultValue);
		if (!ObjectPath.IsValid())
		{
			return nullptr;
		}

		UObject* LoadedObject = ObjectPath.ResolveObject();
		if (!LoadedObject)
		{
			// TryLoad will synchronously load native structs referenced by path literal (e.g. ScriptStruct'/Script/...').
			LoadedObject = ObjectPath.TryLoad();
		}

		return Cast<UScriptStruct>(LoadedObject);
	};

	UEdGraphPin* StructTypePin = GetStructTypePin();
	if (!StructTypePin)
	{
		return nullptr;
	}

	// First check if there's a default value set directly on our pin
	if (StructTypePin->DefaultObject)
	{
		UScriptStruct* Result = Cast<UScriptStruct>(StructTypePin->DefaultObject);
		if (Result)
		{
			return Result;
		}
	}

	// Try loading from the literal default value string (BP stores script struct references this way).
	if (UScriptStruct* Result = ResolveStructFromDefaultString(StructTypePin->DefaultValue))
	{
		return Result;
	}

	// Check if the pin is connected
	if (StructTypePin->LinkedTo.Num() > 0)
	{
		UEdGraphPin* ConnectedPin = StructTypePin->LinkedTo[0];
		if (ConnectedPin)
		{
			// Check the connected pin's default object
			if (ConnectedPin->DefaultObject)
			{
				UScriptStruct* Result = Cast<UScriptStruct>(ConnectedPin->DefaultObject);
				if (Result)
				{
					return Result;
				}
			}

			// Check if it's a self pin that has a literal struct value
			if (ConnectedPin->PinType.PinSubCategoryObject.IsValid())
			{
				UScriptStruct* Result = Cast<UScriptStruct>(ConnectedPin->PinType.PinSubCategoryObject.Get());
				if (Result)
				{
					return Result;
				}
			}

			// Try to get from the pin's default text value (for asset references)
			if (UScriptStruct* Result = ResolveStructFromDefaultString(ConnectedPin->DefaultValue))
			{
				return Result;
			}
		}
	}

	return nullptr;
}

#undef LOCTEXT_NAMESPACE
