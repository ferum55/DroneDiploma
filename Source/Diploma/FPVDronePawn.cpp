
#include "FPVDronePawn.h"
#include "Components/StaticMeshComponent.h"

AFPVDronePawn::AFPVDronePawn()
{
    ArmX = 16.67f;
    ArmY= 14.5f;
    MotorKV = 800.f;
    MotorVoltageLoaded = 23.8f;
    MotorResponseUpRPM = 14.f;
    MotorResponseDownRPM = 10.f;
    MotorMechanicalEfficiency = 0.85f;
    MinOmegaRad = 30.f;

    MaxPitchRate = 360.f;
    MaxRollRate = 360.f;
    MaxYawRate = 360.f;

    PitchPID.P = 0.3f;
    PitchPID.I = 0.f;
    PitchPID.D = 0.008f;
    PitchPID.IntegralClamp = 0.3f;

    RollPID.P = 0.3f;
    RollPID.I = 0.f;
    RollPID.D = 0.008f;
    RollPID.IntegralClamp = 0.3f;

    YawPID.P = 1.f;
    YawPID.I = 0.f;
    YawPID.D = 0.009f;
    YawPID.IntegralClamp = 0.3f;
}
void AFPVDronePawn::BeginPlay()
{
    Super::BeginPlay();
    PitchPID.Reset();
    RollPID.Reset();
    YawPID.Reset();

    InitMotors(); 

    //PlaneMesh->SetLinearDamping(0.f);
    //PlaneMesh->SetAngularDamping(0.2f);
    PlaneMesh->SetMassOverrideInKg(NAME_None, 3.921f, true);
    PlaneMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
    PlaneMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
    const FVector InertiaScale(0.059f, 0.059f, 0.113f);
    PlaneMesh->BodyInstance.InertiaTensorScale = FVector(1.f, 1.f, 0.1f);
    PlaneMesh->RecreatePhysicsState();
    PlaneMesh->SetCenterOfMass(FVector(-0.884f, -0.006f, -0.101f));

    const FVector I = PlaneMesh->GetInertiaTensor();

    UE_LOG(LogTemp, Warning,
        TEXT("New Inertia: X=%.2f Y=%.2f Z=%.2f"),
        I.X, I.Y, I.Z);

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

    Motors[0].LocalPosition = FVector(ArmX, -ArmY, 0.f);
    Motors[0].SpinDirection = 1.f;

    Motors[1].LocalPosition = FVector(ArmX, ArmY, 0.f);
    Motors[1].SpinDirection = -1.f;

    Motors[2].LocalPosition = FVector(-ArmX, -ArmY, 0.f);
    Motors[2].SpinDirection = -1.f;

    Motors[3].LocalPosition = FVector(-ArmX, ArmY, 0.f);
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

    const float RollCmd = FMath::Clamp(RollPID.Update(TargetRollRateNorm, CurrentRollRateNorm, DeltaTime), -0.1f, 0.1f);
    const float PitchCmd = FMath::Clamp(PitchPID.Update(TargetPitchRateNorm, CurrentPitchRateNorm, DeltaTime), -0.1f, 0.1f);
    const float YawCmd = FMath::Clamp(YawPID.Update(TargetYawRateNorm, CurrentYawRateNorm, DeltaTime), -0.2f, 0.2f);

    const float ArmMin = FMath::Min(ArmX, ArmY);

    const float PitchMix = PitchCmd * (ArmMin / FMath::Max(ArmX, 0.001f));
    const float RollMix = RollCmd * (ArmMin / FMath::Max(ArmY, 0.001f));

    const float FL = BaseThrottle - PitchMix - RollMix - YawCmd * Motors[0].SpinDirection;
    const float FR = BaseThrottle - PitchMix + RollMix - YawCmd * Motors[1].SpinDirection;
    const float BL = BaseThrottle + PitchMix - RollMix - YawCmd * Motors[2].SpinDirection;
    const float BR = BaseThrottle + PitchMix + RollMix - YawCmd * Motors[3].SpinDirection;

    Motors[0].Command = FMath::Clamp(FL, 0.f, 1.f);
    Motors[1].Command = FMath::Clamp(FR, 0.f, 1.f);
    Motors[2].Command = FMath::Clamp(BL, 0.f, 1.f);
    Motors[3].Command = FMath::Clamp(BR, 0.f, 1.f);

    /*DebugState.CurrentRollRate = LocalAngVelDeg.X;
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
    DebugState.PitchInput = GetPitchInput();
    DebugState.RollInput = GetRollInput();
    DebugState.YawInput = GetYawInput();

    DebugLogTimer += DeltaTime;

    if (DebugLogTimer >= 0.15f)
    {
        DebugLogTimer = 0.f;

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
        );
    }*/
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

       //Motor.ReactionTorqueNm = MotorPropTorqueCoeff * Motor.CurrentRPM * Motor.CurrentRPM * Motor.SpinDirection;
        const float OmegaRad = FMath::Max(Motor.CurrentRPM * 2.f * PI / 60.f, MinOmegaRad);
        Motor.ReactionTorqueNm = (Motor.MechanicalPowerWatt / OmegaRad) * Motor.SpinDirection;
    }
}


