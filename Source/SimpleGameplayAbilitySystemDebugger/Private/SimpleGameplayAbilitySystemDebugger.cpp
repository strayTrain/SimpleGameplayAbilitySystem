#include "SimpleGameplayAbilitySystemDebugger.h"

#if WITH_GAMEPLAY_DEBUGGER
#include "GameplayDebugger.h"
#include "GameplayDebuggerCategory_AbilityComponent.h"
#include "GameplayDebuggerCategory_AttributeComponent.h"
#include "GameplayDebuggerCategory_AttributeModifier.h"
#endif

#define LOCTEXT_NAMESPACE "FSimpleGameplayAbilitySystemDebuggerModule"

void FSimpleGameplayAbilitySystemDebuggerModule::StartupModule()
{
#if WITH_GAMEPLAY_DEBUGGER
	IGameplayDebugger& GameplayDebuggerModule = IGameplayDebugger::Get();

	GameplayDebuggerModule.RegisterCategory("AbilityComponent",
		IGameplayDebugger::FOnGetCategory::CreateStatic(&FGameplayDebuggerCategory_AbilityComponent::MakeInstance),
		EGameplayDebuggerCategoryState::EnabledInGameAndSimulate);

	GameplayDebuggerModule.RegisterCategory("AttributeComponent",
		IGameplayDebugger::FOnGetCategory::CreateStatic(&FGameplayDebuggerCategory_AttributeComponent::MakeInstance),
		EGameplayDebuggerCategoryState::EnabledInGameAndSimulate);

	GameplayDebuggerModule.RegisterCategory("AttributeModifier",
		IGameplayDebugger::FOnGetCategory::CreateStatic(&FGameplayDebuggerCategory_AttributeModifier::MakeInstance),
		EGameplayDebuggerCategoryState::EnabledInGameAndSimulate);

	GameplayDebuggerModule.NotifyCategoriesChanged();
#endif
}

void FSimpleGameplayAbilitySystemDebuggerModule::ShutdownModule()
{
#if WITH_GAMEPLAY_DEBUGGER
	if (IGameplayDebugger::IsAvailable())
	{
		IGameplayDebugger& GameplayDebuggerModule = IGameplayDebugger::Get();
		GameplayDebuggerModule.UnregisterCategory("AbilityComponent");
		GameplayDebuggerModule.UnregisterCategory("AttributeComponent");
		GameplayDebuggerModule.UnregisterCategory("AttributeModifier");
	}
#endif
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSimpleGameplayAbilitySystemDebuggerModule, SimpleGameplayAbilitySystemDebugger)
