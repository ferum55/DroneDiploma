//// Copyright Epic Games, Inc. All Rights Reserved.
//
//#include "FPVDronePawn.h"
//#include "UObject/ConstructorHelpers.h"
//#include "Camera/CameraComponent.h"
//#include "Components/StaticMeshComponent.h"
//#include "Components/InputComponent.h"
//
//#include "Engine/World.h"
//#include "Engine/StaticMesh.h"
//
//
//#include "GameFramework/InputSettings.h"
//#include "Engine/Engine.h"
//
//#include "Camera/CameraComponent.h"
//#include "Materials/MaterialInterface.h"
//#include "Materials/MaterialInstanceDynamic.h"
//#include "Engine/EngineTypes.h"
//
//#include "Particles/ParticleSystemComponent.h"
//#include "Kismet/GameplayStatics.h"
//
//
//AFPVDronePawn::AFPVDronePawn()
//{
//	bMouseJoystickEnabled = false;
//	// Structure to hold one-time initialization
//	struct FConstructorStatics
//	{
//		ConstructorHelpers::FObjectFinderOptional<UStaticMesh> PlaneMesh;
//		FConstructorStatics()
//			: PlaneMesh(TEXT("/Game/Flying/Meshes/UFO.UFO"))
//		{
//		}
//	};
//	static FConstructorStatics ConstructorStatics;
//
//	// Create static mesh component
//	PlaneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaneMesh0"));
//	PlaneMesh->SetStaticMesh(ConstructorStatics.PlaneMesh.Get());	// Set static mesh
//	RootComponent = PlaneMesh;
//
//	//Unsafe
//	PlaneMesh->SetNotifyRigidBodyCollision(true);
//
//	PlaneMesh->SetSimulatePhysics(true);
//	PlaneMesh->SetEnableGravity(true);
//	//PlaneMesh->SetLinearDamping(0.05f);
//	//PlaneMesh->SetAngularDamping(0.1f);
//	PlaneMesh->SetMassOverrideInKg(NAME_None, 4.5f);
//
//	PlaneMesh->SetCollisionProfileName(TEXT("PhysicsActor"));
//	PlaneMesh->SetCenterOfMass(FVector::ZeroVector);
//
//
//
//	// Create camera component 
//	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera0"));
//	Camera->SetupAttachment(PlaneMesh);
//
//	Camera->bUsePawnControlRotation = false; // Don't rotate camera with controller
//	
//	Camera->SetRelativeRotation(FRotator::ZeroRotator);
//	Camera->FieldOfView = 100.f;
//	Camera->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
//
//	KillCamCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("KillCamCamera0"));
//	KillCamCamera->SetupAttachment(RootComponent);
//	KillCamCamera->bUsePawnControlRotation = false;
//	KillCamCamera->SetActive(false);
//
//	if (Camera)
//	{
//		if (Camera->PostProcessSettings.WeightedBlendables.Array.Num() > 0)
//		{
//			auto& Blendable = Camera->PostProcessSettings.WeightedBlendables.Array[0];
//
//			if (Blendable.Object)
//			{
//				FPVPostProcessMID = UMaterialInstanceDynamic::Create(
//					Cast<UMaterialInterface>(Blendable.Object),
//					this
//				);
//
//				Blendable.Object = FPVPostProcessMID;
//			}
//		}
//	}
//
//
//	Throttle = 0.f;
//	PitchInput = 0.f;
//	RollInput = 0.f;
//	YawInput = 0.f;
//
//	MaxThrust = 2000.f;
//	ThrustScale = 1.f;
//	PitchTorque = 1500.f;
//	RollTorque = 1500.f;
//	YawTorque = 800.f;
//
//	CrashTimer = 3.f;
//
//
//}
//
//void AFPVDronePawn::BeginPlay()
//{
//	Super::BeginPlay();
//	if (GEngine)
//	{
//		GEngine->Exec(GetWorld(), TEXT("stat fps"));
//		GEngine->Exec(GetWorld(), TEXT("stat unit"));
//	}
//
//	BaroZeroZ = GetActorLocation().Z;
//	OperatorLocation = GetActorLocation();
//	
//	TelemetryStartTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
//	PlaneMesh->SetCenterOfMass(FVector::ZeroVector, NAME_None);
//
//	SpawnLocation = GetActorLocation();
//	SpawnRotation = GetActorRotation();
//	LastSpawnWorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
//	PlaneMesh->OnComponentHit.AddDynamic(this, &AFPVDronePawn::OnHit);
//
//	UE_LOG(LogTemp, Warning, TEXT("Actor: %s"), *GetName());
//	UE_LOG(LogTemp, Warning, TEXT("PlaneMesh Rel: %s  World: %s"),
//		*PlaneMesh->GetRelativeLocation().ToString(),
//		*PlaneMesh->GetComponentLocation().ToString());
//
//	UE_LOG(LogTemp, Warning, TEXT("Camera Rel: %s  World: %s  AttachParent: %s"),
//		*Camera->GetRelativeLocation().ToString(),
//		*Camera->GetComponentLocation().ToString(),
//		Camera->GetAttachParent() ? *Camera->GetAttachParent()->GetName() : TEXT("None"));
//
//	const AFPVDronePawn* CDO = GetClass()->GetDefaultObject<AFPVDronePawn>();
//
//	
//
//	const UInputSettings* Settings = GetDefault<UInputSettings>();
//	if (!Settings)
//	{
//		UE_LOG(LogTemp, Error, TEXT("InputSettings not found"));
//		return;
//	}
//
//	const TArray<FInputActionKeyMapping>& Actions = Settings->GetActionMappings();
//
//	int32 Found = 0;
//	for (const FInputActionKeyMapping& A : Actions)
//	{
//		if (A.ActionName == "MouseJoystickToggle")
//		{
//			UE_LOG(LogTemp, Warning, TEXT("MouseJoystickToggle bound to key: %s"), *A.Key.ToString());
//			Found++;
//		}
//	}
//
//	UE_LOG(LogTemp, Warning, TEXT("MouseJoystickToggle mappings found: %d"), Found);
//
//	if (APlayerController* PC = Cast<APlayerController>(GetController()))
//	{
//		PC->SetInputMode(FInputModeGameOnly());
//		PC->bShowMouseCursor = false;
//	}
//	if (!Camera)
//	{
//		UE_LOG(LogTemp, Error, TEXT("PP | Camera is null"));
//	}
//	else
//	{
//		const int32 Count = Camera->PostProcessSettings.WeightedBlendables.Array.Num();
//
//		UE_LOG(LogTemp, Warning, TEXT("PP | Blendables count: %d"), Count);
//
//		if (Count > 0)
//		{
//			FWeightedBlendable& Blendable = Camera->PostProcessSettings.WeightedBlendables.Array[0];
//			UObject* Obj = Blendable.Object;
//
//			UE_LOG(LogTemp, Warning,
//				TEXT("PP | Blendable[0]: %s | Class: %s"),
//				Obj ? *Obj->GetName() : TEXT("None"),
//				Obj ? *Obj->GetClass()->GetName() : TEXT("None")
//			);
//
//			UMaterialInterface* MaterialInterface = Cast<UMaterialInterface>(Obj);
//
//			if (MaterialInterface)
//			{
//				FPVPostProcessMID = UMaterialInstanceDynamic::Create(MaterialInterface, this);
//				Blendable.Object = FPVPostProcessMID;
//			}
//		}
//
//		UE_LOG(LogTemp, Warning,
//			TEXT("PP | MID created: %s"),
//			FPVPostProcessMID ? TEXT("YES") : TEXT("NO")
//		);
//	}
//}
//
//
//void AFPVDronePawn::Tick(float DeltaSeconds)
//{
//	Super::Tick(DeltaSeconds);
//
//	if (bCrashed)
//	{
//		if (bKillCamActive)
//		{
//			UpdateKillCamReplay(DeltaSeconds);
//		}
//		return;
//	}
//
//	RecordKillCamFrame();
//
//	UpdateMouseJoystick();
//
//	float RawThrottle = GetInputAxisValue(TEXT("TestAxis4"));
//	float RawPitch = GetInputAxisValue(TEXT("TestAxis5"));
//	float RawRoll = GetInputAxisValue(TEXT("TestAxis3"));
//	float RawYaw = GetInputAxisValue(TEXT("TestAxis6"));
//
//	float NormThrottle = NormalizeThrottle(RawThrottle);
//	float NormPitch = NormalizeCenteredAxis(RawPitch);
//	float NormRoll = NormalizeCenteredAxis(RawRoll);
//	float NormYaw = NormalizeCenteredAxis(RawYaw);
//	LastDeltaSeconds = DeltaSeconds;
//}
//
//void AFPVDronePawn::NotifyHit(UPrimitiveComponent* MyComp, AActor* Other,
//	UPrimitiveComponent* OtherComp, bool bSelfMoved,
//	FVector HitLocation, FVector HitNormal,
//	FVector NormalImpulse, const FHitResult& Hit)
//{
//	Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved,
//		HitLocation, HitNormal, NormalImpulse, Hit);
//
//	if (ShouldIgnoreCrashHit(NormalImpulse))
//	{
//		return;
//	}
//
//	UE_LOG(LogTemp, Warning, TEXT("CRASH NOTIFY HIT | Other=%s | Location=%s | Impulse=%s | Bomb=%d"),
//		Other ? *Other->GetName() : TEXT("None"),
//		*HitLocation.ToString(),
//		*NormalImpulse.ToString(),
//		bBombArmedState ? 1 : 0
//	);
//
//	HandleCrashExplosion(Hit);
//	HandleCrash(HitLocation);
//}
//
//void AFPVDronePawn::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
//	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
//{
//	const FVector HitLocation = Hit.ImpactPoint.IsNearlyZero() ? Hit.Location : Hit.ImpactPoint;
//
//	if (ShouldIgnoreCrashHit(NormalImpulse))
//	{
//		return;
//	}
//
//	UE_LOG(LogTemp, Warning, TEXT("CRASH COMPONENT HIT | Other=%s | Impulse=%s | ImpactPoint=%s | Bomb=%d"),
//		OtherActor ? *OtherActor->GetName() : TEXT("None"),
//		*NormalImpulse.ToString(),
//		*HitLocation.ToString(),
//		bBombArmedState ? 1 : 0
//	);
//
//	HandleCrashExplosion(Hit);
//	HandleCrash(HitLocation);
//}
//
//void AFPVDronePawn::HandleCrashExplosion(const FHitResult& Hit)
//{
//}
//
//
//void AFPVDronePawn::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
//{
//    // Check if PlayerInputComponent is valid (not NULL)
//	check(PlayerInputComponent);
//	UE_LOG(LogTemp, Warning, TEXT("SetupPlayerInputComponent called for %s"), *GetName());
//
//	PlayerInputComponent->BindAxis("Throttle", this, &AFPVDronePawn::ThrottleInput);
//	PlayerInputComponent->BindAxis("Pitch", this, &AFPVDronePawn::PitchInputAxis);
//	PlayerInputComponent->BindAxis("Roll", this, &AFPVDronePawn::RollInputAxis);
//	PlayerInputComponent->BindAxis("Yaw", this, &AFPVDronePawn::YawInputAxis);
//	PlayerInputComponent->BindAction(
//		"MouseJoystickToggle",
//		IE_Pressed,
//		this,
//		&AFPVDronePawn::ToggleMouseJoystick
//	);
//
//	PlayerInputComponent->BindAction("Arm", IE_Pressed, this, &AFPVDronePawn::ToggleArm);
//	PlayerInputComponent->BindAction("BombArm", IE_Pressed, this, &AFPVDronePawn::ToggleBombArm);
//
//
//	PlayerInputComponent->BindAxis("TestAxis1");
//	PlayerInputComponent->BindAxis("TestAxis2");
//	PlayerInputComponent->BindAxis("TestAxis3");
//	PlayerInputComponent->BindAxis("TestAxis4");
//	PlayerInputComponent->BindAxis("TestAxis5");
//	PlayerInputComponent->BindAxis("TestAxis6");
//	PlayerInputComponent->BindAxis("TestAxis7");
//	PlayerInputComponent->BindAxis("TestAxis8");
//
//
//}
//
//void AFPVDronePawn::ToggleArm()
//{
//	if (bCrashed || bKillCamActive)
//	{
//		return;
//	}
//
//	bArmedState = !bArmedState;
//
//	if (!bArmedState)
//	{
//		Throttle = 0.f;
//		ReceivedThrottle = 0.f;
//		ReceivedPitchInput = 0.f;
//		ReceivedRollInput = 0.f;
//		ReceivedYawInput = 0.f;
//	}
//
//	UE_LOG(LogTemp, Warning, TEXT("Armed: %d"), bArmedState ? 1 : 0);
//}
//
//void AFPVDronePawn::ToggleBombArm()
//{
//	if (bCrashed || bKillCamActive)
//	{
//		return;
//	}
//
//	bBombArmedState = !bBombArmedState;
//
//	UE_LOG(LogTemp, Warning, TEXT("BombArmed: %d"), bBombArmedState ? 1 : 0);
//}
//
//
//void AFPVDronePawn::PitchInputAxis(float Value)
//{
//	PitchInput = NormalizeCenteredAxis(Value); //NormalizeCenteredAxis(Value, 0.654f);
//}
//
//void AFPVDronePawn::RollInputAxis(float Value)
//{
//	RollInput =NormalizeCenteredAxis(Value); //NormalizeCenteredAxis(Value, 0.654f);
//}
//
//void AFPVDronePawn::YawInputAxis(float Value)
//{
//	YawInput = NormalizeCenteredAxis(Value);//NormalizeCenteredAxis(Value, 0.665f);
//}
//
//
//void AFPVDronePawn::ThrottleInput(float Value)
//{
//	//UE_LOG(LogTemp, Warning, TEXT("Throttle axis value: %.3f"), Value);
//
//	Throttle = NormalizeThrottle(Value);
//	/*UE_LOG(LogTemp, Warning, TEXT("Throttle state: %.3f"), Throttle);*/
//}
//
//void AFPVDronePawn::ApplyThrust()
//{
//	if (!PlaneMesh) return;
//
//	const FVector UpVector = PlaneMesh->GetUpVector();
//	const float ClampedThrottle = FMath::Clamp(Throttle, 0.f, 1.f);
//
//	const FVector Force =
//		UpVector * (ClampedThrottle * MaxThrust * ThrustScale);
//
//	PlaneMesh->AddForce(Force);
//}
//
//void AFPVDronePawn::ApplyTorques()
//{
//	if (!PlaneMesh) return;
//
//	const FVector Torque(
//		-RollInput * RollTorque,
//		-PitchInput * PitchTorque,
//		YawInput * YawTorque
//	);
//
//	PlaneMesh->AddTorqueInRadians(
//		PlaneMesh->GetComponentTransform().TransformVectorNoScale(Torque)
//	);
//}
//
//void AFPVDronePawn::UpdateMouseJoystick()
//{
//	if (!bMouseJoystickEnabled)
//		return;
//
//	APlayerController* PC = Cast<APlayerController>(GetController());
//	if (!PC) return;
//
//	float MouseX, MouseY;
//	if (!PC->GetMousePosition(MouseX, MouseY)) return;
//
//	int32 SizeX, SizeY;
//	PC->GetViewportSize(SizeX, SizeY);
//
//	const float CenterX = SizeX * 0.5f;
//	const float CenterY = SizeY * 0.5f;
//	const float Half = SizeY * 0.5f - 10.f - 40.f;
//
//	float dx = FMath::Clamp((MouseX - CenterX) / Half, -1.f, 1.f);
//	float dy = FMath::Clamp((MouseY - CenterY) / Half, -1.f, 1.f);
//
//	dx = FMath::Clamp(dx, -1.f, 1.f);
//	dy = FMath::Clamp(dy, -1.f, 1.f);
//
//	if (FMath::Abs(dx) < MouseDeadZone) dx = 0.f;
//	if (FMath::Abs(dy) < MouseDeadZone) dy = 0.f;
//
//	RollInput = dx;
//	PitchInput = dy;
//}
//
//void AFPVDronePawn::ToggleMouseJoystick()
//{
//	UE_LOG(LogTemp, Warning, TEXT("ToggleMouseJoystick FIRED"));
//
//	bMouseJoystickEnabled = !bMouseJoystickEnabled;
//	UE_LOG(LogTemp, Warning, TEXT("MouseJoystickEnabled = %d"), bMouseJoystickEnabled ? 1 : 0);
//
//	APlayerController* PC = Cast<APlayerController>(GetController());
//	if (!PC) return;
//
//	if (bMouseJoystickEnabled)
//	{
//		CenterMouseCursor();
//		AccumulatedX = 0.f;
//		AccumulatedY = 0.f;
//	}
//	else
//	{
//		PitchInput = 0.f;
//		RollInput = 0.f;
//	}
//}
//
//
//void AFPVDronePawn::CenterMouseCursor()
//{
//	APlayerController* PC = Cast<APlayerController>(GetController());
//	if (!PC) return;
//
//	int32 SizeX, SizeY;
//	PC->GetViewportSize(SizeX, SizeY);
//
//	const int32 CenterX = SizeX / 2;
//	const int32 CenterY = SizeY / 2;
//
//	PC->SetMouseLocation(CenterX, CenterY);
//}
//
//void AFPVDronePawn::UpdateTelemetry()
//{
//	if (!PlaneMesh)
//	{
//		return;
//	}
//
//	const FVector Velocity = PlaneMesh->GetPhysicsLinearVelocity();
//
//	Telemetry.Throttle01 = Throttle;
//	Telemetry.ThrottlePercent = Throttle * 100.f;
//	Telemetry.SpeedMps = Velocity.Size() / 100.f;
//	Telemetry.SpeedKmh = Telemetry.SpeedMps * 3.6f;
//	Telemetry.VerticalSpeedMps = Velocity.Z / 100.f;
//	Telemetry.BaroAltitudeM = GetActorLocation().Z / 100.f;
//	Telemetry.RelativeAltitudeM = (GetActorLocation().Z - BaroZeroZ) / 100.f;
//
//	bool bValid = false;
//	Telemetry.RadioAltitudeM = GetRadioAltitudeMeters(bValid);
//	Telemetry.bRadioAltitudeValid = bValid;
//
//	const FRotator R = GetActorRotation();
//	Telemetry.PitchDeg = R.Pitch;
//	Telemetry.RollDeg = R.Roll;
//	Telemetry.YawDeg = R.Yaw;
//	Telemetry.HeadingDeg = FRotator::ClampAxis(R.Yaw);
//
//	Telemetry.FlightTimeSeconds = GetWorld()
//		? (GetWorld()->GetTimeSeconds() - TelemetryStartTimeSeconds)
//		: 0.f;
//
//	Telemetry.bArmed = bArmedState;
//	Telemetry.FlightMode = TEXT("");
//
//	Telemetry.PackVoltage = 0.f;
//	Telemetry.CellVoltage = 0.f;
//	Telemetry.ConsumedMah = 0.f;
//	Telemetry.CurrentAmp = 0.f;
//	Telemetry.bBatteryValid = false;
//
//	Telemetry.PrimaryLinkPercent = 0.f;
//	Telemetry.bPrimaryLinkValid = false;
//
//	Telemetry.VideoLinkPercent = 0.f;
//	Telemetry.bVideoLinkValid = false;
//
//	Telemetry.TxPowerW = 0.f;
//	Telemetry.bTxPowerValid = false;
//
//	Telemetry.bBombArmed = bBombArmedState;
//	Telemetry.bKillCamActive = bKillCamActive;
//	Telemetry.bCrashed = bCrashed;
//
//	UpdateSignalTelemetry(LastDeltaSeconds);
//}
//
//float AFPVDronePawn::GetRadioAltitudeMeters(bool& bValid) const
//{
//	if (!GetWorld() || !PlaneMesh)
//	{
//		bValid = false;
//		return 0.f;
//	}
//
//	FHitResult Hit;
//	const FVector Start = PlaneMesh->GetComponentLocation();
//	const FVector End = Start - FVector(0.f, 0.f, 100000.f);
//
//	FCollisionQueryParams Params;
//	Params.AddIgnoredActor(this);
//
//	const bool bHit = GetWorld()->LineTraceSingleByChannel(
//		Hit,
//		Start,
//		End,
//		ECC_Visibility,
//		Params
//	);
//
//	bValid = bHit;
//	return bHit ? Hit.Distance / 100.f : 0.f;
//}
//
//float AFPVDronePawn::NormalizeThrottle(float Raw) const
//{
//	// Axis4: мінімум=0.0, центр=0.5, максимум=1.0
//	// Мапимо [0..1] ? [0..1], центр не має значення для тяги
//	const float Min = 0.15f;
//	const float Max = 0.85f;
//	float Value = (Raw - Min) / (Max - Min);
//	Value = FMath::Clamp(Value, 0.f, 1.f);
//	if (Value < 0.02f) Value = 0.f;
//	return Value;
//}
//
//float AFPVDronePawn::NormalizeCenteredAxis(float Raw) const
//{
//	// Axis: мінімум=0.0, центр=0.5, максимум=1.0
//	// Мапимо [0..1] ? [-1..1] відносно центру 0.5
//	float Value = (Raw - 0.5f) * 2.0f;
//	Value = FMath::Clamp(Value, -1.f, 1.f);
//	const float DeadZone = 0.04f;
//	if (FMath::Abs(Value) < DeadZone) Value = 0.f;
//	return Value;
//}
//
//
//float AFPVDronePawn::ComputeAntennaOrientationLossDb(float MaxLossDb) const
//{
//	if (!PlaneMesh)
//	{
//		return 0.f;
//	}
//
//	const FVector DroneLocation = PlaneMesh->GetComponentLocation();
//	const FVector OperatorEyeLocation = OperatorLocation + FVector(0.f, 0.f, 170.f);
//	const FVector DirToOperator = (OperatorEyeLocation - DroneLocation).GetSafeNormal();
//
//	if (DirToOperator.IsNearlyZero())
//	{
//		return 0.f;
//	}
//
//	const FVector AntennaAxis = PlaneMesh->GetComponentTransform().GetUnitAxis(EAxis::Z);
//	const float AxialFactor = FMath::Abs(FVector::DotProduct(AntennaAxis, DirToOperator));
//
//	return MaxLossDb * FMath::Pow(AxialFactor, 2.f);
//}
//
//float AFPVDronePawn::ComputeBodyShadowLossDb(float MaxLossDb) const
//{
//	if (!PlaneMesh)
//	{
//		return 0.f;
//	}
//
//	const FVector DroneLocation = PlaneMesh->GetComponentLocation();
//	const FVector OperatorEyeLocation = OperatorLocation + FVector(0.f, 0.f, 170.f);
//	const FVector WorldDirToOperator = (OperatorEyeLocation - DroneLocation).GetSafeNormal();
//
//	if (WorldDirToOperator.IsNearlyZero())
//	{
//		return 0.f;
//	}
//
//	const FTransform MeshTransform = PlaneMesh->GetComponentTransform();
//	const FVector LocalDir = MeshTransform.InverseTransformVectorNoScale(WorldDirToOperator).GetSafeNormal();
//
//	const float UnderBodyFactor = FMath::Clamp((-LocalDir.Z - 0.2f) / 0.8f, 0.f, 1.f);
//	const float RearBodyFactor = FMath::Clamp((-LocalDir.X - 0.35f) / 0.65f, 0.f, 1.f) * 0.35f;
//	const float ShadowFactor = FMath::Max(UnderBodyFactor, RearBodyFactor);
//
//	return MaxLossDb * ShadowFactor;
//}
//
//void AFPVDronePawn::UpdateSignalTelemetry(float DeltaTime)
//{
//	if (!PlaneMesh)
//	{
//		return;
//	}
//
//	const float SafeDeltaTime = FMath::Max(DeltaTime, 0.016f);
//
//	const float DistanceM = FMath::Max(
//		FVector::Distance(PlaneMesh->GetComponentLocation(), OperatorLocation) / 100.f,
//		1.f
//	);
//
//	const float Obstruction = ComputeOperatorObstructionFactor();
//
//	SignalFadeTimer += SafeDeltaTime;
//
//	if (SignalFadeTimer >= SignalFadeUpdateInterval)
//	{
//		SignalFadeTimer = 0.f;
//		TargetControlFadeLossDb = FMath::FRandRange(0.f, ControlRandomFadeMaxDb);
//		TargetVideoFadeLossDb = FMath::FRandRange(0.f, VideoRandomFadeMaxDb);
//	}
//
//	SmoothedControlFadeLossDb = FMath::FInterpTo(
//		SmoothedControlFadeLossDb,
//		TargetControlFadeLossDb,
//		SafeDeltaTime,
//		SignalFadeInterpSpeed
//	);
//
//	SmoothedVideoFadeLossDb = FMath::FInterpTo(
//		SmoothedVideoFadeLossDb,
//		TargetVideoFadeLossDb,
//		SafeDeltaTime,
//		SignalFadeInterpSpeed
//	);
//
//	const float ControlAntennaLossDb = ComputeAntennaOrientationLossDb(ControlAntennaOrientationMaxLossDb);
//	const float VideoAntennaLossDb = ComputeAntennaOrientationLossDb(VideoAntennaOrientationMaxLossDb);
//
//	const float ControlBodyShadowLossDb = ComputeBodyShadowLossDb(ControlBodyShadowMaxLossDb);
//	const float VideoBodyShadowLossDb = ComputeBodyShadowLossDb(VideoBodyShadowMaxLossDb);
//
//	const float ControlExtraLossDb =
//		Obstruction * ControlObstructionLossDb +
//		ControlAntennaLossDb +
//		ControlBodyShadowLossDb +
//		SmoothedControlFadeLossDb;
//
//	const float VideoExtraLossDb =
//		Obstruction * VideoObstructionLossDb +
//		VideoAntennaLossDb +
//		VideoBodyShadowLossDb +
//		SmoothedVideoFadeLossDb;
//
//	const float RawControlRSSIDbm = ComputeReceivedPowerDbm(
//		ControlTxPowerW,
//		ControlTxAntennaGainDbi,
//		ControlRxAntennaGainDbi,
//		ControlFrequencyMHz,
//		DistanceM,
//		ControlExtraLossDb
//	);
//
//	const float RawVideoRSSIDbm = ComputeReceivedPowerDbm(
//		VideoTxPowerW,
//		VideoAntennaGainDbi,
//		VideoRxAntennaGainDbi,
//		VideoFrequencyMHz,
//		DistanceM,
//		VideoExtraLossDb
//	);
//
//	const float ControlRSSIDbm = FMath::Min(RawControlRSSIDbm, ControlMaxDisplayedRSSIDbm);
//	const float VideoRSSIDbm = FMath::Min(RawVideoRSSIDbm, VideoMaxDisplayedRSSIDbm);
//
//	const float ControlMarginDb = ControlRSSIDbm - ControlReceiverSensitivityDbm;
//	const float VideoMarginDb = VideoRSSIDbm - VideoReceiverSensitivityDbm;
//
//	const float ControlRSSIBarPercent = ComputeSignalBarPercent(ControlRSSIDbm, ControlReceiverSensitivityDbm);
//	const float ControlLQPercent = ComputeControlLQFromMargin(ControlMarginDb);
//	const float VideoQualityPercent = ComputeVideoQualityFromMargin(VideoMarginDb);
//
//	SmoothedControlRSSIDbm = FMath::FInterpTo(
//		SmoothedControlRSSIDbm,
//		ControlRSSIDbm,
//		SafeDeltaTime,
//		SignalSmoothingSpeed
//	);
//
//	SmoothedVideoRSSIDbm = FMath::FInterpTo(
//		SmoothedVideoRSSIDbm,
//		VideoRSSIDbm,
//		SafeDeltaTime,
//		SignalSmoothingSpeed
//	);
//
//	SmoothedControlRSSI = FMath::FInterpTo(
//		SmoothedControlRSSI,
//		ControlRSSIBarPercent,
//		SafeDeltaTime,
//		SignalSmoothingSpeed
//	);
//
//	SmoothedControlLQ = FMath::FInterpTo(
//		SmoothedControlLQ,
//		ControlLQPercent,
//		SafeDeltaTime,
//		SignalSmoothingSpeed
//	);
//
//	SmoothedVideoLink = FMath::FInterpTo(
//		SmoothedVideoLink,
//		VideoQualityPercent,
//		SafeDeltaTime,
//		SignalSmoothingSpeed
//	);
//
//	Telemetry.ControlRSSIDbm = SmoothedControlRSSIDbm;
//	Telemetry.ControlRSSIBarPercent = SmoothedControlRSSI;
//	Telemetry.ControlRSSIPercent = SmoothedControlRSSI;
//	Telemetry.ControlLQPercent = SmoothedControlLQ;
//	Telemetry.ControlSignalMarginDb = SmoothedControlRSSIDbm - ControlReceiverSensitivityDbm;
//	Telemetry.bControlLinkValid = true;
//
//	Telemetry.PrimaryLinkPercent = SmoothedControlLQ;
//	Telemetry.bPrimaryLinkValid = true;
//
//	Telemetry.VideoRSSIDbm = SmoothedVideoRSSIDbm;
//	Telemetry.VideoSignalMarginDb = SmoothedVideoRSSIDbm - VideoReceiverSensitivityDbm;
//	Telemetry.VideoLinkPercent = SmoothedVideoLink;
//	Telemetry.bVideoLinkValid = true;
//
//	Telemetry.TxPowerW = VideoTxPowerW;
//	Telemetry.bTxPowerValid = true;
//
//	UpdateReceivedControlInput(SafeDeltaTime);
//
//	if (FPVPostProcessMID)
//	{
//		const float VideoQuality01 = Telemetry.VideoLinkPercent / 100.f;
//		FPVPostProcessMID->SetScalarParameterValue(TEXT("VideoQuality"), VideoQuality01);
//	}
//
//	static float SignalLogTimer = 0.f;
//	SignalLogTimer += SafeDeltaTime;
//
//	if (bLogSignalDebug && SignalLogTimer >= 0.5f)
//	{
//		SignalLogTimer = 0.f;
//
//		UE_LOG(LogTemp, Warning,
//			TEXT("SIGNAL | Dist=%.1fm Obs=%.2f LossC=%.1f LossV=%.1f | CTRL RSSI=%.1fdBm Bar=%.0f LQ=%.0f EffLQ=%.0f Override=%.1f Margin=%.1fdB | VIDEO RSSI=%.1fdBm Q=%.0f Margin=%.1fdB | VTX=%.1fW | Packet=%d Age=%.3f | Failsafe=%d"),
//			DistanceM,
//			Obstruction,
//			ControlExtraLossDb,
//			VideoExtraLossDb,
//			Telemetry.ControlRSSIDbm,
//			Telemetry.ControlRSSIPercent,
//			Telemetry.ControlLQPercent,
//			EffectiveControlLQ,
//			ControlPacketDebugLQOverride,
//			Telemetry.ControlSignalMarginDb,
//			Telemetry.VideoRSSIDbm,
//			Telemetry.VideoLinkPercent,
//			Telemetry.VideoSignalMarginDb,
//			Telemetry.TxPowerW,
//			bLastControlPacketReceived ? 1 : 0,
//			ControlPacketAgeSeconds,
//			bControlFailsafeActive ? 1 : 0
//		);
//	}
//}
//
//void AFPVDronePawn::UpdateReceivedControlInput(float DeltaTime)
//{
//	const float SafeDeltaTime = FMath::Max(DeltaTime, 0.f);
//
//	float LQ = Telemetry.bControlLinkValid ? Telemetry.ControlLQPercent : 100.f;
//
//	if (ControlPacketDebugLQOverride >= 0.f)
//	{
//		LQ = FMath::Clamp(ControlPacketDebugLQOverride, 0.f, 100.f);
//	}
//	EffectiveControlLQ = LQ;
//
//	if (LQ >= ControlDegradedLQ)
//	{
//		ControlInputScale = 1.f;
//	}
//	else if (LQ <= ControlCriticalLQ)
//	{
//		ControlInputScale = FMath::GetMappedRangeValueClamped(
//			FVector2D(ControlFailsafeLQ, ControlCriticalLQ),
//			FVector2D(ControlMinimumInputScale, ControlCriticalInputScale),
//			LQ
//		);
//	}
//	else
//	{
//		ControlInputScale = FMath::GetMappedRangeValueClamped(
//			FVector2D(ControlCriticalLQ, ControlDegradedLQ),
//			FVector2D(ControlCriticalInputScale, 1.f),
//			LQ
//		);
//	}
//
//	bool bPacketReceivedThisFrame = true;
//
//	if (bSimulateControlPackets)
//	{
//		bPacketReceivedThisFrame = false;
//
//		const float PacketInterval = 1.f / FMath::Max(ControlPacketRateHz, 1.f);
//		ControlPacketAccumulator += SafeDeltaTime;
//		ControlPacketAgeSeconds += SafeDeltaTime;
//
//		while (ControlPacketAccumulator >= PacketInterval)
//		{
//			ControlPacketAccumulator -= PacketInterval;
//
//			const float PacketSuccessChance = FMath::Clamp(LQ / 100.f, 0.f, 1.f);
//
//			if (FMath::FRand() <= PacketSuccessChance)
//			{
//				bPacketReceivedThisFrame = true;
//				ControlPacketAgeSeconds = 0.f;
//			}
//		}
//	}
//	else
//	{
//		ControlPacketAgeSeconds = 0.f;
//	}
//
//	bLastControlPacketReceived = bPacketReceivedThisFrame;
//
//	if (!bControlFailsafeActive)
//	{
//		if (ControlPacketAgeSeconds >= ControlPacketFailsafeTimeout)
//		{
//			bControlFailsafeActive = true;
//			ControlFailsafeActiveTime = 0.f;
//		}
//	}
//	else
//	{
//		ControlFailsafeActiveTime += SafeDeltaTime;
//
//		if (bPacketReceivedThisFrame)
//		{
//			bControlFailsafeActive = false;
//			ControlFailsafeTimer = 0.f;
//			ControlFailsafeActiveTime = 0.f;
//			ControlPacketAgeSeconds = 0.f;
//		}
//	}
//
//	if (bControlFailsafeActive)
//	{
//		ControlInputScale = 0.f;
//
//		if (ControlFailsafeActiveTime < ControlFailsafeHoldSeconds)
//		{
//			ReceivedThrottle = LastValidReceivedThrottle;
//			ReceivedPitchInput = LastValidReceivedPitchInput;
//			ReceivedRollInput = LastValidReceivedRollInput;
//			ReceivedYawInput = LastValidReceivedYawInput;
//		}
//		else
//		{
//			ReceivedThrottle = 0.f;
//			ReceivedPitchInput = 0.f;
//			ReceivedRollInput = 0.f;
//			ReceivedYawInput = 0.f;
//		}
//
//		Telemetry.ControlInputScale = ControlInputScale;
//		Telemetry.bControlFailsafeActive = bControlFailsafeActive;
//		return;
//	}
//
//	if (bPacketReceivedThisFrame)
//	{
//		ReceivedThrottle = Throttle;
//		ReceivedPitchInput = PitchInput * ControlInputScale;
//		ReceivedRollInput = RollInput * ControlInputScale;
//		ReceivedYawInput = YawInput * ControlInputScale;
//
//		LastValidReceivedThrottle = ReceivedThrottle;
//		LastValidReceivedPitchInput = ReceivedPitchInput;
//		LastValidReceivedRollInput = ReceivedRollInput;
//		LastValidReceivedYawInput = ReceivedYawInput;
//	}
//
//	Telemetry.ControlInputScale = ControlInputScale;
//	Telemetry.bControlFailsafeActive = bControlFailsafeActive;
//}
//
//float AFPVDronePawn::ComputeOperatorObstructionFactor() const
//{
//	if (!GetWorld() || !PlaneMesh)
//	{
//		return 1.f;
//	}
//
//	const FVector DroneLocation = PlaneMesh->GetComponentLocation();
//	const FVector OperatorEyeLocation = OperatorLocation + FVector(0.f, 0.f, 170.f);
//
//	const FVector Direction = (OperatorEyeLocation - DroneLocation).GetSafeNormal();
//	const FVector Right = FVector::CrossProduct(Direction, FVector::UpVector).GetSafeNormal();
//	const FVector Up = FVector::CrossProduct(Right, Direction).GetSafeNormal();
//
//	const float TraceSpreadCm = 120.f;
//
//	TArray<FVector> Offsets;
//	Offsets.Add(FVector::ZeroVector);
//	Offsets.Add(Right * TraceSpreadCm);
//	Offsets.Add(-Right * TraceSpreadCm);
//	Offsets.Add(Up * TraceSpreadCm);
//	Offsets.Add(-Up * TraceSpreadCm);
//
//	FCollisionQueryParams Params;
//	Params.AddIgnoredActor(this);
//
//	int32 BlockedCount = 0;
//
//	for (const FVector& Offset : Offsets)
//	{
//		FHitResult Hit;
//
//		const FVector Start = DroneLocation + Offset;
//		const FVector End = OperatorEyeLocation + Offset;
//
//		const bool bHit = GetWorld()->LineTraceSingleByChannel(
//			Hit,
//			Start,
//			End,
//			ECC_Visibility,
//			Params
//		);
//
//		if (bHit)
//		{
//			BlockedCount++;
//		}
//	}
//
//	const float RawObstruction = static_cast<float>(BlockedCount) / static_cast<float>(Offsets.Num());
//
//	return FMath::Clamp(RawObstruction, 0.f, 1.f);
//}
//
//float AFPVDronePawn::WattsToDbm(float Watts) const
//{
//	const float Milliwatts = FMath::Max(Watts * 1000.f, 0.001f);
//	return 10.f * FMath::LogX(10.f, Milliwatts);
//}
//
//float AFPVDronePawn::ComputeFreeSpacePathLossDb(float DistanceM, float FrequencyMHz) const
//{
//	const float DistanceKm = FMath::Max(DistanceM / 1000.f, 0.001f);
//	const float Frequency = FMath::Max(FrequencyMHz, 1.f);
//
//	return 32.44f
//		+ 20.f * FMath::LogX(10.f, DistanceKm)
//		+ 20.f * FMath::LogX(10.f, Frequency);
//}
//
//float AFPVDronePawn::ComputeReceivedPowerDbm(
//	float TxPowerW,
//	float TxGainDbi,
//	float RxGainDbi,
//	float FrequencyMHz,
//	float DistanceM,
//	float ExtraLossDb
//) const
//{
//	const float TxPowerDbm = WattsToDbm(TxPowerW);
//	const float PathLossDb = ComputeFreeSpacePathLossDb(DistanceM, FrequencyMHz);
//
//	return TxPowerDbm + TxGainDbi + RxGainDbi - PathLossDb - ExtraLossDb;
//}
//
//float AFPVDronePawn::ComputeSignalBarPercent(float RSSIDbm, float SensitivityDbm) const
//{
//	return FMath::GetMappedRangeValueClamped(
//		FVector2D(SensitivityDbm - 5.f, SensitivityDbm + 40.f),
//		FVector2D(0.f, 100.f),
//		RSSIDbm
//	);
//}
//
//float AFPVDronePawn::ComputeControlLQFromMargin(float MarginDb) const
//{
//	if (MarginDb >= 18.f)
//	{
//		return 100.f;
//	}
//
//	if (MarginDb >= 6.f)
//	{
//		return FMath::GetMappedRangeValueClamped(
//			FVector2D(6.f, 18.f),
//			FVector2D(75.f, 100.f),
//			MarginDb
//		);
//	}
//
//	if (MarginDb >= 0.f)
//	{
//		return FMath::GetMappedRangeValueClamped(
//			FVector2D(0.f, 6.f),
//			FVector2D(25.f, 75.f),
//			MarginDb
//		);
//	}
//
//	return FMath::GetMappedRangeValueClamped(
//		FVector2D(-8.f, 0.f),
//		FVector2D(0.f, 25.f),
//		MarginDb
//	);
//}
//
//float AFPVDronePawn::ComputeVideoQualityFromMargin(float MarginDb) const
//{
//	if (MarginDb >= 25.f)
//	{
//		return 100.f;
//	}
//
//	if (MarginDb >= 10.f)
//	{
//		return FMath::GetMappedRangeValueClamped(
//			FVector2D(10.f, 25.f),
//			FVector2D(65.f, 100.f),
//			MarginDb
//		);
//	}
//
//	if (MarginDb >= 0.f)
//	{
//		return FMath::GetMappedRangeValueClamped(
//			FVector2D(0.f, 10.f),
//			FVector2D(25.f, 65.f),
//			MarginDb
//		);
//	}
//
//	return FMath::GetMappedRangeValueClamped(
//		FVector2D(-8.f, 0.f),
//		FVector2D(0.f, 25.f),
//		MarginDb
//	);
//}
//
//void AFPVDronePawn::FindKillCamPosition(const FVector& HitLocation)
//{
//	if (!GetWorld()) return;
//
//	// Пробуємо 8 напрямків навколо точки зіткнення
//	const TArray<FVector> Directions = {
//		FVector(1, 0, 0), FVector(-1, 0, 0),
//		FVector(0, 1, 0), FVector(0, -1, 0),
//		FVector(1, 1, 0).GetSafeNormal(),
//		FVector(-1, 1, 0).GetSafeNormal(),
//		FVector(1, -1, 0).GetSafeNormal(),
//		FVector(-1,-1, 0).GetSafeNormal()
//	};
//
//	FCollisionQueryParams Params;
//	Params.AddIgnoredActor(this);
//
//	FVector BestPos = HitLocation + FVector(0, 0, KillCamDistance);
//	float BestDot = -1.f;
//
//	// Беремо вектор польоту дрона до зіткнення
//	const FVector DroneForward = GetActorForwardVector();
//
//	for (const FVector& Dir : Directions)
//	{
//		// Камера збоку-ззаду, трохи вище
//		const FVector Offset = Dir * KillCamDistance + FVector(0, 0, KillCamDistance * 0.5f);
//		const FVector CandidatePos = HitLocation + Offset;
//
//		FHitResult TraceHit;
//		const bool bBlocked = GetWorld()->LineTraceSingleByChannel(
//			TraceHit, HitLocation, CandidatePos,
//			ECC_Visibility, Params
//		);
//
//		if (!bBlocked)
//		{
//			// Перевагу даємо напрямку збоку від вектора польоту
//			const float Dot = FMath::Abs(FVector::DotProduct(Dir, DroneForward));
//			// Dot ~0 = перпендикулярно до польоту = добре видно
//			const float Score = 1.f - Dot;
//			if (Score > BestDot)
//			{
//				BestDot = Score;
//				BestPos = CandidatePos;
//			}
//		}
//	}
//
//	KillCamLocation = BestPos;
//}
//
//void AFPVDronePawn::StartKillCam(const FVector& HitLocation)
//{
//	if (!GetWorld() || !PlaneMesh || !Camera || !KillCamCamera)
//	{
//		return;
//	}
//
//	KillCamTimer = KillCamDuration;
//	KillCamReplayTime = 0.f;
//	KillCamReplayDuration = 0.f;
//
//	CrashLocation = HitLocation;
//	KillCamLocation = HitLocation + FVector(0.f, 0.f, 1000.f);
//
//	KillCamCamera->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
//	KillCamCamera->SetWorldLocation(KillCamLocation);
//	KillCamCamera->SetWorldRotation(
//		(HitLocation - KillCamLocation).GetSafeNormal().ToOrientationRotator()
//	);
//
//	KillCamCamera->SetActive(true);
//	Camera->SetActive(false);
//
//	PlaneMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
//	PlaneMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
//	PlaneMesh->SetSimulatePhysics(false);
//	PlaneMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
//	PlaneMesh->SetVisibility(false);
//
//	if (APlayerController* PC = Cast<APlayerController>(GetController()))
//	{
//		PC->SetViewTarget(this);
//	}
//
//	if (bKillCamExplosionPending && !bKillCamExplosionSpawned)
//	{
//		bKillCamExplosionSpawned = true;
//		SpawnCrashExplosion(CrashLocation);
//	}
//
//	bKillCamActive = true;
//}
//
//
//void AFPVDronePawn::EndKillCam()
//{
//	bKillCamActive = false;
//	KillCamCamera->SetActive(false);
//	Camera->SetActive(true);
//
//	PlaneMesh->SetVisibility(true);
//	PlaneMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
//	PlaneMesh->SetSimulatePhysics(true);
//
//	SetActorLocation(SpawnLocation);
//	SetActorRotation(SpawnRotation);
//
//	PlaneMesh->SetWorldLocationAndRotation(
//		SpawnLocation,
//		SpawnRotation,
//		false,
//		nullptr,
//		ETeleportType::TeleportPhysics
//	);
//
//	PlaneMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
//	PlaneMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
//
//	LastSpawnWorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
//
//	bCrashed = false;
//	CrashTimer = CrashRespawnDelay;
//	KillCamReplayTime = 0.f;
//	KillCamReplayDuration = 0.f;
//	KillCamFrames.Empty();
//
//	ResetDroneStateAfterRespawn();
//}
//void AFPVDronePawn::RecordKillCamFrame()
//{
//	if (!GetWorld() || !PlaneMesh)
//	{
//		return;
//	}
//
//	const float Now = GetWorld()->GetTimeSeconds();
//
//	FKillCamFrame Frame;
//	Frame.Time = Now;
//	Frame.Location = PlaneMesh->GetComponentLocation();
//	Frame.Rotation = PlaneMesh->GetComponentQuat();
//
//	KillCamFrames.Add(Frame);
//
//	const float KeepSeconds = KillCamReplaySeconds + 0.3f;
//
//	while (KillCamFrames.Num() > 1 && Now - KillCamFrames[0].Time > KeepSeconds)
//	{
//		KillCamFrames.RemoveAt(0);
//	}
//}
//
//FTransform AFPVDronePawn::SampleKillCamTransform(float WorldTime) const
//{
//	if (KillCamFrames.Num() == 0)
//	{
//		return PlaneMesh ? PlaneMesh->GetComponentTransform() : FTransform::Identity;
//	}
//
//	if (WorldTime <= KillCamFrames[0].Time)
//	{
//		return FTransform(KillCamFrames[0].Rotation, KillCamFrames[0].Location, FVector(1.f));
//	}
//
//	const int32 LastIndex = KillCamFrames.Num() - 1;
//
//	if (WorldTime >= KillCamFrames[LastIndex].Time)
//	{
//		return FTransform(KillCamFrames[LastIndex].Rotation, KillCamFrames[LastIndex].Location, FVector(1.f));
//	}
//
//	for (int32 i = 0; i < LastIndex; ++i)
//	{
//		const FKillCamFrame& A = KillCamFrames[i];
//		const FKillCamFrame& B = KillCamFrames[i + 1];
//
//		if (WorldTime >= A.Time && WorldTime <= B.Time)
//		{
//			const float Alpha = (WorldTime - A.Time) / FMath::Max(B.Time - A.Time, KINDA_SMALL_NUMBER);
//			const FVector Location = FMath::Lerp(A.Location, B.Location, Alpha);
//			const FQuat Rotation = FQuat::Slerp(A.Rotation, B.Rotation, Alpha).GetNormalized();
//
//			return FTransform(Rotation, Location, FVector(1.f));
//		}
//	}
//
//	return FTransform(KillCamFrames[LastIndex].Rotation, KillCamFrames[LastIndex].Location, FVector(1.f));
//}
//
//void AFPVDronePawn::HandleCrash(const FVector& HitLocation)
//{
//	if (bCrashed)
//	{
//		return;
//	}
//
//	bCrashed = true;
//	CrashLocation = HitLocation;
//	bKillCamExplosionPending = bBombArmedState;
//	bKillCamExplosionSpawned = false;
//
//	StartKillCam(HitLocation);
//}
//
//void AFPVDronePawn::SpawnCrashExplosion(const FVector& HitLocation)
//{
//	if (!ExplosionEffect || !GetWorld())
//	{
//		return;
//	}
//
//	UGameplayStatics::SpawnEmitterAtLocation(
//		GetWorld(),
//		ExplosionEffect,
//		HitLocation,
//		FRotator::ZeroRotator,
//		FVector(1.f)
//	);
//}
//
//void AFPVDronePawn::UpdateKillCamReplay(float DeltaSeconds)
//{
//	if (!KillCamCamera)
//	{
//		EndKillCam();
//		return;
//	}
//
//	KillCamLocation = CrashLocation + FVector(0.f, 0.f, 1000.f);
//
//	KillCamCamera->SetWorldLocation(KillCamLocation);
//	KillCamCamera->SetWorldRotation(
//		(CrashLocation - KillCamLocation).GetSafeNormal().ToOrientationRotator()
//	);
//
//	KillCamTimer -= DeltaSeconds;
//
//	if (KillCamTimer <= 0.f)
//	{
//		EndKillCam();
//	}
//}
//
//bool AFPVDronePawn::ShouldIgnoreCrashHit(const FVector& NormalImpulse) const
//{
//	if (!GetWorld() || !PlaneMesh)
//	{
//		return true;
//	}
//
//	if (bCrashed)
//	{
//		return true;
//	}
//
//	const float Now = GetWorld()->GetTimeSeconds();
//	const float TimeSinceSpawn = Now - LastSpawnWorldTime;
//
//	if (TimeSinceSpawn < CrashSpawnGraceSeconds)
//	{
//		return true;
//	}
//
//	if (bBombArmedState)
//	{
//		return false;
//	}
//
//	const float SpeedMps = PlaneMesh->GetPhysicsLinearVelocity().Size() / 100.f;
//	const float ImpulseSize = NormalImpulse.Size();
//
//	if (SpeedMps < CrashMinImpactSpeedMps && ImpulseSize < CrashMinNormalImpulse)
//	{
//		return true;
//	}
//
//	return false;
//}
//
//void AFPVDronePawn::ResetDroneStateAfterRespawn()
//{
//	Throttle = 0.f;
//	PitchInput = 0.f;
//	RollInput = 0.f;
//	YawInput = 0.f;
//
//	ReceivedThrottle = 0.f;
//	ReceivedPitchInput = 0.f;
//	ReceivedRollInput = 0.f;
//	ReceivedYawInput = 0.f;
//
//	LastValidReceivedThrottle = 0.f;
//	LastValidReceivedPitchInput = 0.f;
//	LastValidReceivedRollInput = 0.f;
//	LastValidReceivedYawInput = 0.f;
//
//	ControlInputScale = 1.f;
//	bControlFailsafeActive = false;
//	ControlFailsafeTimer = 0.f;
//	ControlFailsafeActiveTime = 0.f;
//
//	ControlPacketAccumulator = 0.f;
//	ControlPacketAgeSeconds = 0.f;
//	bLastControlPacketReceived = true;
//
//	SmoothedControlRSSI = 100.f;
//	SmoothedControlLQ = 100.f;
//	SmoothedVideoLink = 100.f;
//
//	SignalFadeTimer = 0.f;
//	TargetControlFadeLossDb = 0.f;
//	TargetVideoFadeLossDb = 0.f;
//	SmoothedControlFadeLossDb = 0.f;
//	SmoothedVideoFadeLossDb = 0.f;
//
//	bArmedState = false;
//	bBombArmedState = false;
//
//	bMouseJoystickEnabled = false;
//	AccumulatedX = 0.f;
//	AccumulatedY = 0.f;
//
//	BaroZeroZ = GetActorLocation().Z;
//	TelemetryStartTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
//
//	Telemetry = FDroneTelemetry();
//	UpdateTelemetry();
//}