
#include "FPVDronePawn.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

AFPVDronePawn::AFPVDronePawn()
{
    ArmX = 16.67f;
    ArmY= 14.5f;
    MotorKV = 900.f;
    MotorResponseUpRPM = 14.f;
    MotorResponseDownRPM = 10.f;
    MotorMechanicalEfficiency = 0.85f;
    MinOmegaRad = 30.f;

    MaxPitchRate = 360.f;
    MaxRollRate = 360.f;
    MaxYawRate = 360.f;

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

    BatterySeriesCells = 6;
    BatteryParallelCells = 3;
    BatteryCellCapacityAh = 5.0f;
    BatteryCellInternalResistanceOhm = 0.012f;
    BatteryCellVoltageFull = 4.2f;
    BatteryCellVoltageNominal = 3.6f;
    BatteryCellVoltageEmpty = 3.0f;
    BatteryUsableFraction = 0.85f;
    BatteryBenchReferenceVoltage = 25.2f;
    BatteryPackMassKg = 1.28f;
}
void AFPVDronePawn::BeginPlay()
{
    Super::BeginPlay();
    PitchPID.Reset();
    RollPID.Reset();
    YawPID.Reset();
    ResetBatteryState();
    InitMotors(); 

    //PlaneMesh->SetLinearDamping(0.f);
    //PlaneMesh->SetAngularDamping(0.2f);
    PlaneMesh->SetMassOverrideInKg(NAME_None, 3.921f, true);
    PlaneMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
    PlaneMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
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


    UpdateTelemetry();
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

    const float BaseThrottle = FMath::Clamp(GetReceivedThrottle(), 0.f, 1.f);

    if (BaseThrottle < 0.02f)
    {
        PitchPID.Reset();
        RollPID.Reset();
        YawPID.Reset();

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

    const float TargetRollRateNorm = -GetReceivedRollInput();
    const float TargetPitchRateNorm = GetReceivedPitchInput();
    const float TargetYawRateNorm = GetReceivedYawInput();

    const float RollCmd = FMath::Clamp(RollPID.Update(TargetRollRateNorm, CurrentRollRateNorm, DeltaTime), -0.05f, 0.05f);
    const float PitchCmd = FMath::Clamp(PitchPID.Update(TargetPitchRateNorm, CurrentPitchRateNorm, DeltaTime), -0.05f, 0.05f);
    const float YawCmd = FMath::Clamp(YawPID.Update(TargetYawRateNorm, CurrentYawRateNorm, DeltaTime), -0.1f, 0.1f);

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
    DebugState.PitchInput = GetReceivedPitchInput();
    DebugState.RollInput = GetReceivedRollInput();
    DebugState.YawInput = GetReceivedYawInput();

    DebugLogTimer += DeltaTime;

    if (DebugLogTimer >= 0.15f)
    {
        DebugLogTimer = 0.f;

        /*
        (LogTemp, Warning,
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
    if (!Mesh) return;

    const FVector WorldVelCm = Mesh->GetPhysicsLinearVelocity();
    if (WorldVelCm.SizeSquared() < 1.f) return;

    const FTransform MeshTransform = Mesh->GetComponentTransform();
    const FVector LocalVelMps = MeshTransform.InverseTransformVectorNoScale(WorldVelCm) / 100.f;

    const FRotator WorldRot = Mesh->GetComponentRotation();
    const float PitchRad = FMath::DegreesToRadians(FMath::Abs(WorldRot.Pitch));
    const float RollRad = FMath::DegreesToRadians(FMath::Abs(WorldRot.Roll));

    const float A_forward_eff = AreaForward * FMath::Cos(PitchRad)
        + AreaVertical * FMath::Sin(PitchRad);
    const float A_lateral_eff = AreaLateral * FMath::Cos(RollRad)
        + AreaVertical * FMath::Sin(RollRad);

    auto ComputeAxisDrag = [&](float VelocityMps, float Cd, float Area) -> float
        {
            const float SpeedAbs = FMath::Abs(VelocityMps);
            const float DragMagnitude = 0.5f * AirDensity * Cd * Area * SpeedAbs * SpeedAbs;
            return -FMath::Sign(VelocityMps) * DragMagnitude;
        };

    const float DragX_N = ComputeAxisDrag(LocalVelMps.X, CdForward, A_forward_eff);
    const float DragY_N = ComputeAxisDrag(LocalVelMps.Y, CdLateral, A_lateral_eff);
    const float DragZBody_N = ComputeAxisDrag(LocalVelMps.Z, CdVertical, AreaVertical);

    const float RotorDiscRadiusM = RotorDiscDiameterM * 0.5f;
    const float TotalRotorDiscAreaM2 = 4.f * PI * RotorDiscRadiusM * RotorDiscRadiusM;
    const float EffectiveRotorAreaM2 = TotalRotorDiscAreaM2 * RotorVerticalAreaFactor;
    const float DragZRotor_N = ComputeAxisDrag(LocalVelMps.Z, RotorVerticalCd, EffectiveRotorAreaM2);

    const FVector LocalDragN(DragX_N, DragY_N, DragZBody_N + DragZRotor_N);
    const FVector WorldDragCm = MeshTransform.TransformVectorNoScale(LocalDragN * 100.f);
    Mesh->AddForce(WorldDragCm);

    // ── Drag debug log ──────────────────────────────────────────────
    //static float DragLogTimer = 0.f;
    //DragLogTimer += LastDeltaSeconds;

    //if (DragLogTimer >= 0.5f)
    //{
    //    DragLogTimer = 0.f;

    //    const FVector WorldVelMps = WorldVelCm / 100.f;
    //    const float SpeedMps = WorldVelMps.Size();
    //    const float SpeedKmh = SpeedMps * 3.6f;
    //    const float SpeedHorizMps = FVector2D(WorldVelMps.X, WorldVelMps.Y).Size();
    //    const float SpeedHorizKmh = SpeedHorizMps * 3.6f;
    //    const float VertSpeedMps = WorldVelMps.Z;

    //    const float TotalDragN = FVector(DragX_N, DragY_N, DragZBody_N + DragZRotor_N).Size();

    //    float TotalThrustN = 0.f;
    //    float ThrottleAvg = 0.f;
    //    for (const FMotorState& M : Motors)
    //    {
    //        TotalThrustN += M.ThrustNewton;
    //        ThrottleAvg += M.CurrentCommand;
    //    }
    //    if (Motors.Num() > 0) ThrottleAvg /= Motors.Num();

    //    // Розкладаємо тягу на горизонтальну і вертикальну складові
    //    const float PitchDeg = FMath::RadiansToDegrees(PitchRad);
    //    const float ThrustVertN = TotalThrustN * FMath::Cos(PitchRad);
    //    const float ThrustHorizN = TotalThrustN * FMath::Sin(PitchRad);

    //    // Сили балансу
    //    const float WeightN = GetPlaneMesh()->GetMass() * 9.81f;
    //    const float CosPitch = FMath::Cos(PitchRad);
    //    const float SinPitch = FMath::Sin(PitchRad);

    //    const float DragZval = DragZBody_N + DragZRotor_N;

    //    const float HorizDragTotal = FMath::Abs(DragX_N) * CosPitch
    //        + FMath::Abs(DragZval) * SinPitch;
    //    const float VertDragTotal = FMath::Abs(DragX_N) * SinPitch
    //        + FMath::Abs(DragZval) * CosPitch;

    //    const float VertBalanceN = ThrustVertN - WeightN - VertDragTotal;
    //    const float HorizBalanceN = ThrustHorizN - HorizDragTotal;

    //    UE_LOG(LogTemp, Warning,
    //        TEXT("[FLIGHT] Thr=%.0f%% Pitch=%.1f°")
    //        TEXT(" | V=%.1fkm/h H=%.1fkm/h Vz=%+.2fm/s")
    //        TEXT(" | Thrust=%.1fN (V:%.1f H:%.1f) Weight=%.1fN")
    //        TEXT(" | DragX=%.1fN DragZ=%.1fN TotalDrag=%.1fN")
    //        TEXT(" | VertBal=%+.1fN HorizBal=%+.1fN")
    //        TEXT(" | A_eff=%.4fm²"),
    //        ThrottleAvg * 100.f,
    //        PitchDeg,
    //        SpeedKmh, SpeedHorizKmh, VertSpeedMps,
    //        TotalThrustN, ThrustVertN, ThrustHorizN, WeightN,
    //        DragX_N, DragZBody_N + DragZRotor_N, TotalDragN,
    //        VertBalanceN, HorizBalanceN,
    //        A_forward_eff
    //    );
    //}
    // ── End drag debug log ──────────────────────────────────────────
}


//float AFPVDronePawn::EvaluateMotorCurrentAmp(float Command) const
//{
//    const float C = FMath::Clamp(Command, 0.f, 1.f);
//    return 65.9f * FMath::Pow(C, 2.718f);;
//}
//float AFPVDronePawn::EvaluateMotorThrustGramsFromCurrent(float CurrentAmp) const
//{
//    static const TArray<float> X = { 0.f, 5.f, 10.f, 15.f, 20.f, 25.f, 30.f, 65.9f };
//    static const TArray<float> Y = { 0.f, 835.f, 1420.f, 1885.f, 2200.f, 2521.f, 2828.f, 4680.f };
//
//    const float A = FMath::Clamp(CurrentAmp, 0.f, X.Last());
//
//    if (A <= X[0]) return Y[0];
//    if (A >= X.Last()) return Y.Last();
//
//    for (int32 i = 0; i < X.Num() - 1; i++)
//    {
//        if (A >= X[i] && A <= X[i + 1])
//        {
//            const float Alpha = (A - X[i]) / (X[i + 1] - X[i]);
//            return FMath::Lerp(Y[i], Y[i + 1], Alpha);
//        }
//    }
//
//    return Y.Last();
//}
//float AFPVDronePawn::EvaluateMotorPowerWattFromCurrent(float CurrentAmp) const
//{
//    static const TArray<float> X = { 0.f, 5.f, 10.f, 15.f, 20.f, 25.f, 30.f, 65.9f };
//    static const TArray<float> Y = { 0.f, 120.40f, 239.90f, 358.70f, 477.20f, 594.80f, 711.40f, 1537.40f };
//
//    const float A = FMath::Clamp(CurrentAmp, 0.f, X.Last());
//
//    if (A <= X[0]) return Y[0];
//    if (A >= X.Last()) return Y.Last();
//
//    for (int32 i = 0; i < X.Num() - 1; i++)
//    {
//        if (A >= X[i] && A <= X[i + 1])
//        {
//            const float Alpha = (A - X[i]) / (X[i + 1] - X[i]);
//            return FMath::Lerp(Y[i], Y[i + 1], Alpha);
//        }
//    }
//
//    return Y.Last();
//}

void AFPVDronePawn::UpdateMotorDynamics(float DeltaTime)
{
    UStaticMeshComponent* Mesh = GetPlaneMesh();
    if (!Mesh)
    {
        return;
    }

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

    const FTransform MeshTransform = Mesh->GetComponentTransform();
    const FVector WorldVelocityCm = Mesh->GetPhysicsLinearVelocity();
    const FVector LocalVelocityMps = MeshTransform.InverseTransformVectorNoScale(WorldVelocityCm) / 100.f;
    const float PropEfficiencyFactor = ComputePropEfficiencyFactor(LocalVelocityMps);

    float TotalCurrentA = 0.f;

    float TotalThrustN = 0.f;
    float TotalElectricalPowerW = 0.f;
    float AvgRPM = 0.f;

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
        Motor.ThrustNewton = ThrustGrams * 0.001f * 9.81f * PropEfficiencyFactor;
        Motor.MechanicalPowerWatt = Motor.ElectricalPowerWatt * MotorMechanicalEfficiency;

        const float OmegaRad = FMath::Max(Motor.CurrentRPM * 2.f * PI / 60.f, MinOmegaRad);
        Motor.ReactionTorqueNm = (Motor.MechanicalPowerWatt / OmegaRad) * Motor.SpinDirection;

        TotalCurrentA += Motor.CurrentDrawAmp;
        TotalThrustN += Motor.ThrustNewton;
        TotalElectricalPowerW += Motor.ElectricalPowerWatt;
        AvgRPM += Motor.CurrentRPM;
    }
    AvgRPM = Motors.Num() > 0 ? AvgRPM / Motors.Num() : 0.f;
    UpdateBatteryState(TotalCurrentA, DeltaTime);

    /*static float MotorLogTimer = 0.f;
    MotorLogTimer += DeltaTime;

    if (MotorLogTimer >= 0.25f)
    {
        MotorLogTimer = 0.f;

        const float WeightN = 3.921f * 9.81f;

        UE_LOG(LogTemp, Warning,
            TEXT("MOTOR | AvgRPM=%.0f | Thrust=%.2fN | Weight=%.2fN | Power=%.1fW | Current=%.2fA | Vbat=%.2fV"),
            AvgRPM,
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

float AFPVDronePawn::GetBatteryCapacityAh() const
{
    return BatteryParallelCells * BatteryCellCapacityAh;
}

float AFPVDronePawn::GetBatteryUsableCapacityAh() const
{
    return GetBatteryCapacityAh() * BatteryUsableFraction;
}

float AFPVDronePawn::GetBatteryInternalResistanceOhm() const
{
    return BatterySeriesCells * (BatteryCellInternalResistanceOhm / FMath::Max(BatteryParallelCells, 1));
}

float AFPVDronePawn::EvaluateCellOCVFromSoC(float SoC) const
{
    static const TArray<float> SocPts = { 0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f };
    static const TArray<float> VoltPts = { 3.00f, 3.40f, 3.55f, 3.65f, 3.70f, 3.75f, 3.80f, 3.87f, 3.95f, 4.08f, 4.20f };

    const float S = FMath::Clamp(SoC, 0.f, 1.f);

    if (S <= SocPts[0])
    {
        return VoltPts[0];
    }

    const int32 Last = SocPts.Num() - 1;
    if (S >= SocPts[Last])
    {
        return VoltPts[Last];
    }

    for (int32 i = 0; i < Last; ++i)
    {
        if (S >= SocPts[i] && S <= SocPts[i + 1])
        {
            const float Alpha = (S - SocPts[i]) / (SocPts[i + 1] - SocPts[i]);
            return FMath::Lerp(VoltPts[i], VoltPts[i + 1], Alpha);
        }
    }

    return VoltPts[Last];
}

void AFPVDronePawn::ResetBatteryState()
{
    BatteryConsumedAh = 0.f;
    BatterySoC = 1.f;
    BatteryOpenCircuitVoltage = BatterySeriesCells * BatteryCellVoltageFull;
    BatteryLoadedVoltage = BatteryOpenCircuitVoltage;
    BatteryTotalCurrentA = 0.f;
}

void AFPVDronePawn::UpdateBatteryState(float TotalCurrentA, float DeltaTime)
{
    BatteryTotalCurrentA = FMath::Max(TotalCurrentA, 0.f);

    BatteryConsumedAh = FMath::Clamp(
        BatteryConsumedAh + BatteryTotalCurrentA * DeltaTime / 3600.f,
        0.f,
        GetBatteryUsableCapacityAh()
    );

    BatterySoC = 1.f - BatteryConsumedAh / FMath::Max(GetBatteryUsableCapacityAh(), KINDA_SMALL_NUMBER);

    BatteryOpenCircuitVoltage = EvaluateCellOCVFromSoC(BatterySoC) * BatterySeriesCells;

    BatteryResistanceScale = EvaluateBatteryResistanceScaleFromSoC(BatterySoC);
    const float EffectiveResistanceOhm = GetBatteryInternalResistanceOhm() * BatteryResistanceScale;

    const float SagVoltage = BatteryTotalCurrentA * EffectiveResistanceOhm;
    const float MinPackVoltage = BatterySeriesCells * BatteryCellVoltageCutoff;

    BatteryLoadedVoltage = FMath::Max(BatteryOpenCircuitVoltage - SagVoltage, MinPackVoltage);

    const float CellLoadedVoltage = BatteryLoadedVoltage / FMath::Max(BatterySeriesCells, 1);
    BatteryOutputScale = EvaluateBatteryOutputScaleFromCellVoltage(CellLoadedVoltage);

    bBatteryLowVoltageWarn = CellLoadedVoltage <= BatteryCellVoltageWarn;
    bBatteryCriticalVoltage = CellLoadedVoltage <= BatteryCellVoltageCritical;
    bBatteryCutoffActive = CellLoadedVoltage <= BatteryCellVoltageCutoff;
}

float AFPVDronePawn::EvaluateBatteryResistanceScaleFromSoC(float SoC) const
{
    const float S = FMath::Clamp(SoC, 0.f, 1.f);

    if (S >= 0.30f)
    {
        return 1.f;
    }

    const float Alpha = S / 0.30f;
    return FMath::Lerp(1.6f, 1.f, Alpha);
}

float AFPVDronePawn::EvaluateBatteryOutputScaleFromCellVoltage(float CellLoadedVoltage) const
{
    if (CellLoadedVoltage <= BatteryCellVoltageCutoff)
    {
        return 0.f;
    }

    if (CellLoadedVoltage >= BatteryCellVoltageCritical)
    {
        return 1.f;
    }

    return FMath::Clamp(
        (CellLoadedVoltage - BatteryCellVoltageCutoff) /
        FMath::Max(BatteryCellVoltageCritical - BatteryCellVoltageCutoff, KINDA_SMALL_NUMBER),
        0.f,
        1.f
    );
}


void AFPVDronePawn::UpdateTelemetry()
{
    Super::UpdateTelemetry();

    Telemetry.FlightMode = TEXT("ACRO");
    Telemetry.PackVoltage = BatteryLoadedVoltage;
    Telemetry.CellVoltage = BatterySeriesCells > 0 ? BatteryLoadedVoltage / BatterySeriesCells : 0.f;
    Telemetry.ConsumedMah = BatteryConsumedAh * 1000.f;
    Telemetry.CurrentAmp = BatteryTotalCurrentA;
    Telemetry.bBatteryValid = true;
    Telemetry.TxPowerW = TxPowerWValue;
    Telemetry.bTxPowerValid = true;
    Telemetry.bBombArmed = bBombArmed;
}
