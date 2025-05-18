#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "GameFramework/WorldSettings.h"
#include "NativeGameplayTags.h"
#include "GameFramework/GameStateBase.h"

// Define Gameplay Tags
UE_DEFINE_GAMEPLAY_TAG(TestAttributeTag, "Test.SGAS.Attributes.MyTestAttribute");

// Test fixture that sets up the persistent test world and subsystem
class FTestFixture
{
public:
	FTestFixture(FName TestName)
	{
		World = CreateTestWorld(TestName);
		Subsystem = EnsureSubsystem(World, TestName);
	}

	~FTestFixture()
	{
		// Clean up if needed; note that in many tests the engine will tear down the world.
	}

	UWorld* GetWorld() const { return World; }
	USimpleEventSubsystem* GetSubsystem() const { return Subsystem; }
	
	// Helper to initialize common test items
	void InitializeSomethingSharedBetweenTests()
	{
		
	}

private:
	static UWorld* CreateTestWorld(const FName& WorldName)
	{
		UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Game, false, WorldName);
		
		if (GEngine)
		{
			FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
			WorldContext.SetCurrentWorld(TestWorld);
		}

		AWorldSettings* WorldSettings = TestWorld->GetWorldSettings();
		if (WorldSettings)
		{
			WorldSettings->SetActorTickEnabled(true);
			WorldSettings->MaxUndilatedFrameTime = 99999;
		}

		if (!TestWorld->GetGameState())
		{
			if (AGameStateBase* TestGameState = TestWorld->SpawnActor<AGameStateBase>())
			{
				TestWorld->SetGameState(TestGameState);
			}
		}

		if (!TestWorld->bIsWorldInitialized)
		{
			TestWorld->InitializeNewWorld(UWorld::InitializationValues()
			                              .ShouldSimulatePhysics(false)
			                              .AllowAudioPlayback(false)
			                              .RequiresHitProxies(false)
			                              .CreatePhysicsScene(true)
			                              .CreateNavigation(false)
			                              .CreateAISystem(false));
		}

		TestWorld->InitializeActorsForPlay(FURL());

		return TestWorld;
	}

	
	static USimpleEventSubsystem* EnsureSubsystem(UWorld* World, const FName& TestName)
	{
		if (!World)
		{
			UE_LOG(LogTemp, Warning, TEXT("World is null in EnsureSubsystem."));
			return nullptr;
		}

		UGameInstance* GameInstance = World->GetGameInstance();
		if (!GameInstance)
		{
			GameInstance = NewObject<UGameInstance>(World);
			World->SetGameInstance(GameInstance);
			GameInstance->Init();
		}

		return GameInstance->GetSubsystem<USimpleEventSubsystem>();
	}

	UWorld* World = nullptr;
	USimpleEventSubsystem* Subsystem = nullptr;
};
