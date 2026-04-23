#include "FPVHUDWidget.h"
#include "Components/TextBlock.h"

void UFPVHUDWidget::ApplyTelemetry(const FDroneTelemetry& InTelemetry)
{
	if (Text_Throttle)
	{
		Text_Throttle->SetText(FText::FromString(
			FString::Printf(TEXT("%.0f%%"), InTelemetry.ThrottlePercent)
		));
	}

	if (Text_Altitude)
	{
		const float DisplayAltitude = InTelemetry.bRadioAltitudeValid
			? InTelemetry.RadioAltitudeM
			: InTelemetry.RelativeAltitudeM;

		Text_Altitude->SetText(FText::FromString(
			FString::Printf(TEXT("%.0f M"), DisplayAltitude)
		));
	}
}

void UFPVHUDWidget::SetFlightModeText(const FString& InText)
{
	if (Text_FlightMode)
	{
		Text_FlightMode->SetText(FText::FromString(InText));
	}
}

void UFPVHUDWidget::SetPrimaryLinkText(const FString& InText)
{
	if (Text_PrimaryLink)
	{
		Text_PrimaryLink->SetText(FText::FromString(InText));
	}
}

void UFPVHUDWidget::SetVideoLinkText(const FString& InText)
{
	if (Text_VideoLink)
	{
		Text_VideoLink->SetText(FText::FromString(InText));
	}
}

void UFPVHUDWidget::SetTxPowerText(const FString& InText)
{
	if (Text_TxPower)
	{
		Text_TxPower->SetText(FText::FromString(InText));
	}
}

void UFPVHUDWidget::SetArmedText(const FString& InText)
{
	if (Text_Armed)
	{
		Text_Armed->SetText(FText::FromString(InText));
	}
}

void UFPVHUDWidget::SetBombText(const FString& InText)
{
	if (Text_Bomb)
	{
		Text_Bomb->SetText(FText::FromString(InText));
	}
}

void UFPVHUDWidget::SetHeadingText(const FString& InText)
{
	if (Text_HeadingDegrees)
	{
		Text_HeadingDegrees->SetText(FText::FromString(InText));
	}
}

void UFPVHUDWidget::SetCompassText(const FString& InText)
{
	if (Text_CompassCardinals)
	{
		Text_CompassCardinals->SetText(FText::FromString(InText));
	}
}

void UFPVHUDWidget::SetTimerText(const FString& InText)
{
	if (Text_Timer)
	{
		Text_Timer->SetText(FText::FromString(InText));
	}
}

void UFPVHUDWidget::SetPitchCue(const FString& ArrowText, const FString& ValueText)
{
	if (Text_PitchArrow)
	{
		Text_PitchArrow->SetText(FText::FromString(ArrowText));
	}

	if (Text_PitchValue)
	{
		Text_PitchValue->SetText(FText::FromString(ValueText));
	}
}