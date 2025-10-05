#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleTimeSynchronizerComponent/SimpleTimeSynchronizer.h"
#include "SimpleTimeSynchronizerExtended.generated.h"

/**
 * Represents a single time synchronization sample with client and server timestamps
 */
USTRUCT(BlueprintType)
struct FSyncSample
{
	GENERATED_BODY()

	// Client timestamp when request was sent
	UPROPERTY()
	double ClientSendTime = 0.0;

	// Server timestamp when request was received
	UPROPERTY()
	double ServerReceiveTime = 0.0;

	// Server timestamp when response was sent
	UPROPERTY()
	double ServerSendTime = 0.0;

	// Client timestamp when response was received
	UPROPERTY()
	double ClientReceiveTime = 0.0;

	// Calculated round-trip time
	UPROPERTY()
	double RTT = 0.0;

	// Calculated clock offset
	UPROPERTY()
	double ClockOffset = 0.0;

	// Whether this sample is valid (not rejected as outlier)
	UPROPERTY()
	bool bIsValid = true;

	FSyncSample() = default;

	FSyncSample(double InClientSendTime, double InServerReceiveTime, double InServerSendTime, double InClientReceiveTime)
		: ClientSendTime(InClientSendTime)
		, ServerReceiveTime(InServerReceiveTime)
		, ServerSendTime(InServerSendTime)
		, ClientReceiveTime(InClientReceiveTime)
	{
		// RTT = (T3 - T0) - (T2 - T1) where server processing time ≈ (T2 - T1)
		RTT = (ClientReceiveTime - ClientSendTime);

		// Clock offset = ((T1 - T0) + (T2 - T3)) / 2
		ClockOffset = ((ServerReceiveTime - ClientSendTime) + (ServerSendTime - ClientReceiveTime)) / 2.0;
	}
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SIMPLEGAMEPLAYABILITYSYSTEM_API USimpleTimeSynchronizerExtended : public USimpleTimeSynchronizer
{
	GENERATED_BODY()

public:
	USimpleTimeSynchronizerExtended();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual double GetServerTime_Implementation() override;

	/* Configuration Properties */

	// Number of samples to keep in the rolling buffer
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Sync|Config", meta = (ClampMin = "4", ClampMax = "128"))
	int32 SampleBufferSize = 32;

	// Base sync interval in seconds when connection is stable
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Sync|Config", meta = (ClampMin = "0.1", ClampMax = "10.0"))
	float BaseSyncInterval = 2.0f;

	// Minimum sync interval when connection is unstable
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Sync|Config", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float MinSyncInterval = 0.5f;

	// Maximum sync interval when connection is very stable
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Sync|Config", meta = (ClampMin = "1.0", ClampMax = "30.0"))
	float MaxSyncInterval = 5.0f;

	// Exponential smoothing factor (higher = more responsive, lower = more stable)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Sync|Config", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float SmoothingAlpha = 0.15f;

	// Multiplier for RTT outlier rejection threshold (samples with RTT > median * this value are rejected)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Sync|Config", meta = (ClampMin = "1.5", ClampMax = "10.0"))
	float OutlierRTTMultiplier = 2.5f;

	// Number of burst sync samples to take immediately after joining
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Sync|Config", meta = (ClampMin = "3", ClampMax = "50"))
	int32 InitialBurstSamples = 10;

	// Interval between burst samples in seconds
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Sync|Config", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float BurstSampleInterval = 0.2f;

	/* State Properties */

	// Rolling buffer of recent sync samples
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Time Sync|State")
	TArray<FSyncSample> SyncSamples;

	// Current estimated clock offset (client time = server time + offset)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Time Sync|State")
	double CurrentClockOffset = 0.0;

	// Current average round-trip time in seconds
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Time Sync|State")
	double CurrentRTT = 0.0;

	// Current RTT variance
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Time Sync|State")
	double RTTVariance = 0.0;

	// Time since last sync request
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Time Sync|State")
	float TimeSinceLastSync = 0.0f;

	// Current adaptive sync interval
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Time Sync|State")
	float CurrentSyncInterval = 2.0f;

	// Number of samples taken so far (used for initial burst)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Time Sync|State")
	int32 TotalSamplesTaken = 0;
	

	/**
	 * Returns the current clock offset estimate (client time = server time + offset)
	 */
	UFUNCTION(BlueprintPure, Category = "Time Sync")
	double GetClockOffset() const { return CurrentClockOffset; }

	/**
	 * Returns the current average round-trip time in seconds
	 */
	UFUNCTION(BlueprintPure, Category = "Time Sync")
	double GetRTT() const { return CurrentRTT; }

	/**
	 * Returns a sync quality metric from 0 (poor) to 1 (excellent)
	 */
	UFUNCTION(BlueprintPure, Category = "Time Sync")
	float GetSyncQuality() const;

	/**
	 * Manually triggers a time sync request (normally done automatically)
	 */
	UFUNCTION(BlueprintCallable, Category = "Time Sync")
	void RequestTimeSync();

	/* RPCs */

	/**
	 * Client requests a time sync from the server
	 * @param ClientSendTime Client's local time when sending this request
	 */
	UFUNCTION(Server, Unreliable)
	void ServerRequestTimeSync(double ClientSendTime);

	/**
	 * Server responds to client's time sync request
	 * @param ClientSendTime Original client send time (echo back)
	 * @param ServerReceiveTime Server time when it received the request
	 * @param ServerSendTime Server time when sending this response
	 */
	UFUNCTION(Client, Unreliable)
	void ClientReceiveTimeSync(double ClientSendTime, double ServerReceiveTime, double ServerSendTime);

private:
	// Process a new sync sample and update offset estimate
	void ProcessSyncSample(const FSyncSample& Sample);

	// Calculate median offset from valid samples
	double CalculateMedianOffset() const;

	// Calculate median RTT from valid samples
	double CalculateMedianRTT() const;

	// Update RTT statistics (average and variance)
	void UpdateRTTStatistics();

	// Reject outlier samples based on RTT
	void RejectOutliers();

	// Update the adaptive sync interval based on connection stability
	void UpdateAdaptiveSyncInterval();

	// Get current local time (for client timestamps)
	double GetLocalTime() const;
};
