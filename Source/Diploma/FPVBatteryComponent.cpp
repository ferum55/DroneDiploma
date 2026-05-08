#include "FPVBatteryComponent.h"

UFPVBatteryComponent::UFPVBatteryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFPVBatteryComponent::ResetBatteryState()
{
	BatteryConsumedAh = 0.f;
	BatterySoC = 1.f;
	BatteryOpenCircuitVoltage = BatterySeriesCells * BatteryCellVoltageFull;
	BatteryLoadedVoltage = BatteryOpenCircuitVoltage;
	BatteryTotalCurrentA = 0.f;
	BatteryResistanceScale = 1.f;
	BatteryOutputScale = 1.f;
	bBatteryLowVoltageWarn = false;
	bBatteryCriticalVoltage = false;
	bBatteryCutoffActive = false;
}

void UFPVBatteryComponent::UpdateBatteryState(float TotalCurrentA, float DeltaTime)
{
	BatteryTotalCurrentA = FMath::Max(TotalCurrentA, 0.f);

	BatteryConsumedAh = FMath::Clamp(
		BatteryConsumedAh + BatteryTotalCurrentA * DeltaTime / 3600.f,
		0.f,
		GetUsableCapacityAh()
	);

	BatterySoC = 1.f - BatteryConsumedAh / FMath::Max(GetUsableCapacityAh(), KINDA_SMALL_NUMBER);

	BatteryOpenCircuitVoltage = EvaluateCellOCVFromSoC(BatterySoC) * BatterySeriesCells;

	BatteryResistanceScale = EvaluateResistanceScaleFromSoC(BatterySoC);
	const float EffectiveResistanceOhm = GetInternalResistanceOhm() * BatteryResistanceScale;

	const float SagVoltage = BatteryTotalCurrentA * EffectiveResistanceOhm;
	const float MinPackVoltage = BatterySeriesCells * BatteryCellVoltageCutoff;

	BatteryLoadedVoltage = FMath::Max(BatteryOpenCircuitVoltage - SagVoltage, MinPackVoltage);

	const float CellLoadedVoltage = GetCellVoltage();
	BatteryOutputScale = EvaluateOutputScaleFromCellVoltage(CellLoadedVoltage);

	bBatteryLowVoltageWarn = CellLoadedVoltage <= BatteryCellVoltageWarn;
	bBatteryCriticalVoltage = CellLoadedVoltage <= BatteryCellVoltageCritical;
	bBatteryCutoffActive = CellLoadedVoltage <= BatteryCellVoltageCutoff;

	static float BatteryAuditLogTimer = 0.f;
	BatteryAuditLogTimer += DeltaTime;

	if (BatteryAuditLogTimer >= 1.0f)
	{
		BatteryAuditLogTimer = 0.f;

		const float CellCurrentA = BatteryParallelCells > 0 ? BatteryTotalCurrentA / BatteryParallelCells : 0.f;
		const float RemainingAh = FMath::Max(GetUsableCapacityAh() - BatteryConsumedAh, 0.f);
		const float RemainingMinutes = BatteryTotalCurrentA > 0.1f ? RemainingAh / BatteryTotalCurrentA * 60.f : 0.f;
		const float PackCRate = GetCapacityAh() > 0.1f ? BatteryTotalCurrentA / GetCapacityAh() : 0.f;

		/*UE_LOG(LogTemp, Warning,
			TEXT("BATTERY_AUDIT | SoC=%.2f Consumed=%.2fAh Rem=%.2fAh RemTime=%.1fmin | V=%.2f Cell=%.2f OCV=%.2f | I=%.1fA CellI=%.1fA C=%.1f | RScale=%.2f Out=%.2f Warn=%d Crit=%d Cut=%d"),
			BatterySoC,
			BatteryConsumedAh,
			RemainingAh,
			RemainingMinutes,
			BatteryLoadedVoltage,
			CellLoadedVoltage,
			BatteryOpenCircuitVoltage,
			BatteryTotalCurrentA,
			CellCurrentA,
			PackCRate,
			BatteryResistanceScale,
			BatteryOutputScale,
			bBatteryLowVoltageWarn ? 1 : 0,
			bBatteryCriticalVoltage ? 1 : 0,
			bBatteryCutoffActive ? 1 : 0
		);*/
	}
}

float UFPVBatteryComponent::GetCapacityAh() const
{
	return BatteryParallelCells * BatteryCellCapacityAh;
}

float UFPVBatteryComponent::GetUsableCapacityAh() const
{
	return GetCapacityAh() * BatteryUsableFraction;
}

float UFPVBatteryComponent::GetInternalResistanceOhm() const
{
	return BatterySeriesCells * (BatteryCellInternalResistanceOhm / FMath::Max(BatteryParallelCells, 1));
}

float UFPVBatteryComponent::GetCellVoltage() const
{
	return BatterySeriesCells > 0 ? BatteryLoadedVoltage / BatterySeriesCells : 0.f;
}

float UFPVBatteryComponent::EvaluateCellOCVFromSoC(float SoC) const
{
	static const TArray<float> SocPts = { 0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f };
	static const TArray<float> VoltPts = { 3.00f, 3.40f, 3.55f, 3.65f, 3.70f, 3.75f, 3.80f, 3.87f, 3.95f, 4.08f, 4.20f };

	const float S = FMath::Clamp(SoC, 0.f, 1.f);

	if (S <= SocPts[0])
	{
		return VoltPts[0];
	}

	const int32 Last = SocPts.Num() - 1;
	if (S >= SocPts[Last])
	{
		return VoltPts[Last];
	}

	for (int32 i = 0; i < Last; ++i)
	{
		if (S >= SocPts[i] && S <= SocPts[i + 1])
		{
			const float Alpha = (S - SocPts[i]) / (SocPts[i + 1] - SocPts[i]);
			return FMath::Lerp(VoltPts[i], VoltPts[i + 1], Alpha);
		}
	}

	return VoltPts[Last];
}

float UFPVBatteryComponent::EvaluateResistanceScaleFromSoC(float SoC) const
{
	const float S = FMath::Clamp(SoC, 0.f, 1.f);

	if (S >= 0.30f)
	{
		return 1.f;
	}

	const float Alpha = S / 0.30f;
	return FMath::Lerp(1.6f, 1.f, Alpha);
}

float UFPVBatteryComponent::EvaluateOutputScaleFromCellVoltage(float CellLoadedVoltage) const
{
	if (CellLoadedVoltage <= BatteryCellVoltageCutoff)
	{
		return 0.f;
	}

	if (CellLoadedVoltage >= BatteryCellVoltageCritical)
	{
		return 1.f;
	}

	return FMath::Clamp(
		(CellLoadedVoltage - BatteryCellVoltageCutoff) /
		FMath::Max(BatteryCellVoltageCritical - BatteryCellVoltageCutoff, KINDA_SMALL_NUMBER),
		0.f,
		1.f
	);
}