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


	PlaneMesh->SetSimulatePhysics(true);
	PlaneMesh->SetEnableGravity(true);
	PlaneMesh->SetLinearDamping(0.05f);
	PlaneMesh->SetAngularDamping(0.1f);
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


}

void ADiplomaPawn::BeginPlay()
{
	Super::BeginPlay();
	BaroZeroZ = GetActorLocation().Z;
	PlaneMesh->SetCenterOfMass(FVector::ZeroVector, NAME_None);

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
	UpdateMouseJoystick();
	ApplyThrust();
	ApplyTorques();
	UpdateTelemetry();
}

void ADiplomaPawn::NotifyHit(class UPrimitiveComponent* MyComp, class AActor* Other, class UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);

	// Deflect along the surface when we collide.
	FRotator CurrentRotation = GetActorRotation();
	SetActorRotation(FQuat::Slerp(CurrentRotation.Quaternion(), HitNormal.ToOrientationQuat(), 0.025f));
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
}



void ADiplomaPawn::PitchInputAxis(float Value)
{
	PitchInput = FMath::Clamp(Value, -1.f, 1.f);
}

void ADiplomaPawn::RollInputAxis(float Value)
{
	RollInput = FMath::Clamp(Value, -1.f, 1.f);
}

void ADiplomaPawn::YawInputAxis(float Value)
{
	YawInput = FMath::Clamp(Value, -1.f, 1.f);
}


void ADiplomaPawn::ThrottleInput(float Value)
{
	//UE_LOG(LogTemp, Warning, TEXT("Throttle axis value: %.3f"), Value);

	Throttle = FMath::Clamp(Throttle + Value * GetWorld()->GetDeltaSeconds(), 0.f, 1.f);
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

