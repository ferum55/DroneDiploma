#include "InfantryCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Components/SkeletalMeshComponent.h"

#include "GameFramework/ProjectileMovementComponent.h"
#include "TimerManager.h"


AInfantryCharacter::AInfantryCharacter()
{
    PrimaryActorTick.bCanEverTick = false;

    WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
    WeaponMesh->SetupAttachment(GetMesh());
    WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    bUseControllerRotationYaw = false;

    GetCharacterMovement()->bOrientRotationToMovement = false;
    GetCharacterMovement()->bUseControllerDesiredRotation = false;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);

    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
    GetCharacterMovement()->MaxAcceleration = 800.0f;
    GetCharacterMovement()->BrakingDecelerationWalking = 1000.0f;
}

void AInfantryCharacter::BeginPlay()
{
    Super::BeginPlay();
    UE_LOG(LogTemp, Warning, TEXT("[INF CHAR] BeginPlay | Actor=%s Controller=%s InitialMoveTarget=%s Location=%s"),
        *GetNameSafe(this),
        *GetNameSafe(GetController()),
        *GetNameSafe(InitialMoveTarget),
        *GetActorLocation().ToString());

    Health = MaxHealth;

    if (WeaponMesh)
    {
        WeaponMesh->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponSocketName);
    }

    SetRunning(false);
}

float AInfantryCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    if (bDead)
    {
        return 0.0f;
    }

    const float AppliedDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

    bool bApplyExplosionImpulse = false;
    FVector ExplosionOrigin = FVector::ZeroVector;

    if (DamageEvent.IsOfType(FRadialDamageEvent::ClassID))
    {
        const FRadialDamageEvent* RadialDamageEvent = static_cast<const FRadialDamageEvent*>(&DamageEvent);
        ExplosionOrigin = RadialDamageEvent->Origin;
        bApplyExplosionImpulse = true;
    }
    else if (DamageCauser)
    {
        ExplosionOrigin = DamageCauser->GetActorLocation();
        bApplyExplosionImpulse = true;
    }

    Health -= DamageAmount;

    if (Health <= 0.0f)
    {
        Health = 0.0f;
        Die(ExplosionOrigin, bApplyExplosionImpulse);
    }

    return AppliedDamage;
}

bool AInfantryCharacter::IsDead() const
{
    return bDead;
}

void AInfantryCharacter::SetRunning(bool bRunning)
{
    GetCharacterMovement()->MaxWalkSpeed = bRunning ? RunSpeed : WalkSpeed;
}

void AInfantryCharacter::FireAtActor(AActor* TargetActor)
{
    if (!TargetActor)
    {
        return;
    }

    FireAtActorWithSpread(TargetActor, AccuracySpreadDegrees);
}

void AInfantryCharacter::FireAtLocation(FVector TargetLocation)
{
    FireAtLocationWithSpread(TargetLocation, AccuracySpreadDegrees);
}
AActor* AInfantryCharacter::GetInitialMoveTarget() const
{
    return InitialMoveTarget;
}

void AInfantryCharacter::FireAtActorWithSpread(AActor* TargetActor, float SpreadDegrees)
{
    FireAtActorWithSpreadInternal(TargetActor, SpreadDegrees, false);
}

void AInfantryCharacter::FireAtLocationWithSpread(FVector TargetLocation, float SpreadDegrees)
{
    FireAtLocationWithSpreadInternal(TargetLocation, SpreadDegrees, false);
}

void AInfantryCharacter::FireAtActorWithSpreadInternal(AActor* TargetActor, float SpreadDegrees, bool bIgnoreCooldown)
{
    if (!TargetActor)
    {
        return;
    }

    FireAtLocationWithSpreadInternal(TargetActor->GetActorLocation(), SpreadDegrees, bIgnoreCooldown);
}

void AInfantryCharacter::FireAtLocationWithSpreadInternal(FVector TargetLocation, float SpreadDegrees, bool bIgnoreCooldown)
{
    if (bDead || !ProjectileClass)
    {
        return;
    }

    UWorld* World = GetWorld();

    if (!World)
    {
        return;
    }

    const float CurrentTime = World->GetTimeSeconds();

    if (!bIgnoreCooldown && CurrentTime - LastFireTime < FireCooldown)
    {
        return;
    }

    LastFireTime = CurrentTime;

    const FVector MuzzleLocation = GetMuzzleLocation();
    FVector AimDirection = TargetLocation - MuzzleLocation;

    if (AimDirection.SizeSquared() < 1.0f)
    {
        return;
    }

    AimDirection.Normalize();

    const float SpreadRadians = FMath::DegreesToRadians(FMath::Max(0.0f, SpreadDegrees));
    AimDirection = FMath::VRandCone(AimDirection, SpreadRadians);

    const FRotator SpawnRotation = AimDirection.Rotation();

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.Instigator = this;

    AActor* Projectile = World->SpawnActor<AActor>(
        ProjectileClass,
        MuzzleLocation,
        SpawnRotation,
        SpawnParams
    );

    if (!Projectile)
    {
        UE_LOG(LogTemp, Warning, TEXT("[INF BULLET FAIL] SpawnActor failed | Pawn=%s ProjectileClass=%s"),
            *GetNameSafe(this),
            *GetNameSafe(ProjectileClass));
        return;
    }

    UProjectileMovementComponent* ProjectileMovement = Projectile->FindComponentByClass<UProjectileMovementComponent>();

    if (ProjectileMovement)
    {
        ProjectileMovement->Velocity = AimDirection * BulletSpeedCmPerSecond;
        ProjectileMovement->InitialSpeed = BulletSpeedCmPerSecond;
        ProjectileMovement->MaxSpeed = BulletSpeedCmPerSecond;
    }

    UE_LOG(LogTemp, Warning, TEXT("[INF BULLET SPAWNED] Pawn=%s Projectile=%s Location=%s Direction=%s Spread=%.1f"),
        *GetNameSafe(this),
        *GetNameSafe(Projectile),
        *MuzzleLocation.ToString(),
        *AimDirection.ToString(),
        SpreadDegrees);
}

void AInfantryCharacter::Die(FVector ExplosionOrigin, bool bApplyExplosionImpulse)
{
    if (bDead)
    {
        return;
    }

    bDead = true;

    GetCharacterMovement()->DisableMovement();
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    USkeletalMeshComponent* CharacterMesh = GetMesh();

    if (CharacterMesh)
    {
        CharacterMesh->SetCollisionProfileName(TEXT("Ragdoll"));
        CharacterMesh->SetAllBodiesSimulatePhysics(true);
        CharacterMesh->SetSimulatePhysics(true);
        CharacterMesh->WakeAllRigidBodies();
        CharacterMesh->bBlendPhysics = true;

        if (bApplyExplosionImpulse)
        {
            CharacterMesh->AddRadialImpulse(
                ExplosionOrigin,
                RagdollImpulseRadius,
                RagdollImpulseStrength,
                ERadialImpulseFalloff::RIF_Linear,
                true
            );
        }
    }

    DetachFromControllerPendingDestroy();
    SetLifeSpan(20.0f);
}

FVector AInfantryCharacter::GetMuzzleLocation() const
{
    if (WeaponMesh && WeaponMesh->DoesSocketExist(MuzzleSocketName))
    {
        return WeaponMesh->GetSocketLocation(MuzzleSocketName);
    }

    if (WeaponMesh)
    {
        return WeaponMesh->GetComponentLocation() + GetActorForwardVector() * 60.0f;
    }

    return GetActorLocation() + GetActorForwardVector() * 60.0f + FVector(0.0f, 0.0f, 50.0f);
}

FRotator AInfantryCharacter::GetAimRotation(FVector TargetLocation) const
{
    return (TargetLocation - GetMuzzleLocation()).Rotation();
}

bool AInfantryCharacter::StartFireBurst(AActor* TargetActor)
{
    return StartFireBurstWithSpread(TargetActor, AccuracySpreadDegrees);
}

bool AInfantryCharacter::StartFireBurstWithSpread(AActor* TargetActor, float SpreadDegrees)
{
    if (bDead)
    {
        return false;
    }

    if (!TargetActor)
    {
        return false;
    }

    if (!ProjectileClass)
    {
        return false;
    }

    UWorld* World = GetWorld();

    if (!World)
    {
        return false;
    }

    const float CurrentTime = World->GetTimeSeconds();

    if (BurstShotsRemaining > 0)
    {
        return false;
    }

    if (CurrentTime - LastBurstStartTime < BurstCooldown)
    {
        return false;
    }

    LastBurstStartTime = CurrentTime;
    BurstTargetActor = TargetActor;
    BurstShotsRemaining = BurstShotCount;
    ActiveBurstSpreadDegrees = SpreadDegrees;

    UE_LOG(LogTemp, Warning, TEXT("[INF FIRE START] Pawn=%s Target=%s Shots=%d Interval=%.2f Spread=%.1f Projectile=%s"),
        *GetNameSafe(this),
        *GetNameSafe(TargetActor),
        BurstShotCount,
        GetSecondsBetweenShots(),
        ActiveBurstSpreadDegrees,
        *GetNameSafe(ProjectileClass));

    FireBurstShot();

    if (BurstShotsRemaining > 0)
    {
        World->GetTimerManager().SetTimer(
            BurstFireTimerHandle,
            this,
            &AInfantryCharacter::FireBurstShot,
            GetSecondsBetweenShots(),
            true
        );
    }

    return true;
}

void AInfantryCharacter::FireBurstShot()
{
    if (bDead || BurstShotsRemaining <= 0)
    {
        if (GetWorld())
        {
            GetWorld()->GetTimerManager().ClearTimer(BurstFireTimerHandle);
        }

        BurstShotsRemaining = 0;
        return;
    }

    AActor* TargetActor = BurstTargetActor.Get();

    if (!TargetActor)
    {
        if (GetWorld())
        {
            GetWorld()->GetTimerManager().ClearTimer(BurstFireTimerHandle);
        }

        BurstShotsRemaining = 0;
        return;
    }

    FireAtActorWithSpreadInternal(TargetActor, ActiveBurstSpreadDegrees, true);

    UE_LOG(LogTemp, Warning, TEXT("[INF BULLET] Shot | Pawn=%s Target=%s Remaining=%d"),
        *GetNameSafe(this),
        *GetNameSafe(TargetActor),
        BurstShotsRemaining - 1);

    BurstShotsRemaining--;

    if (BurstShotsRemaining <= 0)
    {
        if (GetWorld())
        {
            GetWorld()->GetTimerManager().ClearTimer(BurstFireTimerHandle);
        }
    }
}

float AInfantryCharacter::GetSecondsBetweenShots() const
{
    if (FireRateRoundsPerMinute <= 0.0f)
    {
        return 0.1f;
    }

    return 60.0f / FireRateRoundsPerMinute;
}