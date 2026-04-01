// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "DiplomaPawn.generated.h"

USTRUCT(BlueprintType)
struct FDroneTelemetry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	float Throttle01 = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float ThrottlePercent = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float SpeedMps = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float SpeedKmh = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float VerticalSpeedMps = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float BaroAltitudeM = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float RelativeAltitudeM = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float RadioAltitudeM = 0.f;

	UPROPERTY(BlueprintReadOnly)
	bool bRadioAltitudeValid = false;

	UPROPERTY(BlueprintReadOnly)
	float PitchDeg = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float RollDeg = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float YawDeg = 0.f;
};

UCLASS(Config=Game)
class ADiplomaPawn : public APawn
{
	GENERATED_BODY()

	
public:
	ADiplomaPawn();

	// Begin AActor overrides
	virtual void Tick(float DeltaSeconds) override;
	virtual void NotifyHit(class UPrimitiveComponent* MyComp, class AActor* Other, class UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit) override;
	// End AActor overrides

	float GetRollInput() const { return RollInput; }
	float GetPitchInput() const { return PitchInput; }
	float GetYawInput() const { return YawInput; }

	const FDroneTelemetry& GetTelemetry() const { return Telemetry; }

protected:

	/** StaticMesh component that will be the visuals for our flying pawn */
	UPROPERTY(Category = Mesh, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UStaticMeshComponent* PlaneMesh;

	/** Camera component that will be our viewpoint */
	UPROPERTY(Category = Camera, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* Camera;

	//Telemetry
	UPROPERTY(BlueprintReadOnly)
	FDroneTelemetry Telemetry;

	// Begin APawn overrides
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override; // Allows binding actions/axes to functions
	// End APawn overrides

	/** Input Controls */
	void ThrustInput(float Val);
	void PitchInputAxis(float Value);
	void RollInputAxis(float Value);
	void YawInputAxis(float Value);

	/** Apply functions */
	void ThrottleInput(float Value);
	virtual void ApplyThrust();
	virtual void ApplyTorques();


	float Throttle;
	float PitchInput;
	float RollInput;
	float YawInput;

	float BaroZeroZ = 0.f;

	UPROPERTY(EditAnywhere, Category = "UAV|Physics")
	float MaxThrust;

	UPROPERTY(EditAnywhere, Category = "UAV|Physics")
	float ThrustScale;

	UPROPERTY(EditAnywhere, Category = "UAV|Physics")
	float PitchTorque;

	UPROPERTY(EditAnywhere, Category = "UAV|Physics")
	float RollTorque;

	UPROPERTY(EditAnywhere, Category = "UAV|Physics")
	float YawTorque;

	//Mouse Joystick
	UPROPERTY(EditAnywhere, Category = "UAV|MouseJoystick")
	float MouseRange=400;

	UPROPERTY(EditAnywhere, Category = "UAV|MouseJoystick")
	float MouseDeadZone=0.05f;

	bool bMouseJoystickEnabled;

	float AccumulatedX = 0.f;
	float AccumulatedY = 0.f;
	UPROPERTY(EditAnywhere, Category = "UAV|MouseJoystick")
	float MouseSensitivity = 0.05f;

	UPROPERTY(EditAnywhere, Category = "UAV|MouseJoystick")
	float MouseSmoothing = 5.f;

	float NormalizeThrottle(float Raw) const;
	float NormalizeCenteredAxis(float Raw) const;

	//Reset After Crash
	bool bCrashed = false;
	float CrashTimer = 0.f;
	float CrashRespawnDelay = 2.f;
	FVector SpawnLocation;
	FRotator SpawnRotation;
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, FVector NormalImpulse,
		const FHitResult& Hit);



private:

	float GetRadioAltitudeMeters(bool& bValid) const;


public:

	FORCEINLINE class UCameraComponent* GetCamera() const { return Camera; }
	FORCEINLINE class UStaticMeshComponent* GetPlaneMesh() const { return PlaneMesh; }
	virtual void BeginPlay() override;

	void UpdateMouseJoystick();
	void ToggleMouseJoystick();
	void CenterMouseCursor();
	void UpdateTelemetry();
};
