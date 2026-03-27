
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
    PitchPID.P = 0.3f;
    PitchPID.I = 0.f;
    PitchPID.D = 0.f;

    RollPID.P = 0.3f;
    RollPID.I = 0.f;
    RollPID.D = 0.f;

    YawPID.P = 0.2f;
    YawPID.I = 0.f;
    YawPID.D = 0.f;     // було 0.015


}
void AFPVDronePawn::BeginPlay()
{
    Super::BeginPlay();
    InitMotors();
    PlaneMesh->SetCenterOfMass(FVector(15.76f, 0.f, -7.05f), NAME_None);
    PlaneMesh->SetMassOverrideInKg(NAME_None, 4.5f, true);
    PlaneMesh->BodyInstance.InertiaTensorScale = FVector(0.1f, 0.1f, 0.1f);

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
    UE_LOG(LogTemp, Warning, TEXT("INPUT Pitch=%.2f Roll=%.2f Yaw=%.2f Throttle=%.2f"),
        GetPitchInput(), GetRollInput(), YawInput, Throttle);
}

//void AFPVDronePawn::ApplyMotorForces()
//{
//    UStaticMeshComponent* Mesh = GetPlaneMesh();
//    if (!Mesh || Motors.Num() != 4)
//    {
//        return;
//    }
//
//    const FVector UpVector = Mesh->GetComponentQuat().GetUpVector();
//
//    for (const FMotorState& Motor : Motors)
//    {
//        const float ForceMagnitudeN = Motor.ThrustOutput * MaxMotorThrust;
//        const FVector Force = Mesh->GetComponentTransform().TransformVectorNoScale(
//            FVector(0.f, 0.f, ForceMagnitudeN * 100.f)
//        );
//
//        const FVector WorldLocation = Mesh->GetComponentTransform().TransformPosition(Motor.LocalPosition);
//
//        Mesh->AddForceAtLocation(Force, WorldLocation);
//
//        const FVector ReactiveTorque = UpVector * (ForceMagnitudeN * MotorTorqueCoeff);
//        Mesh->AddTorqueInRadians(ReactiveTorque * Motor.SpinDirection);
//    }
//}

void AFPVDronePawn::ApplyMotorForces()
{
    UStaticMeshComponent* Mesh = GetPlaneMesh();
    if (!Mesh || Motors.Num() != 4)
    {
        return;
    }

    const FVector UpVector = Mesh->GetComponentQuat().GetUpVector();
    const FVector COM = Mesh->GetCenterOfMass();

    UE_LOG(LogTemp, Warning, TEXT("=== FRAME ==="));
    UE_LOG(LogTemp, Warning, TEXT("COM=(%.6f %.6f %.6f)"), COM.X, COM.Y, COM.Z);

    for (int i = 0; i < Motors.Num(); ++i)
    {
        const FMotorState& Motor = Motors[i];

        const float ForceMagnitudeN = Motor.ThrustOutput * MaxMotorThrust;
        const FVector Force = UpVector * (ForceMagnitudeN * 100.f);

        const FVector WorldLocation =
            Mesh->GetComponentTransform().TransformPosition(Motor.LocalPosition);

        Mesh->AddForceAtLocation(Force, WorldLocation);

        const FVector ReactiveTorque = UpVector * (ForceMagnitudeN * MotorTorqueCoeff);
        Mesh->AddTorqueInRadians(ReactiveTorque * Motor.SpinDirection);

        const FVector R = WorldLocation - COM;
        const FVector TorqueFromForce = FVector::CrossProduct(R, Force);

        UE_LOG(LogTemp, Warning, TEXT("--- Motor %d ---"), i);
        UE_LOG(LogTemp, Warning, TEXT("LocalPos=(%.2f %.2f %.2f)"),
            Motor.LocalPosition.X, Motor.LocalPosition.Y, Motor.LocalPosition.Z);

        UE_LOG(LogTemp, Warning, TEXT("WorldLocation=(%.6f %.6f %.6f)"),
            WorldLocation.X, WorldLocation.Y, WorldLocation.Z);

        UE_LOG(LogTemp, Warning, TEXT("R=(%.6f %.6f %.6f)"),
            R.X, R.Y, R.Z);

        UE_LOG(LogTemp, Warning, TEXT("Force=(%.6f %.6f %.6f) | |F|=%.2f"),
            Force.X, Force.Y, Force.Z, Force.Size());

        UE_LOG(LogTemp, Warning, TEXT("TorqueFromForce=(%.6f %.6f %.6f)"),
            TorqueFromForce.X, TorqueFromForce.Y, TorqueFromForce.Z);
    }
}