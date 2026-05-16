#include "SPGAIComponent.h"

#include "APCAIComponent.h"
#include "Animation/AnimInstance.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "FPVDronePawn.h"
#include "InfantryAIController.h"
#include "InfantryCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UnrealType.h"

USPGAIComponent::USPGAIComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	static ConstructorHelpers::FClassFinder<AT72TankProjectile> ProjectileFinder(TEXT("/Game/Vehicles/Tank/Shell/BP_T72TankProjectile"));

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DamagedMaterialFinder(
		TEXT("/Game/VigilanteContent/Vehicles/East_SPG_2S3Akatsia/Damaged/Materials/MI_East_SPG_2s3Akatsia_Damaged.MI_East_SPG_2s3Akatsia_Damaged")
	);

	static ConstructorHelpers::FObjectFinder<UParticleSystem> MuzzleFireFinder(
		TEXT("/Game/VigilanteContent/Vehicles/East_SPG_2S3Akatsia/FX/FX_MuzzleFire_01_2S3Akatsia.FX_MuzzleFire_01_2S3Akatsia")
	);

	static ConstructorHelpers::FObjectFinder<UParticleSystem> ShockWaveFinder(
		TEXT("/Game/VigilanteContent/Vehicles/East_SPG_2S3Akatsia/FX/FX_ShockWave_2S3Akatsia.FX_ShockWave_2S3Akatsia")
	);

	static ConstructorHelpers::FObjectFinder<UParticleSystem> BigExplosionFinder(
		TEXT("/Game/VFX/Realistic_Starter_VFX_Pack_Vol2/Particles/Explosion/P_Explosion_Big_B.P_Explosion_Big_B")
	);

	static ConstructorHelpers::FObjectFinder<UParticleSystem> WreckageFinder(
		TEXT("/Game/VigilanteContent/Vehicles/East_SPG_2S3Akatsia/FX/FX_Wreckage_01_2S3Akatsia.FX_Wreckage_01_2S3Akatsia")
	);

	static ConstructorHelpers::FObjectFinder<UParticleSystem> TrackDustFinder(
		TEXT("/Game/VigilanteContent/Vehicles/East_SPG_2S3Akatsia/FX/FX_Dust_WheelTrack_2S3Akatsia.FX_Dust_WheelTrack_2S3Akatsia")
	);

	static ConstructorHelpers::FObjectFinder<UParticleSystem> EngineFireFinder(
		TEXT("/Game/VFX/Realistic_Starter_VFX_Pack_Vol2/Particles/Fire/P_Fire_Big.P_Fire_Big")
	);

	if (ProjectileFinder.Succeeded())
	{
		ProjectileClass = ProjectileFinder.Class;
	}

	if (DamagedMaterialFinder.Succeeded())
	{
		DestroyedMaterial = DamagedMaterialFinder.Object;
	}

	if (MuzzleFireFinder.Succeeded())
	{
		MuzzleFireFX = MuzzleFireFinder.Object;
	}

	if (ShockWaveFinder.Succeeded())
	{
		ShockWaveFX = ShockWaveFinder.Object;
	}

	if (BigExplosionFinder.Succeeded())
	{
		DestroyedExplosionFX = BigExplosionFinder.Object;
	}

	if (WreckageFinder.Succeeded())
	{
		DestroyedWreckageFX = WreckageFinder.Object;
	}

	if (TrackDustFinder.Succeeded())
	{
		TrackDustFX = TrackDustFinder.Object;
	}

	if (EngineFireFinder.Succeeded())
	{
		EngineFireFX = EngineFireFinder.Object;
	}
}

void USPGAIComponent::BeginPlay()
{
	Super::BeginPlay();

	bSPGDestroyed = false;
	bMobilityDestroyed = false;
	bGunDestroyed = false;
	bEngineDestroyed = false;
	bCrewEvacuated = false;
	bMissionStarted = false;
	bTrackHitWhileMoving = false;
	bCrewSpawned = false;
	bAPCSpawned = false;
	bNavObstacleActive = false;

	FiredShots = 0;
	FireTimer = 0.0f;
	CurrentWheelSpeed = 0.0f;
	CurrentTurretRotation = 0.0f;
	CurrentTurretElevation = 0.0f;
	CurrentHatchAngle = 0.0f;
	TargetHatchAngle = 0.0f;
	SPGCrewInsideCount = CrewCount;
	SpawnedCrew.Empty();
	SpawnedAPC = nullptr;

	ApplyDefaultAssetReferences();
	CacheComponents();
	CacheTrackDustComponents();
	RegisterDamageZones();
	SetNavObstacleActive(false);

	SetWheelSpeed(0.0f);
	SetTurretRotation(0.0f);
	SetTurretElevation(0.0f);
	SetAnimFloat(HatchAngleVariableName, 0.0f);
	SetFiringActive(false);
	UpdateTrackDustVFX();

	DebugLog(TEXT("[SPG] BeginPlay"));

	if (bAutoStart)
	{
		StartMission();
	}
}

void USPGAIComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bMissionStarted && CurrentState != ESPGAIState::Burning && CurrentState != ESPGAIState::Destroyed && CurrentState != ESPGAIState::Immobilized)
	{
		TickHatchAnimation(DeltaTime);
		UpdateTrackDustVFX();
		return;
	}

	TickState(DeltaTime);
	TickHatchAnimation(DeltaTime);
	UpdateTrackDustVFX();
}

void USPGAIComponent::ApplyDefaultAssetReferences()
{
	if (!ProjectileClass)
	{
		ProjectileClass = LoadClass<AT72TankProjectile>(nullptr, TEXT("/Game/Vehicles/Tank/Shell/BP_T72TankProjectile.BP_T72TankProjectile_C"));
	}

	if (!DestroyedMaterial)
	{
		DestroyedMaterial = LoadObject<UMaterialInterface>(
			nullptr,
			TEXT("/Game/VigilanteContent/Vehicles/East_SPG_2S3Akatsia/Damaged/Materials/MI_East_SPG_2s3Akatsia_Damaged.MI_East_SPG_2s3Akatsia_Damaged")
		);
	}

	if (!MuzzleFireFX)
	{
		MuzzleFireFX = LoadObject<UParticleSystem>(
			nullptr,
			TEXT("/Game/VigilanteContent/Vehicles/East_SPG_2S3Akatsia/FX/FX_MuzzleFire_01_2S3Akatsia.FX_MuzzleFire_01_2S3Akatsia")
		);
	}

	if (!ShockWaveFX)
	{
		ShockWaveFX = LoadObject<UParticleSystem>(
			nullptr,
			TEXT("/Game/VigilanteContent/Vehicles/East_SPG_2S3Akatsia/FX/FX_ShockWave_2S3Akatsia.FX_ShockWave_2S3Akatsia")
		);
	}

	if (!DestroyedExplosionFX)
	{
		DestroyedExplosionFX = LoadObject<UParticleSystem>(
			nullptr,
			TEXT("/Game/VFX/Realistic_Starter_VFX_Pack_Vol2/Particles/Explosion/P_Explosion_Big_B.P_Explosion_Big_B")
		);
	}

	if (!DestroyedWreckageFX)
	{
		DestroyedWreckageFX = LoadObject<UParticleSystem>(
			nullptr,
			TEXT("/Game/VigilanteContent/Vehicles/East_SPG_2S3Akatsia/FX/FX_Wreckage_01_2S3Akatsia.FX_Wreckage_01_2S3Akatsia")
		);
	}

	if (!TrackDustFX)
	{
		TrackDustFX = LoadObject<UParticleSystem>(
			nullptr,
			TEXT("/Game/VigilanteContent/Vehicles/East_SPG_2S3Akatsia/FX/FX_Dust_WheelTrack_2S3Akatsia.FX_Dust_WheelTrack_2S3Akatsia")
		);
	}

	if (!EngineFireFX)
	{
		EngineFireFX = LoadObject<UParticleSystem>(
			nullptr,
			TEXT("/Game/VFX/Realistic_Starter_VFX_Pack_Vol2/Particles/Fire/P_Fire_Big.P_Fire_Big")
		);
	}

	DebugLog(FString::Printf(
		TEXT("[SPG DEFAULTS] Projectile=%s DamagedMaterial=%s MuzzleFire=%s ShockWave=%s BigExplosion=%s Wreckage=%s TrackDust=%s EngineFire=%s"),
		ProjectileClass ? *ProjectileClass->GetName() : TEXT("NULL"),
		*GetNameSafe(DestroyedMaterial),
		*GetNameSafe(MuzzleFireFX),
		*GetNameSafe(ShockWaveFX),
		*GetNameSafe(DestroyedExplosionFX),
		*GetNameSafe(DestroyedWreckageFX),
		*GetNameSafe(TrackDustFX),
		*GetNameSafe(EngineFireFX)
	));
}

void USPGAIComponent::CacheComponents()
{
	AActor* Owner = GetOwner();

	if (!Owner)
	{
		return;
	}

	MeshComponent = Owner->FindComponentByClass<USkeletalMeshComponent>();

	if (MeshComponent)
	{
		AnimInstance = MeshComponent->GetAnimInstance();
	}

	MuzzlePoint = FindSceneComponentByName(MuzzlePointComponentName);
	EngineFXPoint = FindSceneComponentByName(EngineFXPointName);
	DestroyedFXPoint = FindSceneComponentByName(DestroyedFXPointName);
	DestroyedExplosionFXPoint = FindSceneComponentByName(DestroyedExplosionFXPointName);
	WreckageFXPoint = FindSceneComponentByName(WreckageFXPointName);
	ShockWavePoint = FindSceneComponentByName(ShockWavePointName);
	NavObstacleComponent = FindPrimitiveComponentByName(NavObstacleComponentName);
	PhysicsBlockerComponent = FindPrimitiveComponentByName(PhysicsBlockerComponentName);

	DebugLog(FString::Printf(
		TEXT("[SPG] Components | Mesh=%d Anim=%d Muzzle=%s EngineFX=%s DestroyedFX=%s BigExplosionPoint=%s WreckagePoint=%s ShockWavePoint=%s NavObstacle=%s PhysicsBlocker=%s"),
		MeshComponent ? 1 : 0,
		AnimInstance ? 1 : 0,
		*GetNameSafe(MuzzlePoint),
		*GetNameSafe(EngineFXPoint),
		*GetNameSafe(DestroyedFXPoint),
		*GetNameSafe(DestroyedExplosionFXPoint),
		*GetNameSafe(WreckageFXPoint),
		*GetNameSafe(ShockWavePoint),
		*GetNameSafe(NavObstacleComponent),
		*GetNameSafe(PhysicsBlockerComponent)
	));
}

void USPGAIComponent::CacheTrackDustComponents()
{
	AActor* Owner = GetOwner();

	if (!Owner)
	{
		return;
	}

	TrackDustParticleComponents.Empty();

	TArray<UParticleSystemComponent*> ParticleComponents;
	Owner->GetComponents<UParticleSystemComponent>(ParticleComponents);

	for (UParticleSystemComponent* ParticleComponent : ParticleComponents)
	{
		if (!ParticleComponent)
		{
			continue;
		}

		const FString ComponentName = ParticleComponent->GetName();

		if (!IsTrackDustComponentName(ComponentName))
		{
			continue;
		}

		if (TrackDustFX)
		{
			ParticleComponent->SetTemplate(TrackDustFX);
		}

		ParticleComponent->SetAutoActivate(false);
		ParticleComponent->SetVisibility(false, true);
		ParticleComponent->DeactivateSystem();
		ParticleComponent->SetWorldScale3D(FVector(TrackDustScale));

		TrackDustParticleComponents.Add(ParticleComponent);
	}

	TArray<USceneComponent*> SceneComponents;
	Owner->GetComponents<USceneComponent>(SceneComponents);

	for (USceneComponent* SceneComponent : SceneComponents)
	{
		if (!SceneComponent || Cast<UParticleSystemComponent>(SceneComponent))
		{
			continue;
		}

		const FString ComponentName = SceneComponent->GetName();

		if (!IsTrackDustComponentName(ComponentName))
		{
			continue;
		}

		UParticleSystemComponent* CreatedParticleComponent = CreateTrackDustParticleComponent(SceneComponent);

		if (CreatedParticleComponent)
		{
			TrackDustParticleComponents.Add(CreatedParticleComponent);
		}
	}

	DebugLog(FString::Printf(
		TEXT("[SPG VFX] Track dust components | Count=%d Prefix=%s FX=%s"),
		TrackDustParticleComponents.Num(),
		*TrackDustComponentPrefix,
		*GetNameSafe(TrackDustFX)
	));
}

UParticleSystemComponent* USPGAIComponent::CreateTrackDustParticleComponent(USceneComponent* AttachPoint)
{
	AActor* Owner = GetOwner();

	if (!Owner || !AttachPoint || !TrackDustFX)
	{
		return nullptr;
	}

	UParticleSystemComponent* ParticleComponent = NewObject<UParticleSystemComponent>(Owner);

	if (!ParticleComponent)
	{
		return nullptr;
	}

	ParticleComponent->SetTemplate(TrackDustFX);
	ParticleComponent->SetAutoActivate(false);
	ParticleComponent->bAutoDestroy = false;
	Owner->AddInstanceComponent(ParticleComponent);
	ParticleComponent->AttachToComponent(AttachPoint, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	ParticleComponent->RegisterComponent();
	ParticleComponent->SetRelativeLocation(FVector::ZeroVector);
	ParticleComponent->SetRelativeRotation(FRotator::ZeroRotator);
	ParticleComponent->SetRelativeScale3D(FVector(TrackDustScale));
	ParticleComponent->SetVisibility(false, true);
	ParticleComponent->DeactivateSystem();

	return ParticleComponent;
}

void USPGAIComponent::RegisterDamageZones()
{
	DamageZoneMap.Empty();

	RegisterDamageZoneByName(LeftTrackZoneComponentName, ESPGDamageZone::LeftTrack, TEXT("SPG_LeftTrack"));
	RegisterDamageZoneByName(RightTrackZoneComponentName, ESPGDamageZone::RightTrack, TEXT("SPG_RightTrack"));
	RegisterDamageZoneByName(EngineZoneComponentName, ESPGDamageZone::Engine, TEXT("SPG_Engine"));
	RegisterDamageZoneByName(TurretZoneComponentName, ESPGDamageZone::Turret, TEXT("SPG_Turret"));
	RegisterDamageZoneByName(HullZoneComponentName, ESPGDamageZone::Hull, TEXT("SPG_Hull"));
	RegisterDamageZoneByName(GunBarrelZoneComponentName, ESPGDamageZone::GunBarrel, TEXT("SPG_GunBarrel"));

	DebugLog(FString::Printf(TEXT("[SPG DAMAGE] Registered zones count: %d"), DamageZoneMap.Num()));
}

void USPGAIComponent::RegisterDamageZoneByName(FName ComponentName, ESPGDamageZone Zone, FName OptionalTagName)
{
	AActor* Owner = GetOwner();

	if (!Owner)
	{
		return;
	}

	UPrimitiveComponent* NamedComponent = FindPrimitiveComponentByName(ComponentName);

	if (NamedComponent)
	{
		RegisterDamageZoneComponent(NamedComponent, Zone);
		return;
	}

	TArray<UPrimitiveComponent*> Components;
	Owner->GetComponents<UPrimitiveComponent>(Components);

	for (UPrimitiveComponent* Component : Components)
	{
		if (!Component)
		{
			continue;
		}

		if (Component->ComponentHasTag(OptionalTagName))
		{
			RegisterDamageZoneComponent(Component, Zone);
			return;
		}
	}

	DebugLog(FString::Printf(TEXT("[SPG DAMAGE] Zone component not found | Name=%s Tag=%s"), *ComponentName.ToString(), *OptionalTagName.ToString()));
}

void USPGAIComponent::RegisterDamageZoneComponent(UPrimitiveComponent* Component, ESPGDamageZone Zone)
{
	if (!Component)
	{
		return;
	}

	DamageZoneMap.Add(Component, Zone);

	Component->SetGenerateOverlapEvents(true);
	Component->OnComponentBeginOverlap.RemoveDynamic(this, &USPGAIComponent::OnDamageZoneBeginOverlap);
	Component->OnComponentBeginOverlap.AddDynamic(this, &USPGAIComponent::OnDamageZoneBeginOverlap);

	DebugLog(FString::Printf(TEXT("[SPG DAMAGE] Registered zone %s on %s"), *UEnum::GetValueAsString(Zone), *Component->GetName()));
}

void USPGAIComponent::StartMission()
{
	if (bSPGDestroyed)
	{
		return;
	}

	bMissionStarted = true;
	CurrentRouteIndex = 0;
	CurrentReturnRouteIndex = 0;
	FiredShots = 0;
	FireTimer = 0.0f;

	if (RoutePoints.Num() > 0)
	{
		SetState(ESPGAIState::MovingRoute);
	}
	else if (FirePosition)
	{
		SetState(ESPGAIState::MovingFirePosition);
	}
	else
	{
		SetState(ESPGAIState::Aiming);
	}

	DebugLog(FString::Printf(
		TEXT("[SPG MISSION] Start | RoutePoints=%d FirePosition=%s FireTarget=%s"),
		RoutePoints.Num(),
		*GetNameSafe(FirePosition),
		*GetNameSafe(FireTarget)
	));
}

void USPGAIComponent::StopMission()
{
	bMissionStarted = false;
	SetWheelSpeed(0.0f);
	SetFiringActive(false);
	SetState(ESPGAIState::Idle);
}

void USPGAIComponent::SetState(ESPGAIState NewState)
{
	if (CurrentState == NewState)
	{
		return;
	}

	CurrentState = NewState;

	if (CurrentState == ESPGAIState::Idle ||
		CurrentState == ESPGAIState::Aiming ||
		CurrentState == ESPGAIState::Firing ||
		CurrentState == ESPGAIState::ReturnTurretToDefault ||
		CurrentState == ESPGAIState::CrewEvacWait ||
		CurrentState == ESPGAIState::Immobilized ||
		CurrentState == ESPGAIState::Burning ||
		CurrentState == ESPGAIState::Destroyed)
	{
		SetWheelSpeed(0.0f);
	}

	if (CurrentState != ESPGAIState::Firing)
	{
		SetFiringActive(false);
	}

	DebugLog(FString::Printf(TEXT("[SPG] State changed: %d"), static_cast<int32>(CurrentState)));
}

void USPGAIComponent::TickState(float DeltaTime)
{
	switch (CurrentState)
	{
	case ESPGAIState::Idle:
		SetWheelSpeed(0.0f);
		break;

	case ESPGAIState::MovingRoute:
		TickMovementToActor(DeltaTime, GetCurrentRouteTarget());
		break;

	case ESPGAIState::MovingFirePosition:
		TickMovementToActor(DeltaTime, FirePosition);
		break;

	case ESPGAIState::Aiming:
		SetFiringActive(false);
		TickAiming(DeltaTime);

		if (!bGunDestroyed && IsAimedAtTarget())
		{
			FiredShots = 0;
			FireTimer = 0.0f;
			SetState(ESPGAIState::Firing);
		}
		break;

	case ESPGAIState::Firing:
		TickFiring(DeltaTime);
		break;

	case ESPGAIState::Returning:
		TickReturning(DeltaTime);
		break;

	case ESPGAIState::ReturnTurretToDefault:
		SetWheelSpeed(0.0f);
		SetFiringActive(false);
		TickTurretReturnToDefault(DeltaTime);

		if (IsTurretAtDefault())
		{
			if (RoutePoints.Num() > 0)
			{
				CurrentReturnRouteIndex = RoutePoints.Num() - 1;
				SetState(ESPGAIState::Returning);
			}
			else
			{
				SetState(ESPGAIState::Idle);
				bMissionStarted = false;
			}
		}
		break;

	case ESPGAIState::TrackTurnReaction:
		TickTrackTurnReaction(DeltaTime);
		break;

	case ESPGAIState::CrewEvacWait:
		SetWheelSpeed(0.0f);

		if (!bGunDestroyed)
		{
			TickAiming(DeltaTime);
		}
		break;

	case ESPGAIState::Immobilized:
		SetWheelSpeed(0.0f);
		break;

	case ESPGAIState::EngineCoast:
		TickEngineCoast(DeltaTime);
		break;

	case ESPGAIState::Burning:
		SetWheelSpeed(0.0f);
		SetFiringActive(false);
		break;

	case ESPGAIState::Destroyed:
		SetWheelSpeed(0.0f);
		SetFiringActive(false);
		break;

	default:
		break;
	}
}

AActor* USPGAIComponent::GetCurrentRouteTarget() const
{
	if (!RoutePoints.IsValidIndex(CurrentRouteIndex))
	{
		return nullptr;
	}

	return RoutePoints[CurrentRouteIndex];
}

AActor* USPGAIComponent::GetCurrentReturnTarget() const
{
	if (!RoutePoints.IsValidIndex(CurrentReturnRouteIndex))
	{
		return nullptr;
	}

	return RoutePoints[CurrentReturnRouteIndex];
}

void USPGAIComponent::TickMovementToActor(float DeltaTime, AActor* TargetActor)
{
	AActor* Owner = GetOwner();

	if (bMobilityDestroyed)
	{
		SetWheelSpeed(0.0f);
		return;
	}

	if (!Owner || !TargetActor)
	{
		SetWheelSpeed(0.0f);
		return;
	}

	const float SafeDeltaTime = FMath::Max(DeltaTime, 0.001f);

	const FVector CurrentLocation = Owner->GetActorLocation();
	FVector ToTarget = TargetActor->GetActorLocation() - CurrentLocation;
	ToTarget.Z = 0.0f;

	const float Distance = ToTarget.Size();

	if (Distance <= AcceptanceRadiusCm)
	{
		SetWheelSpeed(0.0f);

		if (CurrentState == ESPGAIState::MovingRoute)
		{
			OnArrivedAtRouteTarget();
		}
		else if (CurrentState == ESPGAIState::MovingFirePosition)
		{
			OnArrivedAtFirePosition();
		}

		return;
	}

	const FVector Direction = ToTarget.GetSafeNormal();

	if (Direction.IsNearlyZero())
	{
		SetWheelSpeed(0.0f);
		return;
	}

	const FRotator CurrentRotation = Owner->GetActorRotation();
	const float DesiredYaw = Direction.Rotation().Yaw;
	const float YawErrorSigned = FMath::FindDeltaAngleDegrees(CurrentRotation.Yaw, DesiredYaw);
	const float YawError = FMath::Abs(YawErrorSigned);
	const float MoveAlpha = YawError > 85.0f ? 0.0f : 1.0f;

	const FRotator NewYawRotation = FMath::RInterpConstantTo(
		FRotator(0.0f, CurrentRotation.Yaw, 0.0f),
		FRotator(0.0f, DesiredYaw, 0.0f),
		SafeDeltaTime,
		BodyTurnSpeedDeg
	);

	const float MoveStep = MoveSpeedCm * MoveAlpha * SafeDeltaTime;
	const FVector Forward = FRotator(0.0f, CurrentRotation.Yaw, 0.0f).Vector();
	const FVector RawNewLocation = CurrentLocation + Forward * FMath::Min(MoveStep, Distance);

	FVector GroundedLocation = RawNewLocation;
	FVector GroundNormal = FVector::UpVector;
	float SlopeDeg = 0.0f;

	if (!ProjectLocationToGround(RawNewLocation, NewYawRotation.Yaw, GroundedLocation, GroundNormal, SlopeDeg))
	{
		SetWheelSpeed(0.0f);
		return;
	}

	if (!IsSlopeDriveable(SlopeDeg))
	{
		SetWheelSpeed(0.0f);
		return;
	}

	FRotator FinalRotation = FRotator(0.0f, NewYawRotation.Yaw, 0.0f);

	if (bAlignToGround)
	{
		FinalRotation = MakeGroundAlignedRotation(NewYawRotation.Yaw, GroundNormal);
	}

	FinalRotation = FMath::RInterpConstantTo(CurrentRotation, FinalRotation, SafeDeltaTime, GroundRotationInterpSpeedDeg);

	Owner->SetActorLocationAndRotation(GroundedLocation, FinalRotation, false, nullptr, ETeleportType::TeleportPhysics);

	const float ActualMoveSpeed = FVector::Dist2D(CurrentLocation, GroundedLocation) / SafeDeltaTime;
	SetWheelSpeed(ActualMoveSpeed * WheelAnimationSpeedScale);

	MovementDebugTimer += SafeDeltaTime;

	if (bMovementDebugLogs && MovementDebugTimer >= MovementDebugInterval)
	{
		MovementDebugTimer = 0.0f;

		DebugLog(FString::Printf(
			TEXT("[SPG MOVE] State=%d Target=%s Dist=%.1f Accept=%.1f Loc=%s TargetLoc=%s YawCur=%.1f Desired=%.1f New=%.1f Err=%.1f Signed=%.1f MoveAlpha=%.2f Speed=%.1f Slope=%.1f Ground=%s"),
			static_cast<int32>(CurrentState),
			*GetNameSafe(TargetActor),
			Distance,
			AcceptanceRadiusCm,
			*CurrentLocation.ToString(),
			*TargetActor->GetActorLocation().ToString(),
			CurrentRotation.Yaw,
			DesiredYaw,
			NewYawRotation.Yaw,
			YawError,
			YawErrorSigned,
			MoveAlpha,
			ActualMoveSpeed,
			SlopeDeg,
			*GroundedLocation.ToString()
		));
	}
}

void USPGAIComponent::TickReturning(float DeltaTime)
{
	if (bMobilityDestroyed)
	{
		SetWheelSpeed(0.0f);
		SetState(ESPGAIState::Immobilized);
		return;
	}

	AActor* TargetActor = GetCurrentReturnTarget();

	if (!TargetActor)
	{
		SetWheelSpeed(0.0f);
		SetState(ESPGAIState::Idle);
		bMissionStarted = false;
		return;
	}

	AActor* Owner = GetOwner();

	if (!Owner)
	{
		SetWheelSpeed(0.0f);
		return;
	}

	TickTurretReturnToDefault(DeltaTime);

	const float Distance = FVector::Dist2D(Owner->GetActorLocation(), TargetActor->GetActorLocation());

	if (Distance <= AcceptanceRadiusCm)
	{
		SetWheelSpeed(0.0f);
		CurrentReturnRouteIndex--;

		if (CurrentReturnRouteIndex >= 0)
		{
			return;
		}

		SetState(ESPGAIState::Idle);
		bMissionStarted = false;
		return;
	}

	TickMovementToActor(DeltaTime, TargetActor);
}

void USPGAIComponent::OnArrivedAtRouteTarget()
{
	CurrentRouteIndex++;

	if (CurrentRouteIndex < RoutePoints.Num())
	{
		return;
	}

	if (FirePosition)
	{
		SetState(ESPGAIState::MovingFirePosition);
	}
	else
	{
		SetState(ESPGAIState::Aiming);
	}
}

void USPGAIComponent::OnArrivedAtFirePosition()
{
	SetWheelSpeed(0.0f);
	SetState(ESPGAIState::Aiming);
}

void USPGAIComponent::BeginReturning()
{
	if (bSPGDestroyed)
	{
		return;
	}

	SetFiringActive(false);
	SetWheelSpeed(0.0f);

	if (bMobilityDestroyed)
	{
		SetState(ESPGAIState::Immobilized);
		return;
	}

	if (RoutePoints.Num() > 0)
	{
		SetState(ESPGAIState::ReturnTurretToDefault);
		return;
	}

	SetState(ESPGAIState::Idle);
	bMissionStarted = false;
}

void USPGAIComponent::TickAiming(float DeltaTime)
{
	if (bGunDestroyed || bSPGDestroyed || bCrewEvacuated)
	{
		SetFiringActive(false);
		return;
	}

	float DesiredTurretYaw = 0.0f;
	float DesiredGunElevation = 0.0f;
	GetDesiredWeaponAngles(DesiredTurretYaw, DesiredGunElevation);

	CurrentTurretRotation = FMath::FInterpConstantTo(CurrentTurretRotation, DesiredTurretYaw, DeltaTime, TurretTurnSpeedDeg);
	CurrentTurretElevation = FMath::FInterpConstantTo(CurrentTurretElevation, DesiredGunElevation, DeltaTime, GunElevationSpeedDeg);

	SetTurretRotation(CurrentTurretRotation);
	SetTurretElevation(CurrentTurretElevation);
}

void USPGAIComponent::TickTurretReturnToDefault(float DeltaTime)
{
	const float TargetTurretYaw = 0.f;
	const float TargetGunElevation = 0.f;
	const float YawDelta = FMath::FindDeltaAngleDegrees(CurrentTurretRotation, TargetTurretYaw);
	const float MaxYawStep = TurretTurnSpeedDeg * DeltaTime;

	CurrentTurretRotation += FMath::Clamp(YawDelta, -MaxYawStep, MaxYawStep);
	CurrentTurretRotation = FMath::UnwindDegrees(CurrentTurretRotation);
	CurrentTurretElevation = FMath::FInterpConstantTo(CurrentTurretElevation, TargetGunElevation, DeltaTime, GunElevationSpeedDeg);

	SetTurretRotation(CurrentTurretRotation);
	SetTurretElevation(CurrentTurretElevation);
}

bool USPGAIComponent::IsTurretAtDefault() const
{
	const float TargetTurretYaw = 0.f;
	const float TargetGunElevation = 0.f;
	const float TurretError = FMath::Abs(FMath::FindDeltaAngleDegrees(CurrentTurretRotation, TargetTurretYaw));
	const float GunError = FMath::Abs(CurrentTurretElevation - TargetGunElevation);

	return TurretError <= FireAngleToleranceDeg && GunError <= FireAngleToleranceDeg;
}

void USPGAIComponent::TickFiring(float DeltaTime)
{
	if (bGunDestroyed || bSPGDestroyed || bCrewEvacuated || CurrentState == ESPGAIState::Destroyed || CurrentState == ESPGAIState::Burning)
	{
		SetFiringActive(false);
		return;
	}

	TickAiming(DeltaTime);

	if (!IsAimedAtTarget())
	{
		SetFiringActive(false);
		return;
	}

	SetFiringActive(false);
	FireTimer += DeltaTime;

	if (FireTimer < FireIntervalSeconds)
	{
		return;
	}

	if (FiredShots >= RequiredShotCount)
	{
		BeginReturning();
		return;
	}

	SetFiringActive(true);
	FireWeapon();
	FiredShots++;
	FireTimer = 0.0f;

	DebugLog(FString::Printf(TEXT("[SPG FIRE] Shot %d/%d"), FiredShots, RequiredShotCount));

	if (FiredShots >= RequiredShotCount)
	{
		BeginReturning();
	}
}

void USPGAIComponent::GetDesiredWeaponAngles(float& OutTurretYawDeg, float& OutGunElevationDeg) const
{
	const AActor* Owner = GetOwner();

	if (!Owner || !FireTarget)
	{
		OutTurretYawDeg = DefaultTurretYawDeg;
		OutGunElevationDeg = FMath::Clamp(DefaultGunElevationDeg, MinGunElevationDeg, MaxGunElevationDeg);
		return;
	}

	const FVector ToTarget = FireTarget->GetActorLocation() - Owner->GetActorLocation();
	const FRotator TargetRotation = ToTarget.Rotation();

	const float BodyYaw = Owner->GetActorRotation().Yaw;
	OutTurretYawDeg = FMath::FindDeltaAngleDegrees(BodyYaw, TargetRotation.Yaw);

	const FVector FlatToTarget(ToTarget.X, ToTarget.Y, 0.0f);
	const float HorizontalDistance = FMath::Max(FlatToTarget.Size(), 1.0f);

	OutGunElevationDeg = FMath::Clamp(
		FMath::RadiansToDegrees(FMath::Atan2(ToTarget.Z, HorizontalDistance)),
		MinGunElevationDeg,
		MaxGunElevationDeg
	);
}

bool USPGAIComponent::IsAimedAtTarget() const
{
	float DesiredTurretYaw = 0.0f;
	float DesiredGunElevation = 0.0f;
	GetDesiredWeaponAngles(DesiredTurretYaw, DesiredGunElevation);

	const float TurretError = FMath::Abs(FMath::FindDeltaAngleDegrees(CurrentTurretRotation, DesiredTurretYaw));
	const float GunError = FMath::Abs(CurrentTurretElevation - DesiredGunElevation);

	return TurretError <= FireAngleToleranceDeg && GunError <= FireAngleToleranceDeg;
}

void USPGAIComponent::FireWeapon()
{
	AActor* Owner = GetOwner();

	if (!Owner || bGunDestroyed || bSPGDestroyed || bCrewEvacuated)
	{
		SetFiringActive(false);
		return;
	}

	PlayMuzzleFlash();

	if (!ProjectileClass || !GetWorld())
	{
		return;
	}

	USceneComponent* CurrentMuzzlePoint = MuzzlePoint ? MuzzlePoint : FindSceneComponentByName(MuzzlePointComponentName);

	const FVector SpawnLocation = CurrentMuzzlePoint
		? CurrentMuzzlePoint->GetComponentLocation()
		: Owner->GetActorLocation() + Owner->GetActorForwardVector() * 500.0f + FVector(0.0f, 0.0f, 150.0f);

	FRotator SpawnRotation = CurrentMuzzlePoint
		? CurrentMuzzlePoint->GetComponentRotation()
		: Owner->GetActorRotation();

	if (bAimProjectileAtFireTarget && FireTarget)
	{
		const FVector AimDirection = (FireTarget->GetActorLocation() - SpawnLocation).GetSafeNormal();

		if (!AimDirection.IsNearlyZero())
		{
			FVector FinalDirection = AimDirection;

			if (ProjectileSpreadDeg > 0.0f)
			{
				FinalDirection = FMath::VRandCone(AimDirection, FMath::DegreesToRadians(ProjectileSpreadDeg));
			}

			SpawnRotation = FinalDirection.Rotation();
		}
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Owner;
	SpawnParams.Instigator = Owner->GetInstigator();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	GetWorld()->SpawnActor<AT72TankProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);
}

void USPGAIComponent::PlayMuzzleFlash()
{
	CallBlueprintEventNoParams(PlayMuzzleFlashEventName);

	AActor* Owner = GetOwner();

	if (!Owner || !GetWorld())
	{
		return;
	}

	USceneComponent* CurrentMuzzlePoint = MuzzlePoint ? MuzzlePoint : FindSceneComponentByName(MuzzlePointComponentName);

	const FVector FXLocation = CurrentMuzzlePoint
		? CurrentMuzzlePoint->GetComponentLocation()
		: Owner->GetActorLocation() + Owner->GetActorForwardVector() * 500.0f + FVector(0.0f, 0.0f, 150.0f);

	const FRotator FXRotation = CurrentMuzzlePoint
		? CurrentMuzzlePoint->GetComponentRotation()
		: Owner->GetActorRotation();

	if (MuzzleFireFX)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), MuzzleFireFX, FXLocation, FXRotation, FVector(MuzzleFireScale), true);
	}

	if (ShockWaveFX)
	{
		USceneComponent* CurrentShockWavePoint = ShockWavePoint ? ShockWavePoint : FindSceneComponentByName(ShockWavePointName);
		FVector ShockWaveLocation = FXLocation;
		FRotator ShockWaveRotation = FRotator::ZeroRotator;

		if (CurrentShockWavePoint)
		{
			ShockWaveLocation = CurrentShockWavePoint->GetComponentLocation();
			ShockWaveRotation = CurrentShockWavePoint->GetComponentRotation();
		}
		else
		{
			FVector GroundLocation;
			FVector GroundNormal;

			if (TraceGroundPoint(FXLocation, GroundLocation, GroundNormal))
			{
				ShockWaveLocation = GroundLocation + GroundNormal * 3.0f;
				ShockWaveRotation = FRotationMatrix::MakeFromZ(GroundNormal).Rotator();
			}
		}

		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ShockWaveFX, ShockWaveLocation, ShockWaveRotation, FVector(ShockWaveScale), true);
	}
}

void USPGAIComponent::ApplyZoneDamage(ESPGDamageZone Zone, FVector HitLocation)
{
	if (bSPGDestroyed || CurrentState == ESPGAIState::Destroyed)
	{
		return;
	}

	DebugLog(FString::Printf(TEXT("[SPG DAMAGE] Zone hit: %s Location=%s"), *UEnum::GetValueAsString(Zone), *HitLocation.ToString()));

	switch (Zone)
	{
	case ESPGDamageZone::Turret:
		DestroySPG(HitLocation);
		break;

	case ESPGDamageZone::GunBarrel:
	case ESPGDamageZone::Hull:
		DisableGunAndRetreat();
		break;

	case ESPGDamageZone::LeftTrack:
	case ESPGDamageZone::RightTrack:
		StartTrackDamageSequence(Zone);
		break;

	case ESPGDamageZone::Engine:
		StartEngineBurning();
		break;

	default:
		break;
	}
}

void USPGAIComponent::DisableGunAndRetreat()
{
	if (bSPGDestroyed || bGunDestroyed)
	{
		return;
	}

	bGunDestroyed = true;
	SetFiringActive(false);
	FireTimer = 0.0f;

	if (!bMobilityDestroyed)
	{
		BeginReturning();
	}
	else
	{
		SetState(ESPGAIState::Immobilized);
	}

	DebugLog(TEXT("[SPG DAMAGE] Gun disabled or hull hit, retreat started"));
}

void USPGAIComponent::StartTrackDamageSequence(ESPGDamageZone Zone)
{
	if (bSPGDestroyed || bMobilityDestroyed)
	{
		return;
	}

	bTrackHitWhileMoving = IsMovementState(CurrentState);
	bMobilityDestroyed = true;
	DamagedTrackZone = Zone;
	TrackTurnDirection = Zone == ESPGDamageZone::LeftTrack ? -1 : 1;

	SetFiringActive(false);
	SetWheelSpeed(0.0f);
	FireTimer = 0.0f;

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(TrackTurnReactionTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(CrewEvacTimerHandle);
		GetWorld()->GetTimerManager().SetTimer(TrackTurnReactionTimerHandle, this, &USPGAIComponent::FinishTrackTurnReaction, TrackTurnReactionDuration, false);
	}

	SetState(ESPGAIState::TrackTurnReaction);

	DebugLog(FString::Printf(TEXT("[SPG DAMAGE] Track damaged: %s MovingAtHit=%d"), *UEnum::GetValueAsString(Zone), bTrackHitWhileMoving ? 1 : 0));
}

void USPGAIComponent::TickTrackTurnReaction(float DeltaTime)
{
	AActor* Owner = GetOwner();

	if (!Owner)
	{
		SetWheelSpeed(0.0f);
		return;
	}

	if (!bGunDestroyed)
	{
		TickAiming(DeltaTime);
	}

	if (!bTrackHitWhileMoving)
	{
		SetWheelSpeed(0.0f);
		return;
	}

	const float SafeDeltaTime = FMath::Max(DeltaTime, 0.001f);
	const FRotator CurrentRotation = Owner->GetActorRotation();
	const float NewYaw = CurrentRotation.Yaw + TrackTurnDirection * TrackTurnReactionSpeedDeg * SafeDeltaTime;

	const FVector CurrentLocation = Owner->GetActorLocation();
	const FVector Forward = FRotator(0.0f, CurrentRotation.Yaw, 0.0f).Vector();
	const FVector RawNewLocation = CurrentLocation + Forward * MoveSpeedCm * TrackReactionForwardSpeedScale * SafeDeltaTime;

	FVector GroundedLocation = RawNewLocation;
	FVector GroundNormal = FVector::UpVector;
	float SlopeDeg = 0.0f;

	if (!ProjectLocationToGround(RawNewLocation, NewYaw, GroundedLocation, GroundNormal, SlopeDeg))
	{
		SetWheelSpeed(0.0f);
		return;
	}

	if (!IsSlopeDriveable(SlopeDeg))
	{
		SetWheelSpeed(0.0f);
		return;
	}

	FRotator FinalRotation = FRotator(0.0f, NewYaw, 0.0f);

	if (bAlignToGround)
	{
		FinalRotation = MakeGroundAlignedRotation(NewYaw, GroundNormal);
	}

	FinalRotation = FMath::RInterpConstantTo(CurrentRotation, FinalRotation, SafeDeltaTime, GroundRotationInterpSpeedDeg);

	Owner->SetActorLocationAndRotation(GroundedLocation, FinalRotation, false, nullptr, ETeleportType::TeleportPhysics);
	SetWheelSpeed(MoveSpeedCm * TrackReactionForwardSpeedScale * WheelAnimationSpeedScale);
}

void USPGAIComponent::FinishTrackTurnReaction()
{
	if (bSPGDestroyed || CurrentState == ESPGAIState::Destroyed)
	{
		return;
	}

	SetWheelSpeed(0.0f);
	SetFiringActive(false);
	FireTimer = 0.0f;
	StartCrewEvacTimer();

	DebugLog(TEXT("[SPG DAMAGE] Track reaction finished, crew evacuation timer started"));
}

void USPGAIComponent::StartCrewEvacTimer()
{
	if (CurrentState == ESPGAIState::CrewEvacWait || bSPGDestroyed || bCrewEvacuated)
	{
		return;
	}

	SetWheelSpeed(0.0f);
	SetFiringActive(false);
	SetState(ESPGAIState::CrewEvacWait);

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(CrewEvacTimerHandle);
		GetWorld()->GetTimerManager().SetTimer(CrewEvacTimerHandle, this, &USPGAIComponent::FinishCrewEvacTimer, CrewEvacDelayAfterImmobilized, false);
	}

	DebugLog(TEXT("[SPG CREW] Crew evacuation timer started"));
}

void USPGAIComponent::FinishCrewEvacTimer()
{
	if (bSPGDestroyed || CurrentState == ESPGAIState::Destroyed)
	{
		return;
	}

	bCrewEvacuated = true;

	SetWheelSpeed(0.0f);
	SetFiringActive(false);
	OpenHatches();
	TriggerCrewEvacuation();
	SetState(ESPGAIState::Immobilized);

	DebugLog(TEXT("[SPG CREW] Crew evacuation started after immobilization"));
}

void USPGAIComponent::StartEngineBurning()
{
	if (bSPGDestroyed || bEngineDestroyed)
	{
		return;
	}

	bEngineDestroyed = true;
	StartEngineFire();

	if (bMobilityDestroyed)
	{
		SetWheelSpeed(0.0f);
		SetFiringActive(false);
		OpenHatches();
		TriggerCrewEvacuation();
		SetState(ESPGAIState::Burning);
		return;
	}

	SetState(ESPGAIState::EngineCoast);

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(EngineMobilityFailureTimerHandle);
		GetWorld()->GetTimerManager().SetTimer(EngineMobilityFailureTimerHandle, this, &USPGAIComponent::ApplyEngineMobilityFailure, EngineMobilityFailureDelay, false);
	}

	DebugLog(TEXT("[SPG DAMAGE] Engine hit, coast started"));
}

void USPGAIComponent::TickEngineCoast(float DeltaTime)
{
	AActor* Owner = GetOwner();

	if (!Owner || bSPGDestroyed)
	{
		SetWheelSpeed(0.0f);
		return;
	}

	if (bMobilityDestroyed)
	{
		SetWheelSpeed(0.0f);
		SetState(ESPGAIState::Burning);
		return;
	}

	const float SafeDeltaTime = FMath::Max(DeltaTime, 0.001f);
	const FRotator CurrentRotation = Owner->GetActorRotation();
	const float CurrentYaw = CurrentRotation.Yaw;

	const FVector CurrentLocation = Owner->GetActorLocation();
	const FVector Forward = FRotator(0.0f, CurrentYaw, 0.0f).Vector();
	const FVector RawNewLocation = CurrentLocation + Forward * MoveSpeedCm * EngineCoastSpeedScale * SafeDeltaTime;

	FVector GroundedLocation = RawNewLocation;
	FVector GroundNormal = FVector::UpVector;
	float SlopeDeg = 0.0f;

	if (!ProjectLocationToGround(RawNewLocation, CurrentYaw, GroundedLocation, GroundNormal, SlopeDeg))
	{
		SetWheelSpeed(0.0f);
		return;
	}

	if (!IsSlopeDriveable(SlopeDeg))
	{
		SetWheelSpeed(0.0f);
		return;
	}

	FRotator FinalRotation = CurrentRotation;

	if (bAlignToGround)
	{
		FinalRotation = MakeGroundAlignedRotation(CurrentYaw, GroundNormal);
	}

	FinalRotation = FMath::RInterpConstantTo(CurrentRotation, FinalRotation, SafeDeltaTime, GroundRotationInterpSpeedDeg);

	Owner->SetActorLocationAndRotation(GroundedLocation, FinalRotation, false, nullptr, ETeleportType::TeleportPhysics);
	SetWheelSpeed(MoveSpeedCm * EngineCoastSpeedScale * WheelAnimationSpeedScale);
}

void USPGAIComponent::ApplyEngineMobilityFailure()
{
	if (bSPGDestroyed || CurrentState == ESPGAIState::Destroyed)
	{
		return;
	}

	bMobilityDestroyed = true;
	bCrewEvacuated = true;

	SetWheelSpeed(0.0f);
	SetFiringActive(false);
	OpenHatches();
	TriggerCrewEvacuation();
	SetState(ESPGAIState::Burning);

	DebugLog(TEXT("[SPG DAMAGE] Engine mobility failed, crew evacuation started"));
}

void USPGAIComponent::DestroySPG(FVector HitLocation)
{
	if (bSPGDestroyed)
	{
		return;
	}

	ClearDamageTimers();

	bSPGDestroyed = true;
	bMobilityDestroyed = true;
	bGunDestroyed = true;

	SetWheelSpeed(0.0f);
	SetFiringActive(false);
	SetNavObstacleActive(true);
	OpenHatches();
	ApplyDestroyedMaterial();
	PlayDestroyedExplosion(HitLocation);
	SetState(ESPGAIState::Destroyed);

	DebugLog(TEXT("[SPG DAMAGE] Destroyed"));
}

void USPGAIComponent::OpenHatches()
{
	TargetHatchAngle = OpenHatchAngle;
	DebugLog(FString::Printf(TEXT("[SPG HATCHES] Open requested | Target=%.1f"), TargetHatchAngle));
}

void USPGAIComponent::TickHatchAnimation(float DeltaTime)
{
	CurrentHatchAngle = FMath::FInterpConstantTo(CurrentHatchAngle, TargetHatchAngle, DeltaTime, HatchAnimSpeedDegPerSec);
	SetAnimFloat(HatchAngleVariableName, CurrentHatchAngle);
}

void USPGAIComponent::TriggerCrewEvacuation()
{
	if (bCrewSpawned)
	{
		return;
	}

	SetNavObstacleActive(true);

	if (GetWorld() && CrewSpawnDelayAfterNavObstacleSeconds > 0.0f)
	{
		GetWorld()->GetTimerManager().ClearTimer(CrewSpawnDelayTimerHandle);
		GetWorld()->GetTimerManager().SetTimer(CrewSpawnDelayTimerHandle, this, &USPGAIComponent::SpawnCrewFromSPGDelayed, CrewSpawnDelayAfterNavObstacleSeconds, false);
	}
	else
	{
		SpawnCrewFromSPG();
	}

	DebugLog(TEXT("[SPG CREW] Crew evacuation requested"));
}

void USPGAIComponent::SpawnCrewFromSPGDelayed()
{
	SpawnCrewFromSPG();
}

void USPGAIComponent::SpawnCrewFromSPG()
{
	if (bCrewSpawned)
	{
		DebugLog(TEXT("[SPG CREW] Crew already spawned"));
		return;
	}

	if (!GetWorld())
	{
		return;
	}

	if (!CrewInfantryClass)
	{
		DebugLog(TEXT("[SPG CREW] CrewInfantryClass is NULL"));
		return;
	}

	SelectedAPCEvacRouteIndex = FindNearestAPCRoutePointIndex();

	if (!APCRoutePoints.IsValidIndex(SelectedAPCEvacRouteIndex))
	{
		DebugLog(TEXT("[SPG CREW] No valid APC evacuation route point found"));
		return;
	}

	bCrewSpawned = true;
	bCrewEvacuated = true;
	SPGCrewInsideCount = 0;

	SpawnedCrew.Empty();
	PendingCrewSpawnIndex = 0;

	RequestAPCSpawnForCrewEvacuation();

	GetWorld()->GetTimerManager().ClearTimer(CrewSpawnStepTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(CrewSpawnStepTimerHandle, this, &USPGAIComponent::SpawnNextCrewMember, CrewSpawnIntervalSeconds, false);

	DebugLog(TEXT("[SPG CREW] Crew stagger spawn started"));
}

void USPGAIComponent::SpawnNextCrewMember()
{
	if (!GetWorld())
	{
		return;
	}

	if (PendingCrewSpawnIndex >= CrewCount)
	{
		UpdateAPCCrewAssignment();
		DebugLog(FString::Printf(TEXT("[SPG CREW] Crew spawn finished | Spawned=%d"), SpawnedCrew.Num()));
		return;
	}

	AInfantryCharacter* CrewMember = SpawnSingleCrewMember(PendingCrewSpawnIndex);

	if (CrewMember)
	{
		SpawnedCrew.Add(CrewMember);
		UpdateAPCCrewAssignment();
	}

	PendingCrewSpawnIndex++;

	if (PendingCrewSpawnIndex < CrewCount)
	{
		GetWorld()->GetTimerManager().ClearTimer(CrewSpawnStepTimerHandle);
		GetWorld()->GetTimerManager().SetTimer(CrewSpawnStepTimerHandle, this, &USPGAIComponent::SpawnNextCrewMember, CrewSpawnIntervalSeconds, false);
	}
	else
	{
		UpdateAPCCrewAssignment();
		DebugLog(FString::Printf(TEXT("[SPG CREW] Crew spawn finished | Spawned=%d"), SpawnedCrew.Num()));
	}
}

AInfantryCharacter* USPGAIComponent::SpawnSingleCrewMember(int32 CrewIndex)
{
	if (!GetWorld() || !CrewInfantryClass)
	{
		return nullptr;
	}

	AActor* Owner = GetOwner();

	const FRotator BaseRotation = Owner ? Owner->GetActorRotation() : FRotator::ZeroRotator;
	const FVector SpawnLocation = GetCrewSpawnLocation(CrewIndex);
	const FRotator SpawnRotation(0.0f, BaseRotation.Yaw + FMath::RandRange(-35.0f, 35.0f), 0.0f);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Owner;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AInfantryCharacter* CrewMember = GetWorld()->SpawnActor<AInfantryCharacter>(CrewInfantryClass, SpawnLocation, SpawnRotation, SpawnParams);

	if (!CrewMember)
	{
		DebugLog(FString::Printf(TEXT("[SPG CREW] Failed to spawn crew member %d"), CrewIndex));
		return nullptr;
	}

	CrewMember->SpawnDefaultController();
	CrewMember->SetRunning(true);
	CrewMember->SetAIAnimState(TEXT("ReturnToPost"));

	AInfantryAIController* CrewController = Cast<AInfantryAIController>(CrewMember->GetController());

	if (CrewController)
	{
		CrewController->BeginMoveToShelterLocation(GetCrewShelterLocation(CrewIndex));
	}
	else
	{
		DebugLog(FString::Printf(TEXT("[SPG CREW] Crew member has no InfantryAIController | Pawn=%s"), *GetNameSafe(CrewMember)));
	}

	DebugLog(FString::Printf(
		TEXT("[SPG CREW] Spawned crew member %d | Pawn=%s Shelter=%s"),
		CrewIndex,
		*GetNameSafe(CrewMember),
		*GetNameSafe(GetSelectedAPCEvacPoint())
	));
	return CrewMember;
}

FVector USPGAIComponent::GetCrewSpawnLocation(int32 CrewIndex) const
{
	AActor* Owner = GetOwner();

	if (!Owner)
	{
		return FVector::ZeroVector;
	}

	const FVector BaseLocation = Owner->GetActorLocation();
	const FVector Forward = Owner->GetActorForwardVector();
	const FVector Right = Owner->GetActorRightVector();
	const FVector UpOffset(0.0f, 0.0f, CrewSpawnZOffsetCm);

	if (CrewIndex == 0)
	{
		return BaseLocation + Right * CrewSpawnSideOffsetCm + UpOffset;
	}

	if (CrewIndex == 1)
	{
		return BaseLocation - Right * CrewSpawnSideOffsetCm + UpOffset;
	}

	if (CrewIndex == 2)
	{
		return BaseLocation + Forward * CrewSpawnFrontOffsetCm + UpOffset;
	}

	if (CrewIndex == 3)
	{
		return BaseLocation - Forward * CrewSpawnFrontOffsetCm + UpOffset;
	}

	const float Offset = (CrewIndex - 3) * CrewSpawnFallbackSpacingCm;

	return BaseLocation - Forward * CrewSpawnFrontOffsetCm + Right * Offset + UpOffset;
}

FVector USPGAIComponent::GetCrewShelterLocation(int32 CrewIndex) const
{
	AActor* EvacPoint = GetSelectedAPCEvacPoint();

	if (!EvacPoint)
	{
		return FVector::ZeroVector;
	}

	const FVector BaseLocation = EvacPoint->GetActorLocation();

	if (CrewCount <= 1)
	{
		return BaseLocation;
	}

	const float AngleStep = 360.0f / FMath::Max(1, CrewCount);
	const float AngleDeg = AngleStep * CrewIndex;

	const FVector Offset = FVector(
		FMath::Cos(FMath::DegreesToRadians(AngleDeg)),
		FMath::Sin(FMath::DegreesToRadians(AngleDeg)),
		0.0f
	) * CrewShelterFormationRadiusCm;

	return BaseLocation + Offset;
}

void USPGAIComponent::SpawnAPCForCrewEvacuation()
{
	if (bAPCSpawned)
	{
		DebugLog(TEXT("[SPG APC] APC already spawned"));
		return;
	}

	if (!GetWorld())
	{
		return;
	}

	if (!APCClass)
	{
		DebugLog(TEXT("[SPG APC] APCClass is NULL"));
		return;
	}

	if (!APCSpawnPoint)
	{
		DebugLog(TEXT("[SPG APC] APCSpawnPoint is NULL"));
		return;
	}

	if (!APCRoutePoints.IsValidIndex(SelectedAPCEvacRouteIndex))
	{
		DebugLog(TEXT("[SPG APC] Selected APC evacuation route index is invalid"));
		return;
	}

	bAPCSpawned = true;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	SpawnedAPC = GetWorld()->SpawnActor<AActor>(
		APCClass,
		APCSpawnPoint->GetActorLocation(),
		APCSpawnPoint->GetActorRotation(),
		SpawnParams
	);

	if (!SpawnedAPC)
	{
		DebugLog(TEXT("[SPG APC] Failed to spawn APC"));
		return;
	}

	UAPCAIComponent* APCAI = FindAPCAIComponent(SpawnedAPC);

	if (!APCAI)
	{
		DebugLog(FString::Printf(TEXT("[SPG APC] APCAIComponent not found | APC=%s"), *GetNameSafe(SpawnedAPC)));
		return;
	}

	APCAI->SetEvacuationCrew(SpawnedCrew);
	APCAI->StartEvacuation(APCSpawnPoint, APCRoutePoints, SelectedAPCEvacRouteIndex);

	DebugLog(FString::Printf(
		TEXT("[SPG APC] Spawned and started | APC=%s Crew=%d EvacIndex=%d EvacPoint=%s"),
		*GetNameSafe(SpawnedAPC),
		SpawnedCrew.Num(),
		SelectedAPCEvacRouteIndex,
		*GetNameSafe(APCRoutePoints[SelectedAPCEvacRouteIndex])
	));
}

void USPGAIComponent::UpdateAPCCrewAssignment()
{
	if (!SpawnedAPC)
	{
		return;
	}

	UAPCAIComponent* APCAI = FindAPCAIComponent(SpawnedAPC);

	if (!APCAI)
	{
		return;
	}

	APCAI->SetEvacuationCrew(SpawnedCrew);
}

UAPCAIComponent* USPGAIComponent::FindAPCAIComponent(AActor* APCActor) const
{
	if (!APCActor)
	{
		return nullptr;
	}

	return APCActor->FindComponentByClass<UAPCAIComponent>();
}

int32 USPGAIComponent::GetSPGCrewInsideCount() const
{
	return SPGCrewInsideCount;
}

int32 USPGAIComponent::GetSpawnedCrewCount() const
{
	return SpawnedCrew.Num();
}

void USPGAIComponent::SetNavObstacleActive(bool bActive)
{
	if (!NavObstacleComponent)
	{
		NavObstacleComponent = FindPrimitiveComponentByName(NavObstacleComponentName);
	}

	if (!NavObstacleComponent)
	{
		DebugLog(FString::Printf(TEXT("[SPG NAV] NavObstacle not found | Expected=%s"), *NavObstacleComponentName.ToString()));
		return;
	}

	bNavObstacleActive = bActive;

	NavObstacleComponent->SetCollisionEnabled(bActive ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	NavObstacleComponent->SetCanEverAffectNavigation(bActive);
	NavObstacleComponent->RecreatePhysicsState();

	DebugLog(FString::Printf(TEXT("[SPG NAV] NavObstacle active=%d | Component=%s"), bActive ? 1 : 0, *GetNameSafe(NavObstacleComponent)));
}

bool USPGAIComponent::IsMovementState(ESPGAIState State) const
{
	return State == ESPGAIState::MovingRoute ||
		State == ESPGAIState::MovingFirePosition ||
		State == ESPGAIState::Returning;
}

void USPGAIComponent::SetWheelSpeed(float Speed)
{
	CurrentWheelSpeed = Speed;
	SetAnimFloat(WheelSpeedVariableName, Speed);
}

void USPGAIComponent::SetTurretRotation(float Angle)
{
	SetAnimFloat(TurretRotationVariableName, Angle);
}

void USPGAIComponent::SetTurretElevation(float Angle)
{
	SetAnimFloat(TurretElevationVariableName, Angle);
}

void USPGAIComponent::SetFiringActive(bool bActive)
{
	CallBlueprintEventBool(SetFiringEventName, bActive);
}

void USPGAIComponent::SetAnimFloat(FName VariableName, float Value)
{
	if (!AnimInstance && MeshComponent)
	{
		AnimInstance = MeshComponent->GetAnimInstance();
	}

	if (!AnimInstance || VariableName.IsNone())
	{
		return;
	}

	FProperty* Property = AnimInstance->GetClass()->FindPropertyByName(VariableName);

	if (!Property)
	{
		return;
	}

	if (FFloatProperty* FloatProperty = CastField<FFloatProperty>(Property))
	{
		FloatProperty->SetPropertyValue_InContainer(AnimInstance, Value);
	}
}

bool USPGAIComponent::CallBlueprintEventNoParams(FName EventName)
{
	AActor* Owner = GetOwner();

	if (!Owner || EventName.IsNone())
	{
		return false;
	}

	UFunction* Function = Owner->FindFunction(EventName);

	if (!Function)
	{
		return false;
	}

	Owner->ProcessEvent(Function, nullptr);
	return true;
}

bool USPGAIComponent::CallBlueprintEventBool(FName EventName, bool bValue)
{
	AActor* Owner = GetOwner();

	if (!Owner || EventName.IsNone())
	{
		return false;
	}

	UFunction* Function = Owner->FindFunction(EventName);

	if (!Function)
	{
		return false;
	}

	struct FBoolEventParams
	{
		bool bValue;
	};

	FBoolEventParams Params;
	Params.bValue = bValue;

	Owner->ProcessEvent(Function, &Params);
	return true;
}

bool USPGAIComponent::IsDroneActor(AActor* Actor) const
{
	if (!Actor)
	{
		return false;
	}

	if (Cast<AFPVDronePawn>(Actor))
	{
		return true;
	}

	if (Actor->ActorHasTag(PlayerDroneTag))
	{
		return true;
	}

	const FString Name = Actor->GetName();

	return Name.Contains(TEXT("Drone")) || Name.Contains(TEXT("FPV"));
}

bool USPGAIComponent::IsDroneWarheadComponent(UPrimitiveComponent* Component) const
{
	if (!Component)
	{
		return false;
	}

	return Component->ComponentHasTag(DroneWarheadComponentTag) || Component->GetFName() == DroneWarheadComponentTag;
}

bool USPGAIComponent::IsDroneWarheadArmed(AActor* DroneActor) const
{
	const AFPVDronePawn* FPVDrone = Cast<AFPVDronePawn>(DroneActor);

	if (!FPVDrone)
	{
		return false;
	}

	return FPVDrone->IsBombArmed();
}

bool USPGAIComponent::IsValidDirectWarheadHit(UPrimitiveComponent* ZoneComponent, AActor* DroneActor, UPrimitiveComponent* DroneHitComponent, FVector& OutHitLocation) const
{
	if (!ZoneComponent || !DroneActor || !DroneHitComponent)
	{
		return false;
	}

	if (!IsDroneActor(DroneActor))
	{
		return false;
	}

	if (bRequireDroneWarheadArmed && !IsDroneWarheadArmed(DroneActor))
	{
		DebugLog(TEXT("[SPG DAMAGE] Direct hit rejected: drone warhead is not armed"));
		return false;
	}

	if (!IsDroneWarheadComponent(DroneHitComponent))
	{
		return false;
	}

	FVector Velocity = DroneHitComponent->GetComponentVelocity();

	if (Velocity.IsNearlyZero())
	{
		Velocity = DroneActor->GetVelocity();
	}

	const float Speed = Velocity.Size();

	if (Speed < MinDirectHitSpeedCm)
	{
		DebugLog(FString::Printf(TEXT("[SPG DAMAGE] Direct hit rejected: speed too low %.1f"), Speed));
		return false;
	}

	const FVector WarheadLocation = DroneHitComponent->GetComponentLocation();
	FVector ClosestPoint = ZoneComponent->GetComponentLocation();

	const float ClosestDistance = ZoneComponent->GetClosestPointOnCollision(WarheadLocation, ClosestPoint);

	if (ClosestDistance < 0.0f)
	{
		ClosestPoint = ZoneComponent->GetComponentLocation();
	}

	OutHitLocation = ClosestPoint;

	return true;
}

void USPGAIComponent::CrashDroneOnVulnerableZoneContact(AActor* DroneActor, UPrimitiveComponent* ZoneComponent, UPrimitiveComponent* DroneHitComponent)
{
	if (!bCrashDroneOnVulnerableZoneContact)
	{
		return;
	}

	AFPVDronePawn* FPVDrone = Cast<AFPVDronePawn>(DroneActor);

	if (!FPVDrone || FPVDrone->IsCrashed())
	{
		return;
	}

	if (!IsDroneWarheadComponent(DroneHitComponent))
	{
		return;
	}

	const float DroneSpeedCm = FPVDrone->GetVelocity().Size();

	if (DroneSpeedCm < MinDroneZoneCrashSpeedCm)
	{
		return;
	}

	FVector CrashLocation = DroneHitComponent ? DroneHitComponent->GetComponentLocation() : FPVDrone->GetActorLocation();

	if (ZoneComponent)
	{
		FVector ClosestPoint = CrashLocation;
		const float ClosestDistance = ZoneComponent->GetClosestPointOnCollision(CrashLocation, ClosestPoint);

		if (ClosestDistance >= 0.0f)
		{
			CrashLocation = ClosestPoint;
		}
	}

	FPVDrone->ForceCrashAtLocation(CrashLocation);
}

ESPGDamageZone USPGAIComponent::GetDamageZoneByComponent(UPrimitiveComponent* Component) const
{
	if (!Component)
	{
		return ESPGDamageZone::None;
	}

	if (const ESPGDamageZone* Zone = DamageZoneMap.Find(Component))
	{
		return *Zone;
	}

	return ESPGDamageZone::None;
}

void USPGAIComponent::OnDamageZoneBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	const ESPGDamageZone Zone = GetDamageZoneByComponent(OverlappedComponent);

	if (Zone == ESPGDamageZone::None)
	{
		return;
	}

	if (!IsDroneActor(OtherActor))
	{
		return;
	}

	FVector HitLocation = OverlappedComponent->GetComponentLocation();
	const bool bValidDirectHit = IsValidDirectWarheadHit(OverlappedComponent, OtherActor, OtherComp, HitLocation);
	const bool bWarheadContact = IsDroneWarheadComponent(OtherComp);

	if (bValidDirectHit)
	{
		ApplyZoneDamage(Zone, HitLocation);
	}
	else
	{
		DebugLog(FString::Printf(TEXT("[SPG DAMAGE] Zone touched but rejected: %s OtherComp=%s"), *UEnum::GetValueAsString(Zone), OtherComp ? *OtherComp->GetName() : TEXT("NULL")));
	}

	if (bWarheadContact)
	{
		CrashDroneOnVulnerableZoneContact(OtherActor, OverlappedComponent, OtherComp);
	}
}

bool USPGAIComponent::TraceGroundPoint(const FVector& WorldPoint, FVector& OutHitLocation, FVector& OutHitNormal) const
{
	UWorld* World = GetWorld();
	const AActor* Owner = GetOwner();

	if (!World || !Owner)
	{
		return false;
	}

	const FVector Start = WorldPoint + FVector(0.0f, 0.0f, GroundTraceUpCm);
	const FVector End = WorldPoint - FVector(0.0f, 0.0f, GroundTraceDownCm);

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Owner);
	Params.bTraceComplex = false;

	FHitResult Hit;

	const bool bHit = World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);

	if (!bHit)
	{
		return false;
	}

	OutHitLocation = Hit.Location;
	OutHitNormal = Hit.ImpactNormal.GetSafeNormal();

	return true;
}

bool USPGAIComponent::ProjectLocationToGround(const FVector& DesiredLocation, float DesiredYaw, FVector& OutLocation, FVector& OutGroundNormal, float& OutSlopeDeg) const
{
	if (!bFollowGround)
	{
		OutLocation = DesiredLocation;
		OutGroundNormal = FVector::UpVector;
		OutSlopeDeg = 0.0f;
		return true;
	}

	const FRotator YawRotation(0.0f, DesiredYaw, 0.0f);
	const FVector Forward = YawRotation.Vector();
	const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	FVector FrontLeftPoint;
	FVector FrontRightPoint;
	FVector RearLeftPoint;
	FVector RearRightPoint;

	bool bHasManualPoints = false;

	if (bUseManualGroundTracePoints)
	{
		FVector LocalFL;
		FVector LocalFR;
		FVector LocalRL;
		FVector LocalRR;

		bHasManualPoints = GetManualGroundTraceOffsets(LocalFL, LocalFR, LocalRL, LocalRR);

		if (bHasManualPoints)
		{
			FrontLeftPoint = TransformGroundTraceOffset(DesiredLocation, DesiredYaw, LocalFL);
			FrontRightPoint = TransformGroundTraceOffset(DesiredLocation, DesiredYaw, LocalFR);
			RearLeftPoint = TransformGroundTraceOffset(DesiredLocation, DesiredYaw, LocalRL);
			RearRightPoint = TransformGroundTraceOffset(DesiredLocation, DesiredYaw, LocalRR);
		}
	}

	if (!bHasManualPoints)
	{
		FrontLeftPoint = DesiredLocation + Forward * GroundTraceHalfLengthCm - Right * GroundTraceHalfWidthCm;
		FrontRightPoint = DesiredLocation + Forward * GroundTraceHalfLengthCm + Right * GroundTraceHalfWidthCm;
		RearLeftPoint = DesiredLocation - Forward * GroundTraceHalfLengthCm - Right * GroundTraceHalfWidthCm;
		RearRightPoint = DesiredLocation - Forward * GroundTraceHalfLengthCm + Right * GroundTraceHalfWidthCm;
	}

	if (!bUseFourPointGroundTrace)
	{
		FVector HitLocation;
		FVector HitNormal;

		if (!TraceGroundPoint(DesiredLocation, HitLocation, HitNormal))
		{
			return false;
		}

		OutGroundNormal = HitNormal;
		OutSlopeDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(OutGroundNormal, FVector::UpVector), -1.0f, 1.0f)));
		OutLocation = FVector(DesiredLocation.X, DesiredLocation.Y, HitLocation.Z + GroundOffsetCm);

		return true;
	}

	FVector FrontLeftHit;
	FVector FrontRightHit;
	FVector RearLeftHit;
	FVector RearRightHit;

	FVector FrontLeftNormal;
	FVector FrontRightNormal;
	FVector RearLeftNormal;
	FVector RearRightNormal;

	const bool bFL = TraceGroundPoint(FrontLeftPoint, FrontLeftHit, FrontLeftNormal);
	const bool bFR = TraceGroundPoint(FrontRightPoint, FrontRightHit, FrontRightNormal);
	const bool bRL = TraceGroundPoint(RearLeftPoint, RearLeftHit, RearLeftNormal);
	const bool bRR = TraceGroundPoint(RearRightPoint, RearRightHit, RearRightNormal);

	if (!bFL || !bFR || !bRL || !bRR)
	{
		return false;
	}

	const FVector FrontCenter = (FrontLeftHit + FrontRightHit) * 0.5f;
	const FVector RearCenter = (RearLeftHit + RearRightHit) * 0.5f;
	const FVector LeftCenter = (FrontLeftHit + RearLeftHit) * 0.5f;
	const FVector RightCenter = (FrontRightHit + RearRightHit) * 0.5f;

	const FVector GroundForward = (FrontCenter - RearCenter).GetSafeNormal();
	const FVector GroundRight = (RightCenter - LeftCenter).GetSafeNormal();

	FVector PlaneNormal = FVector::CrossProduct(GroundForward, GroundRight).GetSafeNormal();

	if (PlaneNormal.Z < 0.0f)
	{
		PlaneNormal *= -1.0f;
	}

	if (PlaneNormal.IsNearlyZero())
	{
		PlaneNormal = (FrontLeftNormal + FrontRightNormal + RearLeftNormal + RearRightNormal).GetSafeNormal();
	}

	if (PlaneNormal.IsNearlyZero())
	{
		PlaneNormal = FVector::UpVector;
	}

	const float AverageZ = (FrontLeftHit.Z + FrontRightHit.Z + RearLeftHit.Z + RearRightHit.Z) * 0.25f;

	OutGroundNormal = PlaneNormal;
	OutSlopeDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(OutGroundNormal, FVector::UpVector), -1.0f, 1.0f)));
	OutLocation = FVector(DesiredLocation.X, DesiredLocation.Y, AverageZ + GroundOffsetCm);

	return true;
}

bool USPGAIComponent::GetManualGroundTraceOffsets(FVector& OutFL, FVector& OutFR, FVector& OutRL, FVector& OutRR) const
{
	AActor* Owner = GetOwner();

	if (!Owner)
	{
		return false;
	}

	USceneComponent* FL = FindSceneComponentByName(GroundTraceFLName);
	USceneComponent* FR = FindSceneComponentByName(GroundTraceFRName);
	USceneComponent* RL = FindSceneComponentByName(GroundTraceRLName);
	USceneComponent* RR = FindSceneComponentByName(GroundTraceRRName);

	if (!FL || !FR || !RL || !RR)
	{
		return false;
	}

	const FTransform OwnerTransform = Owner->GetActorTransform();

	OutFL = OwnerTransform.InverseTransformPosition(FL->GetComponentLocation());
	OutFR = OwnerTransform.InverseTransformPosition(FR->GetComponentLocation());
	OutRL = OwnerTransform.InverseTransformPosition(RL->GetComponentLocation());
	OutRR = OwnerTransform.InverseTransformPosition(RR->GetComponentLocation());

	OutFL.Z = 0.0f;
	OutFR.Z = 0.0f;
	OutRL.Z = 0.0f;
	OutRR.Z = 0.0f;

	return true;
}

FVector USPGAIComponent::TransformGroundTraceOffset(const FVector& DesiredLocation, float DesiredYaw, const FVector& LocalOffset) const
{
	const FRotator YawRotation(0.0f, DesiredYaw, 0.0f);
	return DesiredLocation + YawRotation.RotateVector(LocalOffset);
}

FRotator USPGAIComponent::MakeGroundAlignedRotation(float DesiredYaw, const FVector& GroundNormal) const
{
	const FVector DesiredForward = FRotator(0.0f, DesiredYaw, 0.0f).Vector();
	FVector ProjectedForward = FVector::VectorPlaneProject(DesiredForward, GroundNormal).GetSafeNormal();

	if (ProjectedForward.IsNearlyZero())
	{
		ProjectedForward = DesiredForward;
	}

	return FRotationMatrix::MakeFromXZ(ProjectedForward, GroundNormal).Rotator();
}

bool USPGAIComponent::IsSlopeDriveable(float SlopeDeg) const
{
	return SlopeDeg <= MaxDriveableSlopeDeg;
}

USceneComponent* USPGAIComponent::FindSceneComponentByName(FName ComponentName) const
{
	AActor* Owner = GetOwner();

	if (!Owner)
	{
		return nullptr;
	}

	TArray<USceneComponent*> Components;
	Owner->GetComponents<USceneComponent>(Components);

	for (USceneComponent* Component : Components)
	{
		if (Component && Component->GetFName() == ComponentName)
		{
			return Component;
		}
	}

	return nullptr;
}

UPrimitiveComponent* USPGAIComponent::FindPrimitiveComponentByName(FName ComponentName) const
{
	AActor* Owner = GetOwner();

	if (!Owner)
	{
		return nullptr;
	}

	TArray<UPrimitiveComponent*> Components;
	Owner->GetComponents<UPrimitiveComponent>(Components);

	for (UPrimitiveComponent* Component : Components)
	{
		if (Component && Component->GetFName() == ComponentName)
		{
			return Component;
		}
	}

	return nullptr;
}

FVector USPGAIComponent::GetFXLocation(FName PointName, FVector FallbackLocation) const
{
	USceneComponent* Point = FindSceneComponentByName(PointName);

	if (Point)
	{
		return Point->GetComponentLocation();
	}

	return FallbackLocation;
}

void USPGAIComponent::StartEngineFire()
{
	if (EngineFirePSC || !EngineFireFX || !GetWorld())
	{
		return;
	}

	USceneComponent* Point = EngineFXPoint ? EngineFXPoint : FindSceneComponentByName(EngineFXPointName);

	if (!Point)
	{
		DebugLog(TEXT("[SPG FX] EngineFXPoint not found"));
		return;
	}

	EngineFirePSC = UGameplayStatics::SpawnEmitterAttached(
		EngineFireFX,
		Point,
		NAME_None,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		EAttachLocation::SnapToTarget,
		false
	);

	if (EngineFirePSC)
	{
		EngineFirePSC->SetWorldScale3D(FVector(EngineFireScale));
	}
}

void USPGAIComponent::PlayDestroyedExplosion(FVector HitLocation)
{
	if (!GetWorld())
	{
		return;
	}

	USceneComponent* ExplosionPoint = DestroyedExplosionFXPoint ? DestroyedExplosionFXPoint : FindSceneComponentByName(DestroyedExplosionFXPointName);

	if (!ExplosionPoint)
	{
		ExplosionPoint = DestroyedFXPoint ? DestroyedFXPoint : FindSceneComponentByName(DestroyedFXPointName);
	}

	const FVector ExplosionLocation = ExplosionPoint ? ExplosionPoint->GetComponentLocation() : HitLocation;
	const FRotator ExplosionRotation = ExplosionPoint ? ExplosionPoint->GetComponentRotation() : FRotator::ZeroRotator;

	if (DestroyedExplosionFX)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), DestroyedExplosionFX, ExplosionLocation, ExplosionRotation, FVector(DestroyedExplosionScale), true);
	}

	if (ShockWaveFX)
	{
		USceneComponent* CurrentShockWavePoint = ShockWavePoint ? ShockWavePoint : FindSceneComponentByName(ShockWavePointName);
		FVector ShockWaveLocation = ExplosionLocation;
		FRotator ShockWaveRotation = FRotator::ZeroRotator;

		if (CurrentShockWavePoint)
		{
			ShockWaveLocation = CurrentShockWavePoint->GetComponentLocation();
			ShockWaveRotation = CurrentShockWavePoint->GetComponentRotation();
		}
		else
		{
			FVector GroundLocation;
			FVector GroundNormal;

			if (TraceGroundPoint(ExplosionLocation, GroundLocation, GroundNormal))
			{
				ShockWaveLocation = GroundLocation + GroundNormal * 3.0f;
				ShockWaveRotation = FRotationMatrix::MakeFromZ(GroundNormal).Rotator();
			}
		}

		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ShockWaveFX, ShockWaveLocation, ShockWaveRotation, FVector(ShockWaveScale), true);
	}

	USceneComponent* CurrentWreckagePoint = WreckageFXPoint ? WreckageFXPoint : FindSceneComponentByName(WreckageFXPointName);
	PendingDestroyedWreckageLocation = CurrentWreckagePoint ? CurrentWreckagePoint->GetComponentLocation() : ExplosionLocation;

	if (DestroyedWreckageDelay > 0.0f)
	{
		GetWorld()->GetTimerManager().ClearTimer(DestroyedWreckageTimerHandle);
		GetWorld()->GetTimerManager().SetTimer(DestroyedWreckageTimerHandle, this, &USPGAIComponent::PlayDestroyedWreckage, DestroyedWreckageDelay, false);
	}
	else
	{
		PlayDestroyedWreckage();
	}
}

void USPGAIComponent::PlayDestroyedWreckage()
{
	if (!DestroyedWreckageFX || !GetWorld())
	{
		return;
	}

	USceneComponent* CurrentWreckagePoint = WreckageFXPoint ? WreckageFXPoint : FindSceneComponentByName(WreckageFXPointName);
	const FVector Location = CurrentWreckagePoint ? CurrentWreckagePoint->GetComponentLocation() : PendingDestroyedWreckageLocation;
	const FRotator Rotation = CurrentWreckagePoint ? CurrentWreckagePoint->GetComponentRotation() : FRotator::ZeroRotator;

	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), DestroyedWreckageFX, Location, Rotation, FVector(DestroyedWreckageScale), true);
}

void USPGAIComponent::ApplyDestroyedMaterial()
{
	if (!MeshComponent || !DestroyedMaterial)
	{
		DebugLog(FString::Printf(TEXT("[SPG VISUAL] Destroyed material skipped | Mesh=%d Material=%s"), MeshComponent ? 1 : 0, *GetNameSafe(DestroyedMaterial)));
		return;
	}

	if (!DestroyedMaterial->CheckMaterialUsage_Concurrent(MATUSAGE_SkeletalMesh))
	{
		DebugLog(FString::Printf(TEXT("[SPG VISUAL] Destroyed material rejected: missing skeletal mesh usage | Material=%s"), *GetNameSafe(DestroyedMaterial)));
		return;
	}

	if (DestroyedMaterialElementIndices.Num() > 0)
	{
		for (int32 ElementIndex : DestroyedMaterialElementIndices)
		{
			ApplyDestroyedMaterialToSlot(ElementIndex);
		}

		return;
	}

	if (bApplyDestroyedMaterialToAllSlots)
	{
		const int32 MaterialCount = MeshComponent->GetNumMaterials();

		for (int32 i = 0; i < MaterialCount; ++i)
		{
			ApplyDestroyedMaterialToSlot(i);
		}

		return;
	}

	ApplyDestroyedMaterialToSlot(0);
}

bool USPGAIComponent::ApplyDestroyedMaterialToSlot(int32 MaterialIndex)
{
	if (!MeshComponent || !DestroyedMaterial)
	{
		return false;
	}

	if (MaterialIndex < 0 || MaterialIndex >= MeshComponent->GetNumMaterials())
	{
		DebugLog(FString::Printf(TEXT("[SPG VISUAL] Destroyed material slot rejected | Slot=%d Count=%d"), MaterialIndex, MeshComponent->GetNumMaterials()));
		return false;
	}

	MeshComponent->SetMaterial(MaterialIndex, DestroyedMaterial);
	DebugLog(FString::Printf(TEXT("[SPG VISUAL] Destroyed material applied | Slot=%d Material=%s"), MaterialIndex, *GetNameSafe(DestroyedMaterial)));
	return true;
}

void USPGAIComponent::UpdateTrackDustVFX()
{
	const bool bShouldPlay =
		TrackDustFX &&
		!bSPGDestroyed &&
		CurrentWheelSpeed > TrackDustMinWheelSpeed &&
		(
			CurrentState == ESPGAIState::MovingRoute ||
			CurrentState == ESPGAIState::MovingFirePosition ||
			CurrentState == ESPGAIState::Returning ||
			CurrentState == ESPGAIState::TrackTurnReaction ||
			CurrentState == ESPGAIState::EngineCoast
			);

	for (UParticleSystemComponent* ParticleComponent : TrackDustParticleComponents)
	{
		if (!ParticleComponent)
		{
			continue;
		}

		if (bShouldPlay)
		{
			ParticleComponent->SetVisibility(true, true);

			if (!ParticleComponent->IsActive())
			{
				ParticleComponent->ActivateSystem(true);
			}
		}
		else
		{
			if (ParticleComponent->IsActive())
			{
				ParticleComponent->DeactivateSystem();
			}

			ParticleComponent->SetVisibility(false, true);
		}
	}
}

bool USPGAIComponent::IsTrackDustComponentName(const FString& ComponentName) const
{
	return ComponentName.Contains(TrackDustComponentPrefix) ||
		ComponentName.Contains(TEXT("TrackDust")) ||
		ComponentName.Contains(TEXT("DustTrack")) ||
		ComponentName.Contains(TEXT("WheelTrackDust")) ||
		ComponentName.Contains(TEXT("WheelFX"));
}

void USPGAIComponent::ClearDamageTimers()
{
	if (!GetWorld())
	{
		return;
	}

	GetWorld()->GetTimerManager().ClearTimer(EngineMobilityFailureTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(TrackTurnReactionTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(CrewEvacTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(CrewSpawnDelayTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(CrewSpawnStepTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(DestroyedWreckageTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(APCSpawnDelayTimerHandle);
}

void USPGAIComponent::DebugLog(const FString& Message) const
{
	if (bDebugLogs)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s"), *Message);
	}
}

int32 USPGAIComponent::FindNearestAPCRoutePointIndex() const
{
	AActor* Owner = GetOwner();

	if (!Owner)
	{
		return INDEX_NONE;
	}

	int32 BestIndex = INDEX_NONE;
	float BestDistanceSq = TNumericLimits<float>::Max();

	const FVector ReferenceLocation = Owner->GetActorLocation();

	for (int32 i = 0; i < APCRoutePoints.Num(); ++i)
	{
		AActor* RoutePoint = APCRoutePoints[i];

		if (!RoutePoint)
		{
			continue;
		}

		const float DistanceSq = FVector::DistSquared2D(ReferenceLocation, RoutePoint->GetActorLocation());

		if (DistanceSq < BestDistanceSq)
		{
			BestDistanceSq = DistanceSq;
			BestIndex = i;
		}
	}

	return BestIndex;
}

AActor* USPGAIComponent::GetSelectedAPCEvacPoint() const
{
	if (!APCRoutePoints.IsValidIndex(SelectedAPCEvacRouteIndex))
	{
		return nullptr;
	}

	return APCRoutePoints[SelectedAPCEvacRouteIndex];
}

void USPGAIComponent::RequestAPCSpawnForCrewEvacuation()
{
	if (!GetWorld())
	{
		return;
	}

	if (APCSpawnDelaySeconds > 0.0f)
	{
		GetWorld()->GetTimerManager().ClearTimer(APCSpawnDelayTimerHandle);
		GetWorld()->GetTimerManager().SetTimer(
			APCSpawnDelayTimerHandle,
			this,
			&USPGAIComponent::SpawnAPCForCrewEvacuation,
			APCSpawnDelaySeconds,
			false
		);

		DebugLog(FString::Printf(TEXT("[SPG APC] Spawn delayed by %.1f seconds"), APCSpawnDelaySeconds));
		return;
	}

	SpawnAPCForCrewEvacuation();
}