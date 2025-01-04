// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSimpleGAS, Log, All);

class FSimpleGameplayAbilitySystemModule : public IModuleInterface
{
	public:

		/** IModuleInterface implementation */
		virtual void StartupModule() override;
		virtual void ShutdownModule() override;
};
