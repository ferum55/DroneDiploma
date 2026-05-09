#include "FPVMotorComponent.h"

UFPVMotorComponent::UFPVMotorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFPVMotorComponent::InitMotors()
{
	Motors.SetNum(4);

	Motors[0].LocalPosition = FVector(ArmX, -ArmY, 0.f);
	Motors[0].SpinDirection = 1.f;

	Motors[1].LocalPosition = FVector(ArmX, ArmY, 0.f);
	Motors[1].SpinDirection = -1.f;

	Motors[2].LocalPosition = FVector(-ArmX, -ArmY, 0.f);
	Motors[2].SpinDirection = -1.f;

	Motors[3].LocalPosition = FVector(-ArmX, ArmY, 0.f);
	Motors[3].SpinDirection = 1.f;

	ResetMotors();
}

void UFPVMotorComponent::InitMotors(float InArmX, float InArmY)
{
	ArmX = InArmX;
	ArmY = InArmY;

	InitMotors();
}

void UFPVMotorComponent::ResetMotors()
{
	for (FMotorState& Motor : Motors)
	{
		Motor.Command = 0.f;
		Motor.CurrentCommand = 0.f;
		Motor.TargetRPM = 0.f;
		Motor.CurrentRPM = 0.f;
		Motor.ThrustNewton = 0.f;
		Motor.CurrentDrawAmp = 0.f;
		Motor.ElectricalPowerWatt = 0.f;
		Motor.MechanicalPowerWatt = 0.f;
		Motor.ReactionTorqueNm = 0.f;
	}
}

float UFPVMotorComponent::ComputePropEfficiencyFactor(const FVector& LocalVelocityMps) const
{
	const float VerticalSpeedMps = FMath::Abs(LocalVelocityMps.Z);
	const float ForwardSpeedMps = FMath::Max(LocalVelocityMps.X, 0.f);

	const float VerticalRatio = VerticalSpeedMps / FMath::Max(PropwashSpeedScaleMps, 0.1f);
	const float VerticalPenalty = 0.1f * FMath::Clamp(VerticalRatio * VerticalRatio, 0.f, 1.f);

	const float ForwardStartMps = 12.f;
	const float ForwardFullMps = 32.f;

	float ForwardAlpha = 0.f;
	if (ForwardFullMps > ForwardStartMps)
	{
		ForwardAlpha = FMath::Clamp(
			(ForwardSpeedMps - ForwardStartMps) / (ForwardFullMps - ForwardStartMps),
			0.f,
			1.f
		);
	}

	const float ForwardPenalty = 0.28f * ForwardAlpha * ForwardAlpha;

	const float Efficiency = 1.f - VerticalPenalty - ForwardPenalty;
	return FMath::Clamp(Efficiency, MinPropEfficiency, 1.f);
}

void UFPVMotorComponent::UpdateMotorDynamics(
	float DeltaTime,
	float BatteryLoadedVoltage,
	float BatteryOutputScale,
	float BatteryBenchReferenceVoltage,
	const FVector& LocalVelocityMps
)
{
	static const TArray<float> ThrottlePts = { 0.f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f };
	static const TArray<float> RpmFromThrottlePts = { 0.f, 9097.f, 10518.f, 11816.f, 12897.f, 13765.f, 14404.f };

	static const TArray<float> RpmPts = { 9097.f, 10518.f, 11816.f, 12897.f, 13765.f, 14404.f };
	static const TArray<float> PowerPts = { 312.7f, 497.7f, 734.3f, 1016.3f, 1316.8f, 1606.2f };
	static const TArray<float> ThrustPts = { 1491.f, 2039.f, 2609.f, 3207.f, 3681.f, 4014.f };

	const float LowerThrustExponent = 2.156574f;
	const float LowerPowerExponent = 3.202028f;

	auto InterpCurve = [](const TArray<float>& X, const TArray<float>& Y, float Value) -> float
		{
			if (X.Num() == 0 || Y.Num() == 0 || X.Num() != Y.Num())
			{
				return 0.f;
			}

			if (Value <= X[0])
			{
				return Y[0];
			}

			const int32 Last = X.Num() - 1;
			if (Value >= X[Last])
			{
				return Y[Last];
			}

			for (int32 i = 0; i < Last; ++i)
			{
				if (Value >= X[i] && Value <= X[i + 1])
				{
					const float Alpha = (Value - X[i]) / (X[i + 1] - X[i]);
					return FMath::Lerp(Y[i], Y[i + 1], Alpha);
				}
			}

			return Y[Last];
		};

	auto EvaluateTargetRPMFromCommand = [&](float Command) -> float
		{
			const float C = FMath::Clamp(Command, 0.f, 1.f);
			const float BaseRPM = InterpCurve(ThrottlePts, RpmFromThrottlePts, C);
			const float VoltageScale = BatteryLoadedVoltage / FMath::Max(BatteryBenchReferenceVoltage, KINDA_SMALL_NUMBER);
			return BaseRPM * VoltageScale * BatteryOutputScale;
		};

	auto EvaluatePowerFromRPM = [&](float RPM) -> float
		{
			if (RPM <= 0.f)
			{
				return 0.f;
			}

			if (RPM < RpmPts[0])
			{
				const float Ratio = RPM / RpmPts[0];
				return PowerPts[0] * FMath::Pow(Ratio, LowerPowerExponent);
			}

			return InterpCurve(RpmPts, PowerPts, RPM);
		};

	auto EvaluateThrustFromRPM = [&](float RPM) -> float
		{
			if (RPM <= 0.f)
			{
				return 0.f;
			}

			if (RPM < RpmPts[0])
			{
				const float Ratio = RPM / RpmPts[0];
				return ThrustPts[0] * FMath::Pow(Ratio, LowerThrustExponent);
			}

			return InterpCurve(RpmPts, ThrustPts, RPM);
		};

	const float PropEfficiencyFactor = ComputePropEfficiencyFactor(LocalVelocityMps);
	LastPropEfficiencyFactor = PropEfficiencyFactor;

	TotalCurrentA = 0.f;
	TotalThrustN = 0.f;
	TotalElectricalPowerW = 0.f;
	AverageRPM = 0.f;

	for (int32 i = 0; i < Motors.Num(); ++i)
	{
		FMotorState& Motor = Motors[i];

		Motor.TargetRPM = EvaluateTargetRPMFromCommand(Motor.Command);

		const float ResponseSpeed = (Motor.TargetRPM > Motor.CurrentRPM)
			? MotorResponseUpRPM
			: MotorResponseDownRPM;

		Motor.CurrentCommand = Motor.Command;
		Motor.CurrentRPM = FMath::FInterpTo(Motor.CurrentRPM, Motor.TargetRPM, DeltaTime, ResponseSpeed);

		if (Motor.CurrentRPM < 1.f)
		{
			Motor.CurrentRPM = 0.f;
		}

		const float ElectricalPower = EvaluatePowerFromRPM(Motor.CurrentRPM);
		const float ThrustGrams = EvaluateThrustFromRPM(Motor.CurrentRPM);

		Motor.ElectricalPowerWatt = ElectricalPower;
		Motor.CurrentDrawAmp = BatteryLoadedVoltage > KINDA_SMALL_NUMBER ? ElectricalPower / BatteryLoadedVoltage : 0.f;
		Motor.ThrustNewton = ThrustGrams * 0.001f * 9.81f * PropEfficiencyFactor * MotorThrustScale;
		Motor.MechanicalPowerWatt = Motor.ElectricalPowerWatt * MotorMechanicalEfficiency;

		const float OmegaRad = FMath::Max(Motor.CurrentRPM * 2.f * PI / 60.f, MinOmegaRad);
		Motor.ReactionTorqueNm = (Motor.MechanicalPowerWatt / OmegaRad) * Motor.SpinDirection;

		TotalCurrentA += Motor.CurrentDrawAmp;
		TotalThrustN += Motor.ThrustNewton;
		TotalElectricalPowerW += Motor.ElectricalPowerWatt;
		AverageRPM += Motor.CurrentRPM;
	}

	AverageRPM = Motors.Num() > 0 ? AverageRPM / Motors.Num() : 0.f;

	/*static float MotorLogTimer = 0.f;
	MotorLogTimer += DeltaTime;

	if (MotorLogTimer >= 0.25f)
	{
		MotorLogTimer = 0.f;

		const float WeightN = 3.921f * 9.81f;

		UE_LOG(LogTemp, Warning,
			TEXT("MOTOR | AvgRPM=%.0f | Thrust=%.2fN | Weight=%.2fN | Power=%.1fW | Current=%.2fA | Vbat=%.2fV"),
			AverageRPM,
			TotalThrustN,
			WeightN,
			TotalElectricalPowerW,
			TotalCurrentA,
			BatteryLoadedVoltage
		);

		UE_LOG(LogTemp, Warning,
			TEXT("M0 C=%.3f RPM=%.0f T=%.2fN I=%.2fA | M1 C=%.3f RPM=%.0f T=%.2fN I=%.2fA"),
			Motors[0].Command, Motors[0].CurrentRPM, Motors[0].ThrustNewton, Motors[0].CurrentDrawAmp,
			Motors[1].Command, Motors[1].CurrentRPM, Motors[1].ThrustNewton, Motors[1].CurrentDrawAmp
		);

		UE_LOG(LogTemp, Warning,
			TEXT("M2 C=%.3f RPM=%.0f T=%.2fN I=%.2fA | M3 C=%.3f RPM=%.0f T=%.2fN I=%.2fA"),
			Motors[2].Command, Motors[2].CurrentRPM, Motors[2].ThrustNewton, Motors[2].CurrentDrawAmp,
			Motors[3].Command, Motors[3].CurrentRPM, Motors[3].ThrustNewton, Motors[3].CurrentDrawAmp
		);
	}*/
}