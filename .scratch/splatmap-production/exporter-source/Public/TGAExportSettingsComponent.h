// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TGAExportSettingsComponent.generated.h"

class ALandscapeProxy;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TGAUNREALPLUGIN_API UTGAExportSettingsComponent : public UActorComponent
{
	GENERATED_BODY()
public:

	UPROPERTY(EditAnywhere, Category = "The Game Assembly Exporter", meta = (ToolTip = "If True the owning Actor and its components will not be exported."))
	bool bDoNotExport;

	// A runtime terrain mesh or ground floater explicitly selects its paint source.
	UPROPERTY(EditAnywhere, Category = "The Game Assembly Exporter|Landscape", meta = (ToolTip = "Use this Landscape's paint and material on the owning actor's static meshes during normal TGA export."))
	TObjectPtr<ALandscapeProxy> SourceLandscape = nullptr;

	UPROPERTY(EditAnywhere, Category = "The Game Assembly Exporter|Landscape", meta = (ClampMin = "0.01", EditCondition = "SourceLandscape != nullptr"))
	float SurfaceRepeatMeters = 8.0f;

	UPROPERTY(EditAnywhere, Category = "The Game Assembly Exporter|Landscape", meta = (ClampMin = "0", ClampMax = "1", EditCondition = "SourceLandscape != nullptr"))
	float LandscapeNoiseStrength = 0.0f;

	UPROPERTY(EditAnywhere, Category = "The Game Assembly Exporter|Landscape", meta = (ClampMin = "1", EditCondition = "SourceLandscape != nullptr"))
	float LandscapeNoisePatchSizeCm = 200.0f;

public:	
	// Sets default values for this component's properties
	UTGAExportSettingsComponent();
};
