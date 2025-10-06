#include "GameplayDebuggerCategory_AttributeComponent.h"

#if WITH_GAMEPLAY_DEBUGGER

#include "GameplayDebuggerCategoryReplicator.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeComponent.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeComponentTypes.h"
#include "SimpleGameplayAbilitySystem/Components/Interfaces/SimpleAbilitySystemInterfaces.h"
#include "Engine/Canvas.h"
#include "GameplayDebugger.h"

FGameplayDebuggerCategory_AttributeComponent::FGameplayDebuggerCategory_AttributeComponent()
	: CurrentPage(0)
	, ItemsPerPage(8)
	, CurrentViewMode(0)
{
	bShowOnlyWithDebugActor = false;

	// Enable data pack replication from server to client
	SetDataPackReplication(&DataPack);

	BindKeyPress(EKeys::PageUp.GetFName(), FGameplayDebuggerInputModifier::None, this, &FGameplayDebuggerCategory_AttributeComponent::PrevPage, EGameplayDebuggerInputMode::Local);
	BindKeyPress(EKeys::PageDown.GetFName(), FGameplayDebuggerInputModifier::None, this, &FGameplayDebuggerCategory_AttributeComponent::NextPage, EGameplayDebuggerInputMode::Local);
	BindKeyPress(EKeys::LeftBracket.GetFName(), FGameplayDebuggerInputModifier::None, this, &FGameplayDebuggerCategory_AttributeComponent::PrevViewMode, EGameplayDebuggerInputMode::Local);
	BindKeyPress(EKeys::RightBracket.GetFName(), FGameplayDebuggerInputModifier::None, this, &FGameplayDebuggerCategory_AttributeComponent::NextViewMode, EGameplayDebuggerInputMode::Local);
}

TSharedRef<FGameplayDebuggerCategory> FGameplayDebuggerCategory_AttributeComponent::MakeInstance()
{
	return MakeShareable(new FGameplayDebuggerCategory_AttributeComponent());
}

void FGameplayDebuggerCategory_AttributeComponent::FRepData::Serialize(FArchive& Ar)
{
	int32 NumFloatAttrs = FloatAttributes.Num();
	int32 NumStructAttrs = StructAttributes.Num();
	int32 NumTags = GameplayTags.Num();

	Ar << NumFloatAttrs;
	Ar << NumStructAttrs;
	Ar << NumTags;

	if (Ar.IsLoading())
	{
		FloatAttributes.SetNum(NumFloatAttrs);
		StructAttributes.SetNum(NumStructAttrs);
		GameplayTags.SetNum(NumTags);
	}

	for (FFloatAttributeData& Data : FloatAttributes)
	{
		Ar << Data.AttributeTag;
		Ar << Data.BaseValue;
		Ar << Data.CurrentValue;
	}

	for (FStructAttributeData& Data : StructAttributes)
	{
		Ar << Data.AttributeTag;
		Ar << Data.StructType;
		Ar << Data.StructData;
	}

	for (FGameplayTagData& Data : GameplayTags)
	{
		Ar << Data.Tag;
		Ar << Data.RefCount;
	}
}

FString FGameplayDebuggerCategory_AttributeComponent::GetPropertyValueAsString(const FProperty* Property, const void* ValuePtr, int32 IndentLevel)
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
			return FString::Printf(TEXT("%.3f"), NumericProp->GetFloatingPointPropertyValue(ValuePtr));
		}
		else
		{
			return FString::Printf(TEXT("%lld"), NumericProp->GetSignedIntPropertyValue(ValuePtr));
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
	else if (const FByteProperty* ByteProp = CastField<FByteProperty>(Property))
	{
		if (ByteProp->Enum)
		{
			int64 Value = ByteProp->GetSignedIntPropertyValue(ValuePtr);
			return ByteProp->Enum->GetNameStringByValue(Value);
		}
		return FString::Printf(TEXT("%d"), ByteProp->GetSignedIntPropertyValue(ValuePtr));
	}
	else if (const FObjectPropertyBase* ObjProp = CastField<FObjectPropertyBase>(Property))
	{
		UObject* Obj = ObjProp->GetObjectPropertyValue(ValuePtr);
		return Obj ? Obj->GetName() : TEXT("None");
	}
	else if (const FStructProperty* StructProp = CastField<FStructProperty>(Property))
	{
		// Recursively format nested structs
		FString NestedStructData = FormatStructDataForDisplay(StructProp->Struct, ValuePtr, IndentLevel + 1);
		if (!NestedStructData.IsEmpty())
		{
			return FString::Printf(TEXT("(%s)\n%s"), *StructProp->Struct->GetName(), *NestedStructData);
		}
		return FString::Printf(TEXT("(%s)"), *StructProp->Struct->GetName());
	}
	else if (const FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property))
	{
		FScriptArrayHelper ArrayHelper(ArrayProp, ValuePtr);
		return FString::Printf(TEXT("Array[%d]"), ArrayHelper.Num());
	}

	return FString::Printf(TEXT("[%s]"), *Property->GetCPPType());
}

FString FGameplayDebuggerCategory_AttributeComponent::FormatStructDataForDisplay(const UScriptStruct* StructType, const void* StructData, int32 IndentLevel)
{
	if (!StructType || !StructData)
	{
		return TEXT("");
	}

	TArray<FString> Lines;

	// Create indentation string (4 spaces per level, starting with 1 base indent)
	FString Indent = FString::ChrN((IndentLevel + 1) * 4, TEXT(' '));

	// Iterate through all properties in the struct
	for (TFieldIterator<FProperty> PropIt(StructType); PropIt; ++PropIt)
	{
		FProperty* Property = *PropIt;
		const void* ValuePtr = Property->ContainerPtrToValuePtr<void>(StructData);

		FString PropertyName = Property->GetDisplayNameText().ToString();
		FString PropertyValue = GetPropertyValueAsString(Property, ValuePtr, IndentLevel);

		// Format with color codes: cyan for property name, white for value
		Lines.Add(FString::Printf(TEXT("%s{cyan}%s:{white} %s"), *Indent, *PropertyName, *PropertyValue));
	}

	return FString::Join(Lines, TEXT("\n"));
}

void FGameplayDebuggerCategory_AttributeComponent::CollectData(APlayerController* OwnerPC, AActor* DebugActor)
{
	// This runs on the server and collects authoritative data to replicate to clients
	DataPack.FloatAttributes.Empty();
	DataPack.StructAttributes.Empty();
	DataPack.GameplayTags.Empty();

	if (!DebugActor)
	{
		return;
	}

	// Check if the actor implements the interface
	if (!DebugActor->Implements<UAttributeComponentInterface>())
	{
		return;
	}

	const USimpleAttributeComponent* AttributeComp = IAttributeComponentInterface::Execute_GetSimpleAttributeComponent(DebugActor);
	if (!AttributeComp || !IsValid(AttributeComp))
	{
		return;
	}

	// Collect server-authoritative Float Attributes
	for (const FFloatAttribute& Attr : AttributeComp->AuthorityFloatAttributes.Attributes)
	{
		FFloatAttributeData Data;
		Data.AttributeTag = Attr.AttributeTag.ToString();
		Data.BaseValue = Attr.BaseValue;
		Data.CurrentValue = Attr.CurrentValue;

		DataPack.FloatAttributes.Add(Data);
	}

	// Collect server-authoritative Struct Attributes
	for (const FStructAttribute& Attr : AttributeComp->AuthorityStructAttributes.Attributes)
	{
		FStructAttributeData Data;
		Data.AttributeTag = Attr.AttributeTag.ToString();
		Data.StructType = Attr.StructType ? Attr.StructType->GetName() : TEXT("None");

		// Format struct data for display
		if (Attr.AttributeValue.IsValid() && Attr.StructType)
		{
			const void* StructMemory = Attr.AttributeValue.GetMemory();
			Data.StructData = FormatStructDataForDisplay(Attr.StructType, StructMemory);
		}
		else
		{
			Data.StructData = TEXT("(Invalid)");
		}

		DataPack.StructAttributes.Add(Data);
	}

	// Collect server-authoritative Gameplay Tags
	for (const FGameplayTagCounter& TagCounter : AttributeComp->AuthorityGameplayTags.Tags)
	{
		FGameplayTagData Data;
		Data.Tag = TagCounter.GameplayTag.ToString();
		Data.RefCount = TagCounter.ReferenceCounter;

		DataPack.GameplayTags.Add(Data);
	}
}

void FGameplayDebuggerCategory_AttributeComponent::DrawData(APlayerController* OwnerPC, FGameplayDebuggerCanvasContext& CanvasContext)
{
	// This runs on both server and client

	// Get the currently selected debug actor
	AActor* DebugActor = nullptr;
	if (AGameplayDebuggerCategoryReplicator* Replicator = GetReplicator())
	{
		DebugActor = Replicator->GetDebugActor();
	}

	if (!DebugActor)
	{
		CanvasContext.Printf(TEXT("{yellow}=== ATTRIBUTE COMPONENT ==="));
		CanvasContext.Printf(TEXT("{grey}No debug actor selected"));
		return;
	}

	// Determine if we should show local predicted data
	// Show local data if:
	// 1. We're on a client viewing our own pawn, OR
	// 2. We're on the server (GetNetMode() returns NM_ListenServer or NM_DedicatedServer)
	bool bShowLocalPrediction = false;

	UWorld* World = DebugActor->GetWorld();
	ENetMode NetMode = World ? World->GetNetMode() : NM_Standalone;

	if (OwnerPC && OwnerPC->IsLocalController())
	{
		APawn* ViewerPawn = OwnerPC->GetPawn();
		// Show local prediction if viewing our own pawn or an actor we own
		bShowLocalPrediction = (DebugActor == ViewerPawn) || (DebugActor->GetOwner() == ViewerPawn);
	}

	// On server, always show local data (which is the authoritative data)
	if (NetMode == NM_ListenServer || NetMode == NM_DedicatedServer || NetMode == NM_Standalone)
	{
		bShowLocalPrediction = true;
	}

	CanvasContext.Printf(TEXT("{yellow}=== ATTRIBUTE COMPONENT ==="));
	if (bShowLocalPrediction)
	{
		CanvasContext.Printf(TEXT("{green}[LOCAL PREDICTED VALUES]"));
	}
	else
	{
		CanvasContext.Printf(TEXT("{grey}[SERVER REPLICATED VALUES]"));
	}

	FString ViewModeText;
	switch (CurrentViewMode)
	{
		case 0: ViewModeText = TEXT("FLOAT ATTRIBUTES"); break;
		case 1: ViewModeText = TEXT("STRUCT ATTRIBUTES"); break;
		default: ViewModeText = TEXT("UNKNOWN"); break;
	}

	CanvasContext.Printf(TEXT("View: {cyan}%s {grey}([ ] to switch)"), *ViewModeText);
	CanvasContext.Printf(TEXT(""));

	// Get local data if needed (predicted on client, authoritative on server)
	TArray<FFloatAttributeData> LocalFloatAttrs;
	TArray<FStructAttributeData> LocalStructAttrs;
	TArray<FGameplayTagData> LocalTags;

	// For clients, track which attributes have prediction mismatches
	TSet<FString> MismatchedFloatAttrsBase;
	TSet<FString> MismatchedFloatAttrsCurrent;
	TSet<FString> MismatchedStructAttrs;
	TSet<FString> MismatchedTags;

	if (bShowLocalPrediction)
	{
		USimpleAttributeComponent* AttributeComp = nullptr;
		if (DebugActor->Implements<UAttributeComponentInterface>())
		{
			AttributeComp = IAttributeComponentInterface::Execute_GetSimpleAttributeComponent(DebugActor);
		}
		if (AttributeComp)
		{
			// On server, read from Authority arrays; on client, read from Local predicted arrays
			bool bIsServer = (NetMode == NM_ListenServer || NetMode == NM_DedicatedServer);

			// Collect float attributes
			const TArray<FFloatAttribute>& FloatAttrsSource = bIsServer
				? AttributeComp->AuthorityFloatAttributes.Attributes
				: AttributeComp->LocalFloatAttributes;

			for (const FFloatAttribute& Attr : FloatAttrsSource)
			{
				FFloatAttributeData Data;
				Data.AttributeTag = Attr.AttributeTag.ToString();
				Data.BaseValue = Attr.BaseValue;
				Data.CurrentValue = Attr.CurrentValue;

				// On client, check if predicted value differs from server value
				if (!bIsServer)
				{
					const FFloatAttribute* AuthAttr = AttributeComp->AuthorityFloatAttributes.Attributes.FindByPredicate(
						[&](const FFloatAttribute& A) { return A.AttributeTag == Attr.AttributeTag; });

					if (AuthAttr)
					{
						if (AuthAttr->BaseValue != Attr.BaseValue)
						{
							MismatchedFloatAttrsBase.Add(Data.AttributeTag);
						}
						if (AuthAttr->CurrentValue != Attr.CurrentValue)
						{
							MismatchedFloatAttrsCurrent.Add(Data.AttributeTag);
						}
					}
				}

				LocalFloatAttrs.Add(Data);
			}

			// Collect struct attributes
			const TArray<FStructAttribute>& StructAttrsSource = bIsServer
				? AttributeComp->AuthorityStructAttributes.Attributes
				: AttributeComp->LocalStructAttributes;

			for (const FStructAttribute& Attr : StructAttrsSource)
			{
				FStructAttributeData Data;
				Data.AttributeTag = Attr.AttributeTag.ToString();
				Data.StructType = Attr.StructType ? Attr.StructType->GetName() : TEXT("None");

				// Format struct data for display
				if (Attr.AttributeValue.IsValid() && Attr.StructType)
				{
					const void* StructMemory = Attr.AttributeValue.GetMemory();
					Data.StructData = FormatStructDataForDisplay(Attr.StructType, StructMemory);
				}
				else
				{
					Data.StructData = TEXT("(Invalid)");
				}

				// On client, check if predicted struct differs from server
				if (!bIsServer)
				{
					const FStructAttribute* AuthAttr = AttributeComp->AuthorityStructAttributes.Attributes.FindByPredicate(
						[&](const FStructAttribute& A) { return A.AttributeTag == Attr.AttributeTag; });

					if (AuthAttr && *AuthAttr != Attr)
					{
						MismatchedStructAttrs.Add(Data.AttributeTag);
					}
				}

				LocalStructAttrs.Add(Data);
			}

			// Collect gameplay tags
			const TArray<FGameplayTagCounter>& TagsSource = bIsServer
				? AttributeComp->AuthorityGameplayTags.Tags
				: AttributeComp->LocalGameplayTags;

			for (const FGameplayTagCounter& TagCounter : TagsSource)
			{
				FGameplayTagData Data;
				Data.Tag = TagCounter.GameplayTag.ToString();
				Data.RefCount = TagCounter.ReferenceCounter;

				// On client, check if predicted tag count differs from server
				if (!bIsServer)
				{
					const FGameplayTagCounter* AuthTag = AttributeComp->AuthorityGameplayTags.Tags.FindByPredicate(
						[&](const FGameplayTagCounter& T) { return T.GameplayTag == TagCounter.GameplayTag; });

					if (AuthTag && AuthTag->ReferenceCounter != TagCounter.ReferenceCounter)
					{
						MismatchedTags.Add(Data.Tag);
					}
				}

				LocalTags.Add(Data);
			}
		}
	}

	// Choose which data to display
	const TArray<FFloatAttributeData>& FloatAttrsToShow = bShowLocalPrediction ? LocalFloatAttrs : DataPack.FloatAttributes;
	const TArray<FStructAttributeData>& StructAttrsToShow = bShowLocalPrediction ? LocalStructAttrs : DataPack.StructAttributes;
	const TArray<FGameplayTagData>& TagsToShow = bShowLocalPrediction ? LocalTags : DataPack.GameplayTags;

	// Always show Gameplay Tags first
	CanvasContext.Printf(TEXT("{yellow}Gameplay Tags:"));
	if (TagsToShow.Num() == 0)
	{
		CanvasContext.Printf(TEXT("{grey}  No gameplay tags"));
	}
	else
	{
		for (const FGameplayTagData& Data : TagsToShow)
		{
			bool bMismatch = MismatchedTags.Contains(Data.Tag);
			FString TagColor = bMismatch ? TEXT("red") : TEXT("cyan");

			CanvasContext.Printf(TEXT("  {%s}%s {grey}(Refs: {white}%d{grey})"), *TagColor, *Data.Tag, Data.RefCount);
		}
	}
	CanvasContext.Printf(TEXT(""));

	// Float Attributes View
	if (CurrentViewMode == 0)
	{
		if (FloatAttrsToShow.Num() == 0)
		{
			CanvasContext.Printf(TEXT("{grey}No float attributes"));
		}
		else
		{
			for (const FFloatAttributeData& Data : FloatAttrsToShow)
			{
				bool bBaseMismatch = MismatchedFloatAttrsBase.Contains(Data.AttributeTag);
				bool bCurrentMismatch = MismatchedFloatAttrsCurrent.Contains(Data.AttributeTag);
				bool bAnyMismatch = bBaseMismatch || bCurrentMismatch;

				FString TagColor = bAnyMismatch ? TEXT("red") : TEXT("cyan");
				FString BaseColor = bBaseMismatch ? TEXT("red") : TEXT("white");
				FString CurrentColor = bCurrentMismatch ? TEXT("red") : TEXT("white");

				CanvasContext.Printf(TEXT("{%s}%s"), *TagColor, *Data.AttributeTag);
				CanvasContext.Printf(TEXT("  Base: {%s}%.2f"), *BaseColor, Data.BaseValue);
				CanvasContext.Printf(TEXT("  Current: {%s}%.2f"), *CurrentColor, Data.CurrentValue);
				CanvasContext.Printf(TEXT(""));
			}
		}
	}
	// Struct Attributes View
	else if (CurrentViewMode == 1)
	{
		if (StructAttrsToShow.Num() == 0)
		{
			CanvasContext.Printf(TEXT("{grey}No struct attributes"));
		}
		else
		{
			for (const FStructAttributeData& Data : StructAttrsToShow)
			{
				bool bMismatch = MismatchedStructAttrs.Contains(Data.AttributeTag);
				FString TagColor = bMismatch ? TEXT("red") : TEXT("cyan");

				// Display attribute tag with struct type in brackets (white color)
				CanvasContext.Printf(TEXT("{%s}%s {white}(%s)"), *TagColor, *Data.AttributeTag, *Data.StructType);

				// Display struct data contents (each property on its own line)
				if (!Data.StructData.IsEmpty())
				{
					// Split the formatted data by newlines and print each line
					TArray<FString> Lines;
					Data.StructData.ParseIntoArray(Lines, TEXT("\n"), false);

					for (const FString& Line : Lines)
					{
						// Line already contains color codes, so just print it directly
						CanvasContext.Printf(TEXT("%s"), *Line);
					}
				}

				CanvasContext.Printf(TEXT(""));
			}
		}
	}
}

void FGameplayDebuggerCategory_AttributeComponent::NextPage()
{
	int32 TotalItems = 0;
	switch (CurrentViewMode)
	{
		case 0: TotalItems = DataPack.FloatAttributes.Num(); break;
		case 1: TotalItems = DataPack.StructAttributes.Num(); break;
		case 2: TotalItems = DataPack.GameplayTags.Num(); break;
		default: break;
	}

	int32 TotalPages = FMath::CeilToInt(static_cast<float>(TotalItems) / ItemsPerPage);

	if (CurrentPage < TotalPages - 1)
	{
		CurrentPage++;
	}
}

void FGameplayDebuggerCategory_AttributeComponent::PrevPage()
{
	if (CurrentPage > 0)
	{
		CurrentPage--;
	}
}

void FGameplayDebuggerCategory_AttributeComponent::NextViewMode()
{
	CurrentViewMode = (CurrentViewMode + 1) % 2;
	CurrentPage = 0;
}

void FGameplayDebuggerCategory_AttributeComponent::PrevViewMode()
{
	CurrentViewMode = (CurrentViewMode - 1 + 2) % 2;
	CurrentPage = 0;
}

#endif // WITH_GAMEPLAY_DEBUGGER
