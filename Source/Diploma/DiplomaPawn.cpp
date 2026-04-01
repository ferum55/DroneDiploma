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
	BaroZeroZ = GetActorLocation().Z;
	PlaneMesh->SetCenterOfMass(FVector::ZeroVector, NAME_None);

	SpawnLocation = GetActorLocation();
	SpawnRotation = GetActorRotation();
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
}


void ADiplomaPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bCrashed)
	{
		CrashTimer -= DeltaSeconds;
		if (CrashTimer <= 0.f)
		{
			bCrashed = false;
			PlaneMesh->SetSimulatePhysics(true);
			UE_LOG(LogTemp, Warning, TEXT("Respawned! Control restored."));
		}
		return;
	}
	UpdateMouseJoystick();
	UpdateTelemetry();
	float RawThrottle = GetInputAxisValue(TEXT("TestAxis4"));
	float RawPitch = GetInputAxisValue(TEXT("TestAxis5"));
	float RawRoll = GetInputAxisValue(TEXT("TestAxis3"));
	float RawYaw = GetInputAxisValue(TEXT("TestAxis6"));

	float NormThrottle = NormalizeThrottle(RawThrottle);
	float NormPitch = NormalizeCenteredAxis(RawPitch);
	float NormRoll = NormalizeCenteredAxis(RawRoll);
	float NormYaw = NormalizeCenteredAxis(RawYaw);

	/*UE_LOG(LogTemp, Warning,
		TEXT("RAW  T=%.3f P=%.3f R=%.3f Y=%.3f | NORM  T=%.3f P=%.3f R=%.3f Y=%.3f"),
		RawThrottle, RawPitch, RawRoll, RawYaw,
		NormThrottle, NormPitch, NormRoll, NormYaw
	);*/

	//UE_LOG(LogTemp, Warning, TEXT("RAW=%.3f NORM=%.3f"), RawYaw, NormYaw);
}

void ADiplomaPawn::NotifyHit(class UPrimitiveComponent* MyComp, class AActor* Other, class UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	UE_LOG(LogTemp, Warning, TEXT("CRASH! Hit actor: %s at location: %s"),
		Other ? *Other->GetName() : TEXT("Unknown"),
		*HitLocation.ToString());
	Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);


	FRotator CurrentRotation = GetActorRotation();
	SetActorRotation(FQuat::Slerp(CurrentRotation.Quaternion(), HitNormal.ToOrientationQuat(), 0.025f));

	if (bCrashed) return; 

	bCrashed = true;
	CrashTimer = CrashRespawnDelay;

	UE_LOG(LogTemp, Warning, TEXT("CRASH! Hit actor: %s at location: %s"),
		Other ? *Other->GetName() : TEXT("Unknown"),
		*HitLocation.ToString());

	// Зупиняємо фізику
	PlaneMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
	PlaneMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);

	// Телепортуємо
	SetActorLocation(SpawnLocation);
	SetActorRotation(SpawnRotation);
}

void ADiplomaPawn::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (bCrashed) return;

	bCrashed = true;
	CrashTimer = CrashRespawnDelay;

	UE_LOG(LogTemp, Warning, TEXT("CRASH! Hit: %s | Impulse: %.1f"),
		OtherActor ? *OtherActor->GetName() : TEXT("Unknown"),
		NormalImpulse.Size());

	PlaneMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
	PlaneMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	SetActorLocation(SpawnLocation);
	SetActorRotation(SpawnRotation);

	// Вимикаємо фізику на час таймера
	PlaneMesh->SetSimulatePhysics(false);
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


	PlayerInputComponent->BindAxis("TestAxis1");
	PlayerInputComponent->BindAxis("TestAxis2");
	PlayerInputComponent->BindAxis("TestAxis3");
	PlayerInputComponent->BindAxis("TestAxis4");
	PlayerInputComponent->BindAxis("TestAxis5");
	PlayerInputComponent->BindAxis("TestAxis6");
	PlayerInputComponent->BindAxis("TestAxis7");
	PlayerInputComponent->BindAxis("TestAxis8");
}



void ADiplomaPawn::PitchInputAxis(float Value)
{
	PitchInput = NormalizeCenteredAxis(Value); //NormalizeCenteredAxis(Value, 0.654f);
}

void ADiplomaPawn::RollInputAxis(float Value)
{
	RollInput = -NormalizeCenteredAxis(Value); //NormalizeCenteredAxis(Value, 0.654f);
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
	if (!PlaneMesh) return;

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
	float Shifted = Raw;

	if (Raw > 0.5f)
	{
		Shifted = Raw - 1.0f;
	}

	float Value = Shifted + 0.5f;

	const float Min = 0.15f;
	const float Max = 0.85f;

	Value = (Value - Min) / (Max - Min);

	Value = FMath::Clamp(Value, 0.f, 1.f);

	if (Value < 0.02f)
	{
		Value = 0.f;
	}

	return Value;
}

float ADiplomaPawn::NormalizeCenteredAxis(float Raw) const
{
	// unwrap (як для throttle)
	float Shifted = Raw;

	if (Raw > 0.5f)
	{
		Shifted = Raw - 1.0f;
	}

	// тепер діапазон приблизно [-0.35 .. +0.35]
	float Value = Shifted;

	// нормалізуємо в -1..1
	const float MaxAbs = 0.35f;

	Value = Value / MaxAbs;

	return FMath::Clamp(Value, -1.f, 1.f);
}
