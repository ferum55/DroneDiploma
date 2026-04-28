#include "FPVHUDWidget.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Slate/WidgetTransform.h"

#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"

void UFPVHUDWidget::ApplyTelemetry(const FDroneTelemetry& InTelemetry)
{
	if (Text_PrimaryLink)
	{
		if (InTelemetry.bControlLinkValid)
		{
			Text_PrimaryLink->SetText(FText::FromString(
				FString::Printf(TEXT("%.0f %.0f"), InTelemetry.ControlRSSIPercent, InTelemetry.ControlLQPercent)
			));
		}
		else
		{
			Text_PrimaryLink->SetText(FText::FromString(TEXT("-- --")));
		}
	}

	if (Text_VideoLink)
	{
		if (InTelemetry.bVideoLinkValid)
		{
			Text_VideoLink->SetText(FText::FromString(
				FString::Printf(TEXT("%.0f"), InTelemetry.VideoLinkPercent)
			));
		}
		else
		{
			Text_VideoLink->SetText(FText::FromString(TEXT("--")));
		}
	}

	if (Text_TxPower)
	{
		if (InTelemetry.bTxPowerValid)
		{
			Text_TxPower->SetText(FText::FromString(
				FString::Printf(TEXT("%.1f W"), InTelemetry.TxPowerW)
			));
		}
		else
		{
			Text_TxPower->SetText(FText::GetEmpty());
		}
	}

	if (Text_Armed)
	{
		Text_Armed->SetText(FText::FromString(InTelemetry.bArmed ? TEXT("ARMED") : TEXT("DISARMED")));
	}

	if (Text_Bomb)
	{
		Text_Bomb->SetText(FText::FromString(InTelemetry.bBombArmed ? TEXT("BOMB") : TEXT("SAFE")));
	}

	if (Text_CellVoltage)
	{
		if (InTelemetry.bBatteryValid)
		{
			Text_CellVoltage->SetText(FText::FromString(
				FString::Printf(TEXT("%.1f V"), InTelemetry.CellVoltage)
			));
		}
		else
		{
			Text_CellVoltage->SetText(FText::GetEmpty());
		}
	}

	if (Text_PackVoltage)
	{
		if (InTelemetry.bBatteryValid)
		{
			Text_PackVoltage->SetText(FText::FromString(
				FString::Printf(TEXT("%.1f V"), InTelemetry.PackVoltage)
			));
		}
		else
		{
			Text_PackVoltage->SetText(FText::GetEmpty());
		}
	}

	if (Text_ConsumedMah)
	{
		if (InTelemetry.bBatteryValid)
		{
			Text_ConsumedMah->SetText(FText::FromString(
				FString::Printf(TEXT("%.0f mAh"), InTelemetry.ConsumedMah)
			));
		}
		else
		{
			Text_ConsumedMah->SetText(FText::GetEmpty());
		}
	}

	if (Text_CurrentAmp)
	{
		if (InTelemetry.bBatteryValid)
		{
			Text_CurrentAmp->SetText(FText::FromString(
				FString::Printf(TEXT("%.2f A"), InTelemetry.CurrentAmp)
			));
		}
		else
		{
			Text_CurrentAmp->SetText(FText::GetEmpty());
		}
	}

	if (Text_FlightMode)
	{
		Text_FlightMode->SetText(FText::FromString(InTelemetry.FlightMode));
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

	if (Text_Throttle)
	{
		Text_Throttle->SetText(FText::FromString(
			FString::Printf(TEXT("%.0f%%"), InTelemetry.ThrottlePercent)
		));
	}

	if (Text_Timer)
	{
		const int32 TotalSeconds = FMath::Max(0, FMath::FloorToInt(InTelemetry.FlightTimeSeconds));
		const int32 Minutes = TotalSeconds / 60;
		const int32 Seconds = TotalSeconds % 60;

		Text_Timer->SetText(FText::FromString(
			FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds)
		));
	}

	if (Text_HeadingDegrees)
	{
		Text_HeadingDegrees->SetText(FText::FromString(
			FString::Printf(TEXT("%.0f"), InTelemetry.HeadingDeg)
		));
	}

	if (CompassMaterialInstance)
	{
		float Heading = FMath::Fmod(InTelemetry.HeadingDeg, 360.f);

		if (Heading < 0.f)
		{
			Heading += 360.f;
		}

		float Offset = Heading / 360.f;

		if (bInvertCompassDirection)
		{
			Offset = -Offset;
		}

		Offset += CompassZeroOffset;

		CompassMaterialInstance->SetScalarParameterValue(TEXT("CompassOffset"), Offset);
	}

	const float DisplayPitchDeg = InTelemetry.PitchDeg + CameraPitchOffsetDeg;

	if (Text_PitchArrow)
	{
		FString PitchArrow = TEXT("-");
		if (DisplayPitchDeg > 1.f)
		{
			PitchArrow = TEXT("?");
		}
		else if (DisplayPitchDeg < -1.f)
		{
			PitchArrow = TEXT("?");
		}

		Text_PitchArrow->SetText(FText::FromString(PitchArrow));
	}

	if (Text_PitchValue)
	{
		Text_PitchValue->SetText(FText::FromString(
			FString::Printf(TEXT("%.0f°"), FMath::Abs(DisplayPitchDeg))
		));
	}

	if (Canvas_HorizonRoot)
	{
		FWidgetTransform T = Canvas_HorizonRoot->RenderTransform;
		const float HorizonOffsetY = FMath::Clamp(
	-DisplayPitchDeg * PixelsPerPitchDegree,
	-180.f,
	180.f
);

T.Translation = FVector2D(0.f, HorizonOffsetY);
		T.Angle = -InTelemetry.RollDeg;
		Canvas_HorizonRoot->SetRenderTransform(T);
	}
}

void UFPVHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Image_CompassStrip)
	{
		UObject* Resource = Image_CompassStrip->Brush.GetResourceObject();
		UMaterialInterface* Material = Cast<UMaterialInterface>(Resource);

		if (Material)
		{
			CompassMaterialInstance = UMaterialInstanceDynamic::Create(Material, this);
			Image_CompassStrip->SetBrushFromMaterial(CompassMaterialInstance);
		}
	}
}