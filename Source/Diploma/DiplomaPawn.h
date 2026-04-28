// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "DiplomaPawn.generated.h"

class UMaterialInstanceDynamic;

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

	UPROPERTY(BlueprintReadOnly)
	float HeadingDeg = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float FlightTimeSeconds = 0.f;

	UPROPERTY(BlueprintReadOnly)
	bool bArmed = true;

	UPROPERTY(BlueprintReadOnly)
	FString FlightMode;

	UPROPERTY(BlueprintReadOnly)
	float PackVoltage = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float CellVoltage = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float ConsumedMah = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float CurrentAmp = 0.f;

	UPROPERTY(BlueprintReadOnly)
	bool bBatteryValid = false;

	UPROPERTY(BlueprintReadOnly)
	float PrimaryLinkPercent = 0.f;

	UPROPERTY(BlueprintReadOnly)
	bool bPrimaryLinkValid = false;

	UPROPERTY(BlueprintReadOnly)
	float VideoLinkPercent = 0.f;

	UPROPERTY(BlueprintReadOnly)
	bool bVideoLinkValid = false;

	UPROPERTY(BlueprintReadOnly)
	float TxPowerW = 0.f;

	UPROPERTY(BlueprintReadOnly)
	bool bTxPowerValid = false;

	UPROPERTY(BlueprintReadOnly)
	bool bBombArmed = false;

	UPROPERTY(BlueprintReadOnly)
	float ControlRSSIPercent = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float ControlLQPercent = 0.f;

	UPROPERTY(BlueprintReadOnly)
	bool bControlLinkValid = false;

	UPROPERTY(BlueprintReadOnly)
	float ControlInputScale = 1.f;

	UPROPERTY(BlueprintReadOnly)
	bool bControlFailsafeActive = false;
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

	float GetReceivedThrottle() const { return ReceivedThrottle; }
	float GetReceivedPitchInput() const { return ReceivedPitchInput; }
	float GetReceivedRollInput() const { return ReceivedRollInput; }
	float GetReceivedYawInput() const { return ReceivedYawInput; }
	float GetControlInputScale() const { return ControlInputScale; }
	bool IsControlFailsafeActive() const { return bControlFailsafeActive; }

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

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	FVector OperatorLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	float ControlFrequencyMHz = 915.f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	float VideoFrequencyMHz = 5800.f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	float ControlMaxRangeM = 300.f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	float VideoCleanRangeM = 4500.f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	float VideoUsableRangeM = 10000.f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	float VideoMaxRangeM = 18000.f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	float VideoTxPowerW = 2.5f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	float VideoAntennaGainDbi = 3.5f;

	float SmoothedControlRSSI = 100.f;
	float SmoothedControlLQ = 100.f;
	float SmoothedVideoLink = 100.f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	float SignalSmoothingSpeed = 3.f;
	float LastDeltaSeconds = 0.f;

	float ReceivedThrottle = 0.f;
	float ReceivedPitchInput = 0.f;
	float ReceivedRollInput = 0.f;
	float ReceivedYawInput = 0.f;

	float ControlInputScale = 1.f;
	bool bControlFailsafeActive = false;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	float ControlDegradedLQ = 60.f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	float ControlCriticalLQ = 25.f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	float ControlFailsafeLQ = 10.f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	float ControlCriticalInputScale = 0.35f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	float ControlMinimumInputScale = 0.15f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	float ControlFailsafeEnterDelay = 0.5f;

	UPROPERTY(EditAnywhere, Category = "UAV|Signal")
	float ControlFailsafeRecoverLQ = 25.f;

	float ControlFailsafeTimer = 0.f;

	//Interference
	

	UPROPERTY()
	UMaterialInstanceDynamic* FPVPostProcessMID = nullptr;

	void UpdateSignalTelemetry(float DeltaTime);
	float ComputeOperatorObstructionFactor() const;

	void UpdateReceivedControlInput();

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

	float TelemetryStartTimeSeconds = 0.f;
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
	virtual void UpdateTelemetry();
};
