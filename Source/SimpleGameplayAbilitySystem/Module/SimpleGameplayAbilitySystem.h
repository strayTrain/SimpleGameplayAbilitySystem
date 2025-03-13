// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSimpleGAS, Log, All);

static void SIMPLE_LOG(UObject* WorldContext, FString Msg)
{
	if (!ensure(WorldContext))
	{
		return;
	}

	UWorld* World = WorldContext->GetWorld();
	if (!ensure(World))
	{
		return;
	}

	FString NetPrefix = World->GetNetMode() < NM_Client ? "[SERVER]" : "[CLIENT]";
	UE_LOG(LogSimpleGAS, Log, TEXT("%s %s"), *NetPrefix, *Msg);
}

class FSimpleGameplayAbilitySystemModule : public IModuleInterface
{
	public:

		/** IModuleInterface implementation */
		virtual void StartupModule() override;
		virtual void ShutdownModule() override;
};
