// Copyright 2025 Ahmed Elgoni

#include "SimpleTimeSynchronizer.h"

#include "GameFramework/GameStateBase.h"

USimpleTimeSynchronizer::USimpleTimeSynchronizer()
{
	PrimaryComponentTick.bCanEverTick = false;
}

double USimpleTimeSynchronizer::GetServerTime_Implementation()
{
	if (UWorld* World = GetWorld())
	{
		if (const AGameStateBase* GameState = World->GetGameState())
		{
			return GameState->GetServerWorldTimeSeconds();
		}
	}
	
	return 0.0;
}
