#include "APCAIComponent.h"

#include "FPVDronePawn.h"
#include "InfantryCharacter.h"
#include "InfantryAIController.h"
#include "Animation/AnimInstance.h"
#include "Components/LightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UnrealType.h"


UAPCAIComponent::UAPCAIComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DefaultDestroyedMaterial(
		TEXT("MaterialInstanceConstant'/Game/VigilanteContent/Vehicles/East_APC_MTLB/Damaged/Materials/MI_East_APC_MTLB_Damaged.MI_East_APC_MTLB_Damaged'")
	);

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> DefaultDestroyedFX(
		TEXT("NiagaraSystem'/Game/VigilanteContent/Vehicles/East_APC_MTLB/FX/NS_East_APC_MTLB_Damaged.NS_East_APC_MTLB_Damaged'")
	);

	if (DefaultDestroyedMaterial.Succeeded())
	{
		DestroyedMaterial = DefaultDestroyedMaterial.Object;
	}

	if (DefaultDestroyedFX.Succeeded())
	{
		DestroyedFX = DefaultDestroyedFX.Object;
	}
}

void UAPCAIComponent::BeginPlay()
{
	Super::BeginPlay();

	ApplyDefaultAssetReferences();
	CacheComponents();
	if (!BoardingMovePoint)
	{
		BoardingMovePoint = ResolveScenarioPoint(nullptr, BoardingMovePointName);
	}
	CacheTrackSmokeComponents();
	BindOverlapEvents();
	DisableLights();
	SetNavObstacleActive(false);

	CurrentRearDoorsAngle = 0.0f;
	CurrentHatchAngle = 0.0f;
	TargetRearDoorsAngle = 0.0f;
	TargetHatchAngle = 0.0f;

	SetWheelSpeed(0.0f);
	SetTurretNeutral();
	SnapToGround();
	UpdateAnimInstanceVariables();
	UpdateTrackSmokeVFX();

	DebugLog(TEXT("[APC] BeginPlay"));

	if (bAutoStartEvacuation && GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(
			AutoStartTimerHandle,
			this,
			&UAPCAIComponent::AutoStartEvacuation,
			AutoStartDelaySeconds,
			false
		);
	}
}

void UAPCAIComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bDestroyed)
	{
		TickDoorAnimation(DeltaTime);
		UpdateGroundAlignment(DeltaTime);
		UpdateAnimInstanceVariables();
		UpdateTrackSmokeVFX();
		return;
	}

	TickState(DeltaTime);
	TickDoorAnimation(DeltaTime);
	UpdateGroundAlignment(DeltaTime);
	UpdateAnimInstanceVariables();
	UpdateTrackSmokeVFX();
}

void UAPCAIComponent::ApplyDefaultAssetReferences()
{
	if (!DestroyedMaterial)
	{
		UMaterialInterface* LoadedMaterial = LoadObject<UMaterialInterface>(
			nullptr,
			TEXT("MaterialInstanceConstant'/Game/VigilanteContent/Vehicles/East_APC_MTLB/Damaged/Materials/MI_East_APC_MTLB_Damaged.MI_East_APC_MTLB_Damaged'")
		);

		if (LoadedMaterial)
		{
			DestroyedMaterial = LoadedMaterial;
		}
	}

	if (!DestroyedFX)
	{
		UNiagaraSystem* LoadedFX = LoadObject<UNiagaraSystem>(
			nullptr,
			TEXT("NiagaraSystem'/Game/VigilanteContent/Vehicles/East_APC_MTLB/FX/NS_East_APC_MTLB_Damaged.NS_East_APC_MTLB_Damaged'")
		);

		if (LoadedFX)
		{
			DestroyedFX = LoadedFX;
		}
	}
}

void UAPCAIComponent::StartEvacuation(AActor* InEvacPoint, AActor* InReturnPoint)
{
	if (bDestroyed)
	{
		return;
	}

	EvacPoint = InEvacPoint;
	ReturnPoint = InReturnPoint;
	LoadedCrewCount = 0;
	PendingCrewMember = nullptr;
	bAssignedCrewOrderedToBoard = false;

	if (!EvacPoint)
	{
		DebugLog(TEXT("[APC] StartEvacuation failed: EvacPoint is null"));
		return;
	}

	SetRearDoorsAngle(0.0f);
	SetHatchAngle(0.0f);
	SetState(EAPCAIState::MoveToEvacPoint);

	DebugLog(FString::Printf(TEXT("[APC] StartEvacuation | EvacPoint=%s ReturnPoint=%s"),
		*GetNameSafe(EvacPoint),
		*GetNameSafe(ReturnPoint)));
}

void UAPCAIComponent::DestroyAPC(const FVector& HitLocation)
{
	if (bDestroyed)
	{
		return;
	}

	bDestroyed = true;

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(AutoStartTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(BoardingTimerHandle);
	}

	SetWheelSpeed(0.0f);
	SetRearDoorsAngle(OpenRearDoorsAngle);
	SetHatchAngle(OpenHatchAngle);
	SetTurretNeutral();
	SetState(EAPCAIState::Destroyed);
	ApplyDestroyedVisuals(HitLocation);
	UpdateTrackSmokeVFX();

	DebugLog(FString::Printf(TEXT("[APC] Destroyed | Location=%s LoadedCrew=%d"),
		*HitLocation.ToString(),
		LoadedCrewCount));
}

bool UAPCAIComponent::IsDestroyed() const
{
	return bDestroyed;
}

int32 UAPCAIComponent::GetLoadedCrewCount() const
{
	return LoadedCrewCount;
}

void UAPCAIComponent::CacheComponents()
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

	HitZoneComponent = FindPrimitiveComponentByName(HitZoneComponentName);
	BoardingZoneComponent = FindPrimitiveComponentByName(BoardingZoneComponentName);
	NavObstacleComponent = FindPrimitiveComponentByName(NavObstacleComponentName);

	GroundTraceFL = FindSceneComponentByName(GroundTraceFLName);
	GroundTraceFR = FindSceneComponentByName(GroundTraceFRName);
	GroundTraceRL = FindSceneComponentByName(GroundTraceRLName);
	GroundTraceRR = FindSceneComponentByName(GroundTraceRRName);

	DestroyedFXPoint = FindSceneComponentByName(DestroyedFXPointName);

	DebugLog(FString::Printf(TEXT("[APC] Components | Mesh=%d Anim=%d HitZone=%s BoardingZone=%s NavObstacle=%s Ground=%d%d%d%d"),
		MeshComponent ? 1 : 0,
		AnimInstance ? 1 : 0,
		*GetNameSafe(HitZoneComponent),
		*GetNameSafe(BoardingZoneComponent),
		*GetNameSafe(NavObstacleComponent),
		GroundTraceFL ? 1 : 0,
		GroundTraceFR ? 1 : 0,
		GroundTraceRL ? 1 : 0,
		GroundTraceRR ? 1 : 0));
}

void UAPCAIComponent::CacheTrackSmokeComponents()
{
	AActor* Owner = GetOwner();

	if (!Owner)
	{
		return;
	}

	TrackSmokeParticleComponents.Empty();
	TrackSmokeNiagaraComponents.Empty();

	TArray<UParticleSystemComponent*> ParticleComponents;
	Owner->GetComponents<UParticleSystemComponent>(ParticleComponents);

	for (UParticleSystemComponent* ParticleComponent : ParticleComponents)
	{
		if (!ParticleComponent)
		{
			continue;
		}

		const FString ComponentName = ParticleComponent->GetName();

		if (IsTrackSmokeComponentName(ComponentName))
		{
			TrackSmokeParticleComponents.Add(ParticleComponent);
			ParticleComponent->SetAutoActivate(false);
			ParticleComponent->SetVisibility(false, true);
			ParticleComponent->DeactivateSystem();
		}
	}

	TArray<UNiagaraComponent*> NiagaraComponents;
	Owner->GetComponents<UNiagaraComponent>(NiagaraComponents);

	for (UNiagaraComponent* NiagaraComponent : NiagaraComponents)
	{
		if (!NiagaraComponent)
		{
			continue;
		}

		const FString ComponentName = NiagaraComponent->GetName();

		if (IsTrackSmokeComponentName(ComponentName))
		{
			TrackSmokeNiagaraComponents.Add(NiagaraComponent);
			NiagaraComponent->SetAutoActivate(false);
			NiagaraComponent->SetVisibility(false, true);
			NiagaraComponent->Deactivate();
		}
	}

	DebugLog(FString::Printf(TEXT("[APC VFX] Track smoke components | Cascade=%d Niagara=%d Prefix=%s"),
		TrackSmokeParticleComponents.Num(),
		TrackSmokeNiagaraComponents.Num(),
		*TrackSmokeComponentPrefix));
}

void UAPCAIComponent::BindOverlapEvents()
{
	if (HitZoneComponent)
	{
		HitZoneComponent->SetGenerateOverlapEvents(true);
		HitZoneComponent->OnComponentBeginOverlap.RemoveDynamic(this, &UAPCAIComponent::OnHitZoneBeginOverlap);
		HitZoneComponent->OnComponentBeginOverlap.AddDynamic(this, &UAPCAIComponent::OnHitZoneBeginOverlap);
	}

	if (BoardingZoneComponent)
	{
		BoardingZoneComponent->SetGenerateOverlapEvents(true);
		BoardingZoneComponent->OnComponentBeginOverlap.RemoveDynamic(this, &UAPCAIComponent::OnBoardingZoneBeginOverlap);
		BoardingZoneComponent->OnComponentBeginOverlap.AddDynamic(this, &UAPCAIComponent::OnBoardingZoneBeginOverlap);
	}
}

void UAPCAIComponent::DisableLights()
{
	AActor* Owner = GetOwner();

	if (!Owner)
	{
		return;
	}

	TArray<ULightComponent*> Lights;
	Owner->GetComponents<ULightComponent>(Lights);

	for (ULightComponent* Light : Lights)
	{
		if (!Light)
		{
			continue;
		}

		Light->SetVisibility(false, true);
		Light->SetIntensity(0.0f);
	}

	CallBlueprintEventFloat(SetLightsEmissivityEventName, 0.0f);

	DebugLog(FString::Printf(TEXT("[APC] Lights disabled | Count=%d"), Lights.Num()));
}

void UAPCAIComponent::SnapToGround()
{
	AActor* Owner = GetOwner();

	if (!Owner)
	{
		return;
	}

	FVector GroundedLocation = Owner->GetActorLocation();
	FRotator GroundedRotation = Owner->GetActorRotation();

	if (BuildGroundAlignedPose(Owner->GetActorLocation(), Owner->GetActorRotation().Yaw, 1.0f, GroundedLocation, GroundedRotation))
	{
		Owner->SetActorLocationAndRotation(GroundedLocation, GroundedRotation, false, nullptr, ETeleportType::TeleportPhysics);
	}
}

void UAPCAIComponent::SetState(EAPCAIState NewState)
{
	if (CurrentState == NewState)
	{
		return;
	}

	CurrentState = NewState;

	const bool bShouldEnableNavObstacle =
		CurrentState == EAPCAIState::OpeningRearDoors ||
		CurrentState == EAPCAIState::WaitingForCrew ||
		CurrentState == EAPCAIState::ClosingRearDoors ||
		CurrentState == EAPCAIState::Destroyed;

	SetNavObstacleActive(bShouldEnableNavObstacle);

	DebugLog(FString::Printf(TEXT("[APC] State changed: %d"), static_cast<int32>(CurrentState)));
	if (CurrentState == EAPCAIState::WaitingForCrew)
	{
		OrderAssignedCrewToBoard();
	}
}

void UAPCAIComponent::TickState(float DeltaTime)
{
	switch (CurrentState)
	{
	case EAPCAIState::Inactive:
		SetWheelSpeed(0.0f);
		break;

	case EAPCAIState::MoveToEvacPoint:
		TickMoveToTarget(DeltaTime, EvacPoint, EAPCAIState::OpeningRearDoors);
		break;

	case EAPCAIState::OpeningRearDoors:
		SetWheelSpeed(0.0f);
		SetRearDoorsAngle(OpenRearDoorsAngle);

		if (FMath::Abs(CurrentRearDoorsAngle - OpenRearDoorsAngle) <= 2.0f)
		{
			SetState(EAPCAIState::WaitingForCrew);
		}
		break;

	case EAPCAIState::WaitingForCrew:
		SetWheelSpeed(0.0f);
		SetRearDoorsAngle(OpenRearDoorsAngle);
		CompactAssignedCrew();
		OrderAssignedCrewToBoard();
		TryBoardOverlappingCrew();

		if (ShouldFinishBoarding())
		{
			SetState(EAPCAIState::ClosingRearDoors);
		}
		break;


	case EAPCAIState::ClosingRearDoors:
		SetWheelSpeed(0.0f);
		SetRearDoorsAngle(0.0f);

		if (FMath::Abs(CurrentRearDoorsAngle) <= 2.0f)
		{
			if (ReturnPoint)
			{
				SetState(EAPCAIState::Returning);
			}
			else
			{
				SetState(EAPCAIState::Inactive);
			}
		}
		break;

	case EAPCAIState::Returning:
		TickMoveToTarget(DeltaTime, ReturnPoint, EAPCAIState::Inactive);
		break;

	case EAPCAIState::Destroyed:
		SetWheelSpeed(0.0f);
		SetRearDoorsAngle(OpenRearDoorsAngle);
		SetHatchAngle(OpenHatchAngle);
		break;

	default:
		break;
	}
}

void UAPCAIComponent::TickMoveToTarget(float DeltaTime, AActor* TargetActor, EAPCAIState ArrivedState)
{
	AActor* Owner = GetOwner();

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

		if (bMovementDebugLogs)
		{
			DebugLog(FString::Printf(
				TEXT("[APC MOVE ARRIVED] State=%d Target=%s Loc=%s TargetLoc=%s Dist=%.1f Acceptance=%.1f"),
				static_cast<int32>(CurrentState),
				*GetNameSafe(TargetActor),
				*CurrentLocation.ToString(),
				*TargetActor->GetActorLocation().ToString(),
				Distance,
				AcceptanceRadiusCm
			));
		}

		SetState(ArrivedState);
		return;
	}

	const FVector Direction = ToTarget.GetSafeNormal();

	if (Direction.IsNearlyZero())
	{
		SetWheelSpeed(0.0f);
		return;
	}

	const FRotator CurrentRotation = Owner->GetActorRotation();
	const FRotator TargetYawRotation = Direction.Rotation();

	const float YawErrorSigned = FMath::FindDeltaAngleDegrees(CurrentRotation.Yaw, TargetYawRotation.Yaw);
	const float YawError = FMath::Abs(YawErrorSigned);
	const float MoveAlpha = YawError > 85.0f ? 0.0f : 1.0f;
	const FRotator NewYawRotation = FMath::RInterpConstantTo(
		FRotator(0.0f, CurrentRotation.Yaw, 0.0f),
		FRotator(0.0f, TargetYawRotation.Yaw, 0.0f),
		SafeDeltaTime,
		TurnSpeedDegPerSec
	);

	const float MoveStep = MoveSpeedCmPerSec * MoveAlpha * SafeDeltaTime;
	const FVector Forward = FRotator(0.0f, CurrentRotation.Yaw, 0.0f).Vector();
	const FVector RawNewLocation = CurrentLocation + Forward * FMath::Min(MoveStep, Distance);

	FVector GroundedLocation = RawNewLocation;
	FRotator GroundedRotation = FRotator(0.0f, NewYawRotation.Yaw, 0.0f);

	const bool bGroundOk = BuildGroundAlignedPose(RawNewLocation, NewYawRotation.Yaw, SafeDeltaTime, GroundedLocation, GroundedRotation);

	MovementDebugTimer += SafeDeltaTime;

	if (bMovementDebugLogs && MovementDebugTimer >= MovementDebugInterval)
	{
		MovementDebugTimer = 0.0f;

		DebugLog(FString::Printf(
			TEXT("[APC MOVE] State=%d Target=%s Dist=%.1f Accept=%.1f | Loc=%s Target=%s | Yaw Cur=%.1f Desired=%.1f New=%.1f Err=%.1f Signed=%.1f | MoveAlpha=%.2f Step=%.1f Speed=%.1f | Forward=%s Raw=%s Grounded=%s GroundOk=%d Rot=%s"),
			static_cast<int32>(CurrentState),
			*GetNameSafe(TargetActor),
			Distance,
			AcceptanceRadiusCm,
			*CurrentLocation.ToString(),
			*TargetActor->GetActorLocation().ToString(),
			CurrentRotation.Yaw,
			TargetYawRotation.Yaw,
			NewYawRotation.Yaw,
			YawError,
			YawErrorSigned,
			MoveAlpha,
			MoveStep,
			MoveSpeedCmPerSec,
			*Forward.ToString(),
			*RawNewLocation.ToString(),
			*GroundedLocation.ToString(),
			bGroundOk ? 1 : 0,
			*GroundedRotation.ToString()
		));
	}

	if (!bGroundOk)
	{
		SetWheelSpeed(0.0f);
		return;
	}

	Owner->SetActorLocationAndRotation(GroundedLocation, GroundedRotation, false, nullptr, ETeleportType::TeleportPhysics);

	const float ActualMoveSpeed = FVector::Dist2D(CurrentLocation, GroundedLocation) / SafeDeltaTime;
	SetWheelSpeed(ActualMoveSpeed * WheelAnimationSpeedScale);
}
void UAPCAIComponent::TickDoorAnimation(float DeltaTime)
{
	CurrentRearDoorsAngle = FMath::FInterpConstantTo(CurrentRearDoorsAngle, TargetRearDoorsAngle, DeltaTime, DoorAnimSpeedDegPerSec);
	CurrentHatchAngle = FMath::FInterpConstantTo(CurrentHatchAngle, TargetHatchAngle, DeltaTime, HatchAnimSpeedDegPerSec);
}

void UAPCAIComponent::UpdateGroundAlignment(float DeltaTime)
{
	AActor* Owner = GetOwner();

	if (!Owner)
	{
		return;
	}

	if (CurrentState == EAPCAIState::MoveToEvacPoint || CurrentState == EAPCAIState::Returning)
	{
		return;
	}

	FVector GroundedLocation = Owner->GetActorLocation();
	FRotator GroundedRotation = Owner->GetActorRotation();

	if (!BuildGroundAlignedPose(Owner->GetActorLocation(), Owner->GetActorRotation().Yaw, DeltaTime, GroundedLocation, GroundedRotation))
	{
		return;
	}

	Owner->SetActorLocationAndRotation(GroundedLocation, GroundedRotation, false, nullptr, ETeleportType::TeleportPhysics);
}

bool UAPCAIComponent::BuildGroundAlignedPose(const FVector& DesiredLocation, float DesiredYaw, float DeltaTime, FVector& OutLocation, FRotator& OutRotation) const
{
	AActor* Owner = GetOwner();

	if (!Owner)
	{
		return false;
	}

	float GroundZOffset = 0.0f;
	FVector GroundNormal = FVector::UpVector;

	if (!SampleGroundAtPose(DesiredLocation, FRotator(0.0f, DesiredYaw, 0.0f), GroundZOffset, GroundNormal))
	{
		return false;
	}

	const float SlopeDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(GroundNormal, FVector::UpVector), -1.0f, 1.0f)));

	if (SlopeDeg > MaxGroundAlignSlopeDeg)
	{
		return false;
	}

	const float SafeDeltaTime = FMath::Max(DeltaTime, 0.001f);
	const FVector CurrentLocation = Owner->GetActorLocation();

	OutLocation = DesiredLocation;

	const float TargetZ = DesiredLocation.Z + GroundZOffset + GroundClearanceCm;
	OutLocation.Z = FMath::FInterpTo(CurrentLocation.Z, TargetZ, SafeDeltaTime, GroundInterpSpeed);

	const FVector FlatForward = FRotator(0.0f, DesiredYaw, 0.0f).Vector();
	FVector GroundForward = FVector::VectorPlaneProject(FlatForward, GroundNormal).GetSafeNormal();

	if (GroundForward.IsNearlyZero())
	{
		GroundForward = Owner->GetActorForwardVector();
		GroundForward = FVector::VectorPlaneProject(GroundForward, GroundNormal).GetSafeNormal();
	}

	if (GroundForward.IsNearlyZero())
	{
		GroundForward = FlatForward;
	}

	const FRotator TargetRotation = FRotationMatrix::MakeFromXZ(GroundForward, GroundNormal).Rotator();
	const FRotator CurrentRotation = Owner->GetActorRotation();

	OutRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, SafeDeltaTime, GroundInterpSpeed);

	return true;
}

bool UAPCAIComponent::SampleGroundAtPose(const FVector& ActorLocation, const FRotator& ActorRotation, float& OutGroundZOffset, FVector& OutGroundNormal) const
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();

	OutGroundZOffset = 0.0f;
	OutGroundNormal = FVector::UpVector;

	if (!Owner || !World || !GroundTraceFL || !GroundTraceFR || !GroundTraceRL || !GroundTraceRR)
	{
		return false;
	}

	USceneComponent* Points[4] = { GroundTraceFL, GroundTraceFR, GroundTraceRL, GroundTraceRR };

	FVector HitNormals[4];

	const FTransform CurrentActorTransform = Owner->GetActorTransform();
	const FTransform DesiredTransform(ActorRotation, ActorLocation);

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Owner);
	Params.bTraceComplex = false;

	float TotalZOffset = 0.0f;

	for (int32 i = 0; i < 4; ++i)
	{
		const FVector LocalPoint = CurrentActorTransform.InverseTransformPosition(Points[i]->GetComponentLocation());
		const FVector WorldPoint = DesiredTransform.TransformPosition(LocalPoint);

		const FVector Start = WorldPoint + FVector(0.0f, 0.0f, GroundTraceUpCm);
		const FVector End = WorldPoint - FVector(0.0f, 0.0f, GroundTraceDownCm);

		FHitResult Hit;
		const bool bHit = World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);

		if (!bHit)
		{
			return false;
		}

		TotalZOffset += Hit.ImpactPoint.Z - WorldPoint.Z;
		HitNormals[i] = Hit.ImpactNormal;
	}

	OutGroundZOffset = TotalZOffset * 0.25f;
	OutGroundNormal = (HitNormals[0] + HitNormals[1] + HitNormals[2] + HitNormals[3]).GetSafeNormal();

	if (OutGroundNormal.IsNearlyZero())
	{
		OutGroundNormal = FVector::UpVector;
	}

	return true;
}

void UAPCAIComponent::UpdateAnimInstanceVariables()
{
	if (!AnimInstance && MeshComponent)
	{
		AnimInstance = MeshComponent->GetAnimInstance();
	}

	if (!AnimInstance)
	{
		return;
	}

	SetAnimFloat(TEXT("WheelSpeed"), CurrentWheelSpeed);
	SetAnimFloat(TEXT("RearDoorsAngle"), CurrentRearDoorsAngle);
	SetAnimFloat(TEXT("HatchAngle"), CurrentHatchAngle);
	SetAnimFloat(TEXT("TurretAngle"), 0.0f);
	SetAnimFloat(TEXT("GunElevation"), 0.0f);
}

void UAPCAIComponent::UpdateTrackSmokeVFX()
{
	const bool bShouldPlay =
		bUseTrackSmokeVFX &&
		!bDestroyed &&
		CurrentWheelSpeed > TrackSmokeMinWheelSpeed &&
		(
			CurrentState == EAPCAIState::MoveToEvacPoint ||
			CurrentState == EAPCAIState::Returning
			);

	for (UParticleSystemComponent* ParticleComponent : TrackSmokeParticleComponents)
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

	for (UNiagaraComponent* NiagaraComponent : TrackSmokeNiagaraComponents)
	{
		if (!NiagaraComponent)
		{
			continue;
		}

		if (bShouldPlay)
		{
			NiagaraComponent->SetVisibility(true, true);

			if (!NiagaraComponent->IsActive())
			{
				NiagaraComponent->Activate(true);
			}
		}
		else
		{
			if (NiagaraComponent->IsActive())
			{
				NiagaraComponent->Deactivate();
			}

			NiagaraComponent->SetVisibility(false, true);
		}
	}
}

void UAPCAIComponent::SetAnimFloat(FName VariableName, float Value)
{
	if (!AnimInstance)
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

void UAPCAIComponent::SetWheelSpeed(float Value)
{
	CurrentWheelSpeed = Value;
}

void UAPCAIComponent::SetRearDoorsAngle(float Value)
{
	TargetRearDoorsAngle = FMath::Clamp(Value, 0.0f, OpenRearDoorsAngle);
}

void UAPCAIComponent::SetHatchAngle(float Value)
{
	TargetHatchAngle = FMath::Clamp(Value, 0.0f, OpenHatchAngle);
}

void UAPCAIComponent::SetTurretNeutral()
{
	SetAnimFloat(TEXT("TurretAngle"), 0.0f);
	SetAnimFloat(TEXT("GunElevation"), 0.0f);
}

void UAPCAIComponent::SetNavObstacleActive(bool bActive)
{
	if (!NavObstacleComponent)
	{
		return;
	}

	if (bNavObstacleActive == bActive)
	{
		return;
	}

	bNavObstacleActive = bActive;

	NavObstacleComponent->SetCollisionEnabled(bActive ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	NavObstacleComponent->SetCanEverAffectNavigation(bActive);

	DebugLog(FString::Printf(TEXT("[APC NAV] Obstacle active: %d"), bActive ? 1 : 0));
}

void UAPCAIComponent::ApplyDestroyedVisuals(const FVector& HitLocation)
{
	if (MeshComponent && DestroyedMaterial)
	{
		if (DestroyedMaterialElementIndices.Num() > 0)
		{
			for (int32 MaterialIndex : DestroyedMaterialElementIndices)
			{
				if (MaterialIndex >= 0 && MaterialIndex < MeshComponent->GetNumMaterials())
				{
					MeshComponent->SetMaterial(MaterialIndex, DestroyedMaterial);
				}
			}
		}
		else if (bApplyDestroyedMaterialToAllSlots)
		{
			const int32 MaterialCount = MeshComponent->GetNumMaterials();

			for (int32 i = 0; i < MaterialCount; ++i)
			{
				MeshComponent->SetMaterial(i, DestroyedMaterial);
			}
		}
		else
		{
			MeshComponent->SetMaterial(0, DestroyedMaterial);
		}
	}

	const FVector FXLocation = DestroyedFXPoint
		? DestroyedFXPoint->GetComponentLocation()
		: HitLocation;

	if (DestroyedFX && GetWorld())
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			DestroyedFX,
			FXLocation,
			FRotator::ZeroRotator,
			FVector(1.0f),
			true,
			true,
			ENCPoolMethod::None,
			true
		);
	}
}

void UAPCAIComponent::FinishBoardingCrew()
{
	if (!PendingCrewMember)
	{
		return;
	}

	AInfantryCharacter* BoardedCrewMember = PendingCrewMember;
	PendingCrewMember = nullptr;

	AssignedCrew.Remove(BoardedCrewMember);

	BoardedCrewMember->SetActorHiddenInGame(true);
	BoardedCrewMember->SetActorEnableCollision(false);
	BoardedCrewMember->Destroy();

	LoadedCrewCount++;

	DebugLog(FString::Printf(TEXT("[APC] Crew boarded | Loaded=%d Remaining=%d InitialExpected=%d"),
		LoadedCrewCount,
		GetRemainingAssignedCrewCount(),
		ExpectedCrewCount));
}

void UAPCAIComponent::TryStartBoarding(AActor* OtherActor)
{
	if (CurrentState != EAPCAIState::WaitingForCrew)
	{
		return;
	}

	if (PendingCrewMember)
	{
		return;
	}

	if (LoadedCrewCount >= ExpectedCrewCount)
	{
		return;
	}

	AInfantryCharacter* Infantry = Cast<AInfantryCharacter>(OtherActor);

	if (!Infantry || Infantry->IsDead())
	{
		return;
	}
	if (!IsAssignedCrewMember(OtherActor))
	{
		return;
	}

	PendingCrewMember = Infantry;

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(BoardingTimerHandle);
		GetWorld()->GetTimerManager().SetTimer(
			BoardingTimerHandle,
			this,
			&UAPCAIComponent::FinishBoardingCrew,
			BoardingSecondsPerCrew,
			false
		);
	}

	DebugLog(FString::Printf(TEXT("[APC] Crew boarding started | Pawn=%s Time=%.1f"),
		*GetNameSafe(Infantry),
		BoardingSecondsPerCrew));
}

bool UAPCAIComponent::IsDroneWarheadComponent(UPrimitiveComponent* OtherComp) const
{
	if (!OtherComp)
	{
		return false;
	}

	if (OtherComp->ComponentHasTag(DroneWarheadComponentTag))
	{
		return true;
	}

	return OtherComp->GetFName() == DroneWarheadComponentTag;
}

bool UAPCAIComponent::IsDroneActor(AActor* OtherActor) const
{
	return Cast<AFPVDronePawn>(OtherActor) != nullptr;
}

bool UAPCAIComponent::IsDroneBombArmed(AActor* OtherActor) const
{
	const AFPVDronePawn* Drone = Cast<AFPVDronePawn>(OtherActor);

	if (!Drone)
	{
		return false;
	}

	return Drone->IsBombArmed();
}

void UAPCAIComponent::ForceCrashDrone(AActor* OtherActor, UPrimitiveComponent* OtherComp, const FVector& HitLocation)
{
	AFPVDronePawn* Drone = Cast<AFPVDronePawn>(OtherActor);

	if (!Drone || Drone->IsCrashed())
	{
		return;
	}

	Drone->ForceCrashAtLocation(HitLocation);
}

USceneComponent* UAPCAIComponent::FindSceneComponentByName(FName ComponentName) const
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

UPrimitiveComponent* UAPCAIComponent::FindPrimitiveComponentByName(FName ComponentName) const
{
	return Cast<UPrimitiveComponent>(FindSceneComponentByName(ComponentName));
}

bool UAPCAIComponent::IsTrackSmokeComponentName(const FString& ComponentName) const
{
	return ComponentName.Contains(TrackSmokeComponentPrefix) ||
		ComponentName.Contains(TEXT("TrackSmoke")) ||
		ComponentName.Contains(TEXT("TrackSomke"));
}

void UAPCAIComponent::CallBlueprintEventNoParams(FName FunctionName)
{
	AActor* Owner = GetOwner();

	if (!Owner || FunctionName.IsNone())
	{
		return;
	}

	UFunction* Function = Owner->FindFunction(FunctionName);

	if (!Function)
	{
		return;
	}

	Owner->ProcessEvent(Function, nullptr);
}

void UAPCAIComponent::CallBlueprintEventFloat(FName FunctionName, float Value)
{
	AActor* Owner = GetOwner();

	if (!Owner || FunctionName.IsNone())
	{
		return;
	}

	UFunction* Function = Owner->FindFunction(FunctionName);

	if (!Function)
	{
		return;
	}

	struct FFloatEventParams
	{
		float Value;
	};

	FFloatEventParams Params;
	Params.Value = Value;

	Owner->ProcessEvent(Function, &Params);
}

void UAPCAIComponent::CallBlueprintEventBool(FName FunctionName, bool bValue)
{
	AActor* Owner = GetOwner();

	if (!Owner || FunctionName.IsNone())
	{
		return;
	}

	UFunction* Function = Owner->FindFunction(FunctionName);

	if (!Function)
	{
		return;
	}

	struct FBoolEventParams
	{
		bool bValue;
	};

	FBoolEventParams Params;
	Params.bValue = bValue;

	Owner->ProcessEvent(Function, &Params);
}

void UAPCAIComponent::DebugLog(const FString& Message) const
{
	if (bDebugLogs)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s"), *Message);
	}
}

void UAPCAIComponent::OnHitZoneBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bDestroyed)
	{
		return;
	}

	if (!IsDroneActor(OtherActor))
	{
		return;
	}

	if (!IsDroneWarheadComponent(OtherComp))
	{
		return;
	}

	const FVector HitLocation = OtherComp
		? OtherComp->GetComponentLocation()
		: OverlappedComponent->GetComponentLocation();

	const bool bBombArmed = IsDroneBombArmed(OtherActor);

	ForceCrashDrone(OtherActor, OtherComp, HitLocation);

	if (bRequireBombArmedToDestroy && !bBombArmed)
	{
		DebugLog(TEXT("[APC] Direct warhead contact ignored: bomb is not armed"));
		return;
	}

	DebugLog(FString::Printf(TEXT("[APC] Direct warhead hit accepted | Drone=%s HitLocation=%s"),
		*GetNameSafe(OtherActor),
		*HitLocation.ToString()));

	DestroyAPC(HitLocation);
}

void UAPCAIComponent::OnBoardingZoneBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	TryStartBoarding(OtherActor);
}

void UAPCAIComponent::AutoStartEvacuation()
{
	if (bDestroyed)
	{
		return;
	}

	AActor* ResolvedEvacPoint = ResolveScenarioPoint(DefaultEvacPoint, DefaultEvacPointName);
	AActor* ResolvedReturnPoint = ResolveScenarioPoint(DefaultReturnPoint, DefaultReturnPointName);

	if (!ResolvedEvacPoint)
	{
		DebugLog(FString::Printf(TEXT("[APC] AutoStartEvacuation failed: evac point not found | Direct=%s Name=%s"),
			*GetNameSafe(DefaultEvacPoint),
			*DefaultEvacPointName.ToString()));
		return;
	}

	StartEvacuation(ResolvedEvacPoint, ResolvedReturnPoint);
}

AActor* UAPCAIComponent::ResolveScenarioPoint(AActor* DirectActor, FName ActorName) const
{
	if (DirectActor)
	{
		return DirectActor;
	}

	UWorld* World = GetWorld();

	if (!World || ActorName.IsNone())
	{
		return nullptr;
	}

	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), Actors);

	const FString WantedName = ActorName.ToString();

	for (AActor* Actor : Actors)
	{
		if (!Actor)
		{
			continue;
		}

		const FString CurrentName = Actor->GetName();

		if (CurrentName == WantedName || CurrentName.StartsWith(WantedName))
		{
			return Actor;
		}
	}

	return nullptr;
}

void UAPCAIComponent::TryBoardOverlappingCrew()
{
	if (PendingCrewMember)
	{
		return;
	}

	if (AssignedCrew.Num() <= 0 && LoadedCrewCount >= ExpectedCrewCount)
	{
		return;
	}

	if (BoardingZoneComponent)
	{
		TArray<AActor*> OverlappingActors;
		BoardingZoneComponent->GetOverlappingActors(OverlappingActors, AInfantryCharacter::StaticClass());

		DebugLog(FString::Printf(TEXT("[APC BOARDING CHECK] Zone=%s Overlaps=%d Loaded=%d Expected=%d Pending=%s State=%d"),
			*GetNameSafe(BoardingZoneComponent),
			OverlappingActors.Num(),
			LoadedCrewCount,
			ExpectedCrewCount,
			*GetNameSafe(PendingCrewMember),
			static_cast<int32>(CurrentState)));

		for (AActor* Actor : OverlappingActors)
		{
			if (!Actor)
			{
				continue;
			}

			TryStartBoarding(Actor);

			if (PendingCrewMember)
			{
				return;
			}
		}
	}

	const FVector BoardingLocation = GetBoardingMoveLocation();

	float ClosestDistance = TNumericLimits<float>::Max();
	AInfantryCharacter* ClosestCrewMember = nullptr;

	for (AInfantryCharacter* CrewMember : AssignedCrew)
	{
		if (!CrewMember || CrewMember->IsDead())
		{
			continue;
		}

		const float Distance = FVector::Dist2D(CrewMember->GetActorLocation(), BoardingLocation);

		if (Distance < ClosestDistance)
		{
			ClosestDistance = Distance;
			ClosestCrewMember = CrewMember;
		}

		if (Distance <= BoardingDistanceCm)
		{
			DebugLog(FString::Printf(TEXT("[APC BOARDING DISTANCE] Crew=%s Distance=%.1f Limit=%.1f BoardingLocation=%s CrewLocation=%s"),
				*GetNameSafe(CrewMember),
				Distance,
				BoardingDistanceCm,
				*BoardingLocation.ToString(),
				*CrewMember->GetActorLocation().ToString()));

			TryStartBoarding(CrewMember);
			return;
		}
	}

	DebugLog(FString::Printf(TEXT("[APC BOARDING DISTANCE CHECK] Closest=%s Distance=%.1f Limit=%.1f BoardingLocation=%s"),
		*GetNameSafe(ClosestCrewMember),
		ClosestDistance,
		BoardingDistanceCm,
		*BoardingLocation.ToString()));
}
void UAPCAIComponent::SetEvacuationCrew(const TArray<AInfantryCharacter*>& InCrew)
{
	AssignedCrew.Empty();

	for (AInfantryCharacter* CrewMember : InCrew)
	{
		if (!CrewMember || CrewMember->IsDead())
		{
			continue;
		}

		AssignedCrew.Add(CrewMember);
	}

	ExpectedCrewCount = AssignedCrew.Num();
	bAssignedCrewOrderedToBoard = false;

	DebugLog(FString::Printf(TEXT("[APC CREW] Assigned crew count: %d"), AssignedCrew.Num()));
}
void UAPCAIComponent::SetBoardingMovePoint(AActor* InBoardingMovePoint)
{
	BoardingMovePoint = InBoardingMovePoint;
}

void UAPCAIComponent::OrderAssignedCrewToBoard()
{
	if (bAssignedCrewOrderedToBoard)
	{
		return;
	}

	if (CurrentState != EAPCAIState::WaitingForCrew)
	{
		return;
	}

	CompactAssignedCrew();

	if (AssignedCrew.Num() <= 0)
	{
		return;
	}

	bAssignedCrewOrderedToBoard = true;

	const FVector BoardingLocation = GetBoardingMoveLocation();

	for (AInfantryCharacter* CrewMember : AssignedCrew)
	{
		if (!CrewMember || CrewMember->IsDead())
		{
			continue;
		}

		AInfantryAIController* CrewController = Cast<AInfantryAIController>(CrewMember->GetController());

		if (!CrewController)
		{
			CrewMember->SpawnDefaultController();
			CrewController = Cast<AInfantryAIController>(CrewMember->GetController());
		}

		if (CrewController)
		{
			CrewController->BeginMoveToBoardingZoneLocation(BoardingLocation);
		}
	}

	DebugLog(FString::Printf(TEXT("[APC CREW] Crew ordered to board | Count=%d Target=%s"),
		AssignedCrew.Num(),
		*BoardingLocation.ToString()));
}

bool UAPCAIComponent::IsAssignedCrewMember(AActor* Actor) const
{
	if (AssignedCrew.Num() == 0)
	{
		return true;
	}

	AInfantryCharacter* Infantry = Cast<AInfantryCharacter>(Actor);

	if (!Infantry)
	{
		return false;
	}

	return AssignedCrew.Contains(Infantry);
}

FVector UAPCAIComponent::GetBoardingMoveLocation() const
{
	AActor* Owner = GetOwner();

	if (BoardingZoneComponent)
	{
		const FVector ZoneCenter = BoardingZoneComponent->GetComponentLocation();

		FHitResult Hit;
		FCollisionQueryParams Params;
		Params.bTraceComplex = false;

		if (Owner)
		{
			Params.AddIgnoredActor(Owner);
		}

		const FVector TraceStart = ZoneCenter + FVector(0.0f, 0.0f, 1000.0f);
		const FVector TraceEnd = ZoneCenter - FVector(0.0f, 0.0f, 3000.0f);

		if (GetWorld() && GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params))
		{
			return FVector(ZoneCenter.X, ZoneCenter.Y, Hit.ImpactPoint.Z);
		}

		return FVector(ZoneCenter.X, ZoneCenter.Y, Owner ? Owner->GetActorLocation().Z : ZoneCenter.Z);
	}

	if (Owner)
	{
		const FVector RawLocation = Owner->GetActorTransform().TransformPosition(AutoBoardingMoveLocalOffset);

		FHitResult Hit;
		FCollisionQueryParams Params;
		Params.bTraceComplex = false;
		Params.AddIgnoredActor(Owner);

		const FVector TraceStart = RawLocation + FVector(0.0f, 0.0f, 1000.0f);
		const FVector TraceEnd = RawLocation - FVector(0.0f, 0.0f, 3000.0f);

		if (GetWorld() && GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params))
		{
			return Hit.ImpactPoint;
		}

		return RawLocation;
	}

	return FVector::ZeroVector;
}

void UAPCAIComponent::CompactAssignedCrew()
{
	for (int32 i = AssignedCrew.Num() - 1; i >= 0; --i)
	{
		AInfantryCharacter* CrewMember = AssignedCrew[i];

		if (!CrewMember || CrewMember->IsDead())
		{
			AssignedCrew.RemoveAt(i);
		}
	}
}

int32 UAPCAIComponent::GetRemainingAssignedCrewCount() const
{
	int32 Count = 0;

	for (AInfantryCharacter* CrewMember : AssignedCrew)
	{
		if (CrewMember && !CrewMember->IsDead())
		{
			Count++;
		}
	}

	return Count;
}

bool UAPCAIComponent::ShouldFinishBoarding() const
{
	if (PendingCrewMember)
	{
		return false;
	}

	if (ExpectedCrewCount <= 0 && AssignedCrew.Num() <= 0)
	{
		return false;
	}

	return GetRemainingAssignedCrewCount() <= 0;
}