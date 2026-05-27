#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DroneTelemetry.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "DroneSignalComponent.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class ASignalBoundaryVolume;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DIPLOMA_API UDroneSignalComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDroneSignalComponent();

	void ResetSignalState();

	void SetOperatorLocation(const FVector& NewOperatorLocation);
	const FVector& GetOperatorLocation() const { return OperatorLocation; }
	void UpdateReceivedControlInput(
		float DeltaTime,
		float RawThrottle,
		float RawPitchInput,
		float RawRollInput,
		float RawYawInput,
		FDroneTelemetry& Telemetry
	);

	void UpdateSignalTelemetry(
		float DeltaTime,
		UStaticMeshComponent* DroneMesh,
		float RawThrottle,
		float RawPitchInput,
		float RawRollInput,
		float RawYawInput,
		FDroneTelemetry& Telemetry,
		UMaterialInstanceDynamic* FPVPostProcessMID
	);

	float GetReceivedThrottle() const { return ReceivedThrottle; }
	float GetReceivedPitchInput() const { return ReceivedPitchInput; }
	float GetReceivedRollInput() const { return ReceivedRollInput; }
	float GetReceivedYawInput() const { return ReceivedYawInput; }

	float GetControlInputScale() const { return ControlInputScale; }
	bool IsControlFailsafeActive() const { return bControlFailsafeActive; }

	float GetEffectiveControlLQ() const { return EffectiveControlLQ; }
	bool WasLastControlPacketReceived() const { return bLastControlPacketReceived; }
	float GetControlPacketAgeSeconds() const { return ControlPacketAgeSeconds; }

protected:
	virtual void BeginPlay() override;

	

private:

	UPROPERTY(EditAnywhere, Category = "UAV|Signal|Boundary")
	ASignalBoundaryVolume* SignalBoundaryVolume = nullptr;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal|Boundary")
	bool bUseBoundarySignalPenalty = true;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal|Boundary")
	float BoundaryWarningDistanceCm = 50000.f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal|Boundary")
	float OutsideFullPenaltyDistanceCm = 50000.f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal|Boundary")
	float BoundaryControlWarningLossDb = 8.f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal|Boundary")
	float BoundaryVideoWarningLossDb = 15.f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal|Boundary")
	float BoundaryControlEdgeLossDb = 25.f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal|Boundary")
	float BoundaryVideoEdgeLossDb = 40.f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal|Boundary")
	float BoundaryControlMaxLossDb = 95.f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal|Boundary")
	float BoundaryVideoMaxLossDb = 130.f;

	float ComputeBoundarySignedDistanceCm(UStaticMeshComponent* DroneMesh) const;
	float ComputeGameplaySignalLossDb(UStaticMeshComponent* DroneMesh, bool bVideo) const;



	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	FVector OperatorLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	float ControlFrequencyMHz = 915.f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	float VideoFrequencyMHz = 5800.f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	float VideoTxPowerW = 3.f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	float VideoAntennaGainDbi = 3.5f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	float SignalSmoothingSpeed = 3.f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	float ControlDegradedLQ = 60.f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	float ControlCriticalLQ = 25.f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	float ControlFailsafeLQ = 10.f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	float ControlCriticalInputScale = 0.35f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	float ControlMinimumInputScale = 0.15f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	float ControlFailsafeHoldSeconds = 0.5f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	float ControlTxPowerW = 1.0f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	float ControlTxAntennaGainDbi = 2.0f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	float ControlRxAntennaGainDbi = 2.0f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	float ControlReceiverSensitivityDbm = -112.f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	float VideoRxAntennaGainDbi = 8.0f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	float VideoReceiverSensitivityDbm = -90.f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	float ControlObstructionLossDb = 22.f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	float VideoObstructionLossDb = 12.f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	float VideoDistanceExtraLossStartM = 1000.f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	float VideoDistanceExtraLossFullM = 4000.f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	float VideoDistanceExtraMaxLossDb = 35.f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	float ControlMaxDisplayedRSSIDbm = -35.f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	float VideoMaxDisplayedRSSIDbm = -35.f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	bool bSimulateControlPackets = true;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	float ControlPacketRateHz = 50.f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	float ControlPacketDebugLQOverride = -1.f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	float ControlPacketAgeSeconds = 0.f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	bool bLastControlPacketReceived = true;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	float ControlPacketFailsafeTimeout = 0.6f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	bool bLogSignalDebug = false;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	float ControlAntennaOrientationMaxLossDb = 4.f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	float VideoAntennaOrientationMaxLossDb = 4.f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	float ControlBodyShadowMaxLossDb = 5.f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	float VideoBodyShadowMaxLossDb = 8.f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	float ControlRandomFadeMaxDb = 2.f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	float VideoRandomFadeMaxDb = 4.f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	float SignalFadeUpdateInterval = 0.25f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	float SignalFadeInterpSpeed = 4.f;

	float SmoothedControlRSSI = 100.f;
	float SmoothedControlLQ = 100.f;
	float SmoothedVideoLink = 100.f;

	float ReceivedThrottle = 0.f;
	float ReceivedPitchInput = 0.f;
	float ReceivedRollInput = 0.f;
	float ReceivedYawInput = 0.f;

	float ControlInputScale = 1.f;
	bool bControlFailsafeActive = false;

	float SmoothedControlRSSIDbm = -50.f;
	float SmoothedVideoRSSIDbm = -50.f;

	float ControlFailsafeTimer = 0.f;
	float ControlFailsafeActiveTime = 0.f;
	float EffectiveControlLQ = 100.f;

	float LastValidReceivedThrottle = 0.f;
	float LastValidReceivedPitchInput = 0.f;
	float LastValidReceivedRollInput = 0.f;
	float LastValidReceivedYawInput = 0.f;

	float SignalFadeTimer = 0.f;
	float TargetControlFadeLossDb = 0.f;
	float TargetVideoFadeLossDb = 0.f;
	float SmoothedControlFadeLossDb = 0.f;
	float SmoothedVideoFadeLossDb = 0.f;

	float ControlPacketAccumulator = 0.f;


	float WattsToDbm(float Watts) const;
	float ComputeFreeSpacePathLossDb(float DistanceM, float FrequencyMHz) const;
	float ComputeReceivedPowerDbm(float TxPowerW, float TxGainDbi, float RxGainDbi, float FrequencyMHz, float DistanceM, float ExtraLossDb) const;
	float ComputeSignalBarPercent(float RSSIDbm, float SensitivityDbm) const;
	float ComputeControlLQFromMargin(float MarginDb) const;
	float ComputeVideoQualityFromMargin(float MarginDb) const;

	float ComputeOperatorObstructionFactor(UStaticMeshComponent* DroneMesh) const;
	float ComputeAntennaOrientationLossDb(UStaticMeshComponent* DroneMesh, float MaxLossDb) const;
	float ComputeBodyShadowLossDb(UStaticMeshComponent* DroneMesh, float MaxLossDb) const;
};