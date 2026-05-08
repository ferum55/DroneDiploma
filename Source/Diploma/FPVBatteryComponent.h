#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FPVBatteryComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DIPLOMA_API UFPVBatteryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFPVBatteryComponent();

	float GetBenchReferenceVoltage() const { return BatteryBenchReferenceVoltage; }

	void ResetBatteryState();
	void UpdateBatteryState(float TotalCurrentA, float DeltaTime);

	float GetCapacityAh() const;
	float GetUsableCapacityAh() const;
	float GetInternalResistanceOhm() const;

	float GetLoadedVoltage() const { return BatteryLoadedVoltage; }
	float GetOpenCircuitVoltage() const { return BatteryOpenCircuitVoltage; }
	float GetConsumedAh() const { return BatteryConsumedAh; }
	float GetTotalCurrentA() const { return BatteryTotalCurrentA; }
	float GetSoC() const { return BatterySoC; }
	float GetOutputScale() const { return BatteryOutputScale; }
	float GetResistanceScale() const { return BatteryResistanceScale; }

	float GetCellVoltage() const;
	bool IsLowVoltageWarn() const { return bBatteryLowVoltageWarn; }
	bool IsCriticalVoltage() const { return bBatteryCriticalVoltage; }
	bool IsCutoffActive() const { return bBatteryCutoffActive; }

private:
	UPROPERTY(EditAnywhere, Category = "FPV|Battery")
	int32 BatterySeriesCells = 6;

	UPROPERTY(EditAnywhere, Category = "FPV|Battery")
	int32 BatteryParallelCells = 3;

	UPROPERTY(EditAnywhere, Category = "FPV|Battery")
	float BatteryCellCapacityAh = 5.0f;

	UPROPERTY(EditAnywhere, Category = "FPV|Battery")
	float BatteryCellInternalResistanceOhm = 0.005f;

	UPROPERTY(EditAnywhere, Category = "FPV|Battery")
	float BatteryCellVoltageFull = 4.2f;

	UPROPERTY(EditAnywhere, Category = "FPV|Battery")
	float BatteryCellVoltageNominal = 3.6f;

	UPROPERTY(EditAnywhere, Category = "FPV|Battery")
	float BatteryCellVoltageEmpty = 3.0f;

	UPROPERTY(EditAnywhere, Category = "FPV|Battery")
	float BatteryUsableFraction = 0.85f;

	UPROPERTY(EditAnywhere, Category = "FPV|Battery")
	float BatteryBenchReferenceVoltage = 25.2f;

	UPROPERTY(EditAnywhere, Category = "FPV|Battery")
	float BatteryPackMassKg = 1.28f;

	UPROPERTY(EditAnywhere, Category = "FPV|Battery")
	float BatteryCellVoltageWarn = 3.30f;

	UPROPERTY(EditAnywhere, Category = "FPV|Battery")
	float BatteryCellVoltageCritical = 3.10f;

	UPROPERTY(EditAnywhere, Category = "FPV|Battery")
	float BatteryCellVoltageCutoff = 2.95f;

	float BatteryConsumedAh = 0.f;
	float BatterySoC = 1.f;
	float BatteryOpenCircuitVoltage = 25.2f;
	float BatteryLoadedVoltage = 25.2f;
	float BatteryTotalCurrentA = 0.f;
	float BatteryResistanceScale = 1.f;
	float BatteryOutputScale = 1.f;

	bool bBatteryLowVoltageWarn = false;
	bool bBatteryCriticalVoltage = false;
	bool bBatteryCutoffActive = false;

	float EvaluateCellOCVFromSoC(float SoC) const;
	float EvaluateResistanceScaleFromSoC(float SoC) const;
	float EvaluateOutputScaleFromCellVoltage(float CellLoadedVoltage) const;
};