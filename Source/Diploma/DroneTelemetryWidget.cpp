#include "DroneTelemetryWidget.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"

void UDroneTelemetryWidget::ApplyTelemetry(const FDroneTelemetry& InTelemetry)
{
	
	if (TextSpeed)
	{
		TextSpeed->SetText(FText::FromString(
			FString::Printf(TEXT("SPD %.1f km/h"), InTelemetry.SpeedKmh)
		));
	}

	if (TextThrottle)
	{
		TextThrottle->SetText(FText::FromString(
			FString::Printf(TEXT("THR %.0f%%"), InTelemetry.ThrottlePercent)
		));
	}

	if (TextBaroAlt)
	{
		TextBaroAlt->SetText(FText::FromString(
			FString::Printf(TEXT("BARO %.1f m"), InTelemetry.RelativeAltitudeM)
		));
	}

	if (TextRadioAlt)
	{
		if (InTelemetry.bRadioAltitudeValid)
		{
			TextRadioAlt->SetText(FText::FromString(
				FString::Printf(TEXT("RAD %.1f m"), InTelemetry.RadioAltitudeM)
			));
		}
		else
		{
			TextRadioAlt->SetText(FText::FromString(TEXT("RAD ---")));
		}
	}

	if (TextVerticalSpeed)
	{
		TextVerticalSpeed->SetText(FText::FromString(
			FString::Printf(TEXT("V/S %.1f m/s"), InTelemetry.VerticalSpeedMps)
		));
	}

	if (BarThrottle)
	{
		BarThrottle->SetPercent(InTelemetry.Throttle01);
	}
}

void UDroneTelemetryWidget::ApplyAxisValues(const TArray<float>& AxisValues)
{
	// AxisValues[0] = NormThrottle (вже [0..1])
	// AxisValues[1] = NormPitch   ([-1..1])
	// AxisValues[2] = NormRoll    ([-1..1])
	// AxisValues[3] = NormYaw     ([-1..1])

	if (BarThrottle && AxisValues.IsValidIndex(0))
	{
		BarThrottle->SetPercent(FMath::Clamp(AxisValues[0], 0.f, 1.f));
	}
	if (BarPitch && AxisValues.IsValidIndex(1))
	{
		// Конвертуємо [-1..1] ? [0..1] для прогрес бара
		BarPitch->SetPercent(FMath::Clamp(AxisValues[1] * 0.5f + 0.5f, 0.f, 1.f));
	}
	if (BarRoll && AxisValues.IsValidIndex(2))
	{
		BarRoll->SetPercent(FMath::Clamp(AxisValues[2] * 0.5f + 0.5f, 0.f, 1.f));
	}
	if (BarYaw && AxisValues.IsValidIndex(3))
	{
		BarYaw->SetPercent(FMath::Clamp(AxisValues[3] * 0.5f + 0.5f, 0.f, 1.f));
	}
}

//temp
void UDroneTelemetryWidget::ApplyRawThrottle(float RawThrottle)
{
	if (TextThrottleRaw)
	{
		TextThrottleRaw->SetText(FText::FromString(
			FString::Printf(TEXT("Throttle(Raw): %.3f"), RawThrottle)
		));
	}
}