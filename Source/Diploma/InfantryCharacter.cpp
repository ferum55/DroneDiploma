#include "InfantryCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Components/SkeletalMeshComponent.h"


AInfantryCharacter::AInfantryCharacter()
{
    PrimaryActorTick.bCanEverTick = false;

    WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
    WeaponMesh->SetupAttachment(GetMesh());
    WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    bUseControllerRotationYaw = false;

    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 360.0f, 0.0f);
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

    FireAtLocation(TargetActor->GetActorLocation() + FVector(0.0f, 0.0f, 60.0f));
}

void AInfantryCharacter::FireAtLocation(FVector TargetLocation)
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

    if (CurrentTime - LastFireTime < FireCooldown)
    {
        return;
    }

    LastFireTime = CurrentTime;

    const FVector MuzzleLocation = GetMuzzleLocation();
    FVector ShootDirection = GetAimRotation(TargetLocation).Vector();
    ShootDirection = FMath::VRandCone(ShootDirection, FMath::DegreesToRadians(AccuracySpreadDegrees));

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.Instigator = this;

    World->SpawnActor<AActor>(ProjectileClass, MuzzleLocation, ShootDirection.Rotation(), SpawnParams);
}

AActor* AInfantryCharacter::GetInitialMoveTarget() const
{
    return InitialMoveTarget;
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