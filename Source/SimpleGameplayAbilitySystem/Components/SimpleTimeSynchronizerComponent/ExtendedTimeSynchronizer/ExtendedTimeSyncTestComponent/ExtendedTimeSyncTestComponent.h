#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ExtendedTimeSyncTestComponent.generated.h"

/**
 * Represents a single time synchronization test sample
 */
USTRUCT(BlueprintType)
struct FTimeSyncTestSample
{
	GENERATED_BODY()

	// When this sample was taken (local client time)
	UPROPERTY(BlueprintReadOnly, Category = "Time Sync Test")
	double LocalTimestamp = 0.0;

	// Ground truth server time (from server broadcast)
	UPROPERTY(BlueprintReadOnly, Category = "Time Sync Test")
	double GroundTruthServerTime = 0.0;

	// Advanced method prediction
	UPROPERTY(BlueprintReadOnly, Category = "Time Sync Test")
	double AdvancedPrediction = 0.0;

	// Simple method prediction (GameState replication)
	UPROPERTY(BlueprintReadOnly, Category = "Time Sync Test")
	double SimplePrediction = 0.0;

	// Error for advanced method
	UPROPERTY(BlueprintReadOnly, Category = "Time Sync Test")
	double AdvancedError = 0.0;

	// Error for simple method
	UPROPERTY(BlueprintReadOnly, Category = "Time Sync Test")
	double SimpleError = 0.0;

	// Current RTT at time of sample
	UPROPERTY(BlueprintReadOnly, Category = "Time Sync Test")
	double RTT = 0.0;

	// Current sync quality at time of sample
	UPROPERTY(BlueprintReadOnly, Category = "Time Sync Test")
	float SyncQuality = 0.0f;

	FTimeSyncTestSample() = default;

	FTimeSyncTestSample(double InLocalTime, double InGroundTruth, double InAdvanced, double InSimple, double InRTT, float InQuality)
		: LocalTimestamp(InLocalTime)
		, GroundTruthServerTime(InGroundTruth)
		, AdvancedPrediction(InAdvanced)
		, SimplePrediction(InSimple)
		, RTT(InRTT)
		, SyncQuality(InQuality)
	{
		AdvancedError = FMath::Abs(AdvancedPrediction - GroundTruthServerTime);
		SimpleError = FMath::Abs(SimplePrediction - GroundTruthServerTime);
	}
};

/**
 * Accumulated statistics for a time synchronization method
 */
USTRUCT(BlueprintType)
struct FTimeSyncStatistics
{
	GENERATED_BODY()

	// Number of samples collected
	UPROPERTY(BlueprintReadOnly, Category = "Time Sync Test")
	int32 SampleCount = 0;

	// Mean absolute error in seconds
	UPROPERTY(BlueprintReadOnly, Category = "Time Sync Test")
	double MeanError = 0.0;

	// Maximum error observed in seconds
	UPROPERTY(BlueprintReadOnly, Category = "Time Sync Test")
	double MaxError = 0.0;

	// Minimum error observed in seconds
	UPROPERTY(BlueprintReadOnly, Category = "Time Sync Test")
	double MinError = 0.0;

	// Standard deviation of error
	UPROPERTY(BlueprintReadOnly, Category = "Time Sync Test")
	double StdDeviation = 0.0;

	// Running sum of errors (for mean calculation)
	double ErrorSum = 0.0;

	// Running sum of squared errors (for variance calculation)
	double SquaredErrorSum = 0.0;

	FTimeSyncStatistics() = default;

	void AddSample(double Error)
	{
		SampleCount++;
		ErrorSum += Error;

		// Update mean
		MeanError = ErrorSum / SampleCount;

		// Update min/max
		if (SampleCount == 1)
		{
			MinError = Error;
			MaxError = Error;
		}
		else
		{
			MinError = FMath::Min(MinError, Error);
			MaxError = FMath::Max(MaxError, Error);
		}

		// Update variance/std deviation
		SquaredErrorSum += (Error * Error);
		double Variance = (SquaredErrorSum / SampleCount) - (MeanError * MeanError);
		StdDeviation = FMath::Sqrt(FMath::Max(0.0, Variance)); // Clamp to avoid negative due to floating point errors
	}

	void Reset()
	{
		SampleCount = 0;
		MeanError = 0.0;
		MaxError = 0.0;
		MinError = 0.0;
		StdDeviation = 0.0;
		ErrorSum = 0.0;
		SquaredErrorSum = 0.0;
	}

	// Format statistics as string for display
	FString ToString() const
	{
		return FString::Printf(TEXT("Samples: %d | Mean: %.1fms | Max: %.1fms | Min: %.1fms | StdDev: %.1fms"),
			SampleCount,
			MeanError * 1000.0,
			MaxError * 1000.0,
			MinError * 1000.0,
			StdDeviation * 1000.0);
	}

	// Format for CSV export (no labels)
	FString ToCSVString() const
	{
		return FString::Printf(TEXT("%d,%.6f,%.6f,%.6f,%.6f"),
			SampleCount, MeanError, MaxError, MinError, StdDeviation);
	}
};

class USimpleTimeSynchronizerExtended;

/**
 * Component for comparing time synchronization methods.
 * Add this to your Pawn or PlayerController to test advanced vs simple sync.
 *
 * Server periodically broadcasts ground truth server time.
 * Component compares predictions from advanced sync vs simple GameState replication.
 *
 * Usage:
 * 1. Add this component to your player Pawn or PlayerController
 * 2. Configure network emulation (Edit > Editor Preferences > Play > Network Emulation)
 * 3. Launch PIE with multiplayer
 * 4. View on-screen statistics or export to CSV
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SIMPLEGAMEPLAYABILITYSYSTEM_API UExtendedTimeSyncTestComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UExtendedTimeSyncTestComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/* Configuration Properties */

	// How often the server broadcasts ground truth time (in seconds)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Sync Test|Config", meta = (ClampMin = "0.1", ClampMax = "10.0"))
	float BroadcastFrequency = 1.0f;

	// Number of samples to keep in rolling window (0 = unlimited)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Sync Test|Config", meta = (ClampMin = "0", ClampMax = "10000"))
	int32 StatisticsWindowSize = 100;

	// Display debug information on screen
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Sync Test|Config")
	bool bShowDebugInfo = true;

	// Log all samples to CSV file
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Sync Test|Config")
	bool bEnableCSVLogging = false;

	// Automatically start testing on BeginPlay
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Sync Test|Config")
	bool bAutoStartTest = true;

	// Scale factor for on-screen debug text
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Sync Test|Config", meta = (ClampMin = "0.5", ClampMax = "3.0"))
	float DebugTextScale = 1.5f;

	/* State Properties */

	// Is the test currently running?
	UPROPERTY(BlueprintReadOnly, Category = "Time Sync Test|State")
	bool bIsTestRunning = false;

	// Statistics for advanced sync method
	UPROPERTY(BlueprintReadOnly, Category = "Time Sync Test|State")
	FTimeSyncStatistics AdvancedStats;

	// Statistics for simple sync method
	UPROPERTY(BlueprintReadOnly, Category = "Time Sync Test|State")
	FTimeSyncStatistics SimpleStats;

	// All collected samples (up to window size)
	UPROPERTY(BlueprintReadOnly, Category = "Time Sync Test|State")
	TArray<FTimeSyncTestSample> Samples;

	// How many times advanced method was more accurate
	UPROPERTY(BlueprintReadOnly, Category = "Time Sync Test|State")
	int32 AdvancedWinCount = 0;

	// How many times simple method was more accurate
	UPROPERTY(BlueprintReadOnly, Category = "Time Sync Test|State")
	int32 SimpleWinCount = 0;

	/* Public API */

	// Start the test
	UFUNCTION(BlueprintCallable, Category = "Time Sync Test")
	void StartTest();

	// Stop the test
	UFUNCTION(BlueprintCallable, Category = "Time Sync Test")
	void StopTest();

	// Reset all statistics
	UFUNCTION(BlueprintCallable, Category = "Time Sync Test")
	void ResetStatistics();

	// Export current data to CSV file
	UFUNCTION(BlueprintCallable, Category = "Time Sync Test")
	FString ExportToCSV();

	// Get comparison summary string
	UFUNCTION(BlueprintPure, Category = "Time Sync Test")
	FString GetComparisonSummary() const;

	// Get win rate for advanced method (0-1)
	UFUNCTION(BlueprintPure, Category = "Time Sync Test")
	float GetAdvancedWinRate() const;

private:
	// Reference to time synchronizer component on same actor
	UPROPERTY()
	TObjectPtr<USimpleTimeSynchronizerExtended> TimeSyncComponent;

	// Time since last broadcast
	float TimeSinceLastBroadcast = 0.0f;

	// CSV file handle for logging
	IFileHandle* CSVFileHandle = nullptr;

	// CSV file path
	FString CSVFilePath;

	// Initialize CSV logging
	void InitializeCSVLogging();

	// Close CSV file
	void CloseCSVLogging();

	// Write sample to CSV
	void WriteSampleToCSV(const FTimeSyncTestSample& Sample);

	// Display debug info on screen
	void DisplayDebugInfo();

	// Process a ground truth broadcast (client-side)
	void ProcessGroundTruth(double GroundTruth);
};
