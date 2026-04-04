
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

    // Pitch і Roll — динаміка однакова на симетричному квадрокоптері
    PitchPID.P = 0.08f;   // менш агресивно ніж 0.3
    PitchPID.I = 0.02f;   // компенсує зміщення центру мас
    PitchPID.D = 0.003f;  // гасить перестріл
    PitchPID.IntegralClamp = 0.3f;

    RollPID.P = 0.08f;
    RollPID.I = 0.02f;
    RollPID.D = 0.003f;
    RollPID.IntegralClamp = 0.3f;

    // Yaw — інерція по Z більша, реакція повільніша
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

    PlaneMesh->SetLinearDamping(0.f);
    PlaneMesh->SetAngularDamping(0.2f);
    PlaneMesh->SetMassOverrideInKg(NAME_None, 4.5f, true);
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
    //ApplyAerodynamicDrag();
}
void AFPVDronePawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    UpdateMotorThrusts(DeltaSeconds);
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
        Motor.ThrustOutput = 0.f;
    }
}

void AFPVDronePawn::UpdateMotorThrusts(float DeltaTime)
{
    const float BaseThrottle = FMath::Clamp(Throttle, 0.f, 1.f);

    const float PitchCmd = GetPitchInput() * 0.05f;
    const float RollCmd = -GetRollInput() * 0.05f;
    const float YawCmd = GetYawInput() * 0.03f;

    const float FL = BaseThrottle - PitchCmd - RollCmd - YawCmd * Motors[0].SpinDirection;
    const float FR = BaseThrottle - PitchCmd + RollCmd - YawCmd * Motors[1].SpinDirection;
    const float BL = BaseThrottle + PitchCmd - RollCmd - YawCmd * Motors[2].SpinDirection;
    const float BR = BaseThrottle + PitchCmd + RollCmd - YawCmd * Motors[3].SpinDirection;

    Motors[0].ThrustOutput = FMath::Clamp(FL, 0.f, 1.f);
    Motors[1].ThrustOutput = FMath::Clamp(FR, 0.f, 1.f);
    Motors[2].ThrustOutput = FMath::Clamp(BL, 0.f, 1.f);
    Motors[3].ThrustOutput = FMath::Clamp(BR, 0.f, 1.f);
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

    for (const FMotorState& Motor : Motors)
    {
        const float ThrustN = Motor.ThrustOutput * MaxMotorThrust;
        const FVector Force = UpVector * (ThrustN * 100.f);
        const FVector WorldLocation = MeshTransform.TransformPosition(Motor.LocalPosition);

        Mesh->AddForceAtLocation(Force, WorldLocation);
    }
}


void AFPVDronePawn::ApplyAerodynamicDrag()
{
    UStaticMeshComponent* Mesh = GetPlaneMesh();
    if (!Mesh) return;

    const FVector WorldVel = Mesh->GetPhysicsLinearVelocity();
    if (WorldVel.SizeSquared() < 1.f) return;
    const FTransform MeshTransform = Mesh->GetComponentTransform();
    const FVector LocalVel = MeshTransform.InverseTransformVectorNoScale(WorldVel);
    const FVector LocalDrag(
        -FMath::Sign(LocalVel.X) * DragCoeffHorizontal * LocalVel.X * LocalVel.X,
        -FMath::Sign(LocalVel.Y) * DragCoeffHorizontal * LocalVel.Y * LocalVel.Y,
        -FMath::Sign(LocalVel.Z) * DragCoeffVertical * LocalVel.Z * LocalVel.Z
    );
    const FVector WorldDrag = MeshTransform.TransformVectorNoScale(LocalDrag);
    Mesh->AddForce(WorldDrag);
    
}