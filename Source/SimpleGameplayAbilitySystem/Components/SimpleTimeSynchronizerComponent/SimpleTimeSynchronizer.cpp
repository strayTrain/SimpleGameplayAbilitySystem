//  Copyright 2025 Ahmed Elgoni

#include "SimpleTimeSynchronizer.h"

#include "GameFramework/GameStateBase.h"

USimpleTimeSynchronizer::USimpleTimeSynchronizer()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USimpleTimeSynchronizer::BeginPlay()
{
	Super::BeginPlay();
	SetIsReplicated(true);
}

double USimpleTimeSynchronizer::GetServerTime_Implementation()
{
	return GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
}


