#include "DroneSignalComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "SignalBoundaryVolume.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"


UDroneSignalComponent::UDroneSignalComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}
void UDroneSignalComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!SignalBoundaryVolume && GetWorld())
	{
		SignalBoundaryVolume = Cast<ASignalBoundaryVolume>(
			UGameplayStatics::GetActorOfClass(GetWorld(), ASignalBoundaryVolume::StaticClass())
		);
	}
}

void UDroneSignalComponent::ResetSignalState()
{
	SmoothedControlRSSI = 100.f;
	SmoothedControlLQ = 100.f;
	SmoothedVideoLink = 100.f;

	ReceivedThrottle = 0.f;
	ReceivedPitchInput = 0.f;
	ReceivedRollInput = 0.f;
	ReceivedYawInput = 0.f;

	ControlInputScale = 1.f;
	bControlFailsafeActive = false;

	SmoothedControlRSSIDbm = -50.f;
	SmoothedVideoRSSIDbm = -50.f;

	ControlFailsafeTimer = 0.f;
	ControlFailsafeActiveTime = 0.f;
	EffectiveControlLQ = 100.f;

	LastValidReceivedThrottle = 0.f;
	LastValidReceivedPitchInput = 0.f;
	LastValidReceivedRollInput = 0.f;
	LastValidReceivedYawInput = 0.f;

	SignalFadeTimer = 0.f;
	TargetControlFadeLossDb = 0.f;
	TargetVideoFadeLossDb = 0.f;
	SmoothedControlFadeLossDb = 0.f;
	SmoothedVideoFadeLossDb = 0.f;

	ControlPacketAccumulator = 0.f;
	ControlPacketAgeSeconds = 0.f;
	bLastControlPacketReceived = true;
}

void UDroneSignalComponent::UpdateReceivedControlInput(
	float DeltaTime,
	float RawThrottle,
	float RawPitchInput,
	float RawRollInput,
	float RawYawInput,
	FDroneTelemetry& Telemetry
)
{
	const float SafeDeltaTime = FMath::Max(DeltaTime, 0.f);

	float LQ = Telemetry.bControlLinkValid ? Telemetry.ControlLQPercent : 100.f;

	if (ControlPacketDebugLQOverride >= 0.f)
	{
		LQ = FMath::Clamp(ControlPacketDebugLQOverride, 0.f, 100.f);
	}

	EffectiveControlLQ = LQ;

	if (LQ >= ControlDegradedLQ)
	{
		ControlInputScale = 1.f;
	}
	else if (LQ <= ControlCriticalLQ)
	{
		ControlInputScale = FMath::GetMappedRangeValueClamped(
			FVector2D(ControlFailsafeLQ, ControlCriticalLQ),
			FVector2D(ControlMinimumInputScale, ControlCriticalInputScale),
			LQ
		);
	}
	else
	{
		ControlInputScale = FMath::GetMappedRangeValueClamped(
			FVector2D(ControlCriticalLQ, ControlDegradedLQ),
			FVector2D(ControlCriticalInputScale, 1.f),
			LQ
		);
	}

	bool bPacketReceivedThisFrame = true;

	if (bSimulateControlPackets)
	{
		bPacketReceivedThisFrame = false;

		const float PacketInterval = 1.f / FMath::Max(ControlPacketRateHz, 1.f);
		ControlPacketAccumulator += SafeDeltaTime;
		ControlPacketAgeSeconds += SafeDeltaTime;

		while (ControlPacketAccumulator >= PacketInterval)
		{
			ControlPacketAccumulator -= PacketInterval;

			const float PacketSuccessChance = FMath::Clamp(LQ / 100.f, 0.f, 1.f);

			if (FMath::FRand() <= PacketSuccessChance)
			{
				bPacketReceivedThisFrame = true;
				ControlPacketAgeSeconds = 0.f;
			}
		}
	}
	else
	{
		ControlPacketAgeSeconds = 0.f;
	}

	bLastControlPacketReceived = bPacketReceivedThisFrame;

	if (!bControlFailsafeActive)
	{
		if (ControlPacketAgeSeconds >= ControlPacketFailsafeTimeout)
		{
			bControlFailsafeActive = true;
			ControlFailsafeActiveTime = 0.f;
		}
	}
	else
	{
		ControlFailsafeActiveTime += SafeDeltaTime;

		if (bPacketReceivedThisFrame)
		{
			bControlFailsafeActive = false;
			ControlFailsafeTimer = 0.f;
			ControlFailsafeActiveTime = 0.f;
			ControlPacketAgeSeconds = 0.f;
		}
	}

	if (bControlFailsafeActive)
	{
		ControlInputScale = 0.f;

		if (ControlFailsafeActiveTime < ControlFailsafeHoldSeconds)
		{
			ReceivedThrottle = LastValidReceivedThrottle;
			ReceivedPitchInput = LastValidReceivedPitchInput;
			ReceivedRollInput = LastValidReceivedRollInput;
			ReceivedYawInput = LastValidReceivedYawInput;
		}
		else
		{
			ReceivedThrottle = 0.f;
			ReceivedPitchInput = 0.f;
			ReceivedRollInput = 0.f;
			ReceivedYawInput = 0.f;
		}

		Telemetry.ControlInputScale = ControlInputScale;
		Telemetry.bControlFailsafeActive = bControlFailsafeActive;
		return;
	}

	if (bPacketReceivedThisFrame)
	{
		ReceivedThrottle = RawThrottle;
		ReceivedPitchInput = RawPitchInput * ControlInputScale;
		ReceivedRollInput = RawRollInput * ControlInputScale;
		ReceivedYawInput = RawYawInput * ControlInputScale;

		LastValidReceivedThrottle = ReceivedThrottle;
		LastValidReceivedPitchInput = ReceivedPitchInput;
		LastValidReceivedRollInput = ReceivedRollInput;
		LastValidReceivedYawInput = ReceivedYawInput;
	}

	Telemetry.ControlInputScale = ControlInputScale;
	Telemetry.bControlFailsafeActive = bControlFailsafeActive;
}

void UDroneSignalComponent::UpdateSignalTelemetry(
	float DeltaTime,
	UStaticMeshComponent* DroneMesh,
	float RawThrottle,
	float RawPitchInput,
	float RawRollInput,
	float RawYawInput,
	FDroneTelemetry& Telemetry,
	UMaterialInstanceDynamic* FPVPostProcessMID
)
{
	if (!DroneMesh)
	{
		return;
	}

	const float SafeDeltaTime = FMath::Max(DeltaTime, 0.016f);

	const float DistanceM = FMath::Max(
		FVector::Distance(DroneMesh->GetComponentLocation(), OperatorLocation) / 100.f,
		1.f
	);

	const float Obstruction = ComputeOperatorObstructionFactor(DroneMesh);

	SignalFadeTimer += SafeDeltaTime;

	if (SignalFadeTimer >= SignalFadeUpdateInterval)
	{
		SignalFadeTimer = 0.f;
		TargetControlFadeLossDb = FMath::FRandRange(0.f, ControlRandomFadeMaxDb);
		TargetVideoFadeLossDb = FMath::FRandRange(0.f, VideoRandomFadeMaxDb);
	}

	SmoothedControlFadeLossDb = FMath::FInterpTo(
		SmoothedControlFadeLossDb,
		TargetControlFadeLossDb,
		SafeDeltaTime,
		SignalFadeInterpSpeed
	);

	SmoothedVideoFadeLossDb = FMath::FInterpTo(
		SmoothedVideoFadeLossDb,
		TargetVideoFadeLossDb,
		SafeDeltaTime,
		SignalFadeInterpSpeed
	);

	const float ControlAntennaLossDb = ComputeAntennaOrientationLossDb(DroneMesh, ControlAntennaOrientationMaxLossDb);
	const float VideoAntennaLossDb = ComputeAntennaOrientationLossDb(DroneMesh, VideoAntennaOrientationMaxLossDb);

	const float ControlBodyShadowLossDb = ComputeBodyShadowLossDb(DroneMesh, ControlBodyShadowMaxLossDb);
	const float VideoBodyShadowLossDb = ComputeBodyShadowLossDb(DroneMesh, VideoBodyShadowMaxLossDb);

	const float GameplayControlLossDb = ComputeGameplaySignalLossDb(DroneMesh, false);
	const float GameplayVideoLossDb = ComputeGameplaySignalLossDb(DroneMesh, true);

	const float ControlExtraLossDb =
		Obstruction * ControlObstructionLossDb +
		ControlAntennaLossDb +
		ControlBodyShadowLossDb +
		SmoothedControlFadeLossDb +
		GameplayControlLossDb;

	const float VideoDistanceExtraLossAlpha = FMath::GetMappedRangeValueClamped(
		FVector2D(VideoDistanceExtraLossStartM, VideoDistanceExtraLossFullM),
		FVector2D(0.f, 1.f),
		DistanceM
	);

	const float VideoDistanceExtraLossDb = VideoDistanceExtraLossAlpha * VideoDistanceExtraMaxLossDb;

	const float VideoObstructionDistanceFactor = FMath::GetMappedRangeValueClamped(
		FVector2D(100.f, 1200.f),
		FVector2D(0.35f, 1.f),
		DistanceM
	);

	const float VideoExtraLossDb =
		Obstruction * VideoObstructionLossDb * VideoObstructionDistanceFactor +
		VideoAntennaLossDb +
		VideoBodyShadowLossDb +
		SmoothedVideoFadeLossDb +
		VideoDistanceExtraLossDb;


	const float RawControlRSSIDbm = ComputeReceivedPowerDbm(
		ControlTxPowerW,
		ControlTxAntennaGainDbi,
		ControlRxAntennaGainDbi,
		ControlFrequencyMHz,
		DistanceM,
		ControlExtraLossDb
	);

	const float RawVideoRSSIDbm = ComputeReceivedPowerDbm(
		VideoTxPowerW,
		VideoAntennaGainDbi,
		VideoRxAntennaGainDbi,
		VideoFrequencyMHz,
		DistanceM,
		VideoExtraLossDb
	);

	const float ControlRSSIDbm = FMath::Min(RawControlRSSIDbm, ControlMaxDisplayedRSSIDbm);
	const float VideoRSSIDbm = FMath::Min(RawVideoRSSIDbm, VideoMaxDisplayedRSSIDbm);

	const float ControlMarginDb = ControlRSSIDbm - ControlReceiverSensitivityDbm;
	const float VideoMarginDb = VideoRSSIDbm - VideoReceiverSensitivityDbm;

	const float ControlRSSIBarPercent = ComputeSignalBarPercent(ControlRSSIDbm, ControlReceiverSensitivityDbm);
	const float ControlLQPercent = ComputeControlLQFromMargin(ControlMarginDb);
	const float VideoQualityPercent = ComputeVideoQualityFromMargin(VideoMarginDb);

	SmoothedControlRSSIDbm = FMath::FInterpTo(
		SmoothedControlRSSIDbm,
		ControlRSSIDbm,
		SafeDeltaTime,
		SignalSmoothingSpeed
	);

	SmoothedVideoRSSIDbm = FMath::FInterpTo(
		SmoothedVideoRSSIDbm,
		VideoRSSIDbm,
		SafeDeltaTime,
		SignalSmoothingSpeed
	);

	SmoothedControlRSSI = FMath::FInterpTo(
		SmoothedControlRSSI,
		ControlRSSIBarPercent,
		SafeDeltaTime,
		SignalSmoothingSpeed
	);

	SmoothedControlLQ = FMath::FInterpTo(
		SmoothedControlLQ,
		ControlLQPercent,
		SafeDeltaTime,
		SignalSmoothingSpeed
	);

	SmoothedVideoLink = FMath::FInterpTo(
		SmoothedVideoLink,
		VideoQualityPercent,
		SafeDeltaTime,
		SignalSmoothingSpeed
	);

	Telemetry.ControlRSSIDbm = SmoothedControlRSSIDbm;
	Telemetry.ControlRSSIBarPercent = SmoothedControlRSSI;
	Telemetry.ControlRSSIPercent = SmoothedControlRSSI;
	Telemetry.ControlLQPercent = SmoothedControlLQ;
	Telemetry.ControlSignalMarginDb = SmoothedControlRSSIDbm - ControlReceiverSensitivityDbm;
	Telemetry.bControlLinkValid = true;

	Telemetry.PrimaryLinkPercent = SmoothedControlLQ;
	Telemetry.bPrimaryLinkValid = true;

	Telemetry.VideoRSSIDbm = SmoothedVideoRSSIDbm;
	Telemetry.VideoSignalMarginDb = SmoothedVideoRSSIDbm - VideoReceiverSensitivityDbm;
	Telemetry.VideoLinkPercent = SmoothedVideoLink;
	Telemetry.bVideoLinkValid = true;

	Telemetry.TxPowerW = VideoTxPowerW;
	Telemetry.bTxPowerValid = true;

	UpdateReceivedControlInput(
		SafeDeltaTime,
		RawThrottle,
		RawPitchInput,
		RawRollInput,
		RawYawInput,
		Telemetry
	);

	if (FPVPostProcessMID)
	{
		const float VideoQuality01 = Telemetry.VideoLinkPercent / 100.f;
		FPVPostProcessMID->SetScalarParameterValue(TEXT("VideoQuality"), VideoQuality01);
	}

	static float SignalLogTimer = 0.f;
	SignalLogTimer += SafeDeltaTime;

	if (bLogSignalDebug && SignalLogTimer >= 0.5f)
	{
		SignalLogTimer = 0.f;

		UE_LOG(LogTemp, Warning,
			TEXT("SIGNAL | Dist=%.1fm Obs=%.2f LossC=%.1f LossV=%.1f | CTRL RSSI=%.1fdBm Bar=%.0f LQ=%.0f EffLQ=%.0f Override=%.1f Margin=%.1fdB | VIDEO RSSI=%.1fdBm Q=%.0f Margin=%.1fdB | VTX=%.1fW | Packet=%d Age=%.3f | Failsafe=%d"),
			DistanceM,
			Obstruction,
			ControlExtraLossDb,
			VideoExtraLossDb,
			Telemetry.ControlRSSIDbm,
			Telemetry.ControlRSSIPercent,
			Telemetry.ControlLQPercent,
			EffectiveControlLQ,
			ControlPacketDebugLQOverride,
			Telemetry.ControlSignalMarginDb,
			Telemetry.VideoRSSIDbm,
			Telemetry.VideoLinkPercent,
			Telemetry.VideoSignalMarginDb,
			Telemetry.TxPowerW,
			bLastControlPacketReceived ? 1 : 0,
			ControlPacketAgeSeconds,
			bControlFailsafeActive ? 1 : 0
		);
	}
}

void UDroneSignalComponent::SetOperatorLocation(const FVector& NewOperatorLocation)
{
	OperatorLocation = NewOperatorLocation;
}

float UDroneSignalComponent::WattsToDbm(float Watts) const
{
	const float Milliwatts = FMath::Max(Watts * 1000.f, 0.001f);
	return 10.f * FMath::LogX(10.f, Milliwatts);
}

float UDroneSignalComponent::ComputeFreeSpacePathLossDb(float DistanceM, float FrequencyMHz) const
{
	const float DistanceKm = FMath::Max(DistanceM / 1000.f, 0.001f);
	const float Frequency = FMath::Max(FrequencyMHz, 1.f);

	return 32.44f
		+ 20.f * FMath::LogX(10.f, DistanceKm)
		+ 20.f * FMath::LogX(10.f, Frequency);
}

float UDroneSignalComponent::ComputeReceivedPowerDbm(
	float TxPowerW,
	float TxGainDbi,
	float RxGainDbi,
	float FrequencyMHz,
	float DistanceM,
	float ExtraLossDb
) const
{
	const float TxPowerDbm = WattsToDbm(TxPowerW);
	const float PathLossDb = ComputeFreeSpacePathLossDb(DistanceM, FrequencyMHz);

	return TxPowerDbm + TxGainDbi + RxGainDbi - PathLossDb - ExtraLossDb;
}

float UDroneSignalComponent::ComputeSignalBarPercent(float RSSIDbm, float SensitivityDbm) const
{
	return FMath::GetMappedRangeValueClamped(
		FVector2D(SensitivityDbm - 5.f, SensitivityDbm + 40.f),
		FVector2D(0.f, 100.f),
		RSSIDbm
	);
}

float UDroneSignalComponent::ComputeControlLQFromMargin(float MarginDb) const
{
	if (MarginDb >= 18.f)
	{
		return 100.f;
	}

	if (MarginDb >= 6.f)
	{
		return FMath::GetMappedRangeValueClamped(
			FVector2D(6.f, 18.f),
			FVector2D(75.f, 100.f),
			MarginDb
		);
	}

	if (MarginDb >= 0.f)
	{
		return FMath::GetMappedRangeValueClamped(
			FVector2D(0.f, 6.f),
			FVector2D(25.f, 75.f),
			MarginDb
		);
	}

	return FMath::GetMappedRangeValueClamped(
		FVector2D(-8.f, 0.f),
		FVector2D(0.f, 25.f),
		MarginDb
	);
}

float UDroneSignalComponent::ComputeVideoQualityFromMargin(float MarginDb) const
{
	if (MarginDb >= 25.f)
	{
		return 100.f;
	}

	if (MarginDb >= 10.f)
	{
		return FMath::GetMappedRangeValueClamped(
			FVector2D(10.f, 25.f),
			FVector2D(65.f, 100.f),
			MarginDb
		);
	}

	if (MarginDb >= 0.f)
	{
		return FMath::GetMappedRangeValueClamped(
			FVector2D(0.f, 10.f),
			FVector2D(25.f, 65.f),
			MarginDb
		);
	}

	return FMath::GetMappedRangeValueClamped(
		FVector2D(-8.f, 0.f),
		FVector2D(0.f, 25.f),
		MarginDb
	);
}

float UDroneSignalComponent::ComputeAntennaOrientationLossDb(UStaticMeshComponent* DroneMesh, float MaxLossDb) const
{
	if (!DroneMesh)
	{
		return 0.f;
	}

	const FVector DroneLocation = DroneMesh->GetComponentLocation();
	const FVector OperatorEyeLocation = OperatorLocation + FVector(0.f, 0.f, 170.f);
	const FVector DirToOperator = (OperatorEyeLocation - DroneLocation).GetSafeNormal();

	if (DirToOperator.IsNearlyZero())
	{
		return 0.f;
	}

	const FVector AntennaAxis = DroneMesh->GetComponentTransform().GetUnitAxis(EAxis::Z);
	const float AxialFactor = FMath::Abs(FVector::DotProduct(AntennaAxis, DirToOperator));

	return MaxLossDb * FMath::Pow(AxialFactor, 2.f);
}

float UDroneSignalComponent::ComputeBodyShadowLossDb(UStaticMeshComponent* DroneMesh, float MaxLossDb) const
{
	if (!DroneMesh)
	{
		return 0.f;
	}

	const FVector DroneLocation = DroneMesh->GetComponentLocation();
	const FVector OperatorEyeLocation = OperatorLocation + FVector(0.f, 0.f, 170.f);
	const FVector WorldDirToOperator = (OperatorEyeLocation - DroneLocation).GetSafeNormal();

	if (WorldDirToOperator.IsNearlyZero())
	{
		return 0.f;
	}

	const FTransform MeshTransform = DroneMesh->GetComponentTransform();
	const FVector LocalDir = MeshTransform.InverseTransformVectorNoScale(WorldDirToOperator).GetSafeNormal();

	const float UnderBodyFactor = FMath::Clamp((-LocalDir.Z - 0.2f) / 0.8f, 0.f, 1.f);
	const float RearBodyFactor = FMath::Clamp((-LocalDir.X - 0.35f) / 0.65f, 0.f, 1.f) * 0.35f;
	const float ShadowFactor = FMath::Max(UnderBodyFactor, RearBodyFactor);

	return MaxLossDb * ShadowFactor;
}

float UDroneSignalComponent::ComputeOperatorObstructionFactor(UStaticMeshComponent* DroneMesh) const
{
	if (!GetWorld() || !DroneMesh)
	{
		return 1.f;
	}

	const FVector DroneLocation = DroneMesh->GetComponentLocation();
	const FVector OperatorEyeLocation = OperatorLocation + FVector(0.f, 0.f, 170.f);

	const FVector Direction = (OperatorEyeLocation - DroneLocation).GetSafeNormal();
	const FVector Right = FVector::CrossProduct(Direction, FVector::UpVector).GetSafeNormal();
	const FVector Up = FVector::CrossProduct(Right, Direction).GetSafeNormal();

	const float TraceSpreadCm = 120.f;

	TArray<FVector> Offsets;
	Offsets.Add(FVector::ZeroVector);
	Offsets.Add(Right * TraceSpreadCm);
	Offsets.Add(-Right * TraceSpreadCm);
	Offsets.Add(Up * TraceSpreadCm);
	Offsets.Add(-Up * TraceSpreadCm);

	FCollisionQueryParams Params;

	if (AActor* Owner = GetOwner())
	{
		Params.AddIgnoredActor(Owner);
	}

	int32 BlockedCount = 0;

	for (const FVector& Offset : Offsets)
	{
		FHitResult Hit;

		const FVector Start = DroneLocation + Offset;
		const FVector End = OperatorEyeLocation + Offset;

		const bool bHit = GetWorld()->LineTraceSingleByChannel(
			Hit,
			Start,
			End,
			ECC_Visibility,
			Params
		);

		if (bHit)
		{
			BlockedCount++;
		}
	}

	const float RawObstruction = static_cast<float>(BlockedCount) / static_cast<float>(Offsets.Num());

	return FMath::Clamp(RawObstruction, 0.f, 1.f);
}

float UDroneSignalComponent::ComputeBoundarySignedDistanceCm(UStaticMeshComponent* DroneMesh) const
{
	if (!bUseBoundarySignalPenalty || !DroneMesh || !SignalBoundaryVolume)
	{
		return BIG_NUMBER;
	}

	UBoxComponent* Box = SignalBoundaryVolume->GetBoundaryBox();
	if (!Box)
	{
		return BIG_NUMBER;
	}

	const FVector DroneWorldLocation = DroneMesh->GetComponentLocation();
	const FTransform BoxTransform = Box->GetComponentTransform();

	const FVector LocalLocation = BoxTransform.InverseTransformPositionNoScale(DroneWorldLocation);
	const FVector Extent = Box->GetScaledBoxExtent();

	const FVector AbsLocal(
		FMath::Abs(LocalLocation.X),
		FMath::Abs(LocalLocation.Y),
		FMath::Abs(LocalLocation.Z)
	);

	const FVector DistanceInside = Extent - AbsLocal;

	const float MinInsideDistance = FMath::Min(
		FMath::Min(DistanceInside.X, DistanceInside.Y),
		DistanceInside.Z
	);

	if (MinInsideDistance >= 0.f)
	{
		return MinInsideDistance;
	}

	const FVector OutsideDistance(
		FMath::Max(AbsLocal.X - Extent.X, 0.f),
		FMath::Max(AbsLocal.Y - Extent.Y, 0.f),
		FMath::Max(AbsLocal.Z - Extent.Z, 0.f)
	);

	return -OutsideDistance.Size();
}

float UDroneSignalComponent::ComputeGameplaySignalLossDb(UStaticMeshComponent* DroneMesh, bool bVideo) const
{
	const float SignedDistanceCm = ComputeBoundarySignedDistanceCm(DroneMesh);

	const float WarningLossDb = bVideo ? BoundaryVideoWarningLossDb : BoundaryControlWarningLossDb;
	const float EdgeLossDb = bVideo ? BoundaryVideoEdgeLossDb : BoundaryControlEdgeLossDb;
	const float MaxLossDb = bVideo ? BoundaryVideoMaxLossDb : BoundaryControlMaxLossDb;

	if (SignedDistanceCm > BoundaryWarningDistanceCm)
	{
		return 0.f;
	}

	if (SignedDistanceCm >= 0.f)
	{
		const float WarningAlpha = 1.f - FMath::Clamp(
			SignedDistanceCm / FMath::Max(BoundaryWarningDistanceCm, 1.f),
			0.f,
			1.f
		);

		return FMath::Lerp(0.f, WarningLossDb, WarningAlpha);
	}

	const float OutsideAlpha = FMath::Clamp(
		FMath::Abs(SignedDistanceCm) / FMath::Max(OutsideFullPenaltyDistanceCm, 1.f),
		0.f,
		1.f
	);

	return FMath::Lerp(EdgeLossDb, MaxLossDb, OutsideAlpha);
}