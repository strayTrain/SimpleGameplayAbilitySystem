#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AttributeSet.generated.h"

struct FStructAttribute;
struct FFloatAttribute;

UCLASS()
class SIMPLEGAMEPLAYABILITYSYSTEM_API UAttributeSet : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (TitleProperty = "AttributeName"))
	TArray<FFloatAttribute> FloatAttributes;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (TitleProperty = "AttributeName"))
	TArray<FStructAttribute> StructAttributes;
};
