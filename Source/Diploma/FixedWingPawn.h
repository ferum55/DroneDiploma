//#pragma once
//
//#include "CoreMinimal.h"
//#include "FPVDronePawn.h"
//#include "FixedWingPawn.generated.h"
//
//UCLASS()
//class AFixedWingPawn : public AFPVDronePawn
//{
//	GENERATED_BODY()
//
//public:
//	AFixedWingPawn();
//
//	virtual void BeginPlay() override;
//	virtual void Tick(float DeltaSeconds) override;
//
//protected:
//	void SimulateFixedWing(float DeltaSeconds);
//
//	void ApplyPropulsion(float AirDensity);
//	void ApplyWingHalf(bool bLeftWing, const FVector& LocalPositionCm, float AirDensity, const FVector& LocalVelocityMps, const FVector& LocalAngularRatesRad);
//	void ApplyHorizontalTail(float AirDensity, const FVector& LocalVelocityMps, const FVector& LocalAngularRatesRad);
//	void ApplyVerticalTail(float AirDensity, const FVector& LocalVelocityMps, const FVector& LocalAngularRatesRad);
//	void ApplyParasiteDrag(float AirDensity);
//
//	FVector GetLocalPointVelocityMps(const FVector& LocalVelocityMps, const FVector& LocalAngularRatesRad, const FVector& LocalPositionCm) const;
//
//	float GetAirDensity(float AltitudeM) const;
//	float GetDynamicPressure(float AirDensity, const FVector& LocalVelocityMps) const;
//	float GetStallFactor(float AngleDeg, float StallAngleDeg, float MaxAngleDeg) const;
//	float GetDragCoefficient(float BaseCD0, float LiftOrSideCoeff, float InducedK, float StallFactor, float ExtraStallDrag) const;
//
//	float GetWingLiftCoefficient(float AoADeg) const;
//	float GetTailLiftCoefficient(float AoADeg) const;
//	float GetFinSideforceCoefficient(float BetaDeg) const;
//
//protected:
//	UPROPERTY(EditAnywhere, Category = "UAV|FixedWing|Mass")
//	float AircraftMassKg = 8.0f;
//
//	UPROPERTY(EditAnywhere, Category = "UAV|FixedWing|Mass")
//	FVector CenterOfMassOffsetCm = FVector(0.f, 0.f, 0.f);
//
//	UPROPERTY(EditAnywhere, Category = "UAV|FixedWing|Propulsion")
//	float MaxEngineThrustN = 25.f;
//
//	UPROPERTY(EditAnywhere, Category = "UAV|FixedWing|Wing")
//	float WingAreaM2 = 0.65f;
//
//	UPROPERTY(EditAnywhere, Category = "UAV|FixedWing|Wing")
//	float WingSpanM = 2.2f;
//
//	UPROPERTY(EditAnywhere, Category = "UAV|FixedWing|Wing")
//	float WingIncidenceDeg = 1.0f;
//
//	UPROPERTY(EditAnywhere, Category = "UAV|FixedWing|Wing")
//	float WingZeroLiftAoADeg = -2.f;
//
//	UPROPERTY(EditAnywhere, Category = "UAV|FixedWing|Wing")
//	float WingLiftSlopePerRad = 4.5f;
//
//	UPROPERTY(EditAnywhere, Category = "UAV|FixedWing|Wing")
//	float WingCLMax = 1.2f;
//
//	UPROPERTY(EditAnywhere, Category = "UAV|FixedWing|Wing")
//	float WingStallAoADeg = 14.f;
//
//	UPROPERTY(EditAnywhere, Category = "UAV|FixedWing|Wing")
//	float WingMaxAoADeg = 24.f;
//
//	UPROPERTY(EditAnywhere, Category = "UAV|FixedWing|Wing")
//	float WingCD0 = 0.028f;
//
//	UPROPERTY(EditAnywhere, Category = "UAV|FixedWing|Wing")
//	float WingInducedDragK = 0.07f;
//
//	UPROPERTY(EditAnywhere, Category = "UAV|FixedWing|Wing")
//	float WingStallDragAdd = 0.65f;
//
//	UPROPERTY(EditAnywhere, Category = "UAV|FixedWing|Wing")
//	float AileronLiftDelta = 0.35f;
//
//	// temp for roll recovery
//	UPROPERTY(EditAnywhere, Category = "UAV|FixedWing|Wing")
//	float WingDihedralDeg = 2.f; // кут поперечного V крила
//
//	UPROPERTY(EditAnywhere, Category = "UAV|FixedWing|Wing")
//	float DihedralEffectCoeff = 0.1f;
//
//	UPROPERTY(EditAnywhere, Category = "UAV|FixedWing|Wing")
//	float RollDampingCoeff = 0.6f;
//	//
//
//	UPROPERTY(EditAnywhere, Category = "UAV|FixedWing|Wing")
//	FVector LeftWingLocalPos = FVector(0.f, -110.f, 0.f);
//
//	UPROPERTY(EditAnywhere, Category = "UAV|FixedWing|Wing")
//	FVector RightWingLocalPos = FVector(0.f, 110.f, 0.f);
//
//	UPROPERTY(EditAnywhere, Category = "UAV|FixedWing|Tail")
//	float HorizontalTailAreaM2 = 0.09f;
//
//	UPROPERTY(EditAnywhere, Category = "UAV|FixedWing|Tail")
//	float HorizontalTailIncidenceDeg = 0.5f;
//
//	UPROPERTY(EditAnywhere, Category = "UAV|FixedWing|Tail")
//	float HorizontalTailZeroLiftAoADeg = 0.f;
//
//	UPROPERTY(EditAnywhere, Category = "UAV|FixedWing|Tail")
//	float HorizontalTailLiftSlopePerRad = 4.0f;
//
//	UPROPERTY(EditAnywhere, Category = "UAV|FixedWing|Tail")
//	float HorizontalTailCLMax = 1.1f;
//
//	UPROPERTY(EditAnywhere, Category = "UAV|FixedWing|Tail")
//	float HorizontalTailStallAoADeg = 18.f;
//
//	UPROPERTY(EditAnywhere, Category = "UAV|FixedWing|Tail")
//	float HorizontalTailMaxAoADeg = 30.f;
//
//	UPROPERTY(EditAnywhere, Category = "UAV|FixedWing|Tail")
//	float HorizontalTailCD0 = 0.02f;
//
//	UPROPERTY(EditAnywhere, Category = "UAV|FixedWing|Tail")
//	float HorizontalTailInducedDragK = 0.04f;
//
//	UPROPERTY(EditAnywhere, Category = "UAV|FixedWing|Tail")
//	float HorizontalTailStallDragAdd = 0.25f;
//
//	UPROPERTY(EditAnywhere, Category = "UAV|FixedWing|Tail")
//	float ElevatorAoADeltaDeg = 6.f;
//
//	UPROPERTY(EditAnywhere, Category = "UAV|FixedWing|Tail")
//	FVector HorizontalTailLocalPos = FVector(-95.f, 0.f, 0.f);
//
//	UPROPERTY(EditAnywhere, Category = "UAV|FixedWing|Fin")
//	float VerticalTailAreaM2 = 0.12f;
//
//	UPROPERTY(EditAnywhere, Category = "UAV|FixedWing|Fin")
//	float VerticalTailLiftSlopePerRad = 3.5f;
//
//	UPROPERTY(EditAnywhere, Category = "UAV|FixedWing|Fin")
//	float VerticalTailCYMax = 1.0f;
//
//	UPROPERTY(EditAnywhere, Category = "UAV|FixedWing|Fin")
//	float VerticalTailStallBetaDeg = 20.f;
//
//	UPROPERTY(EditAnywhere, Category = "UAV|FixedWing|Fin")
//	float VerticalTailMaxBetaDeg = 35.f;
//
//	UPROPERTY(EditAnywhere, Category = "UAV|FixedWing|Fin")
//	float VerticalTailCD0 = 0.02f;
//
//	UPROPERTY(EditAnywhere, Category = "UAV|FixedWing|Fin")
//	float VerticalTailInducedDragK = 0.05f;
//
//	UPROPERTY(EditAnywhere, Category = "UAV|FixedWing|Fin")
//	float VerticalTailStallDragAdd = 0.25f;
//
//	UPROPERTY(EditAnywhere, Category = "UAV|FixedWing|Fin")
//	float RudderBetaDeltaDeg = 14.f;
//
//	UPROPERTY(EditAnywhere, Category = "UAV|FixedWing|Fin")
//	FVector VerticalTailLocalPos = FVector(-100.f, 0.f, 25.f);
//
//	UPROPERTY(EditAnywhere, Category = "UAV|FixedWing|Body")
//	float FuselageEquivalentAreaM2 = 0.08f;
//
//	UPROPERTY(EditAnywhere, Category = "UAV|FixedWing|Body")
//	float FuselageCD0 = 0.08f;
//
//	//temp
//	bool bDropTestStarted = false;
//	bool bDropTestLogged = false;
//	float DropTestStartTime = 0.f;
//	float DropTestStartAltitudeM = 0.f;
//};