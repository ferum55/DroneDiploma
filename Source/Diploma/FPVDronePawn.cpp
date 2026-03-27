
#include "FPVDronePawn.h"
#include "Components/StaticMeshComponent.h"

AFPVDronePawn::AFPVDronePawn()
{
    ArmLength = 20.f;

    MaxMotorThrust = 38.f;
    MotorYawTorquePerNewton = 100;

    MaxPitchRate = 120.f;
    MaxRollRate = 120.f;
    MaxYawRate = 60.f;

    // Pitch ≥ Roll Ч динам≥ка однакова на симетричному квадрокоптер≥
    PitchPID.P = 0.08f;   // менш агресивно н≥ж 0.3
    PitchPID.I = 0.02f;   // компенсуЇ зм≥щенн€ центру мас
    PitchPID.D = 0.003f;  // гасить перестр≥л
    PitchPID.IntegralClamp = 0.3f;

    RollPID.P = 0.08f;
    RollPID.I = 0.02f;
    RollPID.D = 0.003f;
    RollPID.IntegralClamp = 0.3f;

    // Yaw Ч ≥нерц≥€ по Z б≥льша, реакц≥€ пов≥льн≥ша
    YawPID.P = 0.05f;
    YawPID.I = 0.01f;
    YawPID.D = 0.004f;
    YawPID.IntegralClamp = 0.2f;
}
void AFPVDronePawn::BeginPlay()
{
    Super::BeginPlay();
    PitchPID.Reset();
    RollPID.Reset();
    YawPID.Reset();
    InitMotors();
    PlaneMesh->SetCenterOfMass(FVector(15.76f, 0.f, -7.05f), NAME_None);
    PlaneMesh->SetMassOverrideInKg(NAME_None, 4.5f, true);
    //PlaneMesh->BodyInstance.InertiaTensorScale = FVector(0.1f, 0.1f, 0.05f);

    for (int i = 0; i < Motors.Num(); i++)
    {
        UE_LOG(LogTemp, Warning, TEXT("Motor %d LocalPos=(%.2f %.2f %.2f)"),
            i,
            Motors[i].LocalPosition.X,
            Motors[i].LocalPosition.Y,
            Motors[i].LocalPosition.Z);
    }
}

void AFPVDronePawn::Tick(float DeltaSeconds)
{
    UpdateMotorThrusts(DeltaSeconds);
    Super::Tick(DeltaSeconds);
}

void AFPVDronePawn::ApplyThrust()
{
    ApplyMotorForces();
}

void AFPVDronePawn::ApplyTorques()
{
    /*Super::ApplyTorques();*/
}

void AFPVDronePawn::InitMotors()
{
    Motors.SetNum(4);

    Motors[0].LocalPosition = FVector(ArmLength, ArmLength, 0.f);
    Motors[0].SpinDirection = 1.f;

    Motors[1].LocalPosition = FVector(ArmLength, -ArmLength, 0.f);
    Motors[1].SpinDirection = -1.f;

    Motors[2].LocalPosition = FVector(-ArmLength, ArmLength, 0.f);
    Motors[2].SpinDirection = 1.f;

    Motors[3].LocalPosition = FVector(-ArmLength, -ArmLength, 0.f);
    Motors[3].SpinDirection = -1.f;

    for (FMotorState& Motor : Motors)
    {
        Motor.ThrustOutput = 0.f;
    }
}

void AFPVDronePawn::UpdateMotorThrusts(float DeltaTime)
{
    UStaticMeshComponent* Mesh = GetPlaneMesh();
    if (!Mesh || Motors.Num() != 4)
    {
        return;
    }

    const FVector WorldAngVel = Mesh->GetPhysicsAngularVelocityInDegrees();
    const FVector LocalAngVel = Mesh->GetComponentTransform().InverseTransformVectorNoScale(WorldAngVel);

    UE_LOG(LogTemp, Warning, TEXT("WorldAngVel: X=%.2f Y=%.2f Z=%.2f"),
        WorldAngVel.X, WorldAngVel.Y, WorldAngVel.Z);
    UE_LOG(LogTemp, Warning, TEXT("LocalAngVel: X=%.2f Y=%.2f Z=%.2f"),
        LocalAngVel.X, LocalAngVel.Y, LocalAngVel.Z);

    const float CurrentPitchRate = LocalAngVel.Y;
    const float CurrentRollRate = LocalAngVel.X;
    const float CurrentYawRate = LocalAngVel.Z;

    const float TargetPitchRate = GetPitchInput() * MaxPitchRate;
    const float TargetRollRate = GetRollInput() * MaxRollRate;
    const float TargetYawRate = YawInput * MaxYawRate;

    const float PitchCmd = FMath::Clamp(PitchPID.Update(TargetPitchRate, CurrentPitchRate, DeltaTime), -1.f, 1.f);
    const float RollCmd = FMath::Clamp(RollPID.Update(TargetRollRate, CurrentRollRate, DeltaTime), -1.f, 1.f);
    const float YawCmd = FMath::Clamp(YawPID.Update(TargetYawRate, CurrentYawRate, DeltaTime), -1.f, 1.f);

    const float BaseThrottle = FMath::Clamp(Throttle, 0.f, 1.f);

    float FL = BaseThrottle - PitchCmd + RollCmd + YawCmd;
    float FR = BaseThrottle - PitchCmd - RollCmd - YawCmd;
    float BL = BaseThrottle + PitchCmd + RollCmd - YawCmd;
    float BR = BaseThrottle + PitchCmd - RollCmd + YawCmd;

    Motors[0].ThrustOutput = FMath::Clamp(FL, 0.f, 1.f);
    Motors[1].ThrustOutput = FMath::Clamp(FR, 0.f, 1.f);
    Motors[2].ThrustOutput = FMath::Clamp(BL, 0.f, 1.f);
    Motors[3].ThrustOutput = FMath::Clamp(BR, 0.f, 1.f);

    /*UE_LOG(LogTemp, Warning, TEXT("FL=%.3f FR=%.3f BL=%.3f BR=%.3f"),
        Motors[0].ThrustOutput, Motors[1].ThrustOutput,
        Motors[2].ThrustOutput, Motors[3].ThrustOutput);
    UE_LOG(LogTemp, Warning, TEXT("PitchCmd=%.3f RollCmd=%.3f YawCmd=%.3f"),
        PitchCmd, RollCmd, YawCmd);
    UE_LOG(LogTemp, Warning, TEXT("AngVel Local: X=%.2f Y=%.2f Z=%.2f"),
        LocalAngVel.X, LocalAngVel.Y, LocalAngVel.Z);
    UE_LOG(LogTemp, Warning, TEXT("INPUT Pitch=%.2f Roll=%.2f Yaw=%.2f Throttle=%.2f"),
        GetPitchInput(), GetRollInput(), YawInput, Throttle);*/
    /*UE_LOG(LogTemp, Warning, TEXT("TargetYaw=%.2f CurrentYaw=%.2f YawCmd=%.3f"),
        TargetYawRate, CurrentYawRate, YawCmd);*/
}


void AFPVDronePawn::ApplyMotorForces()
{
    UStaticMeshComponent* Mesh = GetPlaneMesh();
    if (!Mesh || Motors.Num() != 4) return;

    const FTransform MeshTransform = Mesh->GetComponentTransform();
    const FVector UpVector = MeshTransform.GetUnitAxis(EAxis::Z);

    float TotalYawTorque = 0.f;

    for (const FMotorState& Motor : Motors)
    {
        const float ThrustN = Motor.ThrustOutput * MaxMotorThrust;

        // сила
        const FVector Force = UpVector * (ThrustN * 100.f);
        const FVector WorldLocation = MeshTransform.TransformPosition(Motor.LocalPosition);

        Mesh->AddForceAtLocation(Force, WorldLocation);

        // Ќј ќѕ»„”™ћќ yaw
        TotalYawTorque += ThrustN * Motor.SpinDirection * MotorYawTorquePerNewton;
    }

    // застосовуЇмо один раз
    const FVector LocalTorque(0.f, 0.f, TotalYawTorque * 100.f);
    const FVector WorldTorque = MeshTransform.TransformVectorNoScale(LocalTorque);

    /*Mesh->AddTorqueInRadians(WorldTorque);*/
    /*if (YawInput != 0.f)
    {
        Mesh->AddTorqueInRadians(FVector(0, 0, 50000.f * YawInput));
    }*/
}