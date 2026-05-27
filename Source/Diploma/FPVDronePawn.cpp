#include "FPVDronePawn.h"
#include "DroneSignalComponent.h"
#include "FPVBatteryComponent.h"
#include "FPVMotorComponent.h"
#include "FPVFlightControllerComponent.h"
#include "FPVControllerSettingsSaveGame.h"
#include "ManualRadialDamage.h"
#include "MissionScenarioController.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InputComponent.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Engine/Engine.h"
#include "Engine/EngineTypes.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Particles/ParticleSystem.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/InputSettings.h"
#include "GameFramework/PlayerInput.h"


AFPVDronePawn::AFPVDronePawn()
{
    PrimaryActorTick.bCanEverTick = true;

    struct FConstructorStatics
    {
        ConstructorHelpers::FObjectFinderOptional<UStaticMesh> PlaneMesh;
        FConstructorStatics()
            : PlaneMesh(TEXT("/Game/FPV/FPV.FPV"))
        {
        }
    };
    static FConstructorStatics ConstructorStatics;

    PlaneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaneMesh0"));
    PlaneMesh->SetStaticMesh(ConstructorStatics.PlaneMesh.Get());
    RootComponent = PlaneMesh;
    PlaneMesh->SetCollisionProfileName(TEXT("PhysicsActor"));

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera0"));
    Camera->SetupAttachment(PlaneMesh);
    Camera->bUsePawnControlRotation = false;
    Camera->SetRelativeRotation(FRotator::ZeroRotator);
    Camera->FieldOfView = 100.f;
    Camera->SetRelativeLocation(FVector(0.f, 0.f, 0.f));

    KillCamCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("KillCamCamera0"));
    KillCamCamera->SetupAttachment(RootComponent);
    KillCamCamera->bUsePawnControlRotation = false;
    KillCamCamera->SetActive(false);

    BatteryComponent = CreateDefaultSubobject<UFPVBatteryComponent>(TEXT("BatteryComponent"));
    SignalComponent = CreateDefaultSubobject<UDroneSignalComponent>(TEXT("SignalComponent"));
    MotorComponent = CreateDefaultSubobject<UFPVMotorComponent>(TEXT("MotorComponent"));
    FlightControllerComponent = CreateDefaultSubobject<UFPVFlightControllerComponent>(TEXT("FlightControllerComponent"));

    ControllerSettings = FFPVControllerSettings::MakeDefault();

    Throttle = 0.f;
    PitchInput = 0.f;
    RollInput = 0.f;
    YawInput = 0.f;
}

void AFPVDronePawn::BeginPlay()
{
    Super::BeginPlay();

    /*if (GEngine)
    {
        GEngine->Exec(GetWorld(), TEXT("stat fps"));
        GEngine->Exec(GetWorld(), TEXT("stat unit"));
    }*/

    BaroZeroZ = GetActorLocation().Z;

    if (SignalComponent)
    {
        SignalComponent->SetOperatorLocation(GetActorLocation());
    }
    TelemetryStartTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
    SpawnLocation = GetActorLocation();
    SpawnRotation = GetActorRotation();
    LastSpawnWorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

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

    LoadSavedControllerSettings();

    if (FlightControllerComponent)
    {
        FlightControllerComponent->ResetController();
    }

    if (BatteryComponent)
    {
        BatteryComponent->ResetBatteryState();
    }

    if (SignalComponent)
    {
        SignalComponent->ResetSignalState();
    }


    if (MotorComponent)
    {
        MotorComponent->InitMotors();
    }

    //PlaneMesh->SetLinearDamping(0.f);
    //PlaneMesh->SetAngularDamping(0.2f);
    PlaneMesh->SetCollisionProfileName(TEXT("PhysicsActor"));
    PlaneMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    PlaneMesh->SetCollisionObjectType(ECC_PhysicsBody);
    PlaneMesh->SetCollisionResponseToAllChannels(ECR_Block);
    PlaneMesh->SetNotifyRigidBodyCollision(true);
    PlaneMesh->BodyInstance.SetInstanceNotifyRBCollision(true);
    PlaneMesh->SetSimulatePhysics(true);
    PlaneMesh->SetEnableGravity(true);
    PlaneMesh->SetMassOverrideInKg(NAME_None, 3.921f, true);
    PlaneMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
    PlaneMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
    PlaneMesh->BodyInstance.InertiaTensorScale = FVector(1.f, 1.f, 0.1f);
    PlaneMesh->RecreatePhysicsState();
    PlaneMesh->SetCenterOfMass(FVector(-0.884f, -0.006f, -0.101f));

    //unsafe
    PlaneMesh->OnComponentHit.RemoveDynamic(this, &AFPVDronePawn::OnHit);
    PlaneMesh->OnComponentHit.AddDynamic(this, &AFPVDronePawn::OnHit);
    //



    const FVector I = PlaneMesh->GetInertiaTensor();

    UE_LOG(LogTemp, Warning,
        TEXT("New Inertia: X=%.2f Y=%.2f Z=%.2f"),
        I.X, I.Y, I.Z);

    const FVector ComWorld = PlaneMesh->GetCenterOfMass();
    const FVector ComLocal = PlaneMesh->GetComponentTransform().InverseTransformPosition(ComWorld);

    UE_LOG(LogTemp, Warning, TEXT("COM World: %s"), *ComWorld.ToString());
    UE_LOG(LogTemp, Warning, TEXT("COM Local: %s"), *ComLocal.ToString());

    UE_LOG(LogTemp, Warning, TEXT("Actor Rotation: %s"), *GetActorRotation().ToString());
    UE_LOG(LogTemp, Warning, TEXT("Mesh Rotation: %s"), *PlaneMesh->GetComponentRotation().ToString());


    const TArray<FMotorState>& ActiveMotors = GetActiveMotors();

    for (int i = 0; i < ActiveMotors.Num(); i++)
    {
        UE_LOG(LogTemp, Warning, TEXT("Motor %d LocalPos=(%.2f %.2f %.2f)"),
            i,
            ActiveMotors[i].LocalPosition.X,
            ActiveMotors[i].LocalPosition.Y,
            ActiveMotors[i].LocalPosition.Z);
    }

    UE_LOG(LogTemp, Warning, TEXT("Actor: %s"), *GetName());
    UE_LOG(LogTemp, Warning, TEXT("PlaneMesh Rel: %s  World: %s"),
        *PlaneMesh->GetRelativeLocation().ToString(),
        *PlaneMesh->GetComponentLocation().ToString());

    UE_LOG(LogTemp, Warning, TEXT("Camera Rel: %s  World: %s  AttachParent: %s"),
        *Camera->GetRelativeLocation().ToString(),
        *Camera->GetComponentLocation().ToString(),
        Camera->GetAttachParent() ? *Camera->GetAttachParent()->GetName() : TEXT("None"));
}

void AFPVDronePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    check(PlayerInputComponent);
    UE_LOG(LogTemp, Warning, TEXT("SetupPlayerInputComponent called for %s"), *GetName());

    PlayerInputComponent->BindAxis("Throttle", this, &AFPVDronePawn::ThrottleInput);
    PlayerInputComponent->BindAxis("Pitch", this, &AFPVDronePawn::PitchInputAxis);
    PlayerInputComponent->BindAxis("Roll", this, &AFPVDronePawn::RollInputAxis);
    PlayerInputComponent->BindAxis("Yaw", this, &AFPVDronePawn::YawInputAxis);

    PlayerInputComponent->BindAction("Arm", IE_Pressed, this, &AFPVDronePawn::ToggleArm);
    PlayerInputComponent->BindAction("BombArm", IE_Pressed, this, &AFPVDronePawn::ToggleBombArm);
    PlayerInputComponent->BindAction("CycleFlightMode", IE_Pressed, this, &AFPVDronePawn::CycleFlightMode);

    PlayerInputComponent->BindAxis("ArmSwitch", this, &AFPVDronePawn::ArmSwitchAxis);
    PlayerInputComponent->BindAxis("BombArmSwitch", this, &AFPVDronePawn::BombArmSwitchAxis);
    PlayerInputComponent->BindAxis("FlightModeSwitch", this, &AFPVDronePawn::FlightModeSwitchAxis);
    PlayerInputComponent->BindAxis("AcroTrainerSwitch", this, &AFPVDronePawn::AcroTrainerSwitchAxis);
}

void AFPVDronePawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    LastDeltaSeconds = DeltaSeconds;

    if (bCrashed)
    {
        if (bKillCamActive)
        {
            UpdateKillCamReplay(DeltaSeconds);
        }
        return;
    }

    UpdateMotorThrusts(DeltaSeconds);
    UpdateMotorDynamics(DeltaSeconds);
    ApplyThrust();

    UpdateTelemetry();
}

void AFPVDronePawn::NotifyHit(UPrimitiveComponent* MyComp, AActor* Other,
    UPrimitiveComponent* OtherComp, bool bSelfMoved,
    FVector HitLocation, FVector HitNormal,
    FVector NormalImpulse, const FHitResult& Hit)
{
    Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved,
        HitLocation, HitNormal, NormalImpulse, Hit);

    if (ShouldIgnoreCrashHit(Other, NormalImpulse))
    {
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("CRASH NOTIFY HIT | Other=%s | Location=%s | Impulse=%s | Speed=%.1fkmh | Bomb=%d"),
        Other ? *Other->GetName() : TEXT("None"),
        *HitLocation.ToString(),
        *NormalImpulse.ToString(),
        GetCurrentSpeedKmh(),
        bBombArmedState ? 1 : 0
    );

    HandleCrashExplosion(Hit);
    HandleCrash(HitLocation);
}

void AFPVDronePawn::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    const FVector HitLocation = Hit.ImpactPoint.IsNearlyZero() ? Hit.Location : Hit.ImpactPoint;

    if (ShouldIgnoreCrashHit(OtherActor, NormalImpulse))
    {
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("CRASH COMPONENT HIT | Other=%s | Impulse=%s | ImpactPoint=%s | Speed=%.1fkmh | Bomb=%d"),
        OtherActor ? *OtherActor->GetName() : TEXT("None"),
        *NormalImpulse.ToString(),
        *HitLocation.ToString(),
        GetCurrentSpeedKmh(),
        bBombArmedState ? 1 : 0
    );

    HandleCrashExplosion(Hit);
    HandleCrash(HitLocation);
}

void AFPVDronePawn::ToggleArm()
{
    SetArmState(!bArmedState);
}

void AFPVDronePawn::ToggleBombArm()
{
    SetBombArmState(!bBombArmedState);
}

void AFPVDronePawn::PitchInputAxis(float Value)
{
    PitchInput = ApplyCenteredInputCurve(NormalizeCenteredAxis(Value), ControllerSettings.Pitch.DeadZone, ControllerSettings.Pitch.Expo);
}

void AFPVDronePawn::RollInputAxis(float Value)
{
    RollInput = ApplyCenteredInputCurve(NormalizeCenteredAxis(Value), ControllerSettings.Roll.DeadZone, ControllerSettings.Roll.Expo);
}

void AFPVDronePawn::YawInputAxis(float Value)
{
    YawInput = ApplyCenteredInputCurve(NormalizeCenteredAxis(Value), ControllerSettings.Yaw.DeadZone, ControllerSettings.Yaw.Expo);
}

void AFPVDronePawn::ThrottleInput(float Value)
{
    Throttle = ApplyThrottleInputCurve(NormalizeThrottle(Value), ControllerSettings.ThrottleDeadZone, ControllerSettings.ThrottleExpo);
}

void AFPVDronePawn::UpdateBaseTelemetry()
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


    Telemetry.bBombArmed = bBombArmedState;
    Telemetry.bKillCamActive = bKillCamActive;
    Telemetry.bCrashed = bCrashed;

    UpdateSignalTelemetry(LastDeltaSeconds);
}

float AFPVDronePawn::GetRadioAltitudeMeters(bool& bValid) const
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

float AFPVDronePawn::NormalizeThrottle(float Raw) const
{
    const float Min = 0.15f;
    const float Max = 0.85f;
    const float Value = (Raw - Min) / (Max - Min);
    return FMath::Clamp(Value, 0.f, 1.f);
}

float AFPVDronePawn::NormalizeCenteredAxis(float Raw) const
{
    const float Value = (Raw - 0.5f) * 2.0f;
    return FMath::Clamp(Value, -1.f, 1.f);
}

float AFPVDronePawn::ApplyCenteredInputCurve(float Value, float DeadZone, float Expo) const
{
    const float ClampedDeadZone = FMath::Clamp(DeadZone, 0.0f, 0.95f);
    const float ClampedExpo = FMath::Clamp(Expo, 1.0f, 3.0f);
    const float ClampedValue = FMath::Clamp(Value, -1.0f, 1.0f);
    const float AbsValue = FMath::Abs(ClampedValue);

    if (AbsValue <= ClampedDeadZone)
    {
        return 0.0f;
    }

    return FMath::Sign(ClampedValue) * FMath::Pow(AbsValue, ClampedExpo);
}

float AFPVDronePawn::ApplyThrottleInputCurve(float Value, float DeadZone, float Expo) const
{
    const float ClampedDeadZone = FMath::Clamp(DeadZone, 0.0f, 0.95f);
    const float ClampedExpo = FMath::Clamp(Expo, 1.0f, 3.0f);
    const float ClampedValue = FMath::Clamp(Value, 0.0f, 1.0f);

    if (ClampedValue <= ClampedDeadZone)
    {
        return 0.0f;
    }

    return FMath::Pow(ClampedValue, ClampedExpo);
}

void AFPVDronePawn::LoadSavedControllerSettings()
{
    FFPVControllerSettings LoadedSettings = FFPVControllerSettings::MakeDefault();

    if (UGameplayStatics::DoesSaveGameExist(TEXT("FPVControllerSettings"), 0))
    {
        if (UFPVControllerSettingsSaveGame* Save = Cast<UFPVControllerSettingsSaveGame>(UGameplayStatics::LoadGameFromSlot(TEXT("FPVControllerSettings"), 0)))
        {
            LoadedSettings = Save->Settings;
        }
    }

    ApplyControllerSettings(LoadedSettings, true);
}

void AFPVDronePawn::ApplyControllerSettings(const FFPVControllerSettings& NewSettings, bool bApplyInputMappings)
{
    ControllerSettings = NewSettings;
    ControllerSettings.Clamp();

    if (FlightControllerComponent)
    {
        FlightControllerComponent->ApplyControllerSettings(ControllerSettings);
    }

    if (bApplyInputMappings)
    {
        ApplyKeyboardActionMapping(TEXT("Arm"), ControllerSettings.ArmKey);
        ApplyKeyboardActionMapping(TEXT("BombArm"), ControllerSettings.BombArmKey);
        ApplyKeyboardActionMapping(TEXT("CycleFlightMode"), ControllerSettings.CycleFlightModeKey);
        RebuildInputMappings();
    }
}

bool AFPVDronePawn::IsAllowedKeyboardActionKey(const FKey& Key) const
{
    if (!Key.IsValid())
    {
        return false;
    }

    if (Key == EKeys::AnyKey)
    {
        return false;
    }

    if (Key.IsGamepadKey() || Key.IsMouseButton())
    {
        return false;
    }

    return true;
}

void AFPVDronePawn::ApplyKeyboardActionMapping(const FName& ActionName, const FKey& Key)
{
    if (!IsAllowedKeyboardActionKey(Key))
    {
        return;
    }

    UInputSettings* InputSettings = UInputSettings::GetInputSettings();
    if (!InputSettings)
    {
        return;
    }

    TArray<FInputActionKeyMapping> ExistingMappings;
    InputSettings->GetActionMappingByName(ActionName, ExistingMappings);

    for (const FInputActionKeyMapping& Mapping : ExistingMappings)
    {
        if (!Mapping.Key.IsGamepadKey())
        {
            InputSettings->RemoveActionMapping(Mapping, false);
        }
    }

    InputSettings->AddActionMapping(FInputActionKeyMapping(ActionName, Key), false);
}

void AFPVDronePawn::RebuildInputMappings()
{
    if (UInputSettings* InputSettings = UInputSettings::GetInputSettings())
    {
        InputSettings->SaveKeyMappings();
    }

    if (!GetWorld())
    {
        return;
    }

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (APlayerController* PC = It->Get())
        {
            if (PC->PlayerInput)
            {
                PC->PlayerInput->ForceRebuildingKeyMaps(true);
            }
        }
    }
}

void AFPVDronePawn::UpdateSignalTelemetry(float DeltaTime)
{
    if (!SignalComponent)
    {
        return;
    }

    SignalComponent->UpdateSignalTelemetry(
        DeltaTime,
        PlaneMesh,
        Throttle,
        PitchInput,
        RollInput,
        YawInput,
        Telemetry,
        FPVPostProcessMID
    );
}

void AFPVDronePawn::HandleCrash(const FVector& HitLocation)
{
    if (bCrashed)
    {
        return;
    }

    bCrashed = true;
    CrashLocation = HitLocation;
    bKillCamExplosionPending = bBombArmedState;
    bKillCamExplosionSpawned = false;

    ReportDroneUsedToMission();

    UE_LOG(LogTemp, Warning, TEXT("[DRONE CRASH] Crash handled | Location=%s Speed=%.1fkmh Bomb=%d ExplosionPending=%d"),
        *HitLocation.ToString(),
        GetCurrentSpeedKmh(),
        bBombArmedState ? 1 : 0,
        bKillCamExplosionPending ? 1 : 0);

    StartKillCam(HitLocation);
}

void AFPVDronePawn::ReportDroneUsedToMission()
{
    if (bMissionDroneUseReported)
    {
        return;
    }

    bMissionDroneUseReported = true;

    if (AMissionScenarioController* MissionController = Cast<AMissionScenarioController>(
        UGameplayStatics::GetActorOfClass(GetWorld(), AMissionScenarioController::StaticClass())))
    {
        MissionController->NotifyDroneUsed();
    }
}

void AFPVDronePawn::ForceCrashAtLocation(const FVector& HitLocation)
{
    if (bCrashed)
    {
        return;
    }

    ReportDroneUsedToMission();

    UE_LOG(LogTemp, Warning, TEXT("[DRONE FORCE CRASH] Location=%s Bomb=%d"),
        *HitLocation.ToString(),
        bBombArmedState ? 1 : 0);

    if (bBombArmedState)
    {
        ApplyExplosionDamage(HitLocation);
    }

    HandleCrash(HitLocation);
}
void AFPVDronePawn::SpawnCrashExplosion(const FVector& HitLocation)
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

bool AFPVDronePawn::ShouldIgnoreCrashHit(AActor* OtherActor, const FVector& NormalImpulse) const
{
    if (!GetWorld() || !PlaneMesh)
    {
        return true;
    }

    if (bCrashed)
    {
        return true;
    }

    const float Now = GetWorld()->GetTimeSeconds();
    const float TimeSinceSpawn = Now - LastSpawnWorldTime;

    if (TimeSinceSpawn < CrashSpawnGraceSeconds)
    {
        return true;
    }

    if (IsInfantryBulletActor(OtherActor))
    {
        UE_LOG(LogTemp, Warning, TEXT("[DRONE HIT] Infantry bullet hit | Bullet=%s | Bomb=%d"),
            *GetNameSafe(OtherActor),
            bBombArmedState ? 1 : 0);

        return false;
    }

    const float SpeedKmh = GetCurrentSpeedKmh();
    const float RequiredSpeedKmh = bBombArmedState
        ? CrashBombArmedMinImpactSpeedKmh
        : CrashUnarmedMinImpactSpeedKmh;

    if (SpeedKmh < RequiredSpeedKmh)
    {
        return true;
    }

    return false;
}

bool AFPVDronePawn::IsInfantryBulletActor(const AActor* OtherActor) const
{
    if (!OtherActor)
    {
        return false;
    }

    if (OtherActor->ActorHasTag(InfantryBulletTag))
    {
        return true;
    }

    const FString ActorName = OtherActor->GetName();

    return ActorName.Contains(TEXT("BP_Bullet")) || ActorName.Contains(TEXT("InfantryBullet"));
}

float AFPVDronePawn::GetCurrentSpeedKmh() const
{
    if (!PlaneMesh)
    {
        return 0.0f;
    }

    return PlaneMesh->GetPhysicsLinearVelocity().Size() * 0.036f;
}

void AFPVDronePawn::StartKillCam(const FVector& HitLocation)
{
    if (!GetWorld() || !PlaneMesh || !Camera || !KillCamCamera)
    {
        return;
    }

    KillCamTimer = KillCamDuration;

    CrashLocation = HitLocation;
    KillCamLocation = HitLocation + FVector(0.f, 0.f, 1000.f);

    KillCamCamera->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
    KillCamCamera->SetWorldLocation(KillCamLocation);
    KillCamCamera->SetWorldRotation(
        (HitLocation - KillCamLocation).GetSafeNormal().ToOrientationRotator()
    );

    KillCamCamera->SetActive(true);
    Camera->SetActive(false);

    if (bKillCamExplosionPending)
    {
        PlaneMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
        PlaneMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
        PlaneMesh->SetSimulatePhysics(false);
        PlaneMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        PlaneMesh->SetVisibility(false);

        if (!bKillCamExplosionSpawned)
        {
            bKillCamExplosionSpawned = true;
            SpawnCrashExplosion(CrashLocation);
        }
    }
    else
    {
        PlaneMesh->SetVisibility(true);
        PlaneMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        PlaneMesh->SetSimulatePhysics(true);
        PlaneMesh->SetEnableGravity(true);
        PlaneMesh->WakeAllRigidBodies();
    }

    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        PC->SetViewTarget(this);
    }

    bKillCamActive = true;
}

void AFPVDronePawn::EndKillCam()
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
    bMissionDroneUseReported = false;

    ResetDroneStateAfterRespawn();
}

bool AFPVDronePawn::ShouldHoldKillCamAfterMissionEnd() const
{
    if (!GetWorld())
    {
        return false;
    }

    AMissionScenarioController* MissionController = Cast<AMissionScenarioController>(
        UGameplayStatics::GetActorOfClass(GetWorld(), AMissionScenarioController::StaticClass())
    );

    if (!MissionController)
    {
        return false;
    }

    return MissionController->IsMissionFinished() && MissionController->GetRemainingDroneCount() <= 0;
}

void AFPVDronePawn::UpdateKillCamReplay(float DeltaSeconds)
{
    if (!KillCamCamera)
    {
        EndKillCam();
        return;
    }

    KillCamLocation = CrashLocation + FVector(0.f, 0.f, 1000.f);

    KillCamCamera->SetWorldLocation(KillCamLocation);
    KillCamCamera->SetWorldRotation(
        (CrashLocation - KillCamLocation).GetSafeNormal().ToOrientationRotator()
    );

    KillCamTimer -= DeltaSeconds;

    if (KillCamTimer <= 0.f)
    {
        if (ShouldHoldKillCamAfterMissionEnd())
        {
            KillCamTimer = 0.1f;
            return;
        }

        EndKillCam();
    }
}
void AFPVDronePawn::CycleFlightMode()
{
    if (FlightControllerComponent)
    {
        FlightControllerComponent->CycleFlightMode();
    }
}

FString AFPVDronePawn::GetFlightModeText() const
{
    if (FlightControllerComponent)
    {
        return FlightControllerComponent->GetFlightModeText();
    }

    return TEXT("NO FC");
}

void AFPVDronePawn::ApplyThrust()
{
    ApplyMotorForces();
    ApplyAerodynamicDrag();
}


TArray<FMotorState>& AFPVDronePawn::GetActiveMotors()
{
    check(MotorComponent);
    return MotorComponent->GetMutableMotors();
}

const TArray<FMotorState>& AFPVDronePawn::GetActiveMotors() const
{
    check(MotorComponent);
    return MotorComponent->GetMotors();
}

float AFPVDronePawn::GetActiveArmX() const
{
    return MotorComponent ? MotorComponent->GetArmX() : 16.67f;
}

float AFPVDronePawn::GetActiveArmY() const
{
    return MotorComponent ? MotorComponent->GetArmY() : 14.5f;
}

void AFPVDronePawn::UpdateMotorThrusts(float DeltaTime)
{
    UStaticMeshComponent* Mesh = GetPlaneMesh();
    TArray<FMotorState>& ActiveMotors = GetActiveMotors();

    if (!Mesh || !FlightControllerComponent || ActiveMotors.Num() != 4)
    {
        return;
    }

    FlightControllerComponent->UpdateMotorCommands(
        DeltaTime,
        Mesh,
        GetReceivedThrottle(),
        GetReceivedPitchInput(),
        GetReceivedRollInput(),
        GetReceivedYawInput(),
        IsArmed(),
        ActiveMotors,
        DebugState
    );
}

void AFPVDronePawn::ApplyMotorForces()
{
    UStaticMeshComponent* Mesh = GetPlaneMesh();
    const TArray<FMotorState>& ActiveMotors = GetActiveMotors();

    if (!Mesh || ActiveMotors.Num() != 4)
    {
        return;
    }

    const FTransform MeshTransform = Mesh->GetComponentTransform();
    const FVector UpVector = MeshTransform.GetUnitAxis(EAxis::Z);

    float TotalYawTorqueNm = 0.f;

    for (const FMotorState& Motor : ActiveMotors)
    {
        const FVector Force = UpVector * (Motor.ThrustNewton * 100.f);
        const FVector WorldLocation = MeshTransform.TransformPosition(Motor.LocalPosition);

        Mesh->AddForceAtLocation(Force, WorldLocation);

        TotalYawTorqueNm += -Motor.ReactionTorqueNm;
    }

    const FVector LocalTorque(0.f, 0.f, TotalYawTorqueNm * 10000.f);
    const FVector WorldTorque = MeshTransform.TransformVectorNoScale(LocalTorque);

    Mesh->AddTorqueInRadians(WorldTorque);
}

void AFPVDronePawn::ApplyAerodynamicDrag()
{
    UStaticMeshComponent* Mesh = GetPlaneMesh();
    if (!Mesh)
    {
        return;
    }

    const FVector WorldVelCm = Mesh->GetPhysicsLinearVelocity();
    if (WorldVelCm.SizeSquared() < 1.f)
    {
        return;
    }

    const FTransform MeshTransform = Mesh->GetComponentTransform();

    const FVector WorldVelMps = WorldVelCm / 100.f;
    const FVector LocalVelMps = MeshTransform.InverseTransformVectorNoScale(WorldVelCm) / 100.f;

    const float SpeedMps = LocalVelMps.Size();
    if (SpeedMps < 0.05f)
    {
        return;
    }

    const FVector LocalDir = LocalVelMps / SpeedMps;

    const float AbsX = FMath::Abs(LocalDir.X);
    const float AbsY = FMath::Abs(LocalDir.Y);
    const float AbsZ = FMath::Abs(LocalDir.Z);

    const float BodyCdA =
        CdForward * AreaForward * AbsX +
        CdLateral * AreaLateral * AbsY +
        CdVertical * AreaVertical * AbsZ;

    const float RotorDiscRadiusM = RotorDiscDiameterM * 0.5f;
    const float TotalRotorDiscAreaM2 = 4.f * PI * RotorDiscRadiusM * RotorDiscRadiusM;

    const float RotorIncidence = FMath::Pow(AbsZ, 3.f);

    const float RotorCdA =
        RotorVerticalCd *
        TotalRotorDiscAreaM2 *
        RotorVerticalAreaFactor *
        RotorIncidence;

    const float TotalCdA = BodyCdA + RotorCdA;


    const float DynamicPressure = 0.5f * AirDensity * SpeedMps * SpeedMps;
    const float DragMagnitudeN = DynamicPressure * TotalCdA;


    const FVector LocalDragN = -LocalDir * DragMagnitudeN;
    const FVector WorldDragN = MeshTransform.TransformVectorNoScale(LocalDragN);
    const FVector WorldDragCm = WorldDragN * 100.f;

    Mesh->AddForce(WorldDragCm);

    static float DragLogTimer = 0.f;
    static float FlightAuditLogTimer = 0.f;
    FlightAuditLogTimer += LastDeltaSeconds;

    if (FlightAuditLogTimer >= 0.5f)
    {
        FlightAuditLogTimer = 0.f;

        float TotalThrustN = 0.f;
        float ThrottleAvg = 0.f;
        float AvgRPM = 0.f;
        float TotalPowerW = 0.f;

        const TArray<FMotorState>& ActiveMotors = GetActiveMotors();
        for (const FMotorState& M : ActiveMotors)
        {
            TotalThrustN += M.ThrustNewton;
            ThrottleAvg += M.CurrentCommand;
            AvgRPM += M.CurrentRPM;
            TotalPowerW += M.ElectricalPowerWatt;
        }

        if (ActiveMotors.Num() > 0)
        {
            ThrottleAvg /= ActiveMotors.Num();
            AvgRPM /= ActiveMotors.Num();
        }

        const FVector UpVector = MeshTransform.GetUnitAxis(EAxis::Z);
        const FVector WorldThrustN = UpVector * TotalThrustN;

        const float WeightN = Mesh->GetMass() * 9.81f;
        const FVector WorldWeightN(0.f, 0.f, -WeightN);
        const FVector WorldNetN = WorldThrustN + WorldDragN + WorldWeightN;

        const FVector WorldHorizVelMps(WorldVelMps.X, WorldVelMps.Y, 0.f);
        const FVector HorizDir = WorldHorizVelMps.GetSafeNormal();

        const float SpeedKmh = SpeedMps * 3.6f;
        const float HorizSpeedKmh = WorldHorizVelMps.Size() * 3.6f;
        const float VertSpeedMps = WorldVelMps.Z;

        const float ThrustVertN = WorldThrustN.Z;
        const float DragVertN = WorldDragN.Z;
        const float NetVertN = WorldNetN.Z;

        const float ThrustHorizN = FVector::DotProduct(WorldThrustN, HorizDir);
        const float DragHorizN = FVector::DotProduct(WorldDragN, HorizDir);
        const float NetHorizN = FVector::DotProduct(WorldNetN, HorizDir);

        const float TWR = WeightN > KINDA_SMALL_NUMBER ? TotalThrustN / WeightN : 0.f;
        const float TiltDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(UpVector.Z, -1.f, 1.f)));
        const float RequiredVerticalThrustN = WeightN - DragVertN;

        const float RequiredTiltDeg = TotalThrustN > KINDA_SMALL_NUMBER
            ? FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(RequiredVerticalThrustN / TotalThrustN, 0.f, 1.f)))
            : 0.f;

        const float PropEff = MotorComponent ? MotorComponent->GetLastPropEfficiencyFactor() : 1.f;
        const FRotator R = Mesh->GetComponentRotation();

        /*UE_LOG(LogTemp, Warning,
            TEXT("FLIGHT_AUDIT | Thr=%.0f%% RPM=%.0f Pitch=%.1f Tilt=%.1f ReqTilt=%.1f | V=%.1f H=%.1f Vz=%+.2f | LocalVel X=%.2f Y=%.2f Z=%.2f | PropEff=%.2f"),
            ThrottleAvg * 100.f,
            AvgRPM,
            R.Pitch,
            TiltDeg,
            RequiredTiltDeg,
            SpeedKmh,
            HorizSpeedKmh,
            VertSpeedMps,
            LocalVelMps.X,
            LocalVelMps.Y,
            LocalVelMps.Z,
            PropEff
        );

        UE_LOG(LogTemp, Warning,
            TEXT("FORCE_AUDIT | T=%.1fN W=%.1fN TWR=%.2f | Vert T=%.1f D=%+.1f Net=%+.1f | Horiz T=%.1f D=%+.1f Net=%+.1f | CdA=%.5f Drag=%.1fN | I=%.1fA P=%.0fW Vbat=%.2f"),
            TotalThrustN,
            WeightN,
            TWR,
            ThrustVertN,
            DragVertN,
            NetVertN,
            ThrustHorizN,
            DragHorizN,
            NetHorizN,
            TotalCdA,
            DragMagnitudeN,
            GetBatteryTotalCurrentA(),
            TotalPowerW,
            GetBatteryLoadedVoltage()
        );*/
    }
}

void AFPVDronePawn::UpdateMotorDynamics(float DeltaTime)
{
    UStaticMeshComponent* Mesh = GetPlaneMesh();
    if (!Mesh || !MotorComponent)
    {
        return;
    }

    const FTransform MeshTransform = Mesh->GetComponentTransform();
    const FVector WorldVelocityCm = Mesh->GetPhysicsLinearVelocity();
    const FVector LocalVelocityMps = MeshTransform.InverseTransformVectorNoScale(WorldVelocityCm) / 100.f;

    const float CurrentBatteryLoadedVoltage = BatteryComponent ? BatteryComponent->GetLoadedVoltage() : 25.2f;
    const float CurrentBatteryOutputScale = BatteryComponent ? BatteryComponent->GetOutputScale() : 1.f;
    const float CurrentBatteryBenchReferenceVoltage = BatteryComponent ? BatteryComponent->GetBenchReferenceVoltage() : 25.2f;

    MotorComponent->UpdateMotorDynamics(
        DeltaTime,
        CurrentBatteryLoadedVoltage,
        CurrentBatteryOutputScale,
        CurrentBatteryBenchReferenceVoltage,
        LocalVelocityMps
    );

    if (BatteryComponent)
    {
        BatteryComponent->UpdateBatteryState(MotorComponent->GetTotalCurrentA(), DeltaTime);
    }
}


void AFPVDronePawn::HandleCrashExplosion(const FHitResult& Hit)
{
    const FVector ExplosionLocation = Hit.ImpactPoint.IsNearlyZero() ? Hit.Location : Hit.ImpactPoint;
    ApplyExplosionDamage(ExplosionLocation);
}

void AFPVDronePawn::ApplyExplosionDamage(FVector ExplosionLocation)
{
    if (!IsBombArmed())
    {
        return;
    }

    const FVector DamageOrigin = ExplosionLocation + FVector(0.0f, 0.0f, ExplosionDamageOriginZOffset);

    if (bUseManualExplosionDamage)
    {
        FManualRadialDamage::Apply(
            GetWorld(),
            DamageOrigin,
            this,
            GetController(),
            ManualExplosionMaxDamage,
            ManualExplosionMinDamage,
            ManualExplosionInnerRadiusCm,
            ManualExplosionOuterRadiusCm,
            ManualExplosionFalloff
        );
    }
    UE_LOG(LogTemp, Warning,
        TEXT("[FPV EXPLOSION] Damage applied | Location=%s DamageOrigin=%s Manual=%d Max=%.1f Min=%.1f Inner=%.1f Outer=%.1f"),
        *ExplosionLocation.ToString(),
        *DamageOrigin.ToString(),
        bUseManualExplosionDamage ? 1 : 0,
        ManualExplosionMaxDamage,
        ManualExplosionMinDamage,
        ManualExplosionInnerRadiusCm,
        ManualExplosionOuterRadiusCm);
}

void AFPVDronePawn::UpdateTelemetry()
{
    UpdateBaseTelemetry();

    Telemetry.FlightMode = GetFlightModeText();



    if (BatteryComponent)
    {
        Telemetry.PackVoltage = BatteryComponent->GetLoadedVoltage();
        Telemetry.CellVoltage = BatteryComponent->GetCellVoltage();
        Telemetry.ConsumedMah = BatteryComponent->GetConsumedAh() * 1000.f;
        Telemetry.CurrentAmp = BatteryComponent->GetTotalCurrentA();
        Telemetry.Battery01 = FMath::Clamp(BatteryComponent->GetSoC(), 0.f, 1.f);
        Telemetry.bBatteryValid = true;
    }
    else
    {
        Telemetry.PackVoltage = 0.f;
        Telemetry.CellVoltage = 0.f;
        Telemetry.ConsumedMah = 0.f;
        Telemetry.CurrentAmp = 0.f;
        Telemetry.Battery01 = 0.f;
        Telemetry.bBatteryValid = false;
    }

    if (!SignalComponent)
    {
        Telemetry.TxPowerW = 0.f;
        Telemetry.bTxPowerValid = false;
    }
}

void AFPVDronePawn::ResetDroneStateAfterRespawn()
{
    Throttle = 0.f;
    PitchInput = 0.f;
    RollInput = 0.f;
    YawInput = 0.f;

    bArmedState = false;
    bBombArmedState = false;

    BaroZeroZ = GetActorLocation().Z;
    TelemetryStartTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

    if (FlightControllerComponent)
    {
        FlightControllerComponent->ResetController();
    }

    if (BatteryComponent)
    {
        BatteryComponent->ResetBatteryState();
    }


    if (SignalComponent)
    {
        SignalComponent->ResetSignalState();
    }

    if (MotorComponent)
    {
        MotorComponent->InitMotors();
    }

    if (FlightControllerComponent)
    {
        FlightControllerComponent->ResetController();
    }

    DebugLogTimer = 0.f;
    DebugState = FFPVDebugState();

    Telemetry = FDroneTelemetry();
    UpdateTelemetry();
}

float AFPVDronePawn::GetBatteryLoadedVoltage() const
{
    return BatteryComponent ? BatteryComponent->GetLoadedVoltage() : 0.f;
}

float AFPVDronePawn::GetBatteryConsumedAh() const
{
    return BatteryComponent ? BatteryComponent->GetConsumedAh() : 0.f;
}

float AFPVDronePawn::GetBatteryTotalCurrentA() const
{
    return BatteryComponent ? BatteryComponent->GetTotalCurrentA() : 0.f;
}

float AFPVDronePawn::GetReceivedThrottle() const
{
    return SignalComponent ? SignalComponent->GetReceivedThrottle() : 0.f;
}

float AFPVDronePawn::GetReceivedPitchInput() const
{
    return SignalComponent ? SignalComponent->GetReceivedPitchInput() : 0.f;
}

float AFPVDronePawn::GetReceivedRollInput() const
{
    return SignalComponent ? SignalComponent->GetReceivedRollInput() : 0.f;
}

float AFPVDronePawn::GetReceivedYawInput() const
{
    return SignalComponent ? SignalComponent->GetReceivedYawInput() : 0.f;
}

float AFPVDronePawn::GetControlInputScale() const
{
    return SignalComponent ? SignalComponent->GetControlInputScale() : 1.f;
}

bool AFPVDronePawn::IsControlFailsafeActive() const
{
    return SignalComponent ? SignalComponent->IsControlFailsafeActive() : false;
}

void AFPVDronePawn::SetArmState(bool bNewState)
{
    if (bCrashed || bKillCamActive)
    {
        return;
    }

    if (bArmedState == bNewState)
    {
        return;
    }

    bArmedState = bNewState;

    if (!bArmedState)
    {
        Throttle = 0.f;

        if (SignalComponent)
        {
            SignalComponent->ResetSignalState();
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("Armed: %d"), bArmedState ? 1 : 0);
}

void AFPVDronePawn::SetBombArmState(bool bNewState)
{
    if (bCrashed || bKillCamActive)
    {
        return;
    }

    if (bBombArmedState == bNewState)
    {
        return;
    }

    bBombArmedState = bNewState;

    UE_LOG(LogTemp, Warning, TEXT("BombArmed: %d"), bBombArmedState ? 1 : 0);
}


bool AFPVDronePawn::ReadTwoPositionSwitch(float Value) const
{
    return Value > 0.5f;
}

int32 AFPVDronePawn::ReadThreePositionSwitch(float Value) const
{
    if (Value < 0.33f)
    {
        return 0;
    }

    if (Value < 0.66f)
    {
        return 1;
    }

    return 2;
}

void AFPVDronePawn::ArmSwitchAxis(float Value)
{
    const bool bSwitchOn = ReadTwoPositionSwitch(Value);

    if (!bArmSwitchInitialized)
    {
        bArmSwitchInitialized = true;
        bLastArmSwitchOn = bSwitchOn;
        return;
    }

    if (bSwitchOn == bLastArmSwitchOn)
    {
        return;
    }

    bLastArmSwitchOn = bSwitchOn;
    SetArmState(bSwitchOn);
}

void AFPVDronePawn::BombArmSwitchAxis(float Value)
{
    const bool bSwitchOn = ReadTwoPositionSwitch(Value);

    if (!bBombSwitchInitialized)
    {
        bBombSwitchInitialized = true;
        bLastBombSwitchOn = bSwitchOn;
        return;
    }

    if (bSwitchOn == bLastBombSwitchOn)
    {
        return;
    }

    bLastBombSwitchOn = bSwitchOn;
    SetBombArmState(bSwitchOn);
}

void AFPVDronePawn::FlightModeSwitchAxis(float Value)
{
    const int32 Position = ReadThreePositionSwitch(Value);
    CurrentModeSwitchPosition = Position;

    if (!bModeSwitchInitialized)
    {
        bModeSwitchInitialized = true;
        LastModeSwitchPosition = Position;
        return;
    }

    if (Position == LastModeSwitchPosition)
    {
        return;
    }

    LastModeSwitchPosition = Position;
    ApplyFlightModeFromSwitches();
}

void AFPVDronePawn::AcroTrainerSwitchAxis(float Value)
{
    const bool bSwitchOn = ReadTwoPositionSwitch(Value);
    bCurrentTrainerSwitchOn = bSwitchOn;

    if (!bTrainerSwitchInitialized)
    {
        bTrainerSwitchInitialized = true;
        bLastTrainerSwitchOn = bSwitchOn;
        return;
    }

    if (bSwitchOn == bLastTrainerSwitchOn)
    {
        return;
    }

    bLastTrainerSwitchOn = bSwitchOn;
    ApplyFlightModeFromSwitches();
}

void AFPVDronePawn::ApplyFlightModeFromSwitches()
{
    EFPVFlightMode NewMode = EFPVFlightMode::Acro;

    if (CurrentModeSwitchPosition == 0)
    {
        NewMode = bCurrentTrainerSwitchOn
            ? EFPVFlightMode::AcroTrainer
            : EFPVFlightMode::Acro;
    }
    else if (CurrentModeSwitchPosition == 1)
    {
        NewMode = EFPVFlightMode::Angle;
    }
    else
    {
        NewMode = EFPVFlightMode::Horizon;
    }

    SetFlightModeDirect(NewMode);
}

void AFPVDronePawn::SetFlightModeDirect(EFPVFlightMode NewMode)
{
    if (FlightControllerComponent)
    {
        FlightControllerComponent->SetFlightMode(NewMode);
    }
}
