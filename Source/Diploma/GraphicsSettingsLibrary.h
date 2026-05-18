#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GraphicsSettingsLibrary.generated.h"

UCLASS()
class DIPLOMA_API UGraphicsSettingsLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Graphics Settings")
	static TArray<FString> GetAvailableScreenResolutions();

	UFUNCTION(BlueprintCallable, Category = "Graphics Settings")
	static TArray<FString> GetAvailableScreenResolutionsByAspect(const FString& AspectOption);

	UFUNCTION(BlueprintCallable, Category = "Graphics Settings")
	static FString GetAspectFromResolutionString(const FString& ResolutionOption);

	UFUNCTION(BlueprintCallable, Category = "Graphics Settings")
	static FString GetCurrentResolutionOption();

	UFUNCTION(BlueprintCallable, Category = "Graphics Settings")
	static FString GetCurrentDisplayModeOption();

	UFUNCTION(BlueprintCallable, Category = "Graphics Settings")
	static FString GetCurrentQualityOption();

	UFUNCTION(BlueprintCallable, Category = "Graphics Settings")
	static void ApplyGraphicsSettings(const FString& ResolutionOption, const FString& DisplayModeOption, const FString& QualityOption, bool bSaveSettings);

	UFUNCTION(BlueprintCallable, Category = "Graphics Settings")
	static void ResetGraphicsSettings();
};