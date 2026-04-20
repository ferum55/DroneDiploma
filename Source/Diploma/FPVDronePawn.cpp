
#include "FPVDronePawn.h"
#include "Components/StaticMeshComponent.h"

AFPVDronePawn::AFPVDronePawn()
{
    ArmLength = 24.f;

    MotorKV = 800.f;
    MotorVoltageLoaded = 23.8f;
    MotorResponseUpRPM = 14.f;
    MotorResponseDownRPM = 10.f;
    MotorMechanicalEfficiency = 0.85f;
    MinOmegaRad = 30.f;

    MaxPitchRate = 360.f;
    MaxRollRate = 360.f;
    MaxYawRate = 360.f;

    PitchPID.P = 0.8f; //0.25f;
    PitchPID.I = 0.f;
    PitchPID.D = 0.006f;
    PitchPID.IntegralClamp = 0.3f;

    RollPID.P = 0.8f;
    RollPID.I = 0.f;
    RollPID.D = 0.006f;
    RollPID.IntegralClamp = 0.3f;

    YawPID.P = 0.8f;
    YawPID.I = 0.f;
    YawPID.D = 0.002f;
    YawPID.IntegralClamp = 0.2f;
}
void AFPVDronePawn::BeginPlay()
{
    Super::BeginPlay();
    PitchPID.Reset();
    RollPID.Reset();
    YawPID.Reset();

    InitMotors(); 

    PlaneMesh->SetLinearDamping(0.f);
    PlaneMesh->SetAngularDamping(0.2f);
    PlaneMesh->SetMassOverrideInKg(NAME_None, 3.921f, true);
    PlaneMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
    PlaneMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);

    const FVector ComWorld = PlaneMesh->GetCenterOfMass();
    const FVector ComLocal = PlaneMesh->GetComponentTransform().InverseTransformPosition(ComWorld);

    UE_LOG(LogTemp, Warning, TEXT("COM World: %s"), *ComWorld.ToString());
    UE_LOG(LogTemp, Warning, TEXT("COM Local: %s"), *ComLocal.ToString());

    UE_LOG(LogTemp, Warning, TEXT("Actor Rotation: %s"), *GetActorRotation().ToString());
    UE_LOG(LogTemp, Warning, TEXT("Mesh Rotation: %s"), *PlaneMesh->GetComponentRotation().ToString());

    for (int i = 0; i < Motors.Num(); i++)
    {
        UE_LOG(LogTemp, Warning, TEXT("Motor %d LocalPos=(%.2f %.2f %.2f)"),
            i,
            Motors[i].LocalPosition.X,
            Motors[i].LocalPosition.Y,
            Motors[i].LocalPosition.Z);
    }
    MaxThrust = 0.f;
}

void AFPVDronePawn::ApplyThrust()
{
    ApplyMotorForces();
    ApplyAerodynamicDrag();
}

void AFPVDronePawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (bAutotuneActive)
    {
        TickAutotune(DeltaSeconds);
        UpdateMotorThrusts(DeltaSeconds);
        UpdateMotorDynamics(DeltaSeconds);
        ApplyThrust();
        return;
    }

    UpdateMotorThrusts(DeltaSeconds);
    UpdateMotorDynamics(DeltaSeconds);
    ApplyThrust();
}

void AFPVDronePawn::ApplyTorques()
{
}


void AFPVDronePawn::InitMotors()
{
    Motors.SetNum(4);

    Motors[0].LocalPosition = FVector(ArmLength, -ArmLength, 0.f);
    Motors[0].SpinDirection = 1.f;

    Motors[1].LocalPosition = FVector(ArmLength, ArmLength, 0.f);
    Motors[1].SpinDirection = -1.f;

    Motors[2].LocalPosition = FVector(-ArmLength, -ArmLength, 0.f);
    Motors[2].SpinDirection = -1.f;

    Motors[3].LocalPosition = FVector(-ArmLength, ArmLength, 0.f);
    Motors[3].SpinDirection = 1.f;

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

void AFPVDronePawn::UpdateMotorThrusts(float DeltaTime)
{
    UStaticMeshComponent* Mesh = GetPlaneMesh();
    if (!Mesh || Motors.Num() != 4)
    {
        return;
    }

    const float BaseThrottle = FMath::Clamp(Throttle, 0.f, 1.f);

    /*if (BaseThrottle < 0.02f)
    {
        PitchPID.Reset();
        RollPID.Reset();
        YawPID.Reset();

        for (FMotorState& Motor : Motors)
        {
            Motor.Command = 0.f;
        }

        return;
    }*/

    const FTransform MeshTransform = Mesh->GetComponentTransform();
    const FVector WorldAngVelDeg = Mesh->GetPhysicsAngularVelocityInDegrees();
    const FVector LocalAngVelDeg = MeshTransform.InverseTransformVectorNoScale(WorldAngVelDeg);

    const float CurrentRollRateNorm = LocalAngVelDeg.X / MaxRollRate;
    const float CurrentPitchRateNorm = LocalAngVelDeg.Y / MaxPitchRate;
    const float CurrentYawRateNorm = LocalAngVelDeg.Z / MaxYawRate;

    const float TargetRollRateNorm = -GetRollInput();
    const float TargetPitchRateNorm = GetPitchInput();
    const float TargetYawRateNorm = GetYawInput();

    const float RollCmd = FMath::Clamp(RollPID.Update(TargetRollRateNorm, CurrentRollRateNorm, DeltaTime), -0.20f, 0.20f);
    const float PitchCmd = FMath::Clamp(PitchPID.Update(TargetPitchRateNorm, CurrentPitchRateNorm, DeltaTime), -0.20f, 0.20f);
    const float YawCmd = FMath::Clamp(YawPID.Update(TargetYawRateNorm, CurrentYawRateNorm, DeltaTime), -0.12f, 0.12f);

    const float FL = BaseThrottle - PitchCmd - RollCmd - YawCmd * Motors[0].SpinDirection;
    const float FR = BaseThrottle - PitchCmd + RollCmd - YawCmd * Motors[1].SpinDirection;
    const float BL = BaseThrottle + PitchCmd - RollCmd - YawCmd * Motors[2].SpinDirection;
    const float BR = BaseThrottle + PitchCmd + RollCmd - YawCmd * Motors[3].SpinDirection;

    Motors[0].Command = FMath::Clamp(FL, 0.f, 1.f);
    Motors[1].Command = FMath::Clamp(FR, 0.f, 1.f);
    Motors[2].Command = FMath::Clamp(BL, 0.f, 1.f);
    Motors[3].Command = FMath::Clamp(BR, 0.f, 1.f);
}

void AFPVDronePawn::ApplyMotorForces()
{
    UStaticMeshComponent* Mesh = GetPlaneMesh();
    if (!Mesh || Motors.Num() != 4)
    {
        return;
    }

    const FTransform MeshTransform = Mesh->GetComponentTransform();
    const FVector UpVector = MeshTransform.GetUnitAxis(EAxis::Z);

    float TotalYawTorqueNm = 0.f;

    for (const FMotorState& Motor : Motors)
    {
        const FVector Force = UpVector * (Motor.ThrustNewton * 100.f);
        const FVector WorldLocation = MeshTransform.TransformPosition(Motor.LocalPosition);

        Mesh->AddForceAtLocation(Force, WorldLocation);

        TotalYawTorqueNm += -Motor.ReactionTorqueNm;
    }

    const FVector LocalTorque(0.f, 0.f, TotalYawTorqueNm * 10000.f);
    const FVector WorldTorque = MeshTransform.TransformVectorNoScale(LocalTorque);

    Mesh->AddTorqueInRadians(WorldTorque);
}

float AFPVDronePawn::ComputePropEfficiencyFactor(const FVector& LocalVelocityMps) const
{
    const float VerticalSpeedMps = FMath::Abs(LocalVelocityMps.Z);
    const float ForwardSpeedMps = FMath::Max(LocalVelocityMps.X, 0.f);

    const float VerticalRatio = VerticalSpeedMps / FMath::Max(PropwashSpeedScaleMps, 0.1f);
    const float VerticalPenalty = 0.08f * FMath::Clamp(VerticalRatio * VerticalRatio, 0.f, 1.f);

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

void AFPVDronePawn::ApplyAerodynamicDrag()
{
    UStaticMeshComponent* Mesh = GetPlaneMesh();
    if (!Mesh)
    {
        return;
    }

    const FVector WorldVelCm = Mesh->GetPhysicsLinearVelocity();
    if (WorldVelCm.SizeSquared() < 1.f)
    {
        return;
    }

    const FTransform MeshTransform = Mesh->GetComponentTransform();
    const FVector LocalVelMps = MeshTransform.InverseTransformVectorNoScale(WorldVelCm) / 100.f;

    auto ComputeAxisDrag = [this](float VelocityMps, float Cd, float Area) -> float
        {
            const float SpeedAbs = FMath::Abs(VelocityMps);
            const float DragMagnitude = 0.5f * AirDensity * Cd * Area * SpeedAbs * SpeedAbs;
            return -FMath::Sign(VelocityMps) * DragMagnitude;
        };

    const float DragX_N = ComputeAxisDrag(LocalVelMps.X, CdForward, AreaForward);
    const float DragY_N = ComputeAxisDrag(LocalVelMps.Y, CdLateral, AreaLateral);
    const float DragZBody_N = ComputeAxisDrag(LocalVelMps.Z, CdVertical, AreaVertical);

    const float RotorDiscRadiusM = RotorDiscDiameterM * 0.5f;
    const float TotalRotorDiscAreaM2 = 4.f * PI * RotorDiscRadiusM * RotorDiscRadiusM;
    const float EffectiveRotorAreaM2 = TotalRotorDiscAreaM2 * RotorVerticalAreaFactor;
    const float DragZRotor_N = ComputeAxisDrag(LocalVelMps.Z, RotorVerticalCd, EffectiveRotorAreaM2);

    const FVector LocalDragN(
        DragX_N,
        DragY_N,
        DragZBody_N + DragZRotor_N
    );

    const FVector WorldDragCm = MeshTransform.TransformVectorNoScale(LocalDragN * 100.f);
    Mesh->AddForce(WorldDragCm);
}


float AFPVDronePawn::EvaluateMotorCurrentAmp(float Command) const
{
    const float C = FMath::Clamp(Command, 0.f, 1.f);
    return 65.9f * FMath::Pow(C, 2.718f);;
}
float AFPVDronePawn::EvaluateMotorThrustGramsFromCurrent(float CurrentAmp) const
{
    static const TArray<float> X = { 0.f, 5.f, 10.f, 15.f, 20.f, 25.f, 30.f, 65.9f };
    static const TArray<float> Y = { 0.f, 835.f, 1420.f, 1885.f, 2200.f, 2521.f, 2828.f, 4680.f };

    const float A = FMath::Clamp(CurrentAmp, 0.f, X.Last());

    if (A <= X[0]) return Y[0];
    if (A >= X.Last()) return Y.Last();

    for (int32 i = 0; i < X.Num() - 1; i++)
    {
        if (A >= X[i] && A <= X[i + 1])
        {
            const float Alpha = (A - X[i]) / (X[i + 1] - X[i]);
            return FMath::Lerp(Y[i], Y[i + 1], Alpha);
        }
    }

    return Y.Last();
}float AFPVDronePawn::EvaluateMotorPowerWattFromCurrent(float CurrentAmp) const
{
    static const TArray<float> X = { 0.f, 5.f, 10.f, 15.f, 20.f, 25.f, 30.f, 65.9f };
    static const TArray<float> Y = { 0.f, 120.40f, 239.90f, 358.70f, 477.20f, 594.80f, 711.40f, 1537.40f };

    const float A = FMath::Clamp(CurrentAmp, 0.f, X.Last());

    if (A <= X[0]) return Y[0];
    if (A >= X.Last()) return Y.Last();

    for (int32 i = 0; i < X.Num() - 1; i++)
    {
        if (A >= X[i] && A <= X[i + 1])
        {
            const float Alpha = (A - X[i]) / (X[i + 1] - X[i]);
            return FMath::Lerp(Y[i], Y[i + 1], Alpha);
        }
    }

    return Y.Last();
}

void AFPVDronePawn::UpdateMotorDynamics(float DeltaTime)
{
    UStaticMeshComponent* Mesh = GetPlaneMesh();
    if (!Mesh)
    {
        return;
    }

    const FTransform MeshTransform = Mesh->GetComponentTransform();
    const FVector WorldVelocityCm = Mesh->GetPhysicsLinearVelocity();
    const FVector LocalVelocityMps = MeshTransform.InverseTransformVectorNoScale(WorldVelocityCm) / 100.f;
    const float PropEfficiencyFactor = ComputePropEfficiencyFactor(LocalVelocityMps);

    for (FMotorState& Motor : Motors)
    {
        const float ResponseSpeed = (Motor.Command > Motor.CurrentCommand) ? MotorResponseUpRPM : MotorResponseDownRPM;
        Motor.CurrentCommand = FMath::FInterpTo(Motor.CurrentCommand, Motor.Command, DeltaTime, ResponseSpeed);

        Motor.TargetRPM = Motor.CurrentCommand * MotorKV * MotorVoltageLoaded;
        Motor.CurrentRPM = FMath::FInterpTo(Motor.CurrentRPM, Motor.TargetRPM, DeltaTime, ResponseSpeed);

        Motor.CurrentDrawAmp = EvaluateMotorCurrentAmp(Motor.CurrentCommand);

        const float ThrustGrams = EvaluateMotorThrustGramsFromCurrent(Motor.CurrentDrawAmp);
        Motor.ThrustNewton = ThrustGrams * 0.001f * 9.81f * PropEfficiencyFactor;

        Motor.ElectricalPowerWatt = EvaluateMotorPowerWattFromCurrent(Motor.CurrentDrawAmp);
        Motor.MechanicalPowerWatt = Motor.ElectricalPowerWatt * MotorMechanicalEfficiency;

        Motor.ReactionTorqueNm = MotorPropTorqueCoeff * Motor.CurrentRPM * Motor.CurrentRPM * Motor.SpinDirection;
        
        {
            static float PropLogTimer = 0.f;
            PropLogTimer += DeltaTime;

            if (PropLogTimer >= 0.5f)
            {
                PropLogTimer = 0.f;

                const float TotalKmh = (WorldVelocityCm.Size() / 100.f) * 3.6f;

                UE_LOG(LogTemp, Warning,
                    TEXT("PROP | Thr=%.2f | Cmd=%.3f | I=%.1f A | Speed=%.1f kmh | RPM=%.0f | Thrust=%.2f N | Power=%.0f W"),
                    Throttle,
                    Motor.CurrentCommand,
                    Motor.CurrentDrawAmp,
                    TotalKmh,
                    Motor.CurrentRPM,
                    Motor.ThrustNewton,
                    Motor.ElectricalPowerWatt
                );
            }
        }
    }
}



//Tuning PIDs
void AFPVDronePawn::TuneRollP()
{
    StartAutotune(EAutotuneAxis::Roll);
}

void AFPVDronePawn::TunePitchP()
{
    StartAutotune(EAutotuneAxis::Pitch);
}

void AFPVDronePawn::TuneYawP()
{
    StartAutotune(EAutotuneAxis::Yaw);
}

void AFPVDronePawn::BuildTrainScenario()
{
    AutotuneTrainScenario.Reset();

    AutotuneTrainScenario.Add({ 0.50f,  0.00f });
    AutotuneTrainScenario.Add({ 0.40f,  0.20f });
    AutotuneTrainScenario.Add({ 0.90f,  0.00f });

    AutotuneTrainScenario.Add({ 0.40f, -0.20f });
    AutotuneTrainScenario.Add({ 0.90f,  0.00f });

    AutotuneTrainScenario.Add({ 0.35f,  0.50f });
    AutotuneTrainScenario.Add({ 1.00f,  0.00f });

    AutotuneTrainScenario.Add({ 0.35f, -0.50f });
    AutotuneTrainScenario.Add({ 1.00f,  0.00f });

    AutotuneTrainScenario.Add({ 0.25f,  1.00f });
    AutotuneTrainScenario.Add({ 1.20f,  0.00f });

    AutotuneTrainScenario.Add({ 0.25f, -1.00f });
    AutotuneTrainScenario.Add({ 1.20f,  0.00f });
}

void AFPVDronePawn::BuildValidationScenario()
{
    AutotuneValidationScenario.Reset();

    AutotuneValidationScenario.Add({ 0.50f,  0.00f });
    AutotuneValidationScenario.Add({ 0.45f,  0.35f });
    AutotuneValidationScenario.Add({ 0.90f,  0.00f });

    AutotuneValidationScenario.Add({ 0.30f, -0.75f });
    AutotuneValidationScenario.Add({ 1.00f,  0.00f });

    AutotuneValidationScenario.Add({ 0.20f,  1.00f });
    AutotuneValidationScenario.Add({ 1.20f,  0.00f });
}

void AFPVDronePawn::BuildPCandidates(float MinP, float MaxP, float StepP)
{
    AutotuneCandidates.Reset();

    for (float P = MinP; P <= MaxP + KINDA_SMALL_NUMBER; P += StepP)
    {
        FPIDCandidate Candidate;
        Candidate.P = P;
        Candidate.I = 0.f;
        Candidate.D = 0.f;
        AutotuneCandidates.Add(Candidate);
    }
}

void AFPVDronePawn::StartAutotune(EAutotuneAxis Axis)
{
    if (!PlaneMesh)
    {
        return;
    }

    bAutotuneActive = true;
    bAutotuneValidationPhase = false;
    AutotuneAxis = Axis;

    BuildTrainScenario();
    BuildValidationScenario();
    BuildPCandidates(0.02f, 0.25f, 0.01f);

    BuildTrainScenario();
    BuildValidationScenario();

   /* AutotuneCandidates.Reset();*/

    /*FPIDCandidate A;
    A.P = 0.02f;
    A.I = 0.f;
    A.D = 0.f;
    AutotuneCandidates.Add(A);

    FPIDCandidate B;
    B.P = 0.08f;
    B.I = 0.f;
    B.D = 0.f;
    AutotuneCandidates.Add(B);

    FPIDCandidate C;
    C.P = 0.15f;
    C.I = 0.f;
    C.D = 0.f;
    AutotuneCandidates.Add(C);*/

    AutotuneStartLocation = GetActorLocation();
    AutotuneStartRotation = GetActorRotation();

    BestCost = TNumericLimits<float>::Max();
    BestCandidate = FPIDCandidate();
    BestMetrics = FAutotuneMetrics();

    AutotuneCandidateIndex = 0;

    BeginAutotuneRun();
}

void AFPVDronePawn::BeginAutotuneRun()
{
    if (!AutotuneCandidates.IsValidIndex(AutotuneCandidateIndex))
    {
        bAutotuneActive = false;
        return;
    }

    CurrentCandidate = AutotuneCandidates[AutotuneCandidateIndex];
    ApplyPIDCandidate(CurrentCandidate);
    ResetAutotuneRunState();

    UE_LOG(LogTemp, Warning, TEXT("Autotune run begin"));
    UE_LOG(LogTemp, Warning, TEXT("Axis=%d CandidateIndex=%d P=%.4f I=%.4f D=%.4f"),
        static_cast<int32>(AutotuneAxis),
        AutotuneCandidateIndex,
        CurrentCandidate.P,
        CurrentCandidate.I,
        CurrentCandidate.D);
}
void AFPVDronePawn::SetAutotuneInputs(float AxisInput)
{
    PitchInput = 0.f;
    RollInput = 0.f;
    YawInput = 0.f;

    switch (AutotuneAxis)
    {
    case EAutotuneAxis::Roll:
        RollInput = AxisInput;
        break;

    case EAutotuneAxis::Pitch:
        PitchInput = AxisInput;
        break;

    case EAutotuneAxis::Yaw:
        YawInput = AxisInput;
        break;
    }
}

void AFPVDronePawn::TickAutotune(float DeltaTime)
{
    const TArray<FAutotuneSegment>& Scenario =
        bAutotuneValidationPhase ? AutotuneValidationScenario : AutotuneTrainScenario;

    if (!Scenario.IsValidIndex(AutotuneSegmentIndex))
    {
        FinishAutotuneRun();
        return;
    }

    const FAutotuneSegment& Segment = Scenario[AutotuneSegmentIndex];

    SetAutotuneInputs(Segment.Input);
    Throttle = AutotuneThrottle;

    CollectAutotuneMetrics(DeltaTime);

    AutotuneSegmentTime += DeltaTime;

    if (AutotuneSegmentTime >= Segment.Duration)
    {
        UE_LOG(LogTemp, Warning, TEXT("Segment finished: Index=%d Duration=%.2f Input=%.2f"),
            AutotuneSegmentIndex,
            Segment.Duration,
            Segment.Input);

        AutotuneSegmentIndex++;
        AutotuneSegmentTime = 0.f;
    }
}

void AFPVDronePawn::ApplyPIDCandidate(const FPIDCandidate& Candidate)
{
    switch (AutotuneAxis)
    {
    case EAutotuneAxis::Roll:
        RollPID.P = Candidate.P;
        RollPID.I = Candidate.I;
        RollPID.D = Candidate.D;
        break;

    case EAutotuneAxis::Pitch:
        PitchPID.P = Candidate.P;
        PitchPID.I = Candidate.I;
        PitchPID.D = Candidate.D;
        break;

    case EAutotuneAxis::Yaw:
        YawPID.P = Candidate.P;
        YawPID.I = Candidate.I;
        YawPID.D = Candidate.D;
        break;
    }
}
void AFPVDronePawn::ResetAutotuneRunState()
{
    SetActorLocationAndRotation(
        AutotuneStartLocation,
        AutotuneStartRotation,
        false,
        nullptr,
        ETeleportType::TeleportPhysics
    );

    PlaneMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
    PlaneMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);

    PitchPID.Reset();
    RollPID.Reset();
    YawPID.Reset();

    for (FMotorState& Motor : Motors)
    {
        Motor.Command = 0.f;
    }

    CurrentMetrics = FAutotuneMetrics();
    AutotuneSegmentIndex = 0;
    AutotuneSegmentTime = 0.f;
    AutotunePrevError = 0.f;

    Throttle = AutotuneThrottle;
    PitchInput = 0.f;
    RollInput = 0.f;
    YawInput = 0.f;

    LastPitchCmd = 0.f;
    LastRollCmd = 0.f;
    LastYawCmd = 0.f;

    ReleasePhaseTime = 0.f;
    bWasInReleasePhase = false;
}

float AFPVDronePawn::GetCurrentAxisRateNorm() const
{
    if (!PlaneMesh)
    {
        return 0.f;
    }

    const FTransform MeshTransform = PlaneMesh->GetComponentTransform();
    const FVector WorldAngVelDeg = PlaneMesh->GetPhysicsAngularVelocityInDegrees();
    const FVector LocalAngVelDeg = MeshTransform.InverseTransformVectorNoScale(WorldAngVelDeg);

    switch (AutotuneAxis)
    {
    case EAutotuneAxis::Roll:
        return LocalAngVelDeg.X / MaxRollRate;

    case EAutotuneAxis::Pitch:
        return LocalAngVelDeg.Y / MaxPitchRate;

    case EAutotuneAxis::Yaw:
        return LocalAngVelDeg.Z / MaxYawRate;
    }

    return 0.f;
}

float AFPVDronePawn::GetCurrentTargetRateNorm() const
{
    switch (AutotuneAxis)
    {
    case EAutotuneAxis::Roll:
        return -RollInput;

    case EAutotuneAxis::Pitch:
        return PitchInput;

    case EAutotuneAxis::Yaw:
        return YawInput;
    }

    return 0.f;
}

void AFPVDronePawn::CollectAutotuneMetrics(float DeltaTime)
{
    const float TargetRateNorm = GetCurrentTargetRateNorm();
    const float CurrentRateNorm = GetCurrentAxisRateNorm();
    const float Error = TargetRateNorm - CurrentRateNorm;

    CurrentMetrics.TrackingError += FMath::Abs(Error) * DeltaTime;

    if (FMath::Abs(TargetRateNorm) > KINDA_SMALL_NUMBER)
    {
        const float SignedRate = CurrentRateNorm * FMath::Sign(TargetRateNorm);
        const float SignedTarget = FMath::Abs(TargetRateNorm);

        const float Excess = FMath::Max(0.f, SignedRate - SignedTarget);
        CurrentMetrics.Overshoot += Excess * DeltaTime;

        const float Deficit = FMath::Max(0.f, SignedTarget * 0.85f - SignedRate);
        CurrentMetrics.ResponsePenalty += Deficit * DeltaTime;
    }

    const bool bReleasePhase = FMath::Abs(TargetRateNorm) < KINDA_SMALL_NUMBER;

    if (bReleasePhase)
    {
        if (!bWasInReleasePhase)
        {
            ReleasePhaseTime = 0.f;
        }

        ReleasePhaseTime += DeltaTime;

        if (ReleasePhaseTime <= 0.35f)
        {
            if (FMath::Abs(CurrentRateNorm) > 0.05f)
            {
                CurrentMetrics.ReleaseSettling += DeltaTime;
            }
        }
    }
    else
    {
        ReleasePhaseTime = 0.f;
    }

    bWasInReleasePhase = bReleasePhase;

    if (FMath::Abs(Error) > 0.08f && FMath::Abs(AutotunePrevError) > 0.08f)
    {
        if (FMath::Sign(Error) != FMath::Sign(AutotunePrevError))
        {
            CurrentMetrics.OscillationPenalty += DeltaTime;
        }
    }

    for (const FMotorState& Motor : Motors)
    {
        if (Motor.Command < 0.02f || Motor.Command > 0.98f)
        {
            CurrentMetrics.SaturationPenalty += DeltaTime * 0.25f;
        }
    }

    AutotunePrevError = Error;
}

bool AFPVDronePawn::IsAutotuneCandidateAcceptable(const FAutotuneMetrics& Metrics) const
{
    if (Metrics.ReleaseSettling > 2.f)
    {
        return false;
    }

    if (Metrics.OscillationPenalty > 0.15f)
    {
        return false;
    }

    return true;
}

float AFPVDronePawn::ComputeAutotuneCost(const FAutotuneMetrics& Metrics) const
{
    if (!IsAutotuneCandidateAcceptable(Metrics))
    {
        return 1000000.f;
    }

    return
        10.0f * Metrics.ResponsePenalty +
        4.0f * Metrics.TrackingError +
        1.0f * Metrics.Overshoot +
        0.5f * Metrics.SaturationPenalty;
}

void AFPVDronePawn::FinishAutotuneRun()
{
    CurrentMetrics.Cost = ComputeAutotuneCost(CurrentMetrics);

    UE_LOG(LogTemp, Warning, TEXT("Autotune candidate finished"));
    UE_LOG(LogTemp, Warning, TEXT("Axis=%d CandidateIndex=%d P=%.4f I=%.4f D=%.4f"),
        static_cast<int32>(AutotuneAxis),
        AutotuneCandidateIndex,
        CurrentCandidate.P,
        CurrentCandidate.I,
        CurrentCandidate.D);

    UE_LOG(LogTemp, Warning, TEXT("TrackingError=%.6f ResponsePenalty=%.6f Overshoot=%.6f ReleaseSettling=%.6f OscillationPenalty=%.6f SaturationPenalty=%.6f Cost=%.6f"),
        CurrentMetrics.TrackingError,
        CurrentMetrics.ResponsePenalty,
        CurrentMetrics.Overshoot,
        CurrentMetrics.ReleaseSettling,
        CurrentMetrics.OscillationPenalty,
        CurrentMetrics.SaturationPenalty,
        CurrentMetrics.Cost);

    if (CurrentMetrics.Cost < BestCost)
    {
        BestCost = CurrentMetrics.Cost;
        BestCandidate = CurrentCandidate;
        BestMetrics = CurrentMetrics;

        UE_LOG(LogTemp, Warning, TEXT("New best candidate: P=%.4f I=%.4f D=%.4f Cost=%.6f"),
            BestCandidate.P,
            BestCandidate.I,
            BestCandidate.D,
            BestCost);
    }

    AdvanceAutotuneCandidate();
}

void AFPVDronePawn::AdvanceAutotuneCandidate()
{
    AutotuneCandidateIndex++;

    if (AutotuneCandidates.IsValidIndex(AutotuneCandidateIndex))
    {
        BeginAutotuneRun();
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("Autotune finished for axis %d"),
        static_cast<int32>(AutotuneAxis));

    UE_LOG(LogTemp, Warning, TEXT("Best candidate: P=%.4f I=%.4f D=%.4f Cost=%.6f"),
        BestCandidate.P,
        BestCandidate.I,
        BestCandidate.D,
        BestCost);

    UE_LOG(LogTemp, Warning, TEXT("Best metrics: TrackingError=%.6f ResponsePenalty=%.6f Overshoot=%.6f ReleaseSettling=%.6f OscillationPenalty=%.6f SaturationPenalty=%.6f"),
        BestMetrics.TrackingError,
        BestMetrics.ResponsePenalty,
        BestMetrics.Overshoot,
        BestMetrics.ReleaseSettling,
        BestMetrics.OscillationPenalty,
        BestMetrics.SaturationPenalty);

    ApplyPIDCandidate(BestCandidate);

    bAutotuneActive = false;
    SetAutotuneInputs(0.f);
}