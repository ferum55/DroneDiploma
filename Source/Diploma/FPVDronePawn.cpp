
#include "FPVDronePawn.h"
#include "Components/StaticMeshComponent.h"

AFPVDronePawn::AFPVDronePawn()
{
    ArmLength = 20.f;

    MaxMotorThrust = 38.f;
    MotorTorqueCoeff = 0.12f;

    MaxPitchRate = 180.f;  // було 220
    MaxRollRate = 180.f;  // було 220
    MaxYawRate = 90.f;   // було 120

    // В конструкторі
    PitchPID.P = 0.03f;
    PitchPID.I = 0.f;
    PitchPID.D = 0.f;

    RollPID.P = 0.03f;
    RollPID.I = 0.f;
    RollPID.D = 0.f;

    YawPID.P = 0.02f;
    YawPID.I = 0.f;
    YawPID.D = 0.f;     // було 0.015

    InitMotors();
}
void AFPVDronePawn::BeginPlay()
{
    Super::BeginPlay();
    PlaneMesh->SetCenterOfMass(FVector(15.76f, 0.f, -7.05f), NAME_None);
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
}

void AFPVDronePawn::InitMotors()
{
    Motors.SetNum(4);

    Motors[0].LocalPosition = FVector(ArmLength, ArmLength, 0.f);
    Motors[0].SpinDirection = 1.f;

    Motors[1].LocalPosition = FVector(ArmLength, -ArmLength, 0.f);
    Motors[1].SpinDirection = -1.f;

    Motors[2].LocalPosition = FVector(-ArmLength, ArmLength, 0.f);
    Motors[2].SpinDirection = -1.f;

    Motors[3].LocalPosition = FVector(-ArmLength, -ArmLength, 0.f);
    Motors[3].SpinDirection = 1.f;

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

    UE_LOG(LogTemp, Warning, TEXT("FL=%.3f FR=%.3f BL=%.3f BR=%.3f"),
        Motors[0].ThrustOutput, Motors[1].ThrustOutput,
        Motors[2].ThrustOutput, Motors[3].ThrustOutput);
    UE_LOG(LogTemp, Warning, TEXT("PitchCmd=%.3f RollCmd=%.3f YawCmd=%.3f"),
        PitchCmd, RollCmd, YawCmd);
    UE_LOG(LogTemp, Warning, TEXT("AngVel Local: X=%.2f Y=%.2f Z=%.2f"),
        LocalAngVel.X, LocalAngVel.Y, LocalAngVel.Z);
}

void AFPVDronePawn::ApplyMotorForces()
{
    UStaticMeshComponent* Mesh = GetPlaneMesh();
    if (!Mesh || Motors.Num() != 4)
    {
        return;
    }

    const FVector UpVector = Mesh->GetUpVector();

    for (const FMotorState& Motor : Motors)
    {
        const float ForceMagnitudeN = Motor.ThrustOutput * MaxMotorThrust;
        const FVector Force = UpVector * (ForceMagnitudeN * 100.f);
        const FVector WorldLocation = Mesh->GetComponentTransform().TransformPosition(Motor.LocalPosition);

        Mesh->AddForceAtLocation(Force, WorldLocation);

        const FVector ReactiveTorque = UpVector * (ForceMagnitudeN * MotorTorqueCoeff);
        Mesh->AddTorqueInRadians(ReactiveTorque * Motor.SpinDirection);
    }
}