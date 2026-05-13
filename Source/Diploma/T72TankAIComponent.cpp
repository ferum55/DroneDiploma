#include "T72TankAIComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Components/PrimitiveComponent.h"
#include "TimerManager.h"
#include "InfantryCharacter.h"
#include "InfantryAIController.h"

#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "FPVDronePawn.h"

UT72TankAIComponent::UT72TankAIComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	DestroyedMaterialElementIndices.Add(0);

	static ConstructorHelpers::FClassFinder<AT72TankProjectile> ProjectileFinder(TEXT("/Game/Vehicles/Tank/Shell/BP_T72TankProjectile"));
	if (ProjectileFinder.Succeeded())
	{
		ProjectileClass = ProjectileFinder.Class;
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DamagedMaterialFinder(TEXT("/Game/VigilanteContent/Vehicles/East_Tank_T72b/Damaged/Materials/MI_East_Tank_T72b_Damaged.MI_East_Tank_T72b_Damaged"));
	if (DamagedMaterialFinder.Succeeded())
	{
		DestroyedTankMaterial = DamagedMaterialFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UParticleSystem> HatchSmokeFinder(TEXT("/Game/VFX/Realistic_Starter_VFX_Pack_Vol2/Particles/Smoke/P_Smoke_A.P_Smoke_A"));
	if (HatchSmokeFinder.Succeeded())
	{
		HatchSmokeFX = HatchSmokeFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UParticleSystem> SecondaryExplosionFinder(TEXT("/Game/VFX/Realistic_Starter_VFX_Pack_Vol2/Particles/Explosion/P_Explosion_Big_C.P_Explosion_Big_C"));
	if (SecondaryExplosionFinder.Succeeded())
	{
		EngineSecondaryExplosionFX = SecondaryExplosionFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UParticleSystem> DestroyedExplosionFinder(TEXT("/Game/VFX/Realistic_Starter_VFX_Pack_Vol2/Particles/Explosion/P_Explosion_Big_B.P_Explosion_Big_B"));
	if (DestroyedExplosionFinder.Succeeded())
	{
		HatchKillExplosionFX = DestroyedExplosionFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UParticleSystem> EngineFireFinder(TEXT("/Game/VFX/Realistic_Starter_VFX_Pack_Vol2/Particles/Fire/P_Fire_Wall.P_Fire_Wall"));
	if (EngineFireFinder.Succeeded())
	{
		EngineFireFX = EngineFireFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UParticleSystem> HatchFireFinder(TEXT("/Game/VFX/Realistic_Starter_VFX_Pack_Vol2/Particles/Fire/P_Fire_Big.P_Fire_Big"));
	if (HatchFireFinder.Succeeded())
	{
		HatchKillFireFX = HatchFireFinder.Object;
	}

}



void UT72TankAIComponent::BeginPlay()
{
	Super::BeginPlay();

	bTankDestroyed = false;
	bMobilityDestroyed = false;
	bGunDestroyed = false;
	bEngineDestroyed = false;
	bEngineMobilityFailureApplied = false;
	bCrewEvacuated = false;
	bCrewSpawned = false;
	SpawnedCrew.Empty();

	SetTankNavObstacleActive(false);

	RegisterDamageZones();
	ApplyDefaultAssetReferences();

	CallBlueprintEventFloat(TEXT("SetLightsEmissivity"), 0.0f);

	if (bAutoStart)
	{
		StartMission();
	}
	bCrewSpawned = false;
	SpawnedCrew.Empty();
}

void UT72TankAIComponent::ApplyDefaultAssetReferences()
{
	if (!ProjectileClass)
	{
		ProjectileClass = LoadClass<AT72TankProjectile>(nullptr, TEXT("/Game/Vehicles/Tank/Shell/BP_T72TankProjectile.BP_T72TankProjectile_C"));
	}

	if (!DestroyedTankMaterial)
	{
		DestroyedTankMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/VigilanteContent/Vehicles/East_Tank_T72b/Damaged/Materials/MI_East_Tank_T72b_Damaged.MI_East_Tank_T72b_Damaged"));
	}

	if (!HatchSmokeFX)
	{
		HatchSmokeFX = LoadObject<UParticleSystem>(nullptr, TEXT("/Game/VFX/Realistic_Starter_VFX_Pack_Vol2/Particles/Smoke/P_Smoke_A.P_Smoke_A"));
	}

	if (!EngineSecondaryExplosionFX)
	{
		EngineSecondaryExplosionFX = LoadObject<UParticleSystem>(nullptr, TEXT("/Game/VFX/Realistic_Starter_VFX_Pack_Vol2/Particles/Explosion/P_Explosion_Big_C.P_Explosion_Big_C"));
	}

	if (!HatchKillExplosionFX)
	{
		HatchKillExplosionFX = LoadObject<UParticleSystem>(nullptr, TEXT("/Game/VFX/Realistic_Starter_VFX_Pack_Vol2/Particles/Explosion/P_Explosion_Big_B.P_Explosion_Big_B"));
	}

	if (!EngineFireFX)
	{
		EngineFireFX = LoadObject<UParticleSystem>(nullptr, TEXT("/Game/VFX/Realistic_Starter_VFX_Pack_Vol2/Particles/Fire/P_Fire_Wall.P_Fire_Wall"));
	}

	if (!HatchKillFireFX)
	{
		HatchKillFireFX = LoadObject<UParticleSystem>(nullptr, TEXT("/Game/VFX/Realistic_Starter_VFX_Pack_Vol2/Particles/Fire/P_Fire_Big.P_Fire_Big"));
	}

	DebugLog(FString::Printf(
		TEXT("[T72 DEFAULTS] Projectile=%s DamagedMaterial=%s HatchSmoke=%s SecondaryExplosion=%s DestroyedExplosion=%s EngineFire=%s HatchFire=%s"),
		ProjectileClass ? *ProjectileClass->GetName() : TEXT("NULL"),
		DestroyedTankMaterial ? *DestroyedTankMaterial->GetName() : TEXT("NULL"),
		HatchSmokeFX ? *HatchSmokeFX->GetName() : TEXT("NULL"),
		EngineSecondaryExplosionFX ? *EngineSecondaryExplosionFX->GetName() : TEXT("NULL"),
		HatchKillExplosionFX ? *HatchKillExplosionFX->GetName() : TEXT("NULL"),
		EngineFireFX ? *EngineFireFX->GetName() : TEXT("NULL"),
		HatchKillFireFX ? *HatchKillFireFX->GetName() : TEXT("NULL")
	));
}

void UT72TankAIComponent::RegisterDamageZones()
{
	AActor* Owner = GetOwner();

	if (!Owner)
	{
		return;
	}

	DamageZoneMap.Empty();

	TArray<UPrimitiveComponent*> Components;
	Owner->GetComponents<UPrimitiveComponent>(Components);

	for (UPrimitiveComponent* Component : Components)
	{
		if (!Component)
		{
			continue;
		}

		ET72DamageZone Zone = ET72DamageZone::None;

		if (Component->ComponentHasTag(TEXT("T72_GunBarrel")))
		{
			Zone = ET72DamageZone::GunBarrel;
		}
		else if (Component->ComponentHasTag(TEXT("T72_LeftTrack")))
		{
			Zone = ET72DamageZone::LeftTrack;
		}
		else if (Component->ComponentHasTag(TEXT("T72_RightTrack")))
		{
			Zone = ET72DamageZone::RightTrack;
		}
		else if (Component->ComponentHasTag(TEXT("T72_Engine")))
		{
			Zone = ET72DamageZone::Engine;
		}
		else if (Component->ComponentHasTag(TEXT("T72_TurretHatch")))
		{
			Zone = ET72DamageZone::TurretHatch;
		}
		else if (Component->ComponentHasTag(TEXT("T72_HullHatch")))
		{
			Zone = ET72DamageZone::HullHatch;
		}

		if (Zone == ET72DamageZone::None)
		{
			continue;
		}

		DamageZoneMap.Add(Component, Zone);
		Component->SetGenerateOverlapEvents(true);
		Component->OnComponentBeginOverlap.AddDynamic(this, &UT72TankAIComponent::OnDamageZoneBeginOverlap);

		DebugLog(FString::Printf(TEXT("[T72 DAMAGE] Registered zone %s on component %s"), *UEnum::GetValueAsString(Zone), *Component->GetName()));
	}

	DebugLog(FString::Printf(TEXT("[T72 DAMAGE] Registered zones count: %d"), DamageZoneMap.Num()));
}

ET72DamageZone UT72TankAIComponent::GetDamageZoneByComponent(UPrimitiveComponent* Component) const
{
	if (!Component)
	{
		return ET72DamageZone::None;
	}

	if (const ET72DamageZone* Zone = DamageZoneMap.Find(Component))
	{
		return *Zone;
	}

	return ET72DamageZone::None;
}

bool UT72TankAIComponent::IsDroneActor(AActor* Actor) const
{
	if (!Actor)
	{
		return false;
	}

	if (Actor->ActorHasTag(PlayerDroneTag))
	{
		return true;
	}

	const FString Name = Actor->GetName();

	return Name.Contains(TEXT("Drone")) || Name.Contains(TEXT("FPV"));
}

bool UT72TankAIComponent::IsDroneWarheadComponent(UPrimitiveComponent* Component) const
{
	if (!Component)
	{
		return false;
	}

	return Component->ComponentHasTag(DroneWarheadComponentTag) || Component->GetFName() == DroneWarheadComponentTag;
}

bool UT72TankAIComponent::IsValidDirectWarheadHit(UPrimitiveComponent* ZoneComponent, AActor* DroneActor, UPrimitiveComponent* DroneHitComponent, FVector& OutHitLocation) const
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
		DebugLog(TEXT("[T72 DAMAGE] Direct hit rejected: drone warhead is not armed"));
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
		DebugLog(FString::Printf(TEXT("[T72 DAMAGE] Direct hit rejected: speed too low %.1f"), Speed));
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

	DebugLog(FString::Printf(
		TEXT("[T72 DAMAGE] Direct warhead hit accepted | ZoneComp=%s DroneComp=%s Speed=%.1f HitLocation=%s"),
		*ZoneComponent->GetName(),
		*DroneHitComponent->GetName(),
		Speed,
		*OutHitLocation.ToString()
	));

	return true;
}
bool UT72TankAIComponent::IsDroneWarheadArmed(AActor* DroneActor) const
{
	const AFPVDronePawn* FPVDrone = Cast<AFPVDronePawn>(DroneActor);

	if (!FPVDrone)
	{
		return false;
	}

	return FPVDrone->IsBombArmed();
}

void UT72TankAIComponent::CrashDroneOnVulnerableZoneContact(AActor* DroneActor, UPrimitiveComponent* ZoneComponent, UPrimitiveComponent* DroneHitComponent)
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

	FVector CrashLocation = DroneHitComponent
		? DroneHitComponent->GetComponentLocation()
		: FPVDrone->GetActorLocation();

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

	DebugLog(FString::Printf(TEXT("[T72 DAMAGE] Drone force-crashed after vulnerable zone contact | Drone=%s Bomb=%d Location=%s"),
		*GetNameSafe(FPVDrone),
		FPVDrone->IsBombArmed() ? 1 : 0,
		*CrashLocation.ToString()));
}

void UT72TankAIComponent::OnDamageZoneBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	const ET72DamageZone Zone = GetDamageZoneByComponent(OverlappedComponent);

	if (Zone == ET72DamageZone::None)
	{
		return;
	}

	if (!IsDroneActor(OtherActor))
	{
		return;
	}

	const bool bWarheadContact = IsDroneWarheadComponent(OtherComp);

	FVector HitLocation = OverlappedComponent->GetComponentLocation();
	const bool bValidDirectHit = IsValidDirectWarheadHit(OverlappedComponent, OtherActor, OtherComp, HitLocation);

	if (bValidDirectHit)
	{
		DebugLog(FString::Printf(TEXT("[T72 DAMAGE] Valid direct warhead hit: %s Location=%s"), *UEnum::GetValueAsString(Zone), *HitLocation.ToString()));
		ApplyZoneDamage(Zone, HitLocation);
	}
	else
	{
		DebugLog(FString::Printf(TEXT("[T72 DAMAGE] Vulnerable zone touched but rejected: %s OtherComp=%s"), *UEnum::GetValueAsString(Zone), OtherComp ? *OtherComp->GetName() : TEXT("NULL")));
	}

	if (bWarheadContact)
	{
		CrashDroneOnVulnerableZoneContact(OtherActor, OverlappedComponent, OtherComp);
	}
}
void UT72TankAIComponent::ApplyZoneDamage(ET72DamageZone Zone, FVector HitLocation)
{
	if (bTankDestroyed || CurrentState == ET72TankAIState::Destroyed)
	{
		return;
	}

	DebugLog(FString::Printf(TEXT("[T72 DAMAGE] Zone hit: %s Location=%s"), *UEnum::GetValueAsString(Zone), *HitLocation.ToString()));

	switch (Zone)
	{
	case ET72DamageZone::TurretHatch:
	case ET72DamageZone::HullHatch:
		PlayHatchKillExplosion(HitLocation);
		StartHatchKillFire();
		DestroyTank();
		break;

	case ET72DamageZone::GunBarrel:
		DisableGunAndRetreat();
		break;

	case ET72DamageZone::LeftTrack:
	case ET72DamageZone::RightTrack:
		StartTrackDamageSequence(Zone);
		break;

	case ET72DamageZone::Engine:
		StartEngineBurning();
		break;

	default:
		NotifyDroneImpactOrNearMiss(HitLocation, false);
		break;
	}
}

void UT72TankAIComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	MovementDebugTimer += DeltaTime;

	if (MovementDebugTimer >= 1.0f)
	{
		MovementDebugTimer = 0.0f;

		DebugLog(FString::Printf(
			TEXT("[T72 TICK] MissionStarted=%d State=%d RouteIndex=%d RouteNum=%d FireIndex=%d FireNum=%d MobilityDestroyed=%d"),
			bMissionStarted ? 1 : 0,
			static_cast<int32>(CurrentState),
			CurrentRouteIndex,
			RoutePoints.Num(),
			CurrentFirePositionIndex,
			FirePositions.Num(),
			bMobilityDestroyed ? 1 : 0
		));
	}

	if (!bMissionStarted)
	{
		return;
	}

	TickState(DeltaTime);
}

void UT72TankAIComponent::StartMission()
{
	bMissionStarted = true;
	CurrentRouteIndex = 0;
	CurrentFirePositionIndex = 0;
	FireTimer = 0.0f;
	bFirePatrolInitialized = false;
	bFirePatrolFirstShotDone = false;

	DebugLog(FString::Printf(
		TEXT("[T72 MISSION] StartMission | RoutePoints=%d FirePositions=%d Friendly=%s bAutoStart=%d MobilityDestroyed=%d CurrentState=%d"),
		RoutePoints.Num(),
		FirePositions.Num(),
		FriendlyPosition ? *FriendlyPosition->GetName() : TEXT("NULL"),
		bAutoStart ? 1 : 0,
		bMobilityDestroyed ? 1 : 0,
		static_cast<int32>(CurrentState)
	));

	if (RoutePoints.Num() > 0)
	{
		SetState(ET72TankAIState::MovingRoute);
	}
	else if (FirePositions.Num() > 0)
	{
		SetState(ET72TankAIState::MovingFirePosition);
	}
	else
	{
		SetState(ET72TankAIState::Aiming);
	}
}

void UT72TankAIComponent::StopMission()
{
	bMissionStarted = false;
	SetState(ET72TankAIState::Idle);
	SetWheelSpeed(0.0f);
	SetFiringActive(false);
}

void UT72TankAIComponent::SetState(ET72TankAIState NewState)
{
	if (CurrentState == NewState)
	{
		return;
	}

	CurrentState = NewState;
	FireTimer = 0.0f;

	if (CurrentState == ET72TankAIState::Idle || CurrentState == ET72TankAIState::Aiming || CurrentState == ET72TankAIState::Firing)
	{
		SetWheelSpeed(0.0f);
	}

	if (CurrentState != ET72TankAIState::Firing && CurrentState != ET72TankAIState::FirePositionPatrol)
	{
		SetFiringActive(false);
	}

	DebugLog(FString::Printf(TEXT("T72 State changed: %d"), static_cast<int32>(CurrentState)));
}

void UT72TankAIComponent::TickState(float DeltaTime)
{
	switch (CurrentState)
	{
	case ET72TankAIState::Idle:
		break;

	case ET72TankAIState::EngineCoast:
		TickEngineCoast(DeltaTime);
		break;

	case ET72TankAIState::MovingRoute:
		TickMovementToActor(DeltaTime, GetCurrentRouteTarget());
		break;

	case ET72TankAIState::MovingFirePosition:
		TickMovementToActor(DeltaTime, GetCurrentFirePosition());
		break;

	case ET72TankAIState::Aiming:
		if (bGunDestroyed)
		{
			SetFiringActive(false);
			break;
		}

		TickAiming(DeltaTime);
		if (IsAimedAtFriendly())
		{
			SetState(ET72TankAIState::Firing);
		}
		break;

	case ET72TankAIState::Firing:
		if (bGunDestroyed)
		{
			SetFiringActive(false);
			SetState(ET72TankAIState::Aiming);
			break;
		}

		TickAiming(DeltaTime);
		TickFiring(DeltaTime);
		break;

	case ET72TankAIState::FirePositionPatrol:
		TickFirePositionPatrol(DeltaTime);
		break;

	case ET72TankAIState::Relocating:
		TickMovementToActor(DeltaTime, GetCurrentFirePosition());
		break;

	case ET72TankAIState::Retreating:
		TickMovementToActor(DeltaTime, GetCurrentRouteTarget());
		break;

	case ET72TankAIState::TrackTurnReaction:
		TickTrackTurnReaction(DeltaTime);
		break;

	case ET72TankAIState::TrackFinalShot:
		TickTrackFinalShot(DeltaTime);
		break;

	case ET72TankAIState::TrackCrewEvacWait:
		if (!bGunDestroyed)
		{
			TickAiming(DeltaTime);
		}
		break;

	case ET72TankAIState::Immobilized:
		SetWheelSpeed(0.0f);
		break;

	case ET72TankAIState::Burning:
		if (!bGunDestroyed)
		{
			TickAiming(DeltaTime);
			TickFiring(DeltaTime);
		}
		break;

	case ET72TankAIState::Destroyed:
		SetWheelSpeed(0.0f);
		break;

	default:
		DebugLog(FString::Printf(TEXT("[T72 STATE] Unknown state: %d"), static_cast<int32>(CurrentState)));
		break;
	}
}

void UT72TankAIComponent::TickMovementToActor(float DeltaTime, AActor* TargetActor)
{
	AActor* Owner = GetOwner();
	if (bMobilityDestroyed)
	{
		SetWheelSpeed(0.0f);
		return;
	}

	if (!Owner)
	{
		DebugLog(TEXT("[T72 MOVE] Owner is NULL"));
		SetWheelSpeed(0.0f);
		return;
	}

	if (!TargetActor)
	{
		DebugLog(FString::Printf(
			TEXT("[T72 MOVE] Target is NULL | State=%d RouteIndex=%d RouteNum=%d FireIndex=%d FireNum=%d"),
			static_cast<int32>(CurrentState),
			CurrentRouteIndex,
			RoutePoints.Num(),
			CurrentFirePositionIndex,
			FirePositions.Num()
		));

		SetWheelSpeed(0.0f);
		return;
	}

	if (bMobilityDestroyed)
	{
		DebugLog(FString::Printf(
			TEXT("[T72 MOVE] Blocked by MobilityDestroyed | State=%d Target=%s"),
			static_cast<int32>(CurrentState),
			*TargetActor->GetName()
		));

		SetWheelSpeed(0.0f);
		return;
	}

	const FVector CurrentLocation = Owner->GetActorLocation();
	FVector ToTarget = TargetActor->GetActorLocation() - CurrentLocation;
	ToTarget.Z = 0.0f;

	const float Distance = ToTarget.Size();

	if (MovementDebugTimer < 0.05f)
	{
		DebugLog(FString::Printf(
			TEXT("[T72 MOVE] State=%d Target=%s OwnerLoc=%s TargetLoc=%s Distance=%.1f Acceptance=%.1f"),
			static_cast<int32>(CurrentState),
			*TargetActor->GetName(),
			*CurrentLocation.ToString(),
			*TargetActor->GetActorLocation().ToString(),
			Distance,
			AcceptanceRadiusCm
		));
	}

	if (Distance <= AcceptanceRadiusCm)
	{
		SetWheelSpeed(0.0f);
		OnArrivedAtTarget();
		return;
	}

	const FVector Direction = ToTarget.GetSafeNormal();
	const FRotator CurrentRotation = Owner->GetActorRotation();
	const FRotator TargetYawRotation = Direction.Rotation();

	const float YawError = FMath::Abs(FMath::FindDeltaAngleDegrees(CurrentRotation.Yaw, TargetYawRotation.Yaw));
	const float MoveAlpha = YawError > 65.0f ? 0.35f : 1.0f;

	const FRotator NewYawRotation = FMath::RInterpConstantTo(
		FRotator(0.0f, CurrentRotation.Yaw, 0.0f),
		FRotator(0.0f, TargetYawRotation.Yaw, 0.0f),
		DeltaTime,
		BodyTurnSpeedDeg
	);

	const FVector RawNewLocation = CurrentLocation + Owner->GetActorForwardVector() * MoveSpeedCm * MoveAlpha * DeltaTime;

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

	FinalRotation = FMath::RInterpConstantTo(CurrentRotation, FinalRotation, DeltaTime, GroundRotationInterpSpeedDeg);

	Owner->SetActorRotation(FinalRotation);
	const bool bMoved = Owner->SetActorLocation(GroundedLocation, false);

	if (!bMoved && MovementDebugTimer < 0.05f)
	{
		DebugLog(FString::Printf(
			TEXT("[T72 MOVE] SetActorLocation blocked | Old=%s New=%s"),
			*CurrentLocation.ToString(),
			*GroundedLocation.ToString()
		));
	}

	SetWheelSpeed(MoveSpeedCm * WheelAnimationSpeedScale * MoveAlpha);
}

void UT72TankAIComponent::TickFirePositionPatrol(float DeltaTime)
{
	if (bTankDestroyed || bGunDestroyed || bMobilityDestroyed || CurrentState == ET72TankAIState::Destroyed || CurrentState == ET72TankAIState::Immobilized || CurrentState == ET72TankAIState::Burning) 
	{
		SetWheelSpeed(0.0f);
		return;
	}
	AActor* Owner = GetOwner();

	if (!Owner)
	{
		SetWheelSpeed(0.0f);
		return;
	}

	if (!bFirePatrolInitialized)
	{
		BeginFirePositionPatrol();
		return;
	}

	TickAiming(DeltaTime);

	const bool bFiredNow = TickFiring(DeltaTime);

	if (bFiredNow)
	{
		bFirePatrolFirstShotDone = true;
	}

	if (bFireBeforePatrolMove && !bFirePatrolFirstShotDone)
	{
		SetWheelSpeed(0.0f);
		return;
	}

	const FVector CurrentLocation = Owner->GetActorLocation();
	FVector ToTarget = FirePatrolTarget - CurrentLocation;
	ToTarget.Z = 0.0f;

	const float Distance = ToTarget.Size();

	if (Distance <= AcceptanceRadiusCm)
	{
		FirePatrolDirection *= -1;
		FirePatrolTarget = FirePatrolCenter + FirePatrolAxis * FirePatrolDirection * (FirePatrolLengthCm * 0.5f);
		ToTarget = FirePatrolTarget - CurrentLocation;
		ToTarget.Z = 0.0f;
	}

	const FVector Direction = ToTarget.GetSafeNormal();

	if (Direction.IsNearlyZero())
	{
		SetWheelSpeed(0.0f);
		return;
	}

	const FRotator CurrentRotation = Owner->GetActorRotation();
	const FRotator TargetYawRotation = Direction.Rotation();

	const FRotator NewYawRotation = FMath::RInterpConstantTo(
		FRotator(0.0f, CurrentRotation.Yaw, 0.0f),
		FRotator(0.0f, TargetYawRotation.Yaw, 0.0f),
		DeltaTime,
		BodyTurnSpeedDeg
	);

	const float YawError = FMath::Abs(FMath::FindDeltaAngleDegrees(CurrentRotation.Yaw, TargetYawRotation.Yaw));
	const float MoveAlpha = YawError > 65.0f ? 0.25f : 1.0f;

	const FVector RawNewLocation = CurrentLocation + Owner->GetActorForwardVector() * FirePatrolMoveSpeedCm * MoveAlpha * DeltaTime;

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

	FinalRotation = FMath::RInterpConstantTo(CurrentRotation, FinalRotation, DeltaTime, GroundRotationInterpSpeedDeg);

	Owner->SetActorRotation(FinalRotation);
	Owner->SetActorLocation(GroundedLocation, false);
	SetWheelSpeed(FirePatrolMoveSpeedCm * WheelAnimationSpeedScale * MoveAlpha);
}

void UT72TankAIComponent::TickAiming(float DeltaTime)
{
	if (bGunDestroyed || bTankDestroyed || bCrewEvacuated)
	{
		return;
	}

	AActor* Owner = GetOwner();

	if (!Owner || !FriendlyPosition)
	{
		return;
	}

	const FVector ToTarget = FriendlyPosition->GetActorLocation() - Owner->GetActorLocation();
	const FRotator TargetRotation = ToTarget.Rotation();

	const float BodyYaw = Owner->GetActorRotation().Yaw;
	const float DesiredLocalTurretYaw = FMath::FindDeltaAngleDegrees(BodyYaw, TargetRotation.Yaw);

	CurrentTurretAngle = FMath::FInterpConstantTo(CurrentTurretAngle, DesiredLocalTurretYaw, DeltaTime, GetEffectiveTurretTurnSpeedDeg());
	SetTurretAngle(CurrentTurretAngle);

	const FVector FlatToTarget(ToTarget.X, ToTarget.Y, 0.0f);
	const float HorizontalDistance = FMath::Max(FlatToTarget.Size(), 1.0f);
	const float DesiredElevation = FMath::Clamp(FMath::RadiansToDegrees(FMath::Atan2(ToTarget.Z, HorizontalDistance)), MinGunElevationDeg, MaxGunElevationDeg);

	CurrentGunElevation = FMath::FInterpConstantTo(CurrentGunElevation, DesiredElevation, DeltaTime, GunElevationSpeedDeg);
	SetGunElevation(CurrentGunElevation);
}

float UT72TankAIComponent::GetEffectiveTurretTurnSpeedDeg() const
{
	return bEngineMobilityFailureApplied ? DamagedTurretTurnSpeedDeg : TurretTurnSpeedDeg;
}

bool UT72TankAIComponent::TickFiring(float DeltaTime)
{
	if (bGunDestroyed || bTankDestroyed || bCrewEvacuated || CurrentState == ET72TankAIState::Destroyed || CurrentState == ET72TankAIState::Immobilized || CurrentState == ET72TankAIState::EngineCoast)
	{
		SetFiringActive(false);
		return false;
	}

	if (!IsAimedAtFriendly())
	{
		if (CurrentState != ET72TankAIState::FirePositionPatrol)
		{
			SetState(ET72TankAIState::Aiming);
		}

		return false;
	}

	FireTimer += DeltaTime;

	if (FireTimer >= FireInterval)
	{
		FireTimer = 0.0f;
		FireWeapon();
		DebugLog(TEXT("T72 fired"));
		return true;
	}

	return false;
}

AActor* UT72TankAIComponent::GetCurrentRouteTarget() const
{
	if (!RoutePoints.IsValidIndex(CurrentRouteIndex))
	{
		return nullptr;
	}

	return RoutePoints[CurrentRouteIndex];
}

AActor* UT72TankAIComponent::GetCurrentFirePosition() const
{
	if (!FirePositions.IsValidIndex(CurrentFirePositionIndex))
	{
		return nullptr;
	}

	return FirePositions[CurrentFirePositionIndex];
}

void UT72TankAIComponent::OnArrivedAtTarget()
{
	if (CurrentState == ET72TankAIState::MovingRoute)
	{
		CurrentRouteIndex++;

		if (CurrentRouteIndex >= RoutePoints.Num())
		{
			CurrentRouteIndex = FMath::Max(RoutePoints.Num() - 1, 0);

			if (FirePositions.Num() > 0)
			{
				SetState(ET72TankAIState::MovingFirePosition);
			}
			else
			{
				SetState(ET72TankAIState::Aiming);
			}
		}

		return;
	}

	if (CurrentState == ET72TankAIState::MovingFirePosition || CurrentState == ET72TankAIState::Relocating)
	{
		if (bEnableFirePositionPatrol)
		{
			BeginFirePositionPatrol();
		}
		else
		{
			SetState(ET72TankAIState::Aiming);
		}
	}
}

void UT72TankAIComponent::BeginFirePositionPatrol()
{
	if (bTankDestroyed || bMobilityDestroyed || bGunDestroyed || CurrentState == ET72TankAIState::Destroyed || CurrentState == ET72TankAIState::Immobilized || CurrentState == ET72TankAIState::Burning || CurrentState == ET72TankAIState::TrackTurnReaction || CurrentState == ET72TankAIState::TrackFinalShot || CurrentState == ET72TankAIState::TrackCrewEvacWait)
	{
		SetWheelSpeed(0.0f);
		return;
	}

	AActor* Owner = GetOwner();

	

	if (!Owner)
	{
		return;
	}

	FirePatrolCenter = Owner->GetActorLocation();

	FVector ToFriendly = FriendlyPosition ? FriendlyPosition->GetActorLocation() - FirePatrolCenter : Owner->GetActorForwardVector();
	ToFriendly.Z = 0.0f;

	if (ToFriendly.IsNearlyZero())
	{
		FirePatrolAxis = Owner->GetActorRightVector();
	}
	else
	{
		FirePatrolAxis = FVector::CrossProduct(FVector::UpVector, ToFriendly.GetSafeNormal()).GetSafeNormal();
	}

	FirePatrolDirection = bStartPatrolToRight ? 1 : -1;
	FirePatrolTarget = FirePatrolCenter + FirePatrolAxis * FirePatrolDirection * (FirePatrolLengthCm * 0.5f);
	bFirePatrolInitialized = true;
	bFirePatrolFirstShotDone = false;
	

	SetState(ET72TankAIState::FirePositionPatrol);
	FireTimer = FireInterval;
}

void UT72TankAIComponent::SelectNextFirePosition()
{
	if (FirePositions.Num() <= 1)
	{
		return;
	}

	CurrentFirePositionIndex = (CurrentFirePositionIndex + 1) % FirePositions.Num();
}

bool UT72TankAIComponent::IsAimedAtFriendly() const
{
	const AActor* Owner = GetOwner();

	if (!Owner || !FriendlyPosition)
	{
		return false;
	}

	const FVector ToTarget = FriendlyPosition->GetActorLocation() - Owner->GetActorLocation();
	const float DesiredYaw = ToTarget.Rotation().Yaw;
	const float BodyYaw = Owner->GetActorRotation().Yaw;
	const float DesiredLocalYaw = FMath::FindDeltaAngleDegrees(BodyYaw, DesiredYaw);
	const float Error = FMath::Abs(FMath::FindDeltaAngleDegrees(CurrentTurretAngle, DesiredLocalYaw));

	return Error <= FireAngleToleranceDeg;
}

void UT72TankAIComponent::SetWheelSpeed(float Speed)
{
	CallBlueprintEventFloat(TEXT("SetSpeed"), Speed);
}

void UT72TankAIComponent::SetTurretAngle(float Angle)
{
	CallBlueprintEventFloat(TEXT("SetTurretRotation"), Angle);
}

void UT72TankAIComponent::SetGunElevation(float Angle)
{
	CallBlueprintEventFloat(TEXT("SetGunElevation"), Angle);
}

void UT72TankAIComponent::SetFiringActive(bool bActive)
{
	CallBlueprintEventBool(TEXT("SetFiring"), bActive);
}

void UT72TankAIComponent::PlayMuzzleFlash()
{
	CallBlueprintEventNoParams(TEXT("PlayMuzzleFlash"));
}

bool UT72TankAIComponent::CallBlueprintEventNoParams(FName EventName)
{
	AActor* Owner = GetOwner();

	if (!Owner)
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

bool UT72TankAIComponent::CallBlueprintEventFloat(FName EventName, float Value)
{
	AActor* Owner = GetOwner();

	if (!Owner)
	{
		return false;
	}

	UFunction* Function = Owner->FindFunction(EventName);

	if (!Function)
	{
		return false;
	}

	struct FFloatEventParams
	{
		float Value;
	};

	FFloatEventParams Params;
	Params.Value = Value;

	Owner->ProcessEvent(Function, &Params);
	return true;
}

bool UT72TankAIComponent::CallBlueprintEventBool(FName EventName, bool bValue)
{
	AActor* Owner = GetOwner();

	if (!Owner)
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

void UT72TankAIComponent::DebugLog(const FString& Message) const
{
	if (!bDebugLogs)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("%s"), *Message);
}

void UT72TankAIComponent::FireWeapon()
{
	AActor* Owner = GetOwner();

	if (bGunDestroyed || bTankDestroyed || bCrewEvacuated)
	{
		SetFiringActive(false);
		DebugLog(TEXT("[T72 FIRE] FireWeapon rejected: gun destroyed, tank destroyed, or crew evacuated"));
		return;
	}
	if (!Owner)
	{
		UE_LOG(LogTemp, Error, TEXT("[T72 FIRE] Owner is null"));
		return;
	}

	PlayMuzzleFlash();


	USceneComponent* MuzzlePoint = FindMuzzlePoint();

	const FVector SpawnLocation = MuzzlePoint ? MuzzlePoint->GetComponentLocation() : Owner->GetActorLocation() + Owner->GetActorForwardVector() * 500.0f;
	FRotator SpawnRotation = MuzzlePoint ? MuzzlePoint->GetComponentRotation() : Owner->GetActorRotation();


	if (bAimProjectileAtFriendlyPosition && FriendlyPosition)
	{
		const FVector AimDirection = (FriendlyPosition->GetActorLocation() - SpawnLocation).GetSafeNormal();

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

	AT72TankProjectile* Projectile = GetWorld()->SpawnActor<AT72TankProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);

	if (!Projectile)
	{
		UE_LOG(LogTemp, Error, TEXT("[T72 FIRE] SpawnActor failed"));
		return;
	}
}

USceneComponent* UT72TankAIComponent::FindMuzzlePoint() const
{
	AActor* Owner = GetOwner();

	if (!Owner)
	{
		UE_LOG(LogTemp, Error, TEXT("[T72 MUZZLE] Owner is null"));
		return nullptr;
	}

	TArray<USceneComponent*> Components;
	Owner->GetComponents<USceneComponent>(Components);

	UE_LOG(LogTemp, Warning, TEXT("[T72 MUZZLE] Searching component '%s' on %s. SceneComponents=%d"),
		*MuzzlePointComponentName.ToString(),
		*Owner->GetName(),
		Components.Num());

	for (USceneComponent* Component : Components)
	{
		if (!Component)
		{
			continue;
		}

		if (Component->GetFName() == MuzzlePointComponentName)
		{
			UE_LOG(LogTemp, Warning, TEXT("[T72 MUZZLE] Found muzzle: %s Location=%s Rotation=%s"),
				*Component->GetName(),
				*Component->GetComponentLocation().ToString(),
				*Component->GetComponentRotation().ToString());

			return Component;
		}
	}

	UE_LOG(LogTemp, Error, TEXT("[T72 MUZZLE] MuzzlePoint not found. Expected component name: %s"),
		*MuzzlePointComponentName.ToString());

	return nullptr;
}

void UT72TankAIComponent::NotifyDroneExplosion(FVector ImpactLocation, float ExplosionRadiusCm)
{
	if (bTankDestroyed || CurrentState == ET72TankAIState::Destroyed)
	{
		return;
	}

	const AActor* Owner = GetOwner();

	if (!Owner)
	{
		return;
	}

	const float Distance = FVector::Dist(Owner->GetActorLocation(), ImpactLocation);

	if (Distance <= ExplosionRadiusCm)
	{
		NotifyDroneImpactOrNearMiss(ImpactLocation, false);
		DebugLog(FString::Printf(TEXT("[T72 DAMAGE] Drone explosion near tank Distance=%.1f Radius=%.1f"), Distance, ExplosionRadiusCm));
	}
}

void UT72TankAIComponent::NotifyDroneImpactOrNearMiss(FVector ImpactLocation, bool bHitVulnerableZone)
{
	AActor* Owner = GetOwner();

	if (!Owner || bTankDestroyed || bMobilityDestroyed || CurrentState == ET72TankAIState::Destroyed || CurrentState == ET72TankAIState::Immobilized || CurrentState == ET72TankAIState::Burning || CurrentState == ET72TankAIState::TrackTurnReaction || CurrentState == ET72TankAIState::TrackFinalShot || CurrentState == ET72TankAIState::TrackCrewEvacWait)
	{
		return;
	}

	if (bHitVulnerableZone)
	{
		return;
	}

	const float Distance = FVector::Dist(Owner->GetActorLocation(), ImpactLocation);

	if (Distance > ThreatReactionRadiusCm)
	{
		return;
	}

	bThreatenedByDrone = true;

	if (bEnableFirePositionPatrol && CurrentState != ET72TankAIState::FirePositionPatrol)
	{
		BeginFirePositionPatrol();
	}

	DebugLog(TEXT("T72 threatened by FPV near miss"));
}

void UT72TankAIComponent::DestroyTank()
{
	if (bTankDestroyed)
	{
		return;
	}

	ClearDamageTimers();

	bTankDestroyed = true;
	bMobilityDestroyed = true;
	bGunDestroyed = true;

	SetWheelSpeed(0.0f);
	SetFiringActive(false);
	SetEngineFireActive(false);
	ApplyDestroyedMaterial();
	OpenHatches();
	SetState(ET72TankAIState::Destroyed);

	DebugLog(TEXT("[T72 DAMAGE] Tank destroyed"));
}

void UT72TankAIComponent::StartTrackDamageSequence(ET72DamageZone Zone)
{
	if (bTankDestroyed || bMobilityDestroyed)
	{
		return;
	}

	bTrackHitWhileMoving = IsMovementState(CurrentState);
	bMobilityDestroyed = true;
	bFirePatrolInitialized = false;
	bFirePatrolFirstShotDone = false;
	DamagedTrackZone = Zone;
	TrackTurnDirection = Zone == ET72DamageZone::LeftTrack ? -1 : 1;
	bTrackFinalShotFired = false;

	SetFiringActive(false);
	SetWheelSpeed(0.0f);
	FireTimer = 0.0f;

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(TrackTurnReactionTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(TrackCrewEvacTimerHandle);
		GetWorld()->GetTimerManager().SetTimer(TrackTurnReactionTimerHandle, this, &UT72TankAIComponent::FinishTrackTurnReaction, TrackTurnReactionDuration, false);
	}

	SetState(ET72TankAIState::TrackTurnReaction);

	DebugLog(FString::Printf(
		TEXT("[T72 DAMAGE] Track damaged: %s MovingAtHit=%d"),
		*UEnum::GetValueAsString(Zone),
		bTrackHitWhileMoving ? 1 : 0
	));
}
void UT72TankAIComponent::TickTrackTurnReaction(float DeltaTime)
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


	const float YawDelta = TrackTurnDirection * TrackTurnReactionSpeedDeg * DeltaTime;
	const FRotator CurrentRotation = Owner->GetActorRotation();
	const float NewYaw = CurrentRotation.Yaw + YawDelta;

	const FVector CurrentLocation = Owner->GetActorLocation();
	const FVector RawNewLocation = CurrentLocation + Owner->GetActorForwardVector() * MoveSpeedCm * TrackReactionForwardSpeedScale * DeltaTime;

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

	FinalRotation = FMath::RInterpConstantTo(CurrentRotation, FinalRotation, DeltaTime, GroundRotationInterpSpeedDeg);

	Owner->SetActorRotation(FinalRotation);
	Owner->SetActorLocation(GroundedLocation, false);
	SetWheelSpeed(MoveSpeedCm * TrackReactionForwardSpeedScale * WheelAnimationSpeedScale);
}

void UT72TankAIComponent::FinishTrackTurnReaction()
{
	if (bTankDestroyed || CurrentState == ET72TankAIState::Destroyed)
	{
		return;
	}

	SetWheelSpeed(0.0f);
	SetFiringActive(false);
	FireTimer = FireInterval;

	if (bGunDestroyed)
	{
		StartTrackCrewEvacTimer();
		DebugLog(TEXT("[T72 DAMAGE] Track turn reaction finished, gun destroyed, final shot skipped"));
		return;
	}

	SetState(ET72TankAIState::TrackFinalShot);

	DebugLog(TEXT("[T72 DAMAGE] Track turn reaction finished, final shot allowed"));
}

void UT72TankAIComponent::TickTrackFinalShot(float DeltaTime)
{
	SetWheelSpeed(0.0f);

	if (bTankDestroyed || bGunDestroyed || bCrewEvacuated)
	{
		StartTrackCrewEvacTimer();
		return;
	}

	TickAiming(DeltaTime);

	if (FriendlyPosition && !IsAimedAtFriendly())
	{
		return;
	}

	if (!bTrackFinalShotFired)
	{
		bTrackFinalShotFired = true;
		FireWeapon();
		DebugLog(TEXT("[T72 DAMAGE] Final shot after track damage"));
		StartTrackCrewEvacTimer();
	}
}

void UT72TankAIComponent::StartTrackCrewEvacTimer()
{
	if (CurrentState == ET72TankAIState::TrackCrewEvacWait)
	{
		return;
	}

	SetFiringActive(false);
	SetWheelSpeed(0.0f);
	SetState(ET72TankAIState::TrackCrewEvacWait);

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(TrackCrewEvacTimerHandle);
		GetWorld()->GetTimerManager().SetTimer(TrackCrewEvacTimerHandle, this, &UT72TankAIComponent::FinishTrackCrewEvacTimer, TrackCrewEvacDelay, false);
	}

	DebugLog(TEXT("[T72 DAMAGE] Track crew evacuation timer started"));
}

void UT72TankAIComponent::FinishTrackCrewEvacTimer()
{
	if (bTankDestroyed || CurrentState == ET72TankAIState::Destroyed)
	{
		return;
	}

	bCrewEvacuated = true;

	SetFiringActive(false);
	OpenHatches();
	TriggerCrewEvacuation();
	SetState(ET72TankAIState::Immobilized);

	DebugLog(TEXT("[T72 DAMAGE] Crew evacuated after track damage"));
}

void UT72TankAIComponent::ImmobilizeTank()
{
	if (bTankDestroyed || bMobilityDestroyed)
	{
		return;
	}

	bMobilityDestroyed = true;

	SetWheelSpeed(0.0f);
	SetFiringActive(false);
	OpenHatches();
	SetState(ET72TankAIState::Immobilized);

	DebugLog(TEXT("[T72 DAMAGE] Tank immobilized"));
}

void UT72TankAIComponent::StartEngineBurning()
{
	if (bTankDestroyed || bEngineDestroyed)
	{
		return;
	}

	bEngineDestroyed = true;
	bEngineMobilityFailureApplied = false;
	bEngineFinalShotFired = false;

	SetState(ET72TankAIState::EngineCoast);

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(EngineSecondaryExplosionTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(EngineMobilityFailureTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(EngineHalfBurnTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(EngineBurnoutTimerHandle);

		GetWorld()->GetTimerManager().SetTimer(EngineSecondaryExplosionTimerHandle, this, &UT72TankAIComponent::PlayEngineSecondaryExplosion, EngineSecondaryExplosionDelay, false);
		GetWorld()->GetTimerManager().SetTimer(EngineMobilityFailureTimerHandle, this, &UT72TankAIComponent::ApplyEngineMobilityFailure, EngineMobilityFailureDelay, false);
		GetWorld()->GetTimerManager().SetTimer(EngineBurnoutTimerHandle, this, &UT72TankAIComponent::FinishEngineBurnout, EngineBurnoutTime, false);
	}

	DebugLog(TEXT("[T72 DAMAGE] Engine hit, coast sequence started"));
}

void UT72TankAIComponent::TickEngineCoast(float DeltaTime)
{
	AActor* Owner = GetOwner();

	if (!Owner || bTankDestroyed)
	{
		SetWheelSpeed(0.0f);
		return;
	}

	if (!bGunDestroyed)
	{
		TickAiming(DeltaTime);
	}

	const FRotator CurrentRotation = Owner->GetActorRotation();
	const float CurrentYaw = CurrentRotation.Yaw;

	const FVector CurrentLocation = Owner->GetActorLocation();
	const FVector RawNewLocation = CurrentLocation + Owner->GetActorForwardVector() * MoveSpeedCm * EngineCoastSpeedScale * DeltaTime;

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

	FinalRotation = FMath::RInterpConstantTo(CurrentRotation, FinalRotation, DeltaTime, GroundRotationInterpSpeedDeg);

	Owner->SetActorRotation(FinalRotation);
	Owner->SetActorLocation(GroundedLocation, false);
	SetWheelSpeed(MoveSpeedCm * EngineCoastSpeedScale * WheelAnimationSpeedScale);
}

void UT72TankAIComponent::ApplyEngineMobilityFailure()
{
	if (bTankDestroyed || CurrentState == ET72TankAIState::Destroyed)
	{
		return;
	}

	bEngineMobilityFailureApplied = true;
	bMobilityDestroyed = true;
	bCrewEvacuated = true;

	SetWheelSpeed(0.0f);
	SetFiringActive(false);
	OpenHatches();
	StartHatchSmoke();
	SetState(ET72TankAIState::Burning);
	SpawnCrewFromTank();
	DebugLog(TEXT("[T72 DAMAGE] Engine mobility failed, turret speed reduced, crew evacuation started"));
}

void UT72TankAIComponent::FinishEngineBurnout()
{
	if (bTankDestroyed)
	{
		return;
	}

	FinalizeEngineBurnoutVisuals();

	DebugLog(TEXT("[T72 DAMAGE] Engine burnout finished"));
}

void UT72TankAIComponent::DisableGunAndRetreat()
{
	if (bTankDestroyed || bGunDestroyed)
	{
		return;
	}

	bGunDestroyed = true;

	SetFiringActive(false);
	FireTimer = 0.0f;

	if (!bMobilityDestroyed && RoutePoints.Num() > 0)
	{
		SetState(ET72TankAIState::Retreating);
	}
	else
	{
		SetState(ET72TankAIState::Immobilized);
	}

	DebugLog(TEXT("[T72 DAMAGE] Gun destroyed, retreat started"));
}
void UT72TankAIComponent::OpenHatches()
{
	CallBlueprintEventFloat(TEXT("SetHatchesAngle"), 85.0f);
	CallBlueprintEventFloat(TEXT("Set Hatch Command Angle"), 85.0f);
	CallBlueprintEventFloat(TEXT("Set Hatch Driver Angle"), 85.0f);
}

void UT72TankAIComponent::SetShowDamage(bool bActive)
{
	CallBlueprintEventBool(TEXT("SetShowDamage"), bActive);
}

void UT72TankAIComponent::SetEngineFireActive(bool bActive)
{
	CallBlueprintEventBool(TEXT("SetEngineFireActive"), bActive);
}

USceneComponent* UT72TankAIComponent::FindSceneComponentByName(FName ComponentName) const
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

FVector UT72TankAIComponent::GetFXLocation(FName PointName, FVector FallbackLocation) const
{
	USceneComponent* Point = FindSceneComponentByName(PointName);

	if (Point)
	{
		return Point->GetComponentLocation();
	}

	DebugLog(FString::Printf(TEXT("[T72 FX] Point not found: %s"), *PointName.ToString()));
	return FallbackLocation;
}

void UT72TankAIComponent::PlayEngineSecondaryExplosion()
{
	AActor* Owner = GetOwner();

	if (!Owner || bTankDestroyed)
	{
		return;
	}

	const FVector Location = GetFXLocation(EngineFXPointName, Owner->GetActorLocation());

	if (EngineSecondaryExplosionFX)
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			EngineSecondaryExplosionFX,
			Location,
			FRotator::ZeroRotator,
			FVector(EngineSecondaryExplosionScale),
			true
		);

		DebugLog(TEXT("[T72 FX] Engine secondary explosion spawned"));
	}
	else
	{
		DebugLog(TEXT("[T72 FX] EngineSecondaryExplosionFX is NULL"));
	}

	StartEngineFire();
}

void UT72TankAIComponent::StartEngineFire()
{
	AActor* Owner = GetOwner();

	if (!Owner || bTankDestroyed)
	{
		return;
	}

	USceneComponent* EnginePoint = FindSceneComponentByName(EngineFXPointName);

	if (!EnginePoint)
	{
		DebugLog(FString::Printf(TEXT("[T72 FX] Engine fire point not found: %s"), *EngineFXPointName.ToString()));
		return;
	}

	if (EngineFireFX)
	{
		EngineFirePSC = UGameplayStatics::SpawnEmitterAttached(
			EngineFireFX,
			EnginePoint,
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

		DebugLog(TEXT("[T72 FX] Engine fire attached"));
	}
	else
	{
		DebugLog(TEXT("[T72 FX] EngineFireFX is NULL"));
	}
}

void UT72TankAIComponent::TriggerEngineHalfBurnEffects()
{
	if (bTankDestroyed)
	{
		return;
	}

	bCrewEvacuated = true;

	SetFiringActive(false);
	OpenHatches();
	StartHatchSmoke();
	TriggerCrewEvacuation();

	DebugLog(TEXT("[T72 FX] Engine half-burn effects triggered, crew evacuation started"));
}

void UT72TankAIComponent::StartHatchSmoke()
{
	USceneComponent* CommandPoint = FindSceneComponentByName(CommandHatchSmokePointName);
	USceneComponent* DriverPoint = FindSceneComponentByName(DriverHatchSmokePointName);

	if (!HatchSmokeFX)
	{
		DebugLog(TEXT("[T72 FX] HatchSmokeFX is NULL"));
		return;
	}

	if (CommandPoint)
	{
		CommandHatchSmokePSC = UGameplayStatics::SpawnEmitterAttached(
			HatchSmokeFX,
			CommandPoint,
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			false
		);

		if (CommandHatchSmokePSC)
		{
			CommandHatchSmokePSC->SetWorldScale3D(FVector(HatchSmokeScale));
		}
	}
	else
	{
		DebugLog(FString::Printf(TEXT("[T72 FX] Command hatch smoke point not found: %s"), *CommandHatchSmokePointName.ToString()));
	}

	if (DriverPoint)
	{
		DriverHatchSmokePSC = UGameplayStatics::SpawnEmitterAttached(
			HatchSmokeFX,
			DriverPoint,
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			false
		);

		if (DriverHatchSmokePSC)
		{
			DriverHatchSmokePSC->SetWorldScale3D(FVector(HatchSmokeScale));
		}
	}
	else
	{
		DebugLog(FString::Printf(TEXT("[T72 FX] Driver hatch smoke point not found: %s"), *DriverHatchSmokePointName.ToString()));
	}

	DebugLog(TEXT("[T72 FX] Hatch smoke attached"));
}

void UT72TankAIComponent::PlayHatchKillExplosion(FVector HitLocation)
{
	AActor* Owner = GetOwner();

	if (!Owner)
	{
		return;
	}

	const FVector Location = GetFXLocation(TankExplosionPointName, HitLocation);

	if (HatchKillExplosionFX)
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			HatchKillExplosionFX,
			Location,
			FRotator::ZeroRotator,
			FVector(HatchKillExplosionScale),
			true
		);

		DebugLog(TEXT("[T72 FX] Hatch kill explosion spawned"));
	}
	else
	{
		DebugLog(TEXT("[T72 FX] HatchKillExplosionFX is NULL"));
	}
}

void UT72TankAIComponent::FinalizeEngineBurnoutVisuals()
{
	bTankDestroyed = true;
	bMobilityDestroyed = true;
	bGunDestroyed = true;

	SetWheelSpeed(0.0f);
	SetFiringActive(false);
	ApplyDestroyedMaterial();
	SetState(ET72TankAIState::Destroyed);

	DebugLog(TEXT("[T72 FX] Destroyed tank visual enabled after engine burnout"));
}

void UT72TankAIComponent::TriggerCrewEvacuation()
{
	SetTankNavObstacleActive(true);

	if (GetWorld() && CrewSpawnDelayAfterNavObstacleSeconds > 0.0f)
	{
		GetWorld()->GetTimerManager().ClearTimer(CrewSpawnDelayTimerHandle);
		GetWorld()->GetTimerManager().SetTimer(
			CrewSpawnDelayTimerHandle,
			this,
			&UT72TankAIComponent::SpawnCrewFromTankDelayed,
			CrewSpawnDelayAfterNavObstacleSeconds,
			false
		);
	}
	else
	{
		SpawnCrewFromTank();
	}

	DebugLog(TEXT("[T72 CREW] Crew evacuation requested"));
}

FVector UT72TankAIComponent::GetCrewSpawnLocation(int32 CrewIndex) const
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

	const float Offset = (CrewIndex - 2) * CrewSpawnFallbackSpacingCm;

	return BaseLocation - Forward * CrewSpawnFrontOffsetCm + Right * Offset + UpOffset;
}

void UT72TankAIComponent::SpawnCrewFromTank()
{
	if (bCrewSpawned)
	{
		DebugLog(TEXT("[T72 CREW] Crew already spawned"));
		return;
	}

	if (!GetWorld())
	{
		return;
	}

	if (!CrewInfantryClass)
	{
		DebugLog(TEXT("[T72 CREW] CrewInfantryClass is NULL"));
		return;
	}

	if (!CrewShelterPoint)
	{
		DebugLog(TEXT("[T72 CREW] CrewShelterPoint is NULL"));
		return;
	}

	bCrewSpawned = true;
	SpawnedCrew.Empty();

	AActor* Owner = GetOwner();
	const FRotator BaseRotation = Owner ? Owner->GetActorRotation() : FRotator::ZeroRotator;

	for (int32 i = 0; i < CrewCount; i++)
	{
		const FVector SpawnLocation = GetCrewSpawnLocation(i);
		const FRotator SpawnRotation(0.0f, BaseRotation.Yaw + FMath::RandRange(-35.0f, 35.0f), 0.0f);

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Owner;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		AInfantryCharacter* CrewMember = GetWorld()->SpawnActor<AInfantryCharacter>(
			CrewInfantryClass,
			SpawnLocation,
			SpawnRotation,
			SpawnParams
		);

		if (!CrewMember)
		{
			DebugLog(FString::Printf(TEXT("[T72 CREW] Failed to spawn crew member %d"), i));
			continue;
		}

		CrewMember->SpawnDefaultController();
		CrewMember->SetRunning(true);
		CrewMember->SetAIAnimState(TEXT("ReturnToPost"));

		SpawnedCrew.Add(CrewMember);

		AInfantryAIController* CrewController = Cast<AInfantryAIController>(CrewMember->GetController());

		if (CrewController)
		{
			CrewController->BeginMissionObjectiveMoveToLocation(GetCrewShelterLocation(i), true);
		}
		else
		{
			DebugLog(FString::Printf(TEXT("[T72 CREW] Crew member has no InfantryAIController | Pawn=%s"), *GetNameSafe(CrewMember)));
		}

		DebugLog(FString::Printf(TEXT("[T72 CREW] Spawned crew member %d | Pawn=%s Shelter=%s"),
			i,
			*GetNameSafe(CrewMember),
			*GetNameSafe(CrewShelterPoint)));
	}
}

USkeletalMeshComponent* UT72TankAIComponent::FindTankMesh() const
{
	AActor* Owner = GetOwner();

	if (!Owner)
	{
		return nullptr;
	}

	return Owner->FindComponentByClass<USkeletalMeshComponent>();
}

void UT72TankAIComponent::ApplyDestroyedMaterial()
{
	USkeletalMeshComponent* Mesh = FindTankMesh();

	if (!Mesh)
	{
		DebugLog(TEXT("[T72 VISUAL] SkeletalMeshComponent not found"));
		return;
	}

	if (!DestroyedTankMaterial)
	{
		DebugLog(TEXT("[T72 VISUAL] DestroyedTankMaterial is NULL"));
		return;
	}

	if (DestroyedMaterialElementIndices.Num() == 0)
	{
		Mesh->SetMaterial(0, DestroyedTankMaterial);
		DebugLog(TEXT("[T72 VISUAL] Destroyed material applied to element 0"));
		return;
	}

	for (int32 ElementIndex : DestroyedMaterialElementIndices)
	{
		if (ElementIndex < 0 || ElementIndex >= Mesh->GetNumMaterials())
		{
			DebugLog(FString::Printf(TEXT("[T72 VISUAL] Invalid material element index: %d"), ElementIndex));
			continue;
		}

		Mesh->SetMaterial(ElementIndex, DestroyedTankMaterial);
		DebugLog(FString::Printf(TEXT("[T72 VISUAL] Destroyed material applied to element %d"), ElementIndex));
	}
}

bool UT72TankAIComponent::IsMovementState(ET72TankAIState State) const
{
	return State == ET72TankAIState::MovingRoute
		|| State == ET72TankAIState::MovingFirePosition
		|| State == ET72TankAIState::FirePositionPatrol
		|| State == ET72TankAIState::Relocating
		|| State == ET72TankAIState::Retreating;
}

void UT72TankAIComponent::StartHatchKillFire()
{
	USceneComponent* CommandPoint = FindSceneComponentByName(CommandHatchSmokePointName);

	if (!CommandPoint)
	{
		DebugLog(FString::Printf(TEXT("[T72 FX] Hatch kill fire point not found: %s"), *CommandHatchSmokePointName.ToString()));
		return;
	}

	if (!HatchKillFireFX)
	{
		DebugLog(TEXT("[T72 FX] HatchKillFireFX is NULL"));
		return;
	}

	HatchKillFirePSC = UGameplayStatics::SpawnEmitterAttached(
		HatchKillFireFX,
		CommandPoint,
		NAME_None,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		EAttachLocation::SnapToTarget,
		false
	);

	if (HatchKillFirePSC)
	{
		HatchKillFirePSC->SetWorldScale3D(FVector(HatchKillFireScale));
	}

	DebugLog(TEXT("[T72 FX] Hatch kill fire attached"));
}

void UT72TankAIComponent::ClearDamageTimers()
{
	if (!GetWorld())
	{
		return;
	}

	GetWorld()->GetTimerManager().ClearTimer(EngineBurnoutTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(EngineMobilityFailureTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(EngineSecondaryExplosionTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(EngineHalfBurnTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(TrackTurnReactionTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(TrackCrewEvacTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(CrewSpawnDelayTimerHandle);
}


bool UT72TankAIComponent::TraceGroundPoint(const FVector& WorldPoint, FVector& OutHitLocation, FVector& OutHitNormal) const
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

	FHitResult Hit;

	const bool bHit = World->LineTraceSingleByChannel(
		Hit,
		Start,
		End,
		ECC_Visibility,
		Params
	);

	if (!bHit)
	{
		return false;
	}

	OutHitLocation = Hit.Location;
	OutHitNormal = Hit.ImpactNormal.GetSafeNormal();

	return true;
}

bool UT72TankAIComponent::ProjectLocationToGround(const FVector& DesiredLocation, float DesiredYaw, FVector& OutLocation, FVector& OutGroundNormal, float& OutSlopeDeg) const
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

	FVector FrontLeftPoint;
	FVector FrontRightPoint;
	FVector RearLeftPoint;
	FVector RearRightPoint;

	if (bUseManualGroundTracePoints)
	{
		FVector LocalFL;
		FVector LocalFR;
		FVector LocalRL;
		FVector LocalRR;

		if (!GetManualGroundTraceOffsets(LocalFL, LocalFR, LocalRL, LocalRR))
		{
			return false;
		}

		FrontLeftPoint = TransformGroundTraceOffset(DesiredLocation, DesiredYaw, LocalFL);
		FrontRightPoint = TransformGroundTraceOffset(DesiredLocation, DesiredYaw, LocalFR);
		RearLeftPoint = TransformGroundTraceOffset(DesiredLocation, DesiredYaw, LocalRL);
		RearRightPoint = TransformGroundTraceOffset(DesiredLocation, DesiredYaw, LocalRR);
	}
	else
	{
		FrontLeftPoint = DesiredLocation + Forward * GroundTraceHalfLengthCm - Right * GroundTraceHalfWidthCm;
		FrontRightPoint = DesiredLocation + Forward * GroundTraceHalfLengthCm + Right * GroundTraceHalfWidthCm;
		RearLeftPoint = DesiredLocation - Forward * GroundTraceHalfLengthCm - Right * GroundTraceHalfWidthCm;
		RearRightPoint = DesiredLocation - Forward * GroundTraceHalfLengthCm + Right * GroundTraceHalfWidthCm;
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

FRotator UT72TankAIComponent::MakeGroundAlignedRotation(float DesiredYaw, const FVector& GroundNormal) const
{
	const FVector DesiredForward = FRotator(0.0f, DesiredYaw, 0.0f).Vector();
	FVector ProjectedForward = FVector::VectorPlaneProject(DesiredForward, GroundNormal).GetSafeNormal();

	if (ProjectedForward.IsNearlyZero())
	{
		ProjectedForward = DesiredForward;
	}

	return FRotationMatrix::MakeFromXZ(ProjectedForward, GroundNormal).Rotator();
}

bool UT72TankAIComponent::IsSlopeDriveable(float SlopeDeg) const
{
	return SlopeDeg <= MaxDriveableSlopeDeg;
}

bool UT72TankAIComponent::GetManualGroundTraceOffsets(FVector& OutFL, FVector& OutFR, FVector& OutRL, FVector& OutRR) const
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
		DebugLog(TEXT("[T72 GROUND] Manual ground trace point missing"));
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

FVector UT72TankAIComponent::TransformGroundTraceOffset(const FVector& DesiredLocation, float DesiredYaw, const FVector& LocalOffset) const
{
	const FRotator YawRotation(0.0f, DesiredYaw, 0.0f);
	return DesiredLocation + YawRotation.RotateVector(LocalOffset);
}

FVector UT72TankAIComponent::GetCrewShelterLocation(int32 CrewIndex) const
{
	if (!CrewShelterPoint)
	{
		return FVector::ZeroVector;
	}

	const FVector BaseLocation = CrewShelterPoint->GetActorLocation();

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

UPrimitiveComponent* UT72TankAIComponent::FindTankNavObstacleComponent() const
{
	AActor* Owner = GetOwner();

	if (!Owner)
	{
		return nullptr;
	}

	if (USceneComponent* SceneComponent = FindSceneComponentByName(TankNavObstacleComponentName))
	{
		if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(SceneComponent))
		{
			return PrimitiveComponent;
		}
	}

	TArray<UPrimitiveComponent*> Components;
	Owner->GetComponents<UPrimitiveComponent>(Components);

	for (UPrimitiveComponent* Component : Components)
	{
		if (!Component)
		{
			continue;
		}

		if (Component->GetName().Contains(TEXT("TankNavObstacle")))
		{
			return Component;
		}
	}

	return nullptr;
}

void UT72TankAIComponent::SetTankNavObstacleActive(bool bActive)
{
	UPrimitiveComponent* NavObstacle = FindTankNavObstacleComponent();

	if (!NavObstacle)
	{
		DebugLog(FString::Printf(TEXT("[T72 CREW] TankNavObstacle not found | Expected=%s"), *TankNavObstacleComponentName.ToString()));
		return;
	}

	NavObstacle->SetCollisionEnabled(bActive ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	NavObstacle->SetCanEverAffectNavigation(bActive);
	NavObstacle->RecreatePhysicsState();

	DebugLog(FString::Printf(TEXT("[T72 CREW] TankNavObstacle active=%d | Component=%s"),
		bActive ? 1 : 0,
		*GetNameSafe(NavObstacle)));
}

void UT72TankAIComponent::SpawnCrewFromTankDelayed()
{
	SpawnCrewFromTank();
}

