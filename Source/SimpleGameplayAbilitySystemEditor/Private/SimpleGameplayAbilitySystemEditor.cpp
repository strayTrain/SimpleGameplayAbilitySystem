#include "SimpleGameplayAbilitySystemEditor.h"
#include "StructAttributeCustomization.h"
#include "PropertyEditorModule.h"
#include "Modules/ModuleManager.h"
#include "SimpleGameplayAbilitySystem/SimpleGameplayAbilityComponent/SimpleAbilityComponentTypes.h"

#define LOCTEXT_NAMESPACE "FSimpleGameplayAbilitySystemEditorModule"

void FSimpleGameplayAbilitySystemEditorModule::StartupModule()
{
	// Register property customizations
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomPropertyTypeLayout(
		FStructAttribute::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FStructAttributeCustomization::MakeInstance)
	);
    
	PropertyModule.NotifyCustomizationModuleChanged();
}

void FSimpleGameplayAbilitySystemEditorModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomPropertyTypeLayout(FStructAttribute::StaticStruct()->GetFName());
	}
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FSimpleGameplayAbilitySystemEditorModule, SimpleGameplayAbilitySystemEditor)