#include "DiplomaHUD.h"
#include "DiplomaPawn.h"
#include "DroneTelemetryWidget.h"
#include "Engine/Canvas.h"
#include "Blueprint/UserWidget.h"

void ADiplomaHUD::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PC = GetOwningPlayerController();
	if (!PC)
	{
		return;
	}

	if (TelemetryWidgetClass)
	{
		TelemetryWidget = CreateWidget<UDroneTelemetryWidget>(PC, TelemetryWidgetClass);
		if (TelemetryWidget)
		{
			TelemetryWidget->AddToViewport();
		}
	}

	if (FPVHUDWidgetClass)
	{
		FPVHUDWidget = CreateWidget<UUserWidget>(PC, FPVHUDWidgetClass);
		if (FPVHUDWidget)
		{
			FPVHUDWidget->AddToViewport();
		}
	}
}
void ADiplomaHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas)
	{
		return;
	}

	/*const float CX = Canvas->ClipX * 0.5f;
	const float CY = Canvas->ClipY * 0.5f;

	const float H = Canvas->ClipY;
	const float Half = H * 0.5f - 10.f;
	const float C = 40.f;

	DrawLine(CX - Half + C, CY - Half, CX + Half - C, CY - Half, FLinearColor::White, LineThickness);
	DrawLine(CX - Half + C, CY + Half, CX + Half - C, CY + Half, FLinearColor::White, LineThickness);
	DrawLine(CX - Half, CY - Half + C, CX - Half, CY + Half - C, FLinearColor::White, LineThickness);
	DrawLine(CX + Half, CY - Half + C, CX + Half, CY + Half - C, FLinearColor::White, LineThickness);

	

	const float DotX = CX + FMath::Clamp(Pawn->GetRollInput(), -1.f, 1.f) * Half;
	const float DotY = CY + FMath::Clamp(Pawn->GetPitchInput(), -1.f, 1.f) * Half;

	DrawRect(
		FLinearColor::White,
		DotX - DotSize * 0.5f,
		DotY - DotSize * 0.5f,
		DotSize,
		DotSize
	);*/

	ADiplomaPawn* Pawn = Cast<ADiplomaPawn>(GetOwningPawn());
	if (!Pawn)
	{
		return;
	}

	if (TelemetryWidget)
	{
		TelemetryWidget->ApplyTelemetry(Pawn->GetTelemetry());

		TArray<float> AxisValues;
		AxisValues.Add(Pawn->GetTelemetry().Throttle01);
		AxisValues.Add((Pawn->GetPitchInput() + 1.f) * 0.5f);
		AxisValues.Add((Pawn->GetRollInput() + 1.f) * 0.5f);
		AxisValues.Add((Pawn->GetYawInput() + 1.f) * 0.5f);

		TelemetryWidget->ApplyAxisValues(AxisValues);

		const float RawThrottle = Pawn->GetInputAxisValue(TEXT("TestAxis4"));
		TelemetryWidget->ApplyRawThrottle(RawThrottle);
	}
}
