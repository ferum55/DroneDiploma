#include "DiplomaHUD.h"
#include "DiplomaPawn.h"
#include "DroneTelemetryWidget.h"
#include "Engine/Canvas.h"
#include "Blueprint/UserWidget.h"

void ADiplomaHUD::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PC = GetOwningPlayerController();
	if (!PC || !TelemetryWidgetClass)
	{
		return;
	}

	TelemetryWidget = CreateWidget<UDroneTelemetryWidget>(PC, TelemetryWidgetClass);
	if (TelemetryWidget)
	{
		TelemetryWidget->AddToViewport();
	}
}
void ADiplomaHUD::DrawHUD()
{
	Super::DrawHUD();


	if (!Canvas) {
		return;
	}
	/*else {
		DrawText(TEXT("HUD WORKS"), FLinearColor::Red, 50.f, 50.f, nullptr, 2.f, false);
	}*/

	const float CX = Canvas->ClipX * 0.5f;
	const float CY = Canvas->ClipY * 0.5f;

	const float H = Canvas->ClipY;
	const float Half = H * 0.5f - 10.f;
	const float C = 40.f; // corner gap

	// Верхня лінія
	DrawLine(CX - Half + C, CY - Half, CX + Half - C, CY - Half, FLinearColor::White, LineThickness);
	// Нижня лінія
	DrawLine(CX - Half + C, CY + Half, CX + Half - C, CY + Half, FLinearColor::White, LineThickness);
	// Ліва лінія
	DrawLine(CX - Half, CY - Half + C, CX - Half, CY + Half - C, FLinearColor::White, LineThickness);
	// Права лінія
	DrawLine(CX + Half, CY - Half + C, CX + Half, CY + Half - C, FLinearColor::White, LineThickness);

	ADiplomaPawn* Pawn = Cast<ADiplomaPawn>(GetOwningPawn());
	if (!Pawn) return;

	const float DotX = CX + FMath::Clamp(Pawn->GetRollInput(), -1.f, 1.f) * Half;
	const float DotY = CY + FMath::Clamp(Pawn->GetPitchInput(), -1.f, 1.f) * Half;

	DrawRect(
		FLinearColor::White,
		DotX - DotSize * 0.5f,
		DotY - DotSize * 0.5f,
		DotSize,
		DotSize
	);

	if (TelemetryWidget)
	{
		TelemetryWidget->ApplyTelemetry(Pawn->GetTelemetry());
	}
}
