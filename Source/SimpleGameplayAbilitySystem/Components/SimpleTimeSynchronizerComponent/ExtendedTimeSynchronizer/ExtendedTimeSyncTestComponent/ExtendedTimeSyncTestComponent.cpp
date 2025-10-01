// Copyright 2025 Ahmed Elgoni

#include "ExtendedTimeSyncTestComponent.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleTimeSynchronizerComponent/ExtendedTimeSynchronizer/SimpleTimeSynchronizerExtended.h"
#include "GameFramework/GameStateBase.h"
#include "Engine/Engine.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

UExtendedTimeSyncTestComponent::UExtendedTimeSyncTestComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	SetIsReplicatedByDefault(false); // Component doesn't need replication, only RPCs
}

void UExtendedTimeSyncTestComponent::BeginPlay()
{
	Super::BeginPlay();

	// Find or create TimeSynchronizer component on the same actor
	TimeSyncComponent = GetOwner()->FindComponentByClass<USimpleTimeSynchronizerExtended>();

	if (!TimeSyncComponent)
	{
		// Create one if it doesn't exist
		TimeSyncComponent = NewObject<USimpleTimeSynchronizerExtended>(GetOwner(), USimpleTimeSynchronizerExtended::StaticClass(), TEXT("AutoCreatedTimeSyncComponent"));
		TimeSyncComponent->RegisterComponent();
	}

	if (bAutoStartTest)
	{
		StartTest();
	}
}

void UExtendedTimeSyncTestComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Ensure CSV file is properly closed when component is destroyed
	if (bEnableCSVLogging && CSVFileHandle)
	{
		CloseCSVLogging();
	}

	Super::EndPlay(EndPlayReason);
}

void UExtendedTimeSyncTestComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsTestRunning)
	{
		return;
	}

	// Client: Periodically sample predictions against GameState's replicated server time
	// We use GameState's ServerWorldTimeSeconds as "ground truth" since it's replicated from server
	if (!GetOwner()->HasAuthority())
	{
		TimeSinceLastBroadcast += DeltaTime;

		if (TimeSinceLastBroadcast >= BroadcastFrequency)
		{
			if (!TimeSyncComponent)
			{
				return;
			}

			// Advanced: Predicts current server time using local clock + offset
			double AdvancedPrediction = TimeSyncComponent->GetServerTime();

			// Simple: Uses replicated ServerWorldTimeSeconds (delayed by network)
			UWorld* World = GetWorld();
			if (!World || !World->GetGameState())
			{
				return; // GameState not ready yet
			}
			double SimplePrediction = World->GetGameState()->GetServerWorldTimeSeconds();

			// Calculate how much advanced method leads simple method
			double RTT = TimeSyncComponent->GetRTT();
			double ExpectedLead = RTT / 2.0; // Expected one-way latency
			double ActualLead = AdvancedPrediction - SimplePrediction;

			// For CSV: Use the best estimate of "true" server time
			// The advanced prediction attempts to predict the current server time
			// The simple method is known to be behind by ~RTT/2
			// So our best guess at true server time is: SimplePrediction + RTT/2
			double EstimatedTrueServerTime = SimplePrediction + ExpectedLead;

			// Measure both methods against this estimate
			double AdvancedError = FMath::Abs(AdvancedPrediction - EstimatedTrueServerTime);
			double SimpleError = FMath::Abs(SimplePrediction - EstimatedTrueServerTime);

			float Quality = TimeSyncComponent->GetSyncQuality();

			// Create sample for storage/CSV
			FTimeSyncTestSample Sample(
				GetWorld()->GetTimeSeconds(),
				EstimatedTrueServerTime, // Best estimate of true server time
				AdvancedPrediction,
				SimplePrediction,
				RTT,
				Quality
			);
			Sample.AdvancedError = AdvancedError;
			Sample.SimpleError = SimpleError;

			// Add to samples array (rolling window)
			if (StatisticsWindowSize > 0 && Samples.Num() >= StatisticsWindowSize)
			{
				Samples.RemoveAt(0);
			}
			Samples.Add(Sample);

			// Update statistics
			AdvancedStats.AddSample(AdvancedError);
			SimpleStats.AddSample(SimpleError);

			// Track wins
			if (AdvancedError < SimpleError)
			{
				AdvancedWinCount++;
			}
			else if (SimpleError < AdvancedError)
			{
				SimpleWinCount++;
			}

			// Log to CSV if enabled
			if (bEnableCSVLogging && CSVFileHandle)
			{
				WriteSampleToCSV(Sample);
			}

			TimeSinceLastBroadcast = 0.0f;
		}
	}

	// Display debug info (all machines)
	if (bShowDebugInfo)
	{
		DisplayDebugInfo();
	}
}

void UExtendedTimeSyncTestComponent::StartTest()
{
	bIsTestRunning = true;
	ResetStatistics();

	if (bEnableCSVLogging && GetOwnerRole() != ROLE_Authority)
	{
		InitializeCSVLogging();
	}

	if (GEngine)
	{
		FString Message = GetOwnerRole() == ROLE_Authority ?
			TEXT("Time Sync Test Started (SERVER)") :
			TEXT("Time Sync Test Started (CLIENT)");
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, Message);
	}
}

void UExtendedTimeSyncTestComponent::StopTest()
{
	bIsTestRunning = false;

	if (bEnableCSVLogging && GetOwnerRole() != ROLE_Authority)
	{
		CloseCSVLogging();
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("Time Sync Test Stopped"));
	}
}

void UExtendedTimeSyncTestComponent::ResetStatistics()
{
	AdvancedStats.Reset();
	SimpleStats.Reset();
	Samples.Empty();
	AdvancedWinCount = 0;
	SimpleWinCount = 0;
	TimeSinceLastBroadcast = 0.0f;
}

FString UExtendedTimeSyncTestComponent::ExportToCSV()
{
	if (Samples.Num() == 0)
	{
		return TEXT("No samples to export");
	}

	// Generate filename with timestamp
	FString Timestamp = FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
	FString Filename = FString::Printf(TEXT("TimeSyncTest_%s.csv"), *Timestamp);
	FString Directory = FPaths::ProjectSavedDir() / TEXT("TimeSyncTests");
	FString FilePath = Directory / Filename;

	// Create directory if it doesn't exist
	IFileManager::Get().MakeDirectory(*Directory, true);

	// Build CSV content
	FString CSVContent;

	// Header
	CSVContent += TEXT("LocalTime,GroundTruth,AdvancedPrediction,SimplePrediction,AdvancedError,SimpleError,RTT,SyncQuality\n");

	// Data rows
	for (const FTimeSyncTestSample& Sample : Samples)
	{
		CSVContent += FString::Printf(TEXT("%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.3f\n"),
			Sample.LocalTimestamp,
			Sample.GroundTruthServerTime,
			Sample.AdvancedPrediction,
			Sample.SimplePrediction,
			Sample.AdvancedError,
			Sample.SimpleError,
			Sample.RTT,
			Sample.SyncQuality);
	}

	// Summary statistics at the end
	CSVContent += TEXT("\n--- Summary Statistics ---\n");
	CSVContent += TEXT("Method,SampleCount,MeanError,MaxError,MinError,StdDev\n");
	CSVContent += FString::Printf(TEXT("Advanced,%s\n"), *AdvancedStats.ToCSVString());
	CSVContent += FString::Printf(TEXT("Simple,%s\n"), *SimpleStats.ToCSVString());
	CSVContent += FString::Printf(TEXT("\nAdvanced Win Rate,%.2f%%\n"), GetAdvancedWinRate() * 100.0f);

	// Write to file
	if (FFileHelper::SaveStringToFile(CSVContent, *FilePath))
	{
		return FString::Printf(TEXT("Exported to: %s"), *FilePath);
	}
	else
	{
		return FString::Printf(TEXT("Failed to export to: %s"), *FilePath);
	}
}

FString UExtendedTimeSyncTestComponent::GetComparisonSummary() const
{
	if (AdvancedStats.SampleCount == 0)
	{
		return TEXT("No samples collected yet");
	}

	FString Summary;
	Summary += FString::Printf(TEXT("=== Time Sync Comparison ===\n"));
	Summary += FString::Printf(TEXT("Total Samples: %d\n\n"), AdvancedStats.SampleCount);
	Summary += FString::Printf(TEXT("Advanced Method:\n  %s\n\n"), *AdvancedStats.ToString());
	Summary += FString::Printf(TEXT("Simple Method:\n  %s\n\n"), *SimpleStats.ToString());
	Summary += FString::Printf(TEXT("Advanced Win Rate: %.1f%%\n"), GetAdvancedWinRate() * 100.0f);

	// Determine winner
	if (AdvancedStats.MeanError < SimpleStats.MeanError)
	{
		double Improvement = ((SimpleStats.MeanError - AdvancedStats.MeanError) / SimpleStats.MeanError) * 100.0;
		Summary += FString::Printf(TEXT("Winner: Advanced Method (%.1f%% better)\n"), Improvement);
	}
	else if (SimpleStats.MeanError < AdvancedStats.MeanError)
	{
		double Improvement = ((AdvancedStats.MeanError - SimpleStats.MeanError) / AdvancedStats.MeanError) * 100.0;
		Summary += FString::Printf(TEXT("Winner: Simple Method (%.1f%% better)\n"), Improvement);
	}
	else
	{
		Summary += TEXT("Result: Tie\n");
	}

	return Summary;
}

float UExtendedTimeSyncTestComponent::GetAdvancedWinRate() const
{
	int32 TotalComparisons = AdvancedWinCount + SimpleWinCount;
	if (TotalComparisons == 0)
	{
		return 0.5f;
	}

	return static_cast<float>(AdvancedWinCount) / TotalComparisons;
}

void UExtendedTimeSyncTestComponent::InitializeCSVLogging()
{
	// Generate filename with timestamp
	FString Timestamp = FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
	FString Filename = FString::Printf(TEXT("TimeSyncTest_%s.csv"), *Timestamp);
	FString Directory = FPaths::ProjectSavedDir() / TEXT("TimeSyncTests");
	CSVFilePath = Directory / Filename;

	// Create directory if it doesn't exist
	IFileManager::Get().MakeDirectory(*Directory, true);

	// Open file for writing
	CSVFileHandle = FPlatformFileManager::Get().GetPlatformFile().OpenWrite(*CSVFilePath, false, false);

	if (CSVFileHandle)
	{
		// Write header
		FString Header = TEXT("LocalTime,GroundTruth,AdvancedPrediction,SimplePrediction,AdvancedError,SimpleError,RTT,SyncQuality\n");
		FTCHARToUTF8 UTF8Header(*Header);
		CSVFileHandle->Write((const uint8*)UTF8Header.Get(), UTF8Header.Length());

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, FString::Printf(TEXT("CSV Logging to: %s"), *CSVFilePath));
		}
	}
}

void UExtendedTimeSyncTestComponent::CloseCSVLogging()
{
	if (CSVFileHandle)
	{
		// Write summary
		FString Summary = TEXT("\n--- Summary Statistics ---\n");
		Summary += TEXT("Method,SampleCount,MeanError,MaxError,MinError,StdDev\n");
		Summary += FString::Printf(TEXT("Advanced,%s\n"), *AdvancedStats.ToCSVString());
		Summary += FString::Printf(TEXT("Simple,%s\n"), *SimpleStats.ToCSVString());
		Summary += FString::Printf(TEXT("\nAdvanced Win Rate,%.2f%%\n"), GetAdvancedWinRate() * 100.0f);

		FTCHARToUTF8 UTF8Summary(*Summary);
		CSVFileHandle->Write((const uint8*)UTF8Summary.Get(), UTF8Summary.Length());

		delete CSVFileHandle;
		CSVFileHandle = nullptr;

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("CSV Log Closed"));
		}
	}
}

void UExtendedTimeSyncTestComponent::WriteSampleToCSV(const FTimeSyncTestSample& Sample)
{
	if (!CSVFileHandle)
	{
		return;
	}

	FString Line = FString::Printf(TEXT("%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.3f\n"),
		Sample.LocalTimestamp,
		Sample.GroundTruthServerTime,
		Sample.AdvancedPrediction,
		Sample.SimplePrediction,
		Sample.AdvancedError,
		Sample.SimpleError,
		Sample.RTT,
		Sample.SyncQuality);

	FTCHARToUTF8 UTF8Line(*Line);
	CSVFileHandle->Write((const uint8*)UTF8Line.Get(), UTF8Line.Length());
	CSVFileHandle->Flush();
}

void UExtendedTimeSyncTestComponent::DisplayDebugInfo()
{
	if (!GEngine)
	{
		return;
	}

	// Different display for server vs client
	if (GetOwnerRole() == ROLE_Authority)
	{
		// Server: Just show that we're broadcasting
		FString ServerInfo = FString::Printf(TEXT("[SERVER] Broadcasting at %.1fHz | Next in %.1fs"),
			1.0f / BroadcastFrequency,
			BroadcastFrequency - TimeSinceLastBroadcast);
		GEngine->AddOnScreenDebugMessage(1000, 0.0f, FColor::White, ServerInfo, true, FVector2D(DebugTextScale, DebugTextScale));
	}
	else
	{
		// Client: Show full comparison
		int32 Line = 1000;
		float Duration = 0.0f;

		// Title
		GEngine->AddOnScreenDebugMessage(Line++, Duration, FColor::White,
			TEXT("=== TIME SYNC COMPARISON TEST ==="), true, FVector2D(DebugTextScale, DebugTextScale));

		// Sample count and sync info
		if (TimeSyncComponent)
		{
			FString InfoLine = FString::Printf(TEXT("Samples: %d | RTT: %.1fms | Quality: %.1f%%"),
				AdvancedStats.SampleCount,
				TimeSyncComponent->GetRTT() * 1000.0,
				TimeSyncComponent->GetSyncQuality() * 100.0f);
			GEngine->AddOnScreenDebugMessage(Line++, Duration, FColor::Cyan, InfoLine, true, FVector2D(DebugTextScale, DebugTextScale));
		}

		GEngine->AddOnScreenDebugMessage(Line++, Duration, FColor::White, TEXT(""), true, FVector2D(DebugTextScale, DebugTextScale));

		// Advanced method stats (green if winning, red if losing)
		FColor AdvancedColor = (AdvancedStats.MeanError <= SimpleStats.MeanError || SimpleStats.SampleCount == 0) ? FColor::Green : FColor::Red;
		GEngine->AddOnScreenDebugMessage(Line++, Duration, FColor::Yellow,
			TEXT("ADVANCED METHOD (with sync component):"), true, FVector2D(DebugTextScale, DebugTextScale));
		GEngine->AddOnScreenDebugMessage(Line++, Duration, AdvancedColor,
			FString::Printf(TEXT("  Mean: %.1fms | Max: %.1fms | StdDev: %.1fms"),
				AdvancedStats.MeanError * 1000.0,
				AdvancedStats.MaxError * 1000.0,
				AdvancedStats.StdDeviation * 1000.0),
			true, FVector2D(DebugTextScale, DebugTextScale));

		GEngine->AddOnScreenDebugMessage(Line++, Duration, FColor::White, TEXT(""), true, FVector2D(DebugTextScale, DebugTextScale));

		// Simple method stats (green if winning, red if losing)
		FColor SimpleColor = (SimpleStats.MeanError < AdvancedStats.MeanError) ? FColor::Green : FColor::Red;
		GEngine->AddOnScreenDebugMessage(Line++, Duration, FColor::Yellow,
			TEXT("SIMPLE METHOD (GameState replication):"), true, FVector2D(DebugTextScale, DebugTextScale));
		GEngine->AddOnScreenDebugMessage(Line++, Duration, SimpleColor,
			FString::Printf(TEXT("  Mean: %.1fms | Max: %.1fms | StdDev: %.1fms"),
				SimpleStats.MeanError * 1000.0,
				SimpleStats.MaxError * 1000.0,
				SimpleStats.StdDeviation * 1000.0),
			true, FVector2D(DebugTextScale, DebugTextScale));

		GEngine->AddOnScreenDebugMessage(Line++, Duration, FColor::White, TEXT(""), true, FVector2D(DebugTextScale, DebugTextScale));

		// Win rate
		float WinRate = GetAdvancedWinRate();
		FColor WinRateColor = (WinRate >= 0.5f) ? FColor::Green : FColor::Red;
		GEngine->AddOnScreenDebugMessage(Line++, Duration, WinRateColor,
			FString::Printf(TEXT("Advanced Win Rate: %.1f%% (%d/%d)"),
				WinRate * 100.0f,
				AdvancedWinCount,
				AdvancedWinCount + SimpleWinCount),
			true, FVector2D(DebugTextScale, DebugTextScale));

		// Overall winner
		if (AdvancedStats.SampleCount >= 5)
		{
			if (AdvancedStats.MeanError < SimpleStats.MeanError)
			{
				double Improvement = ((SimpleStats.MeanError - AdvancedStats.MeanError) / SimpleStats.MeanError) * 100.0;
				GEngine->AddOnScreenDebugMessage(Line++, Duration, FColor::Green,
					FString::Printf(TEXT("WINNER: Advanced (%.1f%% better)"), Improvement),
					true, FVector2D(DebugTextScale, DebugTextScale));
			}
			else if (SimpleStats.MeanError < AdvancedStats.MeanError)
			{
				double Improvement = ((AdvancedStats.MeanError - SimpleStats.MeanError) / AdvancedStats.MeanError) * 100.0;
				GEngine->AddOnScreenDebugMessage(Line++, Duration, FColor::Red,
					FString::Printf(TEXT("WINNER: Simple (%.1f%% better)"), Improvement),
					true, FVector2D(DebugTextScale, DebugTextScale));
			}
		}
	}
}

void UExtendedTimeSyncTestComponent::ProcessGroundTruth(double GroundTruth)
{
	if (!TimeSyncComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("TimeSyncTestComponent: No TimeSyncComponent available!"));
		return;
	}

	// Get current predictions from both methods
	double AdvancedPrediction = TimeSyncComponent->GetServerTime();
	double SimplePrediction = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();

	// Get current sync metrics
	double RTT = TimeSyncComponent->GetRTT();
	float Quality = TimeSyncComponent->GetSyncQuality();

	// Create sample
	FTimeSyncTestSample Sample(
		GetWorld()->GetTimeSeconds(),
		GroundTruth,
		AdvancedPrediction,
		SimplePrediction,
		RTT,
		Quality
	);

	// Debug log
	UE_LOG(LogTemp, Log, TEXT("TimeSyncTest: GT=%.3f, Adv=%.3f (err=%.1fms), Simple=%.3f (err=%.1fms), RTT=%.1fms, Q=%.1f%%"),
		GroundTruth, AdvancedPrediction, Sample.AdvancedError * 1000.0,
		SimplePrediction, Sample.SimpleError * 1000.0,
		RTT * 1000.0, Quality * 100.0f);

	// Add to samples array (rolling window)
	if (StatisticsWindowSize > 0 && Samples.Num() >= StatisticsWindowSize)
	{
		Samples.RemoveAt(0);
	}
	Samples.Add(Sample);

	// Update statistics
	AdvancedStats.AddSample(Sample.AdvancedError);
	SimpleStats.AddSample(Sample.SimpleError);

	// Track wins
	if (Sample.AdvancedError < Sample.SimpleError)
	{
		AdvancedWinCount++;
	}
	else if (Sample.SimpleError < Sample.AdvancedError)
	{
		SimpleWinCount++;
	}

	// Log to CSV if enabled
	if (bEnableCSVLogging && CSVFileHandle)
	{
		WriteSampleToCSV(Sample);
	}
}
