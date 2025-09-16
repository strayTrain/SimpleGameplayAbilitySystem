//  Copyright 2025 Ahmed Elgoni

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SimpleTimeSynchronizer.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SIMPLEGAMEPLAYABILITYSYSTEM_API USimpleTimeSynchronizer : public UActorComponent
{
	GENERATED_BODY()

public:
	USimpleTimeSynchronizer();

	virtual void BeginPlay() override;

	/**
	 * Returns the server time if called on the server.
	 * Returns the clients estimation of the server time if called on the client.
	 * By default, uses GetWorld()->GetGameState()->GetServerWorldTimeSeconds() to get the server time.
	 * Override this function to provide a custom network time synchronisation implementation.
	 * @return The current server time in seconds
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, BlueprintCallable, Category = "AttributeComponent|Utility")
	double GetServerTime();
	virtual double GetServerTime_Implementation();
};
