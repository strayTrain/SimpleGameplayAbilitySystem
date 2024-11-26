// Copyright Epic Games, Inc. All Rights Reserved.

#include "SimpleGameplayAbilitySystem.h"
#include "GameplayTagsManager.h"
#include "SimpleGameplayAbilitySystem/DefaultTags/DefaultTags.h"

#define LOCTEXT_NAMESPACE "FSimpleGameplayAbilitySystemModule"

void FSimpleGameplayAbilitySystemModule::StartupModule()
{
	UGameplayTagsManager::Get().AddTagIniSearchPath(FPaths::ProjectPluginsDir() / TEXT("SimpleGameplayAbilitySystem/Config/Tags"));
	FDefaultTags::InitializeDefaultTags();
}

void FSimpleGameplayAbilitySystemModule::ShutdownModule()
{
	
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FSimpleGameplayAbilitySystemModule, SimpleGameplayAbilitySystem)