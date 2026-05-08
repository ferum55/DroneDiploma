#pragma once

#include "CoreMinimal.h"
#include "DroneTelemetry.generated.h"

USTRUCT(BlueprintType)
struct FDroneTelemetry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	float Throttle01 = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float ThrottlePercent = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float SpeedMps = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float SpeedKmh = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float VerticalSpeedMps = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float BaroAltitudeM = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float RelativeAltitudeM = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float RadioAltitudeM = 0.f;

	UPROPERTY(BlueprintReadOnly)
	bool bRadioAltitudeValid = false;

	UPROPERTY(BlueprintReadOnly)
	float PitchDeg = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float RollDeg = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float YawDeg = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float HeadingDeg = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float FlightTimeSeconds = 0.f;

	UPROPERTY(BlueprintReadOnly)
	bool bArmed = false;

	UPROPERTY(BlueprintReadOnly)
	FString FlightMode;

	UPROPERTY(BlueprintReadOnly)
	float Battery01 = 1.f;

	UPROPERTY(BlueprintReadOnly)
	float PackVoltage = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float CellVoltage = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float ConsumedMah = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float CurrentAmp = 0.f;

	UPROPERTY(BlueprintReadOnly)
	bool bBatteryValid = false;

	UPROPERTY(BlueprintReadOnly)
	float PrimaryLinkPercent = 0.f;

	UPROPERTY(BlueprintReadOnly)
	bool bPrimaryLinkValid = false;

	UPROPERTY(BlueprintReadOnly)
	float VideoLinkPercent = 0.f;

	UPROPERTY(BlueprintReadOnly)
	bool bVideoLinkValid = false;

	UPROPERTY(BlueprintReadOnly)
	float TxPowerW = 0.f;

	UPROPERTY(BlueprintReadOnly)
	bool bTxPowerValid = false;

	UPROPERTY(BlueprintReadOnly)
	bool bBombArmed = false;

	UPROPERTY(BlueprintReadOnly)
	float ControlRSSIPercent = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float ControlLQPercent = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float ControlRSSIDbm = -130.f;

	UPROPERTY(BlueprintReadOnly)
	float ControlRSSIBarPercent = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float VideoRSSIDbm = -130.f;

	UPROPERTY(BlueprintReadOnly)
	float ControlSignalMarginDb = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float VideoSignalMarginDb = 0.f;

	UPROPERTY(BlueprintReadOnly)
	bool bControlLinkValid = false;

	UPROPERTY(BlueprintReadOnly)
	float ControlInputScale = 1.f;

	UPROPERTY(BlueprintReadOnly)
	bool bControlFailsafeActive = false;

	UPROPERTY(BlueprintReadOnly)
	bool bKillCamActive = false;

	UPROPERTY(BlueprintReadOnly)
	bool bCrashed = false;
};