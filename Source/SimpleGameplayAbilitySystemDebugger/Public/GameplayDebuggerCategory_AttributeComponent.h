#pragma once

#include "CoreMinimal.h"

#if WITH_GAMEPLAY_DEBUGGER
#include "GameplayDebuggerCategory.h"

class AActor;
class APlayerController;
class USimpleAttributeComponent;

class FGameplayDebuggerCategory_AttributeComponent : public FGameplayDebuggerCategory
{
public:
	FGameplayDebuggerCategory_AttributeComponent();

	virtual void CollectData(APlayerController* OwnerPC, AActor* DebugActor) override;
	virtual void DrawData(APlayerController* OwnerPC, FGameplayDebuggerCanvasContext& CanvasContext) override;

	static TSharedRef<FGameplayDebuggerCategory> MakeInstance();

protected:
	struct FFloatAttributeData
	{
		FString AttributeTag;
		float BaseValue;
		float CurrentValue;

		FFloatAttributeData() : BaseValue(0.0f), CurrentValue(0.0f) {}
	};

	struct FStructAttributeData
	{
		FString AttributeTag;
		FString StructType;
		FString StructData;

		FStructAttributeData() {}
	};

	struct FGameplayTagData
	{
		FString Tag;
		int32 RefCount;

		FGameplayTagData() : RefCount(0) {}
	};

	// Replicated data pack (server-authoritative data)
	struct FRepData
	{
		TArray<FFloatAttributeData> FloatAttributes;
		TArray<FStructAttributeData> StructAttributes;
		TArray<FGameplayTagData> GameplayTags;

		void Serialize(FArchive& Ar);
	};

	FRepData DataPack;

	// Pagination and view mode
	int32 CurrentPage;
	int32 ItemsPerPage;
	int32 CurrentViewMode; // 0 = Float Attributes, 1 = Struct Attributes, 2 = Gameplay Tags

	void NextPage();
	void PrevPage();
	void NextViewMode();
	void PrevViewMode();

	// Helper function to format struct data for display
	static FString FormatStructDataForDisplay(const UScriptStruct* StructType, const void* StructData, int32 IndentLevel = 0);
	static FString GetPropertyValueAsString(const FProperty* Property, const void* ValuePtr, int32 IndentLevel = 0);
};

#endif // WITH_GAMEPLAY_DEBUGGER
