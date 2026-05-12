#include "FPVFlightControllerComponent.h"
#include "Components/StaticMeshComponent.h"

UFPVFlightControllerComponent::UFPVFlightControllerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	PitchPID.P = 0.25f;
	PitchPID.I = 0.f;
	PitchPID.D = 0.01f;
	PitchPID.IntegralClamp = 0.3f;

	RollPID.P = 0.25f;
	RollPID.I = 0.f;
	RollPID.D = 0.01f;
	RollPID.IntegralClamp = 0.3f;

	YawPID.P = 1.f;
	YawPID.I = 0.f;
	YawPID.D = 0.075f;
	YawPID.IntegralClamp = 0.3f;
}

void UFPVFlightControllerComponent::ResetController()
{
	PitchPID.Reset();
	RollPID.Reset();
	YawPID.Reset();

	FlightModeDebugTimer = 0.f;
	DebugLogTimer = 0.f;
}

void UFPVFlightControllerComponent::CycleFlightMode()
{
	const int32 Current = static_cast<int32>(FlightMode);
	const int32 Next = (Current + 1) % 4;

	FlightMode = static_cast<EFPVFlightMode>(Next);

	ResetController();

	UE_LOG(LogTemp, Warning, TEXT("FlightMode: %s"), *GetFlightModeText());
}

void UFPVFlightControllerComponent::SetFlightMode(EFPVFlightMode NewMode)
{
	if (FlightMode == NewMode)
	{
		return;
	}

	FlightMode = NewMode;

	ResetController();

	UE_LOG(LogTemp, Warning, TEXT("FlightMode: %s"), *GetFlightModeText());
}

FString UFPVFlightControllerComponent::GetFlightModeText() const
{
	switch (FlightMode)
	{
	case EFPVFlightMode::Acro:
		return TEXT("ACRO");

	case EFPVFlightMode::Angle:
		return TEXT("ANGLE");

	case EFPVFlightMode::Horizon:
		return TEXT("HORIZON");

	case EFPVFlightMode::AcroTrainer:
		return TEXT("ACRO TRAINER");

	default:
		return TEXT("ACRO");
	}
}

float UFPVFlightControllerComponent::ComputeAngleRateNorm(float TargetAngleDeg, float CurrentAngleDeg, float MaxRateDeg) const
{
	const float ErrorDeg = CurrentAngleDeg - TargetAngleDeg;
	const float TargetRateDeg = FMath::Clamp(ErrorDeg * AngleLevelGain, -MaxRateDeg, MaxRateDeg);

	return TargetRateDeg / FMath::Max(MaxRateDeg, KINDA_SMALL_NUMBER);
}

float UFPVFlightControllerComponent::ApplyAcroTrainerLimit(float TargetRateNorm, float CurrentAngleDeg, float LimitDeg, float MaxRateDeg) const
{
	const float AbsAngle = FMath::Abs(CurrentAngleDeg);

	if (AbsAngle < LimitDeg)
	{
		return TargetRateNorm;
	}

	const float LimitAngleDeg = FMath::Sign(CurrentAngleDeg) * LimitDeg;
	const float ReturnRateDeg = FMath::Clamp((CurrentAngleDeg - LimitAngleDeg) * AcroTrainerReturnGain, -MaxRateDeg, MaxRateDeg);
	const float ReturnRateNorm = ReturnRateDeg / FMath::Max(MaxRateDeg, KINDA_SMALL_NUMBER);

	if (CurrentAngleDeg * TargetRateNorm < 0.f)
	{
		return ReturnRateNorm;
	}

	return TargetRateNorm;
}

void UFPVFlightControllerComponent::UpdateMotorCommands(
	float DeltaTime,
	UStaticMeshComponent* Mesh,
	float Throttle,
	float PitchInput,
	float RollInput,
	float YawInput,
	bool bArmed,
	TArray<FMotorState>& Motors,
	FFPVDebugState& DebugState
)
{
	if (!Mesh || Motors.Num() != 4)
	{
		return;
	}

	const float BaseThrottle = FMath::Clamp(Throttle, 0.f, 1.f);

	if (!bArmed)
	{
		ResetController();

		for (FMotorState& Motor : Motors)
		{
			Motor.Command = 0.f;
		}

		return;
	}

	const FTransform MeshTransform = Mesh->GetComponentTransform();
	const FVector WorldAngVelDeg = Mesh->GetPhysicsAngularVelocityInDegrees();
	const FVector LocalAngVelDeg = MeshTransform.InverseTransformVectorNoScale(WorldAngVelDeg);

	const float CurrentRollRateNorm = LocalAngVelDeg.X / MaxRollRate;
	const float CurrentPitchRateNorm = LocalAngVelDeg.Y / MaxPitchRate;
	const float CurrentYawRateNorm = LocalAngVelDeg.Z / MaxYawRate;

	const FRotator MeshRotation = Mesh->GetComponentRotation();
	const float CurrentPitchDeg = FRotator::NormalizeAxis(MeshRotation.Pitch);
	const float CurrentRollDeg = FRotator::NormalizeAxis(MeshRotation.Roll);

	const float AcroTargetRollRateNorm = -RollInput;
	const float AcroTargetPitchRateNorm = PitchInput;
	const float TargetYawRateNorm = YawInput;

	float TargetRollRateNorm = AcroTargetRollRateNorm;
	float TargetPitchRateNorm = AcroTargetPitchRateNorm;

	float TargetRollDeg = 0.f;
	float TargetPitchDeg = 0.f;
	float AngleRollRateNorm = 0.f;
	float AnglePitchRateNorm = 0.f;
	float HorizonAlpha = 0.f;
	bool bDidFlightModeDebugLog = false;

	if (FlightMode == EFPVFlightMode::Angle)
	{
		TargetRollDeg = RollInput * AngleMaxRollDeg;
		TargetPitchDeg = -PitchInput * AngleMaxPitchDeg;

		AngleRollRateNorm = ComputeAngleRateNorm(TargetRollDeg, CurrentRollDeg, MaxRollRate);
		AnglePitchRateNorm = ComputeAngleRateNorm(TargetPitchDeg, CurrentPitchDeg, MaxPitchRate);

		TargetRollRateNorm = AngleRollRateNorm;
		TargetPitchRateNorm = AnglePitchRateNorm;
	}
	else if (FlightMode == EFPVFlightMode::Horizon)
	{
		const float StickAmount = FMath::Max(FMath::Abs(PitchInput), FMath::Abs(RollInput));

		HorizonAlpha = FMath::GetMappedRangeValueClamped(
			FVector2D(HorizonTransitionStart, HorizonTransitionEnd),
			FVector2D(0.f, 1.f),
			StickAmount
		);

		TargetRollDeg = RollInput * AngleMaxRollDeg;
		TargetPitchDeg = -PitchInput * AngleMaxPitchDeg;

		const float AngleTargetRollRateNorm = ComputeAngleRateNorm(TargetRollDeg, CurrentRollDeg, MaxRollRate);
		const float AngleTargetPitchRateNorm = ComputeAngleRateNorm(TargetPitchDeg, CurrentPitchDeg, MaxPitchRate);

		TargetRollRateNorm = FMath::Lerp(AngleTargetRollRateNorm, AcroTargetRollRateNorm, HorizonAlpha);
		TargetPitchRateNorm = FMath::Lerp(AngleTargetPitchRateNorm, AcroTargetPitchRateNorm, HorizonAlpha);
	}
	else if (FlightMode == EFPVFlightMode::AcroTrainer)
	{
		TargetRollRateNorm = ApplyAcroTrainerLimit(AcroTargetRollRateNorm, CurrentRollDeg, AcroTrainerMaxRollDeg, MaxRollRate);
		TargetPitchRateNorm = ApplyAcroTrainerLimit(AcroTargetPitchRateNorm, CurrentPitchDeg, AcroTrainerMaxPitchDeg, MaxPitchRate);
	}

	const float RollCmd = FMath::Clamp(RollPID.Update(TargetRollRateNorm, CurrentRollRateNorm, DeltaTime), -0.05f, 0.05f);
	const float PitchCmd = FMath::Clamp(PitchPID.Update(TargetPitchRateNorm, CurrentPitchRateNorm, DeltaTime), -0.05f, 0.05f);
	const float YawCmd = FMath::Clamp(YawPID.Update(TargetYawRateNorm, CurrentYawRateNorm, DeltaTime), -0.1f, 0.1f);

	FlightModeDebugTimer += DeltaTime;

	if (bLogFlightModeDebug && FlightModeDebugTimer >= FlightModeDebugInterval)
	{
		FlightModeDebugTimer = 0.f;

		const float RollErrorDeg = TargetRollDeg - CurrentRollDeg;
		const float PitchErrorDeg = TargetPitchDeg - CurrentPitchDeg;
		bDidFlightModeDebugLog = true;

		/*UE_LOG(LogTemp, Warning,
			TEXT("MODE_DBG | Mode=%s Thr=%.2f | In P=%.2f R=%.2f Y=%.2f | Ang P=%.1f R=%.1f | TgtAng P=%.1f R=%.1f | Err P=%+.1f R=%+.1f"),
			*GetFlightModeText(),
			BaseThrottle,
			PitchInput,
			RollInput,
			YawInput,
			CurrentPitchDeg,
			CurrentRollDeg,
			TargetPitchDeg,
			TargetRollDeg,
			PitchErrorDeg,
			RollErrorDeg
		);

		UE_LOG(LogTemp, Warning,
			TEXT("RATE_DBG | CurRate P=%+.1f R=%+.1f Y=%+.1f | TgtRate P=%+.1f R=%+.1f Y=%+.1f | Cmd P=%+.4f R=%+.4f Y=%+.4f"),
			LocalAngVelDeg.Y,
			LocalAngVelDeg.X,
			LocalAngVelDeg.Z,
			TargetPitchRateNorm * MaxPitchRate,
			TargetRollRateNorm * MaxRollRate,
			TargetYawRateNorm * MaxYawRate,
			PitchCmd,
			RollCmd,
			YawCmd
		);*/
	}

	const float ArmX = Motors[0].LocalPosition.X != 0.f ? FMath::Abs(Motors[0].LocalPosition.X) : 16.67f;
	const float ArmY = Motors[0].LocalPosition.Y != 0.f ? FMath::Abs(Motors[0].LocalPosition.Y) : 14.5f;
	const float ArmMin = FMath::Min(ArmX, ArmY);

	const float PitchMix = PitchCmd * (ArmMin / FMath::Max(ArmX, 0.001f));
	const float RollMix = RollCmd * (ArmMin / FMath::Max(ArmY, 0.001f));

	const float FL = BaseThrottle - PitchMix - RollMix - YawCmd * Motors[0].SpinDirection;
	const float FR = BaseThrottle - PitchMix + RollMix - YawCmd * Motors[1].SpinDirection;
	const float BL = BaseThrottle + PitchMix - RollMix - YawCmd * Motors[2].SpinDirection;
	const float BR = BaseThrottle + PitchMix + RollMix - YawCmd * Motors[3].SpinDirection;

	if (bDidFlightModeDebugLog)
	{
		/*UE_LOG(LogTemp, Warning,
			TEXT("MIX_DBG | PitchMix=%+.4f RollMix=%+.4f YawCmd=%+.4f | FL=%.3f FR=%.3f BL=%.3f BR=%.3f"),
			PitchMix,
			RollMix,
			YawCmd,
			FL,
			FR,
			BL,
			BR
		);*/
	}

	Motors[0].Command = FMath::Clamp(FL, 0.f, 1.f);
	Motors[1].Command = FMath::Clamp(FR, 0.f, 1.f);
	Motors[2].Command = FMath::Clamp(BL, 0.f, 1.f);
	Motors[3].Command = FMath::Clamp(BR, 0.f, 1.f);

	DebugState.CurrentRollRate = LocalAngVelDeg.X;
	DebugState.CurrentPitchRate = LocalAngVelDeg.Y;
	DebugState.CurrentYawRate = LocalAngVelDeg.Z;

	DebugState.TargetRollRate = TargetRollRateNorm * MaxRollRate;
	DebugState.TargetPitchRate = TargetPitchRateNorm * MaxPitchRate;
	DebugState.TargetYawRate = TargetYawRateNorm * MaxYawRate;

	DebugState.RollCmd = RollCmd;
	DebugState.PitchCmd = PitchCmd;
	DebugState.YawCmd = YawCmd;

	DebugState.FL = FL;
	DebugState.FR = FR;
	DebugState.BL = BL;
	DebugState.BR = BR;

	DebugState.Throttle = BaseThrottle;
	DebugState.PitchInput = PitchInput;
	DebugState.RollInput = RollInput;
	DebugState.YawInput = YawInput;

	DebugLogTimer += DeltaTime;

	if (DebugLogTimer >= 0.15f)
	{
		DebugLogTimer = 0.f;

		/*
		UE_LOG(LogTemp, Warning,
			TEXT("CTRL | Thr=%.2f | In P=%.2f R=%.2f Y=%.2f | Rate P=%.1f/%.1f R=%.1f/%.1f Y=%.1f/%.1f | Cmd P=%.3f R=%.3f Y=%.3f | M FL=%.3f FR=%.3f BL=%.3f BR=%.3f"),
			DebugState.Throttle,
			DebugState.PitchInput,
			DebugState.RollInput,
			DebugState.YawInput,
			DebugState.CurrentPitchRate,
			DebugState.TargetPitchRate,
			DebugState.CurrentRollRate,
			DebugState.TargetRollRate,
			DebugState.CurrentYawRate,
			DebugState.TargetYawRate,
			DebugState.PitchCmd,
			DebugState.RollCmd,
			DebugState.YawCmd,
			DebugState.FL,
			DebugState.FR,
			DebugState.BL,
			DebugState.BR
		);*/
	}
}