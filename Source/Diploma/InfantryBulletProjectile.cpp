#include "InfantryBulletProjectile.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

AInfantryBulletProjectile::AInfantryBulletProjectile()
{
    PrimaryActorTick.bCanEverTick = true;

    CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
    RootComponent = CollisionComponent;

    CollisionComponent->InitSphereRadius(2.0f);
    CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    CollisionComponent->SetCollisionObjectType(ECC_WorldDynamic);
    CollisionComponent->SetCollisionResponseToAllChannels(ECR_Block);
    CollisionComponent->SetNotifyRigidBodyCollision(true);

    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    ProjectileMovement->InitialSpeed = 90000.0f;
    ProjectileMovement->MaxSpeed = 90000.0f;
    ProjectileMovement->bRotationFollowsVelocity = true;
    ProjectileMovement->bShouldBounce = false;
    ProjectileMovement->ProjectileGravityScale = 0.0f;
    ProjectileMovement->bForceSubStepping = true;
    ProjectileMovement->MaxSimulationTimeStep = 0.005f;
    ProjectileMovement->MaxSimulationIterations = 8;

    InitialLifeSpan = 3.0f;
}

void AInfantryBulletProjectile::BeginPlay()
{
    Super::BeginPlay();

    PreviousLocation = GetActorLocation();
    InitialLifeSpan = LifeSeconds;

    if (CollisionComponent)
    {
        CollisionComponent->OnComponentHit.AddDynamic(this, &AInfantryBulletProjectile::OnBulletHit);
    }
}

void AInfantryBulletProjectile::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!bDrawDebugTrail)
    {
        PreviousLocation = GetActorLocation();
        return;
    }

    UWorld* World = GetWorld();

    if (!World)
    {
        return;
    }

    const FVector CurrentLocation = GetActorLocation();

    DrawDebugLine(
        World,
        PreviousLocation,
        CurrentLocation,
        FColor::Red,
        false,
        DebugTrailLifeSeconds,
        0,
        DebugTrailThickness
    );

    PreviousLocation = CurrentLocation;
}

void AInfantryBulletProjectile::OnBulletHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    if (OtherActor && OtherActor != this && OtherActor != GetOwner())
    {
        UGameplayStatics::ApplyPointDamage(
            OtherActor,
            Damage,
            GetVelocity().GetSafeNormal(),
            Hit,
            GetInstigatorController(),
            this,
            nullptr
        );
    }
    if (bDrawDebugTrail && GetWorld())
    {
        DrawDebugSphere(
            GetWorld(),
            Hit.ImpactPoint,
            DebugImpactSphereRadius,
            8,
            FColor::Yellow,
            false,
            1.0f,
            0,
            1.5f
        );
    }

    Destroy();
}