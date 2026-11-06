#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

class ALandscapeProxy;

// Channels are canonical named-layer weights: Dunes, Rough, Cliff, unused.
// Capturing source allocation and composing the grid are separate so the same
// exact-byte compositor can be exercised without editor assets or saved maps.
struct FTGALandscapePaintTile
{
	FString Name;
	FIntPoint SectionBase = FIntPoint::ZeroValue;
	int32 Quads = 63;
	TArray<FColor> Weights;
};

struct FTGALandscapeAtlas
{
	FString Error;
	int32 Width = 0;
	int32 Height = 0;
	TArray<FColor> Weights;
	FVector4d MaskU;
	FVector4d MaskV;

	bool IsValid() const { return Error.IsEmpty() && !Weights.IsEmpty(); }
};

struct FTGALandscapeMaterialExport
{
	FString Error;
	FString AtlasName;
	FString MaterialName;
	TSharedPtr<FJsonObject> Material;

	bool IsValid() const { return Error.IsEmpty() && Material.IsValid(); }
};

namespace TGALandscapeExport
{
	FTGALandscapeAtlas ComposeAtlas(const TArray<FTGALandscapePaintTile>& Tiles, const FTransform& Transform);
	FTGALandscapeAtlas CaptureAtlas(ALandscapeProxy* Landscape);
	FTGALandscapeMaterialExport ExportMaterial(ALandscapeProxy* Landscape, const FString& SavePath,
		float SurfaceRepeatMeters, float NoiseStrength, float NoisePatchSizeCm);
}
