#include "TGALandscapeExport.h"

#include "DDSFile.h"
#include "Engine/Texture2D.h"
#include "LandscapeComponent.h"
#include "LandscapeLayerInfoObject.h"
#include "LandscapeProxy.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	FTGALandscapeAtlas Failure(const FString& Message)
	{
		FTGALandscapeAtlas Result;
		Result.Error = Message;
		return Result;
	}

	TSharedPtr<FJsonValue> Parameter(const FString& Name, int32 Type, TSharedPtr<FJsonValue> Value)
	{
		TSharedPtr<FJsonObject> Json = MakeShared<FJsonObject>();
		Json->SetStringField(TEXT("Name"), Name);
		Json->SetNumberField(TEXT("Type"), Type);
		Json->SetField(TEXT("Value"), Value);
		return MakeShared<FJsonValueObject>(Json);
	}

	TSharedPtr<FJsonValue> VectorParameter(const FString& Name, const FVector4d& Value)
	{
		TArray<TSharedPtr<FJsonValue>> Values;
		for (double Number : {Value.X, Value.Y, Value.Z, Value.W})
		{
			Values.Add(MakeShared<FJsonValueNumber>(Number));
		}
		return Parameter(Name, 1, MakeShared<FJsonValueArray>(Values));
	}

	TSharedPtr<FJsonValue> TextureParameter(const FString& Role, const FString& Name)
	{
		TSharedPtr<FJsonObject> Value = MakeShared<FJsonObject>();
		Value->SetStringField(TEXT("Name"), Name);
		Value->SetStringField(TEXT("Path"), Name);
		Value->SetStringField(TEXT("AssetImportPath"), Name + TEXT(".dds"));
		return Parameter(Role, 3, MakeShared<FJsonValueObject>(Value));
	}

	// Average each linear byte independently, including alpha. Base-level weights
	// stay exact; no sRGB conversion or normalized-weight filtering is involved.
	bool WriteAtlas(const FTGALandscapeAtlas& Atlas, const FString& Filename)
	{
		using namespace UE::DDS;
		uint32 MipCount = 1;
		for (int32 Size = FMath::Max(Atlas.Width, Atlas.Height); Size > 1; Size >>= 1)
		{
			++MipCount;
		}
		TUniquePtr<FDDSFile> DDS(FDDSFile::CreateEmpty2D(Atlas.Width, Atlas.Height, MipCount,
			EDXGIFormat::R8G8B8A8_UNORM, FDDSFile::CREATE_FLAG_NONE));
		if (!DDS)
		{
			return false;
		}
		int32 Width = Atlas.Width;
		int32 Height = Atlas.Height;
		TArray<FColor> Pixels = Atlas.Weights;
		for (uint32 Level = 0; Level < MipCount; ++Level)
		{
			uint8* Data = static_cast<uint8*>(DDS->Mips[Level].Data);
			for (int32 Index = 0; Index < Pixels.Num(); ++Index)
			{
				Data[Index * 4] = Pixels[Index].R;
				Data[Index * 4 + 1] = Pixels[Index].G;
				Data[Index * 4 + 2] = Pixels[Index].B;
				Data[Index * 4 + 3] = Pixels[Index].A;
			}
			const int32 NextWidth = FMath::Max(1, Width / 2);
			const int32 NextHeight = FMath::Max(1, Height / 2);
			TArray<FColor> Next;
			Next.SetNumUninitialized(NextWidth * NextHeight);
			for (int32 Y = 0; Y < NextHeight; ++Y)
			{
				for (int32 X = 0; X < NextWidth; ++X)
				{
					uint32 Sums[4] = {};
					int32 Count = 0;
					// Integer area ranges include the last row/column of odd grids.
					for (int32 SourceY = Y * Height / NextHeight; SourceY < (Y + 1) * Height / NextHeight; ++SourceY)
					{
						for (int32 SourceX = X * Width / NextWidth; SourceX < (X + 1) * Width / NextWidth; ++SourceX)
						{
							const FColor& Pixel = Pixels[SourceY * Width + SourceX];
							Sums[0] += Pixel.R; Sums[1] += Pixel.G; Sums[2] += Pixel.B; Sums[3] += Pixel.A;
							++Count;
						}
					}
					Next[Y * NextWidth + X] = FColor((Sums[0] + Count / 2) / Count, (Sums[1] + Count / 2) / Count,
						(Sums[2] + Count / 2) / Count, (Sums[3] + Count / 2) / Count);
				}
			}
			Pixels = MoveTemp(Next);
			Width = NextWidth;
			Height = NextHeight;
		}
		TArray64<uint8> Bytes;
		return DDS->WriteDDS(Bytes, EDDSFormatVersion::D3D10) == EDDSError::OK && FFileHelper::SaveArrayToFile(Bytes, *Filename);
	}
}

FTGALandscapeAtlas TGALandscapeExport::ComposeAtlas(const TArray<FTGALandscapePaintTile>& Tiles, const FTransform& Transform)
{
	const FVector Scale = Transform.GetScale3D();
	if (Tiles.IsEmpty() || Transform.ContainsNaN() || !Transform.GetRotation().Equals(FQuat::Identity, 0.000001) ||
		Scale.X <= 0 || Scale.Y <= 0 || Scale.Z <= 0)
	{
		return Failure(TEXT("Landscape must have components, positive scale and no rotation."));
	}
	const int32 Quads = Tiles[0].Quads;
	if (Quads <= 0 || Quads > 255)
	{
		return Failure(TEXT("Unsupported Landscape component size."));
	}
	FIntPoint Minimum = Tiles[0].SectionBase;
	FIntPoint Maximum = Minimum;
	for (const FTGALandscapePaintTile& Tile : Tiles)
	{
		if (Tile.Quads != Quads || Tile.Weights.Num() != (Quads + 1) * (Quads + 1))
		{
			return Failure(TEXT("Landscape component dimensions or sample counts differ: ") + Tile.Name);
		}
		Minimum.X = FMath::Min(Minimum.X, Tile.SectionBase.X);
		Minimum.Y = FMath::Min(Minimum.Y, Tile.SectionBase.Y);
		Maximum.X = FMath::Max(Maximum.X, Tile.SectionBase.X);
		Maximum.Y = FMath::Max(Maximum.Y, Tile.SectionBase.Y);
	}
	const int64 Width = int64(Maximum.X) - Minimum.X + Quads + 1;
	const int64 Height = int64(Maximum.Y) - Minimum.Y + Quads + 1;
	if (Width <= 0 || Height <= 0 || Width > 8192 || Height > 8192)
	{
		return Failure(TEXT("Landscape atlas exceeds the supported 8192 sample extent."));
	}
	FTGALandscapeAtlas Result;
	Result.Width = int32(Width);
	Result.Height = int32(Height);
	Result.Weights.SetNumZeroed(Result.Width * Result.Height);
	TBitArray<> Written(false, Result.Weights.Num());
	TSet<FIntPoint> Locations;
	for (const FTGALandscapePaintTile& Tile : Tiles)
	{
		const FIntPoint Offset = Tile.SectionBase - Minimum;
		if (Offset.X % Quads || Offset.Y % Quads || Locations.Contains(Offset))
		{
			return Failure(TEXT("Duplicate or off-grid Landscape component: ") + Tile.Name);
		}
		Locations.Add(Offset);
		for (int32 Y = 0; Y <= Quads; ++Y)
		{
			for (int32 X = 0; X <= Quads; ++X)
			{
				const int32 Index = (Offset.Y + Y) * Result.Width + Offset.X + X;
				const FColor Pixel = Tile.Weights[Y * (Quads + 1) + X];
				if (Written[Index] && Result.Weights[Index] != Pixel)
				{
					return Failure(TEXT("Shared Landscape border weights differ: ") + Tile.Name);
				}
				Result.Weights[Index] = Pixel;
				Written[Index] = true;
			}
		}
	}
	for (bool Present : Written)
	{
		if (!Present)
		{
			return Failure(TEXT("Landscape component grid contains holes; use a complete rectangle."));
		}
	}
	const FVector Origin = Transform.GetTranslation() + FVector(Minimum.X * Scale.X, Minimum.Y * Scale.Y, 0);
	Result.MaskU = FVector4d(1.0 / (Scale.X * Width), 0, 0, (0.5 - Origin.X / Scale.X) / Width);
	Result.MaskV = FVector4d(0, 1.0 / (Scale.Y * Height), 0, (0.5 - Origin.Y / Scale.Y) / Height);
	return Result;
}

FTGALandscapeAtlas TGALandscapeExport::CaptureAtlas(ALandscapeProxy* Landscape)
{
	if (!IsValid(Landscape))
	{
		return Failure(TEXT("Landscape source is missing."));
	}
	TArray<ULandscapeComponent*> Components;
	Landscape->GetComponents(Components);
	TArray<FTGALandscapePaintTile> Tiles;
	for (ULandscapeComponent* Component : Components)
	{
		if (Component->NumSubsections != 1 || Component->ComponentSizeQuads != 63 || Component->SubsectionSizeQuads != 63)
		{
			return Failure(TEXT("Landscape export currently requires one subsection and 63 quads per component: ") + Component->GetName());
		}
		FTGALandscapePaintTile Tile;
		Tile.Name = Component->GetName();
		Tile.SectionBase = Component->GetSectionBase();
		Tile.Quads = Component->ComponentSizeQuads;
		const FTransform Relative = Component->GetRelativeTransform();
		if (!Relative.GetRotation().Equals(FQuat::Identity, 0.000001) ||
			!Relative.GetScale3D().Equals(FVector::OneVector, 0.000001) ||
			!Relative.GetTranslation().Equals(FVector(Tile.SectionBase.X, Tile.SectionBase.Y, 0), 0.000001))
		{
			return Failure(TEXT("Landscape component transform does not match its section grid: ") + Tile.Name);
		}
		Tile.Weights.Init(FColor(0, 0, 0, 0), 64 * 64);
		TSet<FName> SeenLayers;
		TMap<int32, TArray64<uint8>> SourceCopies;
		const auto& Textures = Component->GetWeightmapTextures();
		for (const FWeightmapLayerAllocationInfo& Allocation : Component->GetWeightmapLayerAllocations())
		{
			if (!Allocation.IsAllocated())
			{
				continue;
			}
			if (!Allocation.LayerInfo || !Textures.IsValidIndex(Allocation.WeightmapTextureIndex) || Allocation.WeightmapTextureChannel >= 4)
			{
				return Failure(TEXT("Invalid Landscape layer allocation: ") + Tile.Name);
			}
			const FName LayerName = Allocation.LayerInfo->GetLayerName();
			const int32 CanonicalChannel = LayerName == TEXT("Snow_Dunes") ? 0 : LayerName == TEXT("Snow_Rough") ? 1 : LayerName == TEXT("Snow_Cliff") ? 2 : INDEX_NONE;
			if (CanonicalChannel == INDEX_NONE || SeenLayers.Contains(LayerName))
			{
				return Failure(TEXT("Unsupported or duplicate Landscape layer '") + LayerName.ToString() + TEXT("' on ") + Tile.Name);
			}
			SeenLayers.Add(LayerName);
			if (!SourceCopies.Contains(Allocation.WeightmapTextureIndex))
			{
				UTexture2D* Texture = Textures[Allocation.WeightmapTextureIndex];
				TArray64<uint8> Bytes;
				if (!Texture || !Texture->Source.IsValid() || Texture->Source.GetFormat() != TSF_BGRA8 ||
					Texture->Source.GetSizeX() != 64 || Texture->Source.GetSizeY() != 64 ||
					!Texture->Source.GetMipData(Bytes, 0) || Bytes.Num() != 64 * 64 * 4)
				{
					return Failure(TEXT("Landscape weightmap source must be linear BGRA8, 64x64: ") + Tile.Name);
				}
				SourceCopies.Add(Allocation.WeightmapTextureIndex, MoveTemp(Bytes));
			}
			const TArray64<uint8>& Bytes = SourceCopies[Allocation.WeightmapTextureIndex];
			// Allocation channel numbers are RGBA, while Source pixels are BGRA.
			const int32 SourceByte[] = {2, 1, 0, 3};
			for (int32 Index = 0; Index < Tile.Weights.Num(); ++Index)
			{
				const uint8 Weight = Bytes[Index * 4 + SourceByte[Allocation.WeightmapTextureChannel]];
				if (CanonicalChannel == 0) { Tile.Weights[Index].R = Weight; }
				else if (CanonicalChannel == 1) { Tile.Weights[Index].G = Weight; }
				else { Tile.Weights[Index].B = Weight; }
			}
		}
		Tiles.Add(MoveTemp(Tile));
	}
	return ComposeAtlas(Tiles, Landscape->GetActorTransform());
}

FTGALandscapeMaterialExport TGALandscapeExport::ExportMaterial(ALandscapeProxy* Landscape, const FString& SavePath,
	float SurfaceRepeatMeters, float NoiseStrength, float NoisePatchSizeCm)
{
	FTGALandscapeMaterialExport Result;
	if (!FMath::IsFinite(SurfaceRepeatMeters) || SurfaceRepeatMeters <= 0 || !FMath::IsFinite(NoiseStrength) ||
		NoiseStrength < 0 || NoiseStrength > 1 || !FMath::IsFinite(NoisePatchSizeCm) || NoisePatchSizeCm < 1)
	{
		Result.Error = TEXT("Invalid Landscape surface-repeat or noise controls.");
		return Result;
	}
	if (!Landscape || !Landscape->GetLandscapeMaterial() || Landscape->GetLandscapeMaterial()->GetMaterial()->GetName() != TEXT("MM_Landscape"))
	{
		Result.Error = TEXT("Linked Landscape must use the supported MM_Landscape material profile.");
		return Result;
	}
	FTGALandscapeAtlas Atlas = CaptureAtlas(Landscape);
	if (!Atlas.IsValid())
	{
		Result.Error = Atlas.Error;
		return Result;
	}
	const uint32 Identity = FCrc::StrCrc32(*Landscape->GetPathName());
	Result.AtlasName = FString::Printf(TEXT("T_AGP_Landscape_%08X_Weights"), Identity);
	Result.MaterialName = FString::Printf(TEXT("MI_AGP_Landscape_%08X"), Identity);
	Result.Material = MakeShared<FJsonObject>();
	Result.Material->SetStringField(TEXT("Name"), Result.MaterialName);
	Result.Material->SetStringField(TEXT("Parent"), Landscape->GetLandscapeMaterial()->GetName());
	TArray<TSharedPtr<FJsonValue>> Params;
	Params.Add(VectorParameter(TEXT("MaskU"), Atlas.MaskU));
	Params.Add(VectorParameter(TEXT("MaskV"), Atlas.MaskV));
	const double RepeatCm = double(SurfaceRepeatMeters) * 100;
	Params.Add(VectorParameter(TEXT("SurfaceMapping"), FVector4d(1 / RepeatCm, 1 / RepeatCm, 2800 / RepeatCm, 2800 / RepeatCm)));
	Params.Add(VectorParameter(TEXT("Layer2MaskChannels"), FVector4d(0, 1, 0, 0)));
	Params.Add(VectorParameter(TEXT("Layer3MaskChannels"), FVector4d(0, 0, 1, 0)));
	Params.Add(VectorParameter(TEXT("NoiseControls"), FVector4d(NoiseStrength, NoisePatchSizeCm, 0, 0)));
	float Debug = 0;
	Landscape->GetLandscapeMaterial()->GetScalarParameterValue(FMaterialParameterInfo(TEXT("DEBUG")), Debug);
	Params.Add(Parameter(TEXT("DEBUG"), 0, MakeShared<FJsonValueNumber>(Debug)));
	Params.Add(TextureParameter(TEXT("SplatMask"), Result.AtlasName));
	const TCHAR* Layers[] = {TEXT("Dunes"), TEXT("Rough"), TEXT("Cliff")};
	for (int32 Layer = 0; Layer < 3; ++Layer)
	{
		Params.Add(TextureParameter(FString::Printf(TEXT("Layer%dBaseColor"), Layer + 1), FString::Printf(TEXT("T_8M_Snow_%s_BaseColor"), Layers[Layer])));
		Params.Add(TextureParameter(FString::Printf(TEXT("Layer%dNormal"), Layer + 1), FString::Printf(TEXT("T_8M_Snow_%s_Normal"), Layers[Layer])));
		Params.Add(TextureParameter(FString::Printf(TEXT("Layer%dORM"), Layer + 1), FString::Printf(TEXT("T_8M_Snow_%s_OcclusionRoughnessMetallic"), Layers[Layer])));
	}
	Params.Add(TextureParameter(TEXT("BlendNoise"), TEXT("T_Splatmap_TransitionNoise")));
	Result.Material->SetArrayField(TEXT("Parameters"), Params);
	const FString Filename = FPaths::Combine(SavePath, Result.AtlasName + TEXT(".dds"));
	if (!WriteAtlas(Atlas, Filename))
	{
		Result.Error = TEXT("Could not write combined Landscape DDS: ") + Filename;
		Result.Material.Reset();
	}
	return Result;
}
