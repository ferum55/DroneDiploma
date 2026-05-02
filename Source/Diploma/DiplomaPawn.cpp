// Copyright Epic Games, Inc. All Rights Reserved.

#include "DiplomaPawn.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InputComponent.h"

#include "Engine/World.h"
#include "Engine/StaticMesh.h"


#include "GameFramework/InputSettings.h"
#include "Engine/Engine.h"

#include "Camera/CameraComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/EngineTypes.h"

#include "Particles/ParticleSystemComponent.h"
#include "Kismet/GameplayStatics.h"


ADiplomaPawn::ADiplomaPawn()
{
	bMouseJoystickEnabled = false;
	// Structure to hold one-time initialization
	struct FConstructorStatics
	{
		ConstructorHelpers::FObjectFinderOptional<UStaticMesh> PlaneMesh;
		FConstructorStatics()
			: PlaneMesh(TEXT("/Game/Flying/Meshes/UFO.UFO"))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;

	// Create static mesh component
	PlaneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaneMesh0"));
	PlaneMesh->SetStaticMesh(ConstructorStatics.PlaneMesh.Get());	// Set static mesh
	RootComponent = PlaneMesh;

	//Unsafe
	PlaneMesh->SetNotifyRigidBodyCollision(true);

	PlaneMesh->SetSimulatePhysics(true);
	PlaneMesh->SetEnableGravity(true);
	//PlaneMesh->SetLinearDamping(0.05f);
	//PlaneMesh->SetAngularDamping(0.1f);
	PlaneMesh->SetMassOverrideInKg(NAME_None, 4.5f);

	PlaneMesh->SetCollisionProfileName(TEXT("PhysicsActor"));
	PlaneMesh->SetCenterOfMass(FVector::ZeroVector);



	// Create camera component 
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera0"));
	Camera->SetupAttachment(PlaneMesh);

	Camera->bUsePawnControlRotation = false; // Don't rotate camera with controller
	
	Camera->SetRelativeRotation(FRotator::ZeroRotator);
	Camera->FieldOfView = 100.f;
	Camera->SetRelativeLocation(FVector(0.f, 0.f, 0.f));

	KillCamCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("KillCamCamera0"));
	KillCamCamera->SetupAttachment(RootComponent);
	KillCamCamera->bUsePawnControlRotation = false;
	KillCamCamera->SetActive(false);

	if (Camera)
	{
		if (Camera->PostProcessSettings.WeightedBlendables.Array.Num() > 0)
		{
			auto& Blendable = Camera->PostProcessSettings.WeightedBlendables.Array[0];

			if (Blendable.Object)
			{
				FPVPostProcessMID = UMaterialInstanceDynamic::Create(
					Cast<UMaterialInterface>(Blendable.Object),
					this
				);

				Blendable.Object = FPVPostProcessMID;
			}
		}
	}


	Throttle = 0.f;
	PitchInput = 0.f;
	RollInput = 0.f;
	YawInput = 0.f;

	MaxThrust = 2000.f;
	ThrustScale = 1.f;
	PitchTorque = 1500.f;
	RollTorque = 1500.f;
	YawTorque = 800.f;

	CrashTimer = 3.f;


}

void ADiplomaPawn::BeginPlay()
{
	Super::BeginPlay();
	if (GEngine)
	{
		GEngine->Exec(GetWorld(), TEXT("stat fps"));
		GEngine->Exec(GetWorld(), TEXT("stat unit"));
	}

	BaroZeroZ = GetActorLocation().Z;
	OperatorLocation = GetActorLocation();
	
	TelemetryStartTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	PlaneMesh->SetCenterOfMass(FVector::ZeroVector, NAME_None);

	SpawnLocation = GetActorLocation();
	SpawnRotation = GetActorRotation();
	LastSpawnWorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	PlaneMesh->OnComponentHit.AddDynamic(this, &ADiplomaPawn::OnHit);

	UE_LOG(LogTemp, Warning, TEXT("Actor: %s"), *GetName());
	UE_LOG(LogTemp, Warning, TEXT("PlaneMesh Rel: %s  World: %s"),
		*PlaneMesh->GetRelativeLocation().ToString(),
		*PlaneMesh->GetComponentLocation().ToString());

	UE_LOG(LogTemp, Warning, TEXT("Camera Rel: %s  World: %s  AttachParent: %s"),
		*Camera->GetRelativeLocation().ToString(),
		*Camera->GetComponentLocation().ToString(),
		Camera->GetAttachParent() ? *Camera->GetAttachParent()->GetName() : TEXT("None"));

	


	const UInputSettings* Settings = GetDefault<UInputSettings>();
	if (!Settings)
	{
		UE_LOG(LogTemp, Error, TEXT("InputSettings not found"));
		return;
	}

	const TArray<FInputActionKeyMapping>& Actions = Settings->GetActionMappings();

	int32 Found = 0;
	for (const FInputActionKeyMapping& A : Actions)
	{
		if (A.ActionName == "MouseJoystickToggle")
		{
			UE_LOG(LogTemp, Warning, TEXT("MouseJoystickToggle bound to key: %s"), *A.Key.ToString());
			Found++;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("MouseJoystickToggle mappings found: %d"), Found);

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->bShowMouseCursor = false;
	}
	if (!Camera)
	{
		UE_LOG(LogTemp, Error, TEXT("PP | Camera is null"));
	}
	else
	{
		const int32 Count = Camera->PostProcessSettings.WeightedBlendables.Array.Num();

		UE_LOG(LogTemp, Warning, TEXT("PP | Blendables count: %d"), Count);

		if (Count > 0)
		{
			FWeightedBlendable& Blendable = Camera->PostProcessSettings.WeightedBlendables.Array[0];
			UObject* Obj = Blendable.Object;

			UE_LOG(LogTemp, Warning,
				TEXT("PP | Blendable[0]: %s | Class: %s"),
				Obj ? *Obj->GetName() : TEXT("None"),
				Obj ? *Obj->GetClass()->GetName() : TEXT("None")
			);

			UMaterialInterface* MaterialInterface = Cast<UMaterialInterface>(Obj);

			if (MaterialInterface)
			{
				FPVPostProcessMID = UMaterialInstanceDynamic::Create(MaterialInterface, this);
				Blendable.Object = FPVPostProcessMID;
			}
		}

		UE_LOG(LogTemp, Warning,
			TEXT("PP | MID created: %s"),
			FPVPostProcessMID ? TEXT("YES") : TEXT("NO")
		);
	}
}


void ADiplomaPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bCrashed)
	{
		if (bKillCamActive)
		{
			UpdateKillCamReplay(DeltaSeconds);
		}
		return;
	}

	RecordKillCamFrame();

	UpdateMouseJoystick();

	float RawThrottle = GetInputAxisValue(TEXT("TestAxis4"));
	float RawPitch = GetInputAxisValue(TEXT("TestAxis5"));
	float RawRoll = GetInputAxisValue(TEXT("TestAxis3"));
	float RawYaw = GetInputAxisValue(TEXT("TestAxis6"));

	float NormThrottle = NormalizeThrottle(RawThrottle);
	float NormPitch = NormalizeCenteredAxis(RawPitch);
	float NormRoll = NormalizeCenteredAxis(RawRoll);
	float NormYaw = NormalizeCenteredAxis(RawYaw);
	LastDeltaSeconds = DeltaSeconds;
}

void ADiplomaPawn::NotifyHit(UPrimitiveComponent* MyComp, AActor* Other,
	UPrimitiveComponent* OtherComp, bool bSelfMoved,
	FVector HitLocation, FVector HitNormal,
	FVector NormalImpulse, const FHitResult& Hit)
{
	Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved,
		HitLocation, HitNormal, NormalImpulse, Hit);

	if (ShouldIgnoreCrashHit(NormalImpulse))
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("CRASH NOTIFY HIT | Other=%s | Location=%s | Impulse=%s"),
		Other ? *Other->GetName() : TEXT("None"),
		*HitLocation.ToString(),
		*NormalImpulse.ToString()
	);

	HandleCrash(HitLocation);
}

void ADiplomaPawn::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	const FVector HitLocation = Hit.ImpactPoint.IsNearlyZero() ? Hit.Location : Hit.ImpactPoint;

	if (ShouldIgnoreCrashHit(NormalImpulse))
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("CRASH COMPONENT HIT | Other=%s | Impulse=%s | ImpactPoint=%s"),
		OtherActor ? *OtherActor->GetName() : TEXT("None"),
		*NormalImpulse.ToString(),
		*HitLocation.ToString()
	);

	HandleCrash(HitLocation);
}


void ADiplomaPawn::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
    // Check if PlayerInputComponent is valid (not NULL)
	check(PlayerInputComponent);
	UE_LOG(LogTemp, Warning, TEXT("SetupPlayerInputComponent called for %s"), *GetName());

	PlayerInputComponent->BindAxis("Throttle", this, &ADiplomaPawn::ThrottleInput);
	PlayerInputComponent->BindAxis("Pitch", this, &ADiplomaPawn::PitchInputAxis);
	PlayerInputComponent->BindAxis("Roll", this, &ADiplomaPawn::RollInputAxis);
	PlayerInputComponent->BindAxis("Yaw", this, &ADiplomaPawn::YawInputAxis);
	PlayerInputComponent->BindAction(
		"MouseJoystickToggle",
		IE_Pressed,
		this,
		&ADiplomaPawn::ToggleMouseJoystick
	);

	PlayerInputComponent->BindAction("Arm", IE_Pressed, this, &ADiplomaPawn::ToggleArm);
	PlayerInputComponent->BindAction("BombArm", IE_Pressed, this, &ADiplomaPawn::ToggleBombArm);


	PlayerInputComponent->BindAxis("TestAxis1");
	PlayerInputComponent->BindAxis("TestAxis2");
	PlayerInputComponent->BindAxis("TestAxis3");
	PlayerInputComponent->BindAxis("TestAxis4");
	PlayerInputComponent->BindAxis("TestAxis5");
	PlayerInputComponent->BindAxis("TestAxis6");
	PlayerInputComponent->BindAxis("TestAxis7");
	PlayerInputComponent->BindAxis("TestAxis8");


}

void ADiplomaPawn::ToggleArm()
{
	if (bCrashed || bKillCamActive)
	{
		return;
	}

	bArmedState = !bArmedState;

	if (!bArmedState)
	{
		Throttle = 0.f;
		ReceivedThrottle = 0.f;
		ReceivedPitchInput = 0.f;
		ReceivedRollInput = 0.f;
		ReceivedYawInput = 0.f;
	}

	UE_LOG(LogTemp, Warning, TEXT("Armed: %d"), bArmedState ? 1 : 0);
}

void ADiplomaPawn::ToggleBombArm()
{
	if (bCrashed || bKillCamActive)
	{
		return;
	}

	bBombArmedState = !bBombArmedState;

	UE_LOG(LogTemp, Warning, TEXT("BombArmed: %d"), bBombArmedState ? 1 : 0);
}


void ADiplomaPawn::PitchInputAxis(float Value)
{
	PitchInput = NormalizeCenteredAxis(Value); //NormalizeCenteredAxis(Value, 0.654f);
}

void ADiplomaPawn::RollInputAxis(float Value)
{
	RollInput =NormalizeCenteredAxis(Value); //NormalizeCenteredAxis(Value, 0.654f);
}

void ADiplomaPawn::YawInputAxis(float Value)
{
	YawInput = NormalizeCenteredAxis(Value);//NormalizeCenteredAxis(Value, 0.665f);
}


void ADiplomaPawn::ThrottleInput(float Value)
{
	//UE_LOG(LogTemp, Warning, TEXT("Throttle axis value: %.3f"), Value);

	Throttle = NormalizeThrottle(Value);
	/*UE_LOG(LogTemp, Warning, TEXT("Throttle state: %.3f"), Throttle);*/
}

void ADiplomaPawn::ApplyThrust()
{
	if (!PlaneMesh) return;

	const FVector UpVector = PlaneMesh->GetUpVector();
	const float ClampedThrottle = FMath::Clamp(Throttle, 0.f, 1.f);

	const FVector Force =
		UpVector * (ClampedThrottle * MaxThrust * ThrustScale);

	PlaneMesh->AddForce(Force);
}

void ADiplomaPawn::ApplyTorques()
{
	if (!PlaneMesh) return;

	const FVector Torque(
		-RollInput * RollTorque,
		-PitchInput * PitchTorque,
		YawInput * YawTorque
	);

	PlaneMesh->AddTorqueInRadians(
		PlaneMesh->GetComponentTransform().TransformVectorNoScale(Torque)
	);
}

void ADiplomaPawn::UpdateMouseJoystick()
{
	if (!bMouseJoystickEnabled)
		return;

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	float MouseX, MouseY;
	if (!PC->GetMousePosition(MouseX, MouseY)) return;

	int32 SizeX, SizeY;
	PC->GetViewportSize(SizeX, SizeY);

	const float CenterX = SizeX * 0.5f;
	const float CenterY = SizeY * 0.5f;
	const float Half = SizeY * 0.5f - 10.f - 40.f;

	float dx = FMath::Clamp((MouseX - CenterX) / Half, -1.f, 1.f);
	float dy = FMath::Clamp((MouseY - CenterY) / Half, -1.f, 1.f);

	dx = FMath::Clamp(dx, -1.f, 1.f);
	dy = FMath::Clamp(dy, -1.f, 1.f);

	if (FMath::Abs(dx) < MouseDeadZone) dx = 0.f;
	if (FMath::Abs(dy) < MouseDeadZone) dy = 0.f;

	RollInput = dx;
	PitchInput = dy;
}

void ADiplomaPawn::ToggleMouseJoystick()
{
	UE_LOG(LogTemp, Warning, TEXT("ToggleMouseJoystick FIRED"));

	bMouseJoystickEnabled = !bMouseJoystickEnabled;
	UE_LOG(LogTemp, Warning, TEXT("MouseJoystickEnabled = %d"), bMouseJoystickEnabled ? 1 : 0);

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	if (bMouseJoystickEnabled)
	{
		CenterMouseCursor();
		AccumulatedX = 0.f;
		AccumulatedY = 0.f;
	}
	else
	{
		PitchInput = 0.f;
		RollInput = 0.f;
	}
}


void ADiplomaPawn::CenterMouseCursor()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	int32 SizeX, SizeY;
	PC->GetViewportSize(SizeX, SizeY);

	const int32 CenterX = SizeX / 2;
	const int32 CenterY = SizeY / 2;

	PC->SetMouseLocation(CenterX, CenterY);
}

void ADiplomaPawn::UpdateTelemetry()
{
	if (!PlaneMesh)
	{
		return;
	}

	const FVector Velocity = PlaneMesh->GetPhysicsLinearVelocity();

	Telemetry.Throttle01 = Throttle;
	Telemetry.ThrottlePercent = Throttle * 100.f;
	Telemetry.SpeedMps = Velocity.Size() / 100.f;
	Telemetry.SpeedKmh = Telemetry.SpeedMps * 3.6f;
	Telemetry.VerticalSpeedMps = Velocity.Z / 100.f;
	Telemetry.BaroAltitudeM = GetActorLocation().Z / 100.f;
	Telemetry.RelativeAltitudeM = (GetActorLocation().Z - BaroZeroZ) / 100.f;

	bool bValid = false;
	Telemetry.RadioAltitudeM = GetRadioAltitudeMeters(bValid);
	Telemetry.bRadioAltitudeValid = bValid;

	const FRotator R = GetActorRotation();
	Telemetry.PitchDeg = R.Pitch;
	Telemetry.RollDeg = R.Roll;
	Telemetry.YawDeg = R.Yaw;
	Telemetry.HeadingDeg = FRotator::ClampAxis(R.Yaw);

	Telemetry.FlightTimeSeconds = GetWorld()
		? (GetWorld()->GetTimeSeconds() - TelemetryStartTimeSeconds)
		: 0.f;

	Telemetry.bArmed = bArmedState;
	Telemetry.FlightMode = TEXT("");

	Telemetry.PackVoltage = 0.f;
	Telemetry.CellVoltage = 0.f;
	Telemetry.ConsumedMah = 0.f;
	Telemetry.CurrentAmp = 0.f;
	Telemetry.bBatteryValid = false;

	Telemetry.PrimaryLinkPercent = 0.f;
	Telemetry.bPrimaryLinkValid = false;

	Telemetry.VideoLinkPercent = 0.f;
	Telemetry.bVideoLinkValid = false;

	Telemetry.TxPowerW = 0.f;
	Telemetry.bTxPowerValid = false;

	Telemetry.bBombArmed = bBombArmedState;
	Telemetry.bKillCamActive = bKillCamActive;
	Telemetry.bCrashed = bCrashed;

	UpdateSignalTelemetry(LastDeltaSeconds);
}

float ADiplomaPawn::GetRadioAltitudeMeters(bool& bValid) const
{
	if (!GetWorld() || !PlaneMesh)
	{
		bValid = false;
		return 0.f;
	}

	FHitResult Hit;
	const FVector Start = PlaneMesh->GetComponentLocation();
	const FVector End = Start - FVector(0.f, 0.f, 100000.f);

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit,
		Start,
		End,
		ECC_Visibility,
		Params
	);

	bValid = bHit;
	return bHit ? Hit.Distance / 100.f : 0.f;
}

float ADiplomaPawn::NormalizeThrottle(float Raw) const
{
	// Axis4: мінімум=0.0, центр=0.5, максимум=1.0
	// Мапимо [0..1] ? [0..1], центр не має значення для тяги
	const float Min = 0.15f;
	const float Max = 0.85f;
	float Value = (Raw - Min) / (Max - Min);
	Value = FMath::Clamp(Value, 0.f, 1.f);
	if (Value < 0.02f) Value = 0.f;
	return Value;
}

float ADiplomaPawn::NormalizeCenteredAxis(float Raw) const
{
	// Axis: мінімум=0.0, центр=0.5, максимум=1.0
	// Мапимо [0..1] ? [-1..1] відносно центру 0.5
	float Value = (Raw - 0.5f) * 2.0f;
	Value = FMath::Clamp(Value, -1.f, 1.f);
	const float DeadZone = 0.04f;
	if (FMath::Abs(Value) < DeadZone) Value = 0.f;
	return Value;
}

void ADiplomaPawn::UpdateSignalTelemetry(float DeltaTime)
{
	if (!PlaneMesh)
	{
		return;
	}

	const float DistanceM = FVector::Distance(
		PlaneMesh->GetComponentLocation(),
		OperatorLocation
	) / 100.f;

	const float Obstruction = ComputeOperatorObstructionFactor();

	const float ControlAlpha = FMath::Clamp(
		DistanceM / FMath::Max(ControlMaxRangeM, 1.f),
		0.f,
		1.f
	);

	float ControlRSSI = 100.f * (1.f - FMath::Pow(ControlAlpha, 1.35f));
	ControlRSSI -= Obstruction * 20.f;
	ControlRSSI = FMath::Clamp(ControlRSSI, 0.f, 100.f);

	float ControlLQ = 100.f;

	if (ControlRSSI < 45.f)
	{
		ControlLQ = FMath::GetMappedRangeValueClamped(
			FVector2D(10.f, 45.f),
			FVector2D(20.f, 100.f),
			ControlRSSI
		);
	}

	ControlLQ = FMath::GetMappedRangeValueClamped(
		FVector2D(0.f, 70.f),
		FVector2D(0.f, 100.f),
		ControlRSSI
	);

	ControlLQ -= Obstruction * 35.f;
	ControlLQ = FMath::Clamp(ControlLQ, 0.f, 100.f);

	float VideoQuality = 100.f;

	if (DistanceM <= VideoCleanRangeM)
	{
		const float CleanAlpha = FMath::Clamp(
			DistanceM / FMath::Max(VideoCleanRangeM, 1.f),
			0.f,
			1.f
		);

		VideoQuality = FMath::Lerp(100.f, 85.f, FMath::Pow(CleanAlpha, 1.2f));
	}
	else if (DistanceM <= VideoUsableRangeM)
	{
		const float UsableAlpha = FMath::Clamp(
			(DistanceM - VideoCleanRangeM) / FMath::Max(VideoUsableRangeM - VideoCleanRangeM, 1.f),
			0.f,
			1.f
		);

		VideoQuality = FMath::Lerp(85.f, 45.f, FMath::Pow(UsableAlpha, 1.1f));
	}
	else
	{
		const float MaxAlpha = FMath::Clamp(
			(DistanceM - VideoUsableRangeM) / FMath::Max(VideoMaxRangeM - VideoUsableRangeM, 1.f),
			0.f,
			1.f
		);

		VideoQuality = FMath::Lerp(45.f, 0.f, FMath::Pow(MaxAlpha, 0.85f));
	}

	VideoQuality -= Obstruction * 45.f;
	VideoQuality = FMath::Clamp(VideoQuality, 0.f, 100.f);

	SmoothedControlRSSI = FMath::FInterpTo(SmoothedControlRSSI, ControlRSSI, DeltaTime, SignalSmoothingSpeed);
	SmoothedControlLQ = FMath::FInterpTo(SmoothedControlLQ, ControlLQ, DeltaTime, SignalSmoothingSpeed);
	SmoothedVideoLink = FMath::FInterpTo(SmoothedVideoLink, VideoQuality, DeltaTime, SignalSmoothingSpeed);

	Telemetry.ControlRSSIPercent = SmoothedControlRSSI;
	Telemetry.ControlLQPercent = SmoothedControlLQ;
	Telemetry.bControlLinkValid = true;

	Telemetry.PrimaryLinkPercent = SmoothedControlRSSI;
	Telemetry.bPrimaryLinkValid = true;

	Telemetry.VideoLinkPercent = SmoothedVideoLink;
	Telemetry.bVideoLinkValid = true;

	UpdateReceivedControlInput(DeltaTime);

	static float SignalLogTimer = 0.f;
	SignalLogTimer += DeltaTime;

	if (SignalLogTimer >= 0.5f)
	{
		SignalLogTimer = 0.f;

		/*UE_LOG(LogTemp, Warning,
			TEXT("SIGNAL | Dist=%.1fm | Obstruction=%.2f | CTRL RSSI=%.0f LQ=%.0f | InputScale=%.2f | Failsafe=%d | FSTime=%.2f | VIDEO=%.0f"),
			DistanceM,
			Obstruction,
			Telemetry.ControlRSSIPercent,
			Telemetry.ControlLQPercent,
			ControlInputScale,
			bControlFailsafeActive ? 1 : 0,
			ControlFailsafeActiveTime,
			Telemetry.VideoLinkPercent
		);*/
	}
	if (FPVPostProcessMID)
	{
		const float VideoQuality01 = Telemetry.VideoLinkPercent / 100.f;
		FPVPostProcessMID->SetScalarParameterValue(TEXT("VideoQuality"), VideoQuality01);
		static float PPLogTimer = 0.f;
		PPLogTimer += DeltaTime;

		if (PPLogTimer >= 0.5f)
		{
			PPLogTimer = 0.f;

			/*UE_LOG(LogTemp, Warning,
				TEXT("PP | VideoQuality01=%.2f | MID=%s"),
				VideoQuality01,
				FPVPostProcessMID ? TEXT("YES") : TEXT("NO")
			);*/
		}
	}
}

void ADiplomaPawn::UpdateReceivedControlInput(float DeltaTime)
{
	const float SafeDeltaTime = FMath::Max(DeltaTime, 0.f);
	const float LQ = Telemetry.bControlLinkValid ? Telemetry.ControlLQPercent : 100.f;

	if (!bControlFailsafeActive)
	{
		if (LQ <= ControlFailsafeLQ)
		{
			ControlFailsafeTimer += SafeDeltaTime;

			if (ControlFailsafeTimer >= ControlFailsafeEnterDelay)
			{
				bControlFailsafeActive = true;
				ControlFailsafeActiveTime = 0.f;
			}
		}
		else
		{
			ControlFailsafeTimer = 0.f;
		}
	}
	else
	{
		ControlFailsafeActiveTime += SafeDeltaTime;

		if (LQ >= ControlFailsafeRecoverLQ)
		{
			bControlFailsafeActive = false;
			ControlFailsafeTimer = 0.f;
			ControlFailsafeActiveTime = 0.f;
		}
	}

	if (bControlFailsafeActive)
	{
		ControlInputScale = 0.f;

		if (ControlFailsafeActiveTime < ControlFailsafeHoldSeconds)
		{
			ReceivedThrottle = LastValidReceivedThrottle;
			ReceivedPitchInput = LastValidReceivedPitchInput;
			ReceivedRollInput = LastValidReceivedRollInput;
			ReceivedYawInput = LastValidReceivedYawInput;
		}
		else
		{
			ReceivedThrottle = 0.f;
			ReceivedPitchInput = 0.f;
			ReceivedRollInput = 0.f;
			ReceivedYawInput = 0.f;
		}
	}
	else
	{
		if (LQ >= ControlDegradedLQ)
		{
			ControlInputScale = 1.f;
		}
		else if (LQ <= ControlCriticalLQ)
		{
			ControlInputScale = FMath::GetMappedRangeValueClamped(
				FVector2D(ControlFailsafeLQ, ControlCriticalLQ),
				FVector2D(ControlMinimumInputScale, ControlCriticalInputScale),
				LQ
			);
		}
		else
		{
			ControlInputScale = FMath::GetMappedRangeValueClamped(
				FVector2D(ControlCriticalLQ, ControlDegradedLQ),
				FVector2D(ControlCriticalInputScale, 1.f),
				LQ
			);
		}

		ReceivedThrottle = Throttle;
		ReceivedPitchInput = PitchInput * ControlInputScale;
		ReceivedRollInput = RollInput * ControlInputScale;
		ReceivedYawInput = YawInput * ControlInputScale;

		LastValidReceivedThrottle = ReceivedThrottle;
		LastValidReceivedPitchInput = ReceivedPitchInput;
		LastValidReceivedRollInput = ReceivedRollInput;
		LastValidReceivedYawInput = ReceivedYawInput;
	}

	Telemetry.ControlInputScale = ControlInputScale;
	Telemetry.bControlFailsafeActive = bControlFailsafeActive;
}

float ADiplomaPawn::ComputeOperatorObstructionFactor() const
{
	if (!GetWorld() || !PlaneMesh)
	{
		return 1.f;
	}

	const FVector DroneLocation = PlaneMesh->GetComponentLocation();
	const FVector OperatorEyeLocation = OperatorLocation + FVector(0.f, 0.f, 170.f);

	const FVector Direction = (OperatorEyeLocation - DroneLocation).GetSafeNormal();
	const FVector Right = FVector::CrossProduct(Direction, FVector::UpVector).GetSafeNormal();
	const FVector Up = FVector::CrossProduct(Right, Direction).GetSafeNormal();

	const float TraceSpreadCm = 120.f;

	TArray<FVector> Offsets;
	Offsets.Add(FVector::ZeroVector);
	Offsets.Add(Right * TraceSpreadCm);
	Offsets.Add(-Right * TraceSpreadCm);
	Offsets.Add(Up * TraceSpreadCm);
	Offsets.Add(-Up * TraceSpreadCm);

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	int32 BlockedCount = 0;

	for (const FVector& Offset : Offsets)
	{
		FHitResult Hit;

		const FVector Start = DroneLocation + Offset;
		const FVector End = OperatorEyeLocation + Offset;

		const bool bHit = GetWorld()->LineTraceSingleByChannel(
			Hit,
			Start,
			End,
			ECC_Visibility,
			Params
		);

		if (bHit)
		{
			BlockedCount++;
		}
	}

	const float RawObstruction = static_cast<float>(BlockedCount) / static_cast<float>(Offsets.Num());

	return FMath::Clamp(RawObstruction, 0.f, 1.f);
}

void ADiplomaPawn::FindKillCamPosition(const FVector& HitLocation)
{
	if (!GetWorld()) return;

	// Пробуємо 8 напрямків навколо точки зіткнення
	const TArray<FVector> Directions = {
		FVector(1, 0, 0), FVector(-1, 0, 0),
		FVector(0, 1, 0), FVector(0, -1, 0),
		FVector(1, 1, 0).GetSafeNormal(),
		FVector(-1, 1, 0).GetSafeNormal(),
		FVector(1, -1, 0).GetSafeNormal(),
		FVector(-1,-1, 0).GetSafeNormal()
	};

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	FVector BestPos = HitLocation + FVector(0, 0, KillCamDistance);
	float BestDot = -1.f;

	// Беремо вектор польоту дрона до зіткнення
	const FVector DroneForward = GetActorForwardVector();

	for (const FVector& Dir : Directions)
	{
		// Камера збоку-ззаду, трохи вище
		const FVector Offset = Dir * KillCamDistance + FVector(0, 0, KillCamDistance * 0.5f);
		const FVector CandidatePos = HitLocation + Offset;

		FHitResult TraceHit;
		const bool bBlocked = GetWorld()->LineTraceSingleByChannel(
			TraceHit, HitLocation, CandidatePos,
			ECC_Visibility, Params
		);

		if (!bBlocked)
		{
			// Перевагу даємо напрямку збоку від вектора польоту
			const float Dot = FMath::Abs(FVector::DotProduct(Dir, DroneForward));
			// Dot ~0 = перпендикулярно до польоту = добре видно
			const float Score = 1.f - Dot;
			if (Score > BestDot)
			{
				BestDot = Score;
				BestPos = CandidatePos;
			}
		}
	}

	KillCamLocation = BestPos;
}

void ADiplomaPawn::StartKillCam(const FVector& HitLocation)
{
	if (!GetWorld() || !PlaneMesh || !Camera || !KillCamCamera)
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();

	FKillCamFrame ImpactFrame;
	ImpactFrame.Time = Now;
	ImpactFrame.Location = PlaneMesh->GetComponentLocation();
	ImpactFrame.Rotation = PlaneMesh->GetComponentQuat();
	KillCamFrames.Add(ImpactFrame);

	if (KillCamFrames.Num() < 2)
	{
		KillCamReplayStartWorldTime = Now;
		KillCamReplayEndWorldTime = Now;
		KillCamReplayDuration = 0.1f;
	}
	else
	{
		KillCamReplayEndWorldTime = Now;
		KillCamReplayStartWorldTime = FMath::Max(KillCamFrames[0].Time, Now - KillCamReplaySeconds);
		KillCamReplayDuration = FMath::Max(KillCamReplayEndWorldTime - KillCamReplayStartWorldTime, 0.1f);
	}

	KillCamReplayTime = 0.f;
	KillCamTimer = KillCamDuration;

	const FTransform StartTransform = SampleKillCamTransform(KillCamReplayStartWorldTime);
	const FVector ReplayStartLocation = StartTransform.GetLocation();
	const FVector ReplayEndLocation = HitLocation;
	const FVector MidLocation = (ReplayStartLocation + ReplayEndLocation) * 0.5f;

	FVector FlightDirection = (ReplayEndLocation - ReplayStartLocation).GetSafeNormal();

	if (FlightDirection.IsNearlyZero())
	{
		FlightDirection = GetActorForwardVector();
	}

	FVector Side = FVector::CrossProduct(FlightDirection, FVector::UpVector).GetSafeNormal();

	if (Side.IsNearlyZero())
	{
		Side = GetActorRightVector();
	}

	TArray<FVector> CandidateDirections;
	CandidateDirections.Add(Side);
	CandidateDirections.Add(-Side);
	CandidateDirections.Add((Side + FlightDirection * -0.35f).GetSafeNormal());
	CandidateDirections.Add((-Side + FlightDirection * -0.35f).GetSafeNormal());
	CandidateDirections.Add((Side + FlightDirection * 0.35f).GetSafeNormal());
	CandidateDirections.Add((-Side + FlightDirection * 0.35f).GetSafeNormal());

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	FVector BestCameraLocation = MidLocation + Side * KillCamDistance + FVector(0.f, 0.f, KillCamCameraHeight);
	float BestScore = -1.f;

	for (const FVector& Dir : CandidateDirections)
	{
		const FVector CandidateLocation = MidLocation + Dir * KillCamDistance + FVector(0.f, 0.f, KillCamCameraHeight);

		FHitResult HitToMid;
		FHitResult HitToImpact;

		const bool bBlockedMid = GetWorld()->LineTraceSingleByChannel(
			HitToMid,
			CandidateLocation,
			MidLocation,
			ECC_Visibility,
			Params
		);

		const bool bBlockedImpact = GetWorld()->LineTraceSingleByChannel(
			HitToImpact,
			CandidateLocation,
			ReplayEndLocation,
			ECC_Visibility,
			Params
		);

		float Score = 0.f;

		if (!bBlockedMid)
		{
			Score += 1.f;
		}

		if (!bBlockedImpact)
		{
			Score += 1.f;
		}

		Score += FVector::DotProduct(Dir, Side) * 0.2f;

		if (Score > BestScore)
		{
			BestScore = Score;
			BestCameraLocation = CandidateLocation;
		}
	}

	KillCamLocation = BestCameraLocation;

	KillCamCamera->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	
	const FVector InitialLookTarget = bKillCamTrackDrone ? ReplayStartLocation : HitLocation;

	KillCamCamera->SetWorldLocation(KillCamLocation);
	KillCamCamera->SetWorldRotation(
		(InitialLookTarget - KillCamLocation).GetSafeNormal().ToOrientationRotator()
	);
	KillCamCamera->SetActive(true);
	Camera->SetActive(false);

	PlaneMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
	PlaneMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	PlaneMesh->SetSimulatePhysics(false);
	PlaneMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PlaneMesh->SetVisibility(true);

	PlaneMesh->SetWorldLocationAndRotation(
		StartTransform.GetLocation(),
		StartTransform.GetRotation(),
		false,
		nullptr,
		ETeleportType::TeleportPhysics
	);

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetViewTarget(this);
	}

	bKillCamActive = true;
}

void ADiplomaPawn::EndKillCam()
{
	bKillCamActive = false;
	KillCamCamera->SetActive(false);
	Camera->SetActive(true);

	PlaneMesh->SetVisibility(true);
	PlaneMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	PlaneMesh->SetSimulatePhysics(true);

	SetActorLocation(SpawnLocation);
	SetActorRotation(SpawnRotation);

	PlaneMesh->SetWorldLocationAndRotation(
		SpawnLocation,
		SpawnRotation,
		false,
		nullptr,
		ETeleportType::TeleportPhysics
	);

	PlaneMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
	PlaneMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);

	LastSpawnWorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

	bCrashed = false;
	CrashTimer = CrashRespawnDelay;
	KillCamReplayTime = 0.f;
	KillCamReplayDuration = 0.f;
	KillCamFrames.Empty();

	ResetDroneStateAfterRespawn();
}
void ADiplomaPawn::RecordKillCamFrame()
{
	if (!GetWorld() || !PlaneMesh)
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();

	FKillCamFrame Frame;
	Frame.Time = Now;
	Frame.Location = PlaneMesh->GetComponentLocation();
	Frame.Rotation = PlaneMesh->GetComponentQuat();

	KillCamFrames.Add(Frame);

	const float KeepSeconds = KillCamReplaySeconds + 0.3f;

	while (KillCamFrames.Num() > 1 && Now - KillCamFrames[0].Time > KeepSeconds)
	{
		KillCamFrames.RemoveAt(0);
	}
}

FTransform ADiplomaPawn::SampleKillCamTransform(float WorldTime) const
{
	if (KillCamFrames.Num() == 0)
	{
		return PlaneMesh ? PlaneMesh->GetComponentTransform() : FTransform::Identity;
	}

	if (WorldTime <= KillCamFrames[0].Time)
	{
		return FTransform(KillCamFrames[0].Rotation, KillCamFrames[0].Location, FVector(1.f));
	}

	const int32 LastIndex = KillCamFrames.Num() - 1;

	if (WorldTime >= KillCamFrames[LastIndex].Time)
	{
		return FTransform(KillCamFrames[LastIndex].Rotation, KillCamFrames[LastIndex].Location, FVector(1.f));
	}

	for (int32 i = 0; i < LastIndex; ++i)
	{
		const FKillCamFrame& A = KillCamFrames[i];
		const FKillCamFrame& B = KillCamFrames[i + 1];

		if (WorldTime >= A.Time && WorldTime <= B.Time)
		{
			const float Alpha = (WorldTime - A.Time) / FMath::Max(B.Time - A.Time, KINDA_SMALL_NUMBER);
			const FVector Location = FMath::Lerp(A.Location, B.Location, Alpha);
			const FQuat Rotation = FQuat::Slerp(A.Rotation, B.Rotation, Alpha).GetNormalized();

			return FTransform(Rotation, Location, FVector(1.f));
		}
	}

	return FTransform(KillCamFrames[LastIndex].Rotation, KillCamFrames[LastIndex].Location, FVector(1.f));
}

void ADiplomaPawn::HandleCrash(const FVector& HitLocation)
{
	if (bCrashed)
	{
		return;
	}

	bCrashed = true;
	CrashLocation = HitLocation;
	bKillCamExplosionPending = bBombArmedState;
	bKillCamExplosionSpawned = false;

	StartKillCam(HitLocation);
}

void ADiplomaPawn::SpawnCrashExplosion(const FVector& HitLocation)
{
	if (!ExplosionEffect || !GetWorld())
	{
		return;
	}

	UGameplayStatics::SpawnEmitterAtLocation(
		GetWorld(),
		ExplosionEffect,
		HitLocation,
		FRotator::ZeroRotator,
		FVector(1.f)
	);
}

void ADiplomaPawn::UpdateKillCamReplay(float DeltaSeconds)
{
	if (!PlaneMesh)
	{
		EndKillCam();
		return;
	}

	KillCamReplayTime += DeltaSeconds;

	const float Alpha = FMath::Clamp(
		KillCamReplayTime / FMath::Max(KillCamReplayDuration, KINDA_SMALL_NUMBER),
		0.f,
		1.f
	);

	const float ReplayWorldTime = FMath::Lerp(
		KillCamReplayStartWorldTime,
		KillCamReplayEndWorldTime,
		Alpha
	);

	const FTransform ReplayTransform = SampleKillCamTransform(ReplayWorldTime);

	PlaneMesh->SetWorldLocationAndRotation(
		ReplayTransform.GetLocation(),
		ReplayTransform.GetRotation(),
		false,
		nullptr,
		ETeleportType::TeleportPhysics
	);

	if (KillCamCamera)
	{
		const FVector LookTarget = bKillCamTrackDrone
			? ReplayTransform.GetLocation()
			: CrashLocation;

		const FRotator TargetRotation =
			(LookTarget - KillCamCamera->GetComponentLocation()).GetSafeNormal().ToOrientationRotator();

		const FRotator NewRotation = FMath::RInterpTo(
			KillCamCamera->GetComponentRotation(),
			TargetRotation,
			DeltaSeconds,
			KillCamRotationInterpSpeed
		);

		KillCamCamera->SetWorldRotation(NewRotation);
	}


	if (Alpha >= 1.f)
	{
		if (bKillCamExplosionPending && !bKillCamExplosionSpawned)
		{
			bKillCamExplosionSpawned = true;
			SpawnCrashExplosion(CrashLocation);
		}

		KillCamTimer -= DeltaSeconds;

		if (KillCamTimer <= 0.f)
		{
			EndKillCam();
		}
	}
}
bool ADiplomaPawn::ShouldIgnoreCrashHit(const FVector& NormalImpulse) const
{
	if (!GetWorld() || !PlaneMesh)
	{
		return true;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	const float TimeSinceSpawn = Now - LastSpawnWorldTime;

	const float SpeedMps = PlaneMesh->GetPhysicsLinearVelocity().Size() / 100.f;
	const float ImpulseSize = NormalImpulse.Size();

	if (TimeSinceSpawn < CrashSpawnGraceSeconds)
	{
		/*UE_LOG(LogTemp, Warning, TEXT("CRASH IGNORED | SpawnGrace=%.2f | Speed=%.2f m/s | Impulse=%.0f"),
			TimeSinceSpawn,
			SpeedMps,
			ImpulseSize
		);*/

		return true;
	}

	if (SpeedMps < CrashMinImpactSpeedMps && ImpulseSize < CrashMinNormalImpulse)
	{
		/*UE_LOG(LogTemp, Warning, TEXT("CRASH IGNORED | WeakHit | Speed=%.2f m/s | Impulse=%.0f"),
			SpeedMps,
			ImpulseSize
		);*/

		return true;
	}

	return false;
}

void ADiplomaPawn::ResetDroneStateAfterRespawn()
{
	Throttle = 0.f;
	PitchInput = 0.f;
	RollInput = 0.f;
	YawInput = 0.f;

	ReceivedThrottle = 0.f;
	ReceivedPitchInput = 0.f;
	ReceivedRollInput = 0.f;
	ReceivedYawInput = 0.f;

	LastValidReceivedThrottle = 0.f;
	LastValidReceivedPitchInput = 0.f;
	LastValidReceivedRollInput = 0.f;
	LastValidReceivedYawInput = 0.f;

	ControlInputScale = 1.f;
	bControlFailsafeActive = false;
	ControlFailsafeTimer = 0.f;
	ControlFailsafeActiveTime = 0.f;

	SmoothedControlRSSI = 100.f;
	SmoothedControlLQ = 100.f;
	SmoothedVideoLink = 100.f;

	bArmedState = false;
	bBombArmedState = false;

	bMouseJoystickEnabled = false;
	AccumulatedX = 0.f;
	AccumulatedY = 0.f;

	BaroZeroZ = GetActorLocation().Z;
	TelemetryStartTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

	Telemetry = FDroneTelemetry();
	UpdateTelemetry();
}