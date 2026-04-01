#pragma once

#include "CoreMinimal.h"
#include "DiplomaPawn.h"
#include "FPVDronePawn.generated.h"

USTRUCT(BlueprintType)
struct FPIDController
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere) float P = 1.f;
    UPROPERTY(EditAnywhere) float I = 0.f;
    UPROPERTY(EditAnywhere) float D = 0.f;

    float Integral = 0.f;
    float PrevError = 0.f;

    UPROPERTY(EditAnywhere) float IntegralClamp = 1.f;

    float Update(float Target, float Current, float DeltaTime)
    {
        const float Error = Target - Current;
        Integral = FMath::Clamp(Integral + Error * DeltaTime, -IntegralClamp, IntegralClamp);
        const float Derivative = (Error - PrevError) / DeltaTime;
        PrevError = Error;
        return P * Error + I * Integral + D * Derivative;
    }

    void Reset()
    {
        Integral = 0.f;
        PrevError = 0.f;
    }
};

USTRUCT(BlueprintType)
struct FMotorState
{
    GENERATED_BODY()

    // Позиція мотора відносно центру мас (у локальних координатах)
    UPROPERTY(EditAnywhere) FVector LocalPosition = FVector::ZeroVector;

    // Напрямок обертання: +1 CCW, -1 CW (впливає на yaw torque)
    UPROPERTY(EditAnywhere) float SpinDirection = 1.f;

    // Поточна тяга мотора (0..1)
    float ThrustOutput = 0.f;
};

UCLASS()
class AFPVDronePawn : public ADiplomaPawn
{
    GENERATED_BODY()

public:
    AFPVDronePawn();

    virtual void Tick(float DeltaSeconds) override;
    virtual void BeginPlay() override;

protected:
    virtual void ApplyThrust() override;
    //virtual void ApplyTorques() override;

private:

    UPROPERTY(EditAnywhere, Category = "FPV|Physics")
    float DragCoeffHorizontal = 0.0014f;  // лобовий опір (XY в локальних)

    UPROPERTY(EditAnywhere, Category = "FPV|Physics")
    float DragCoeffVertical = 0.005f;     // дисковий опір (Z в локальних)

    UPROPERTY(EditAnywhere, Category = "FPV|Physics")
    float AngularDragCoeff = 500.f;

    // Мотори: FL, FR, BL, BR
    UPROPERTY(EditAnywhere, Category = "FPV|Motors")
    TArray<FMotorState> Motors;

    // Відстань від центру до мотора по X і Y (половина wheelbase)
    UPROPERTY(EditAnywhere, Category = "FPV|Motors")
    float ArmLength = 13.f; // 130mm → 13cm → в Unreal units (см)

    // Максимальна тяга одного мотора в Ньютонах
    UPROPERTY(EditAnywhere, Category = "FPV|Motors")
    float MaxMotorThrust = 20.f;

    // Коефіцієнт реактивного торку мотора (yaw від різниці CW/CCW)
    UPROPERTY(EditAnywhere, Category = "FPV|Motors")
    float MotorYawTorquePerNewton;

    // Цільові кутові швидкості (deg/s) — задаються стіком
    UPROPERTY(EditAnywhere, Category = "FPV|Rates")
    float MaxPitchRate = 360.f;

    UPROPERTY(EditAnywhere, Category = "FPV|Rates")
    float MaxRollRate = 360.f;

    UPROPERTY(EditAnywhere, Category = "FPV|Rates")
    float MaxYawRate = 180.f;

    // PID контролери
    UPROPERTY(EditAnywhere, Category = "FPV|PID")
    FPIDController PitchPID;

    UPROPERTY(EditAnywhere, Category = "FPV|PID")
    FPIDController RollPID;

    UPROPERTY(EditAnywhere, Category = "FPV|PID")
    FPIDController YawPID;

    void InitMotors();
    void UpdateMotorThrusts(float DeltaTime);
    void ApplyMotorForces();
    void ApplyAerodynamicDrag();
};