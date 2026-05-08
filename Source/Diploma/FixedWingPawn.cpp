//#include "FixedWingPawn.h"
//#include "Components/StaticMeshComponent.h"
//#include "DrawDebugHelpers.h"
//
//AFixedWingPawn::AFixedWingPawn()
//{
//	PrimaryActorTick.bCanEverTick = true;
//}
//
//void AFixedWingPawn::BeginPlay()
//{
//	Super::BeginPlay();
//
//	if (!PlaneMesh)
//	{
//		return;
//	}
//
//	PlaneMesh->SetMassOverrideInKg(NAME_None, AircraftMassKg);
//	PlaneMesh->SetCenterOfMass(CenterOfMassOffsetCm, NAME_None);
//	//temp
//	PlaneMesh->SetPhysicsLinearVelocity(GetActorForwardVector() * 3500.f);
//	UE_LOG(LogTemp, Warning, TEXT("ForwardVector=%s"), *PlaneMesh->GetForwardVector().ToString());
//	UE_LOG(LogTemp, Warning, TEXT("RightVector=%s"), *PlaneMesh->GetRightVector().ToString());
//	UE_LOG(LogTemp, Warning, TEXT("UpVector=%s"), *PlaneMesh->GetUpVector().ToString());
//
//	bDropTestStarted = false;
//	bDropTestLogged = false;
//	DropTestStartTime = 0.f;
//	DropTestStartAltitudeM = 0.f;
//
//}
//
//void AFixedWingPawn::Tick(float DeltaSeconds)
//{
//	Super::Tick(DeltaSeconds);
//
//	if (bCrashed || !PlaneMesh)
//	{
//		return;
//	}
//
//	SimulateFixedWing(DeltaSeconds);
//	UpdateTelemetry();
//	
//	//logs
//
//	const FTransform MeshTransform = PlaneMesh->GetComponentTransform();
//	const FVector WorldVelocityCm = PlaneMesh->GetPhysicsLinearVelocity();
//	const FVector LocalVelocityMps = MeshTransform.InverseTransformVectorNoScale(WorldVelocityCm) / 100.f;
//
//	const FVector WorldAngularRatesRad = PlaneMesh->GetPhysicsAngularVelocityInRadians();
//	const FVector LocalAngularRatesRad = MeshTransform.InverseTransformVectorNoScale(WorldAngularRatesRad);
//
//	const float ForwardSpeed = LocalVelocityMps.X;
//	const float WorldVerticalMps = WorldVelocityCm.Z / 100.f;
//	const float AoADeg = FMath::RadiansToDegrees(FMath::Atan2(-LocalVelocityMps.Z, FMath::Max(FMath::Abs(ForwardSpeed), 0.1f)));
//	const float BetaDeg = FMath::RadiansToDegrees(FMath::Atan2(LocalVelocityMps.Y, FMath::Max(FMath::Abs(ForwardSpeed), 0.1f)));
//
//	const FRotator R = PlaneMesh->GetComponentRotation();
//	const float AltitudeM = GetActorLocation().Z / 100.f;
//	const float TimeSec = GetWorld()->GetTimeSeconds();
//
//	const float InPitch = GetPitchInput();
//	const float InRoll = GetRollInput();
//	const float InYaw = GetYawInput();
//
//	const float ElevatorDeltaDeg = InPitch * ElevatorAoADeltaDeg;
//
//	const float RollRateDeg = FMath::RadiansToDegrees(LocalAngularRatesRad.X);
//	const float PitchRateDeg = FMath::RadiansToDegrees(LocalAngularRatesRad.Y);
//	const float YawRateDeg = FMath::RadiansToDegrees(LocalAngularRatesRad.Z);
//
//	UE_LOG(
//		LogTemp,
//		Warning,
//		TEXT("T=%.2f InRoll=%.2f InPitch=%.2f InYaw=%.2f Roll=%.2f Pitch=%.2f Yaw=%.2f P=%.2f Q=%.2f R=%.2f Forward=%.2f Vy=%.2f Vz=%.2f WorldVertical=%.2f AoA=%.2f Beta=%.2f Alt=%.2f"),
//		TimeSec,
//		InRoll,
//		InPitch,
//		InYaw,
//		R.Roll,
//		R.Pitch,
//		R.Yaw,
//		RollRateDeg,
//		PitchRateDeg,
//		YawRateDeg,
//		LocalVelocityMps.X,
//		LocalVelocityMps.Y,
//		LocalVelocityMps.Z,
//		WorldVerticalMps,
//		AoADeg,
//		BetaDeg,
//		AltitudeM
//	);
//
//	const FVector Origin = PlaneMesh->GetComponentLocation();
//	const FVector Forward = PlaneMesh->GetForwardVector();
//	const FVector Up = PlaneMesh->GetUpVector();
//	const FVector Right = PlaneMesh->GetRightVector();
//
//	DrawDebugLine(GetWorld(), Origin, Origin + Forward * 300.f, FColor::Red, false, 0.f, 0, 2.f);
//	DrawDebugLine(GetWorld(), Origin, Origin + Right * 300.f, FColor::Green, false, 0.f, 0, 2.f);
//	DrawDebugLine(GetWorld(), Origin, Origin + Up * 300.f, FColor::Blue, false, 0.f, 0, 2.f);
//
//
//	if (PlaneMesh && GetWorld())
//	{
//		const float Altitude = GetActorLocation().Z / 100.f;
//
//		if (!bDropTestStarted)
//		{
//			bDropTestStarted = true;
//			DropTestStartTime = GetWorld()->GetTimeSeconds();
//			DropTestStartAltitudeM = Altitude;
//
//			UE_LOG(LogTemp, Warning, TEXT("DROP TEST START: Time=%.2f Alt=%.2f"),
//				DropTestStartTime,
//				DropTestStartAltitudeM);
//		}
//
//		if (!bDropTestLogged && Altitude <= DropTestStartAltitudeM - 10.f)
//		{
//			bDropTestLogged = true;
//
//			const float Elapsed = GetWorld()->GetTimeSeconds() - DropTestStartTime;
//
//			UE_LOG(LogTemp, Warning, TEXT("DROP TEST 10M: Elapsed=%.2f sec StartAlt=%.2f CurrentAlt=%.2f"),
//				Elapsed,
//				DropTestStartAltitudeM,
//				Altitude);
//		}
//	}
//
//}
//
//void AFixedWingPawn::SimulateFixedWing(float DeltaSeconds)
//{
//	if (!PlaneMesh)
//	{
//		return;
//	}
//
//
//
//	const FTransform MeshTransform = PlaneMesh->GetComponentTransform();
//	const FVector WorldVelocityCm = PlaneMesh->GetPhysicsLinearVelocity();
//	const FVector LocalVelocityMps = MeshTransform.InverseTransformVectorNoScale(WorldVelocityCm) / 100.f;
//
//	const FVector WorldAngularRatesRad = PlaneMesh->GetPhysicsAngularVelocityInRadians();
//	const FVector LocalAngularRatesRad = MeshTransform.InverseTransformVectorNoScale(WorldAngularRatesRad);
//
//	const float AltitudeM = GetActorLocation().Z / 100.f;
//	const float AirDensity = GetAirDensity(AltitudeM);
//
//	//roll damping
//	const float qBody = GetDynamicPressure(AirDensity, LocalVelocityMps);
//	const float RollRateRad = LocalAngularRatesRad.X;
//	const float RollDampingTorqueNm = -RollRateRad * RollDampingCoeff * qBody * WingAreaM2 * WingSpanM;
//	const FVector LocalTorqueCm(RollDampingTorqueNm * 100.f, 0.f, 0.f);
//	const FVector WorldTorque = PlaneMesh->GetComponentTransform().TransformVectorNoScale(LocalTorqueCm);
//	PlaneMesh->AddTorqueInRadians(WorldTorque);
//	//
//
//	ApplyPropulsion(AirDensity);
//	ApplyWingHalf(true, LeftWingLocalPos, AirDensity, LocalVelocityMps, LocalAngularRatesRad);
//	ApplyWingHalf(false, RightWingLocalPos, AirDensity, LocalVelocityMps, LocalAngularRatesRad);
//	ApplyHorizontalTail(AirDensity, LocalVelocityMps, LocalAngularRatesRad);
//	ApplyVerticalTail(AirDensity, LocalVelocityMps, LocalAngularRatesRad);
//	ApplyParasiteDrag(AirDensity);
//}
//
//void AFixedWingPawn::ApplyPropulsion(float AirDensity)
//{
//	if (!PlaneMesh)
//	{
//		return;
//	}
//
//	const float DensityFactor = FMath::Clamp(AirDensity / 1.225f, 0.3f, 1.f);
//	const float ThrustN = FMath::Clamp(Throttle, 0.f, 1.f) * MaxEngineThrustN * DensityFactor;
//
//	const FVector LocalForceCm = FVector(ThrustN * 100.f, 0.f, 0.f);
//	const FVector WorldForceCm = PlaneMesh->GetComponentTransform().TransformVectorNoScale(LocalForceCm);
//
//	PlaneMesh->AddForce(WorldForceCm);
//}
//
//void AFixedWingPawn::ApplyWingHalf(bool bLeftWing, const FVector& LocalPositionCm, float AirDensity, const FVector& LocalVelocityMps, const FVector& LocalAngularRatesRad)
//{
//	if (!PlaneMesh)
//	{
//		return;
//	}
//
//	const FVector SectionVelocityMps = GetLocalPointVelocityMps(LocalVelocityMps, LocalAngularRatesRad, LocalPositionCm);
//	const float SectionSpeedMps = SectionVelocityMps.Size();
//
//	if (SectionSpeedMps < 0.5f)
//	{
//		return;
//	}
//
//	const float q = GetDynamicPressure(AirDensity, SectionVelocityMps);
//	const float ForwardSpeedMps = FMath::Max(FMath::Abs(LocalVelocityMps.X), 0.1f);
//
//	float AoADeg = FMath::RadiansToDegrees(FMath::Atan2(-SectionVelocityMps.Z, FMath::Max(SectionVelocityMps.X, 0.1f)));
//	AoADeg += WingIncidenceDeg;
//
//	const float BetaDeg = FMath::RadiansToDegrees(FMath::Atan2(LocalVelocityMps.Y, ForwardSpeedMps));
//
//	const float BankDeg = FMath::Abs(FMath::UnwindDegrees(PlaneMesh->GetComponentRotation().Roll));
//	const float InputAbs = FMath::Abs(RollInput);
//
//	const float InputFade = 1.f - FMath::Clamp(InputAbs / 0.3f, 0.f, 1.f);
//	const float BankFade = FMath::GetMappedRangeValueClamped(
//		FVector2D(0.f, 80.f),
//		FVector2D(1.f, 0.25f),
//		BankDeg
//	);
//
//	const float EffectiveDihedral = DihedralEffectCoeff * InputFade * BankFade;
//	const float DihedralAoADeg = BetaDeg * EffectiveDihedral;
//	AoADeg += bLeftWing ? -DihedralAoADeg : DihedralAoADeg;
//
//	const float StallFactor = GetStallFactor(AoADeg, WingStallAoADeg, WingMaxAoADeg);
//
//	float CL = GetWingLiftCoefficient(AoADeg);
//
//	const float ControlSign = bLeftWing ? 1.f : -1.f;
//	CL += ControlSign * RollInput * AileronLiftDelta * StallFactor;
//	CL = FMath::Clamp(CL, -WingCLMax, WingCLMax);
//
//	const float CD = GetDragCoefficient(WingCD0, CL, WingInducedDragK, StallFactor, WingStallDragAdd);
//
//	const float HalfWingAreaM2 = WingAreaM2 * 0.5f;
//	const float LiftN = q * HalfWingAreaM2 * CL;
//	const float DragN = q * HalfWingAreaM2 * CD;
//
//	const FVector LocalForceCm(-DragN * 100.f, 0.f, LiftN * 100.f);
//	const FVector WorldForceCm = PlaneMesh->GetComponentTransform().TransformVectorNoScale(LocalForceCm);
//	const FVector WorldPos = PlaneMesh->GetComponentTransform().TransformPosition(LocalPositionCm);
//
//	PlaneMesh->AddForceAtLocation(WorldForceCm, WorldPos);
//}
//void AFixedWingPawn::ApplyHorizontalTail(float AirDensity, const FVector& LocalVelocityMps, const FVector& LocalAngularRatesRad)
//{
//	if (!PlaneMesh)
//	{
//		return;
//	}
//
//	const FVector SectionVelocityMps = GetLocalPointVelocityMps(LocalVelocityMps, LocalAngularRatesRad, HorizontalTailLocalPos);
//	const float SectionSpeedMps = SectionVelocityMps.Size();
//
//	if (SectionSpeedMps < 0.5f)
//	{
//		return;
//	}
//
//	const float q = GetDynamicPressure(AirDensity, SectionVelocityMps);
//	const float ForwardSpeedMps = FMath::Max(SectionVelocityMps.X, 0.1f);
//
//	float AoADeg = FMath::RadiansToDegrees(FMath::Atan2(-SectionVelocityMps.Z, ForwardSpeedMps));
//	AoADeg += HorizontalTailIncidenceDeg;
//	AoADeg += PitchInput * ElevatorAoADeltaDeg;
//
//	const float StallFactor = GetStallFactor(AoADeg, HorizontalTailStallAoADeg, HorizontalTailMaxAoADeg);
//	const float CL = GetTailLiftCoefficient(AoADeg);
//	const float CD = GetDragCoefficient(HorizontalTailCD0, CL, HorizontalTailInducedDragK, StallFactor, HorizontalTailStallDragAdd);
//
//	const float LiftN = q * HorizontalTailAreaM2 * CL;
//	const float DragN = q * HorizontalTailAreaM2 * CD;
//
//	const FVector LocalForceCm = FVector(-DragN * 100.f, 0.f, LiftN * 100.f);
//	const FVector WorldForceCm = PlaneMesh->GetComponentTransform().TransformVectorNoScale(LocalForceCm);
//	const FVector WorldPos = PlaneMesh->GetComponentTransform().TransformPosition(HorizontalTailLocalPos);
//
//	PlaneMesh->AddForceAtLocation(WorldForceCm, WorldPos);
//}
//
//void AFixedWingPawn::ApplyVerticalTail(float AirDensity, const FVector& LocalVelocityMps, const FVector& LocalAngularRatesRad)
//{
//	if (!PlaneMesh)
//	{
//		return;
//	}
//
//	const FVector SectionVelocityMps = GetLocalPointVelocityMps(LocalVelocityMps, LocalAngularRatesRad, VerticalTailLocalPos);
//	const float SectionSpeedMps = SectionVelocityMps.Size();
//
//	if (SectionSpeedMps < 0.5f)
//	{
//		return;
//	}
//
//	const float q = GetDynamicPressure(AirDensity, SectionVelocityMps);
//	const float ForwardSpeedMps = FMath::Max(SectionVelocityMps.X, 0.1f);
//
//	float BetaDeg = FMath::RadiansToDegrees(FMath::Atan2(SectionVelocityMps.Y, ForwardSpeedMps));
//	BetaDeg += YawInput * RudderBetaDeltaDeg;
//
//	const float StallFactor = GetStallFactor(BetaDeg, VerticalTailStallBetaDeg, VerticalTailMaxBetaDeg);
//	const float CY = GetFinSideforceCoefficient(BetaDeg);
//	const float CD = GetDragCoefficient(VerticalTailCD0, CY, VerticalTailInducedDragK, StallFactor, VerticalTailStallDragAdd);
//
//	const float SideForceN = q * VerticalTailAreaM2 * CY;
//	const float DragN = q * VerticalTailAreaM2 * CD;
//
//	const FVector LocalForceCm = FVector(-DragN * 100.f, SideForceN * 100.f, 0.f);
//	const FVector WorldForceCm = PlaneMesh->GetComponentTransform().TransformVectorNoScale(LocalForceCm);
//	const FVector WorldPos = PlaneMesh->GetComponentTransform().TransformPosition(VerticalTailLocalPos);
//
//	PlaneMesh->AddForceAtLocation(WorldForceCm, WorldPos);
//}
//
//void AFixedWingPawn::ApplyParasiteDrag(float AirDensity)
//{
//	if (!PlaneMesh)
//	{
//		return;
//	}
//
//	const FVector WorldVelocityCm = PlaneMesh->GetPhysicsLinearVelocity();
//	const float SpeedMps = WorldVelocityCm.Size() / 100.f;
//
//	if (SpeedMps < 0.1f)
//	{
//		return;
//	}
//
//	const float q = 0.5f * AirDensity * SpeedMps * SpeedMps;
//	const float DragN = q * FuselageEquivalentAreaM2 * FuselageCD0;
//
//	const FVector WorldForceCm = -WorldVelocityCm.GetSafeNormal() * (DragN * 100.f);
//	PlaneMesh->AddForce(WorldForceCm);
//}
//
//FVector AFixedWingPawn::GetLocalPointVelocityMps(const FVector& LocalVelocityMps, const FVector& LocalAngularRatesRad, const FVector& LocalPositionCm) const
//{
//	const FVector LocalPositionM = LocalPositionCm / 100.f;
//	return LocalVelocityMps + FVector::CrossProduct(LocalAngularRatesRad, LocalPositionM);
//}
//
//float AFixedWingPawn::GetAirDensity(float AltitudeM) const
//{
//	const float SeaLevelDensity = 1.225f;
//	const float ScaleHeightM = 8500.f;
//	return SeaLevelDensity * FMath::Exp(-AltitudeM / ScaleHeightM);
//}
//
//float AFixedWingPawn::GetDynamicPressure(float AirDensity, const FVector& LocalVelocityMps) const
//{
//	return 0.5f * AirDensity * LocalVelocityMps.SizeSquared();
//}
//
//float AFixedWingPawn::GetStallFactor(float AngleDeg, float StallAngleDeg, float MaxAngleDeg) const
//{
//	const float AbsAngle = FMath::Abs(AngleDeg);
//
//	if (AbsAngle <= StallAngleDeg)
//	{
//		return 1.f;
//	}
//
//	const float T = FMath::Clamp((AbsAngle - StallAngleDeg) / FMath::Max(MaxAngleDeg - StallAngleDeg, 0.001f), 0.f, 1.f);
//	return FMath::Lerp(1.f, 0.12f, T);
//}
//
//float AFixedWingPawn::GetDragCoefficient(float BaseCD0, float LiftOrSideCoeff, float InducedK, float StallFactor, float ExtraStallDrag) const
//{
//	const float Induced = InducedK * LiftOrSideCoeff * LiftOrSideCoeff;
//	const float StallDrag = (1.f - StallFactor) * ExtraStallDrag;
//	return BaseCD0 + Induced + StallDrag;
//}
//
//float AFixedWingPawn::GetWingLiftCoefficient(float AoADeg) const
//{
//	const float AlphaRad = FMath::DegreesToRadians(AoADeg - WingZeroLiftAoADeg);
//	float CL = WingLiftSlopePerRad * AlphaRad;
//	CL = FMath::Clamp(CL, -WingCLMax, WingCLMax);
//
//	const float StallFactor = GetStallFactor(AoADeg, WingStallAoADeg, WingMaxAoADeg);
//	return CL * StallFactor;
//}
//
//float AFixedWingPawn::GetTailLiftCoefficient(float AoADeg) const
//{
//	const float AlphaRad = FMath::DegreesToRadians(AoADeg - HorizontalTailZeroLiftAoADeg);
//	float CL = HorizontalTailLiftSlopePerRad * AlphaRad;
//	CL = FMath::Clamp(CL, -HorizontalTailCLMax, HorizontalTailCLMax);
//
//	const float StallFactor = GetStallFactor(AoADeg, HorizontalTailStallAoADeg, HorizontalTailMaxAoADeg);
//	return CL * StallFactor;
//}
//
//float AFixedWingPawn::GetFinSideforceCoefficient(float BetaDeg) const
//{
//	const float BetaRad = FMath::DegreesToRadians(BetaDeg);
//	float CY = -VerticalTailLiftSlopePerRad * BetaRad;
//	CY = FMath::Clamp(CY, -VerticalTailCYMax, VerticalTailCYMax);
//
//	const float StallFactor = GetStallFactor(BetaDeg, VerticalTailStallBetaDeg, VerticalTailMaxBetaDeg);
//	return CY * StallFactor;
//}