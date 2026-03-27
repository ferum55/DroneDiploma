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