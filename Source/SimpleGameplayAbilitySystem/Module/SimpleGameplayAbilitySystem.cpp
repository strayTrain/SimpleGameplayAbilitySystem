#include "SimpleGameplayAbilitySystem.h"
#include "GameplayTagsManager.h"

#define LOCTEXT_NAMESPACE "FSimpleGameplayAbilitySystemModule"

DEFINE_LOG_CATEGORY(LogSimpleGAS);

void FSimpleGameplayAbilitySystemModule::StartupModule()
{
	UGameplayTagsManager::Get().AddTagIniSearchPath(FPaths::ProjectPluginsDir() / TEXT("SimpleGameplayAbilitySystem/Config/Tags"));
}

void FSimpleGameplayAbilitySystemModule::ShutdownModule()
{
	
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FSimpleGameplayAbilitySystemModule, SimpleGameplayAbilitySystem)