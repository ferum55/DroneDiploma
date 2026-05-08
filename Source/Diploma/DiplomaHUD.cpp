#include "DiplomaHUD.h"
#include "FPVDronePawn.h"
#include "FPVHUDWidget.h"

void ADiplomaHUD::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PC = GetOwningPlayerController();
	if (!PC || !FPVHUDWidgetClass)
	{
		return;
	}

	FPVHUDWidget = CreateWidget<UFPVHUDWidget>(PC, FPVHUDWidgetClass);
	if (FPVHUDWidget)
	{
		FPVHUDWidget->AddToViewport();
	}
}

void ADiplomaHUD::DrawHUD()
{
	Super::DrawHUD();

	AFPVDronePawn* Pawn = Cast<AFPVDronePawn>(GetOwningPawn());
	if (!Pawn || !FPVHUDWidget)
	{
		return;
	}

	const bool bHideFPVHUD = Pawn->IsKillCamActive() || Pawn->IsCrashed();

	FPVHUDWidget->SetVisibility(
		bHideFPVHUD ? ESlateVisibility::Hidden : ESlateVisibility::Visible
	);

	if (bHideFPVHUD)
	{
		return;
	}

	FPVHUDWidget->ApplyTelemetry(Pawn->GetTelemetry());
}

//void ADiplomaHUD::DrawHUD()
//{
//	Super::DrawHUD();
//
//	AFPVDronePawn* Pawn = Cast<AFPVDronePawn>(GetOwningPawn());
//	if (!Pawn || !FPVHUDWidget)
//	{
//		return;
//	}
//
//	FPVHUDWidget->ApplyTelemetry(Pawn->GetTelemetry());
//}