#include "AGPAcceptanceLibrary.h"
#include "LandscapeComponent.h"
#include "LandscapeLayerInfoObject.h"
#include "LandscapeProxy.h"
#include "Engine/Texture2D.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_MODULE(FDefaultModuleImpl, AGPAcceptanceTools)

FString UAGPAcceptanceLibrary::DescribeLandscape(ALandscapeProxy* Landscape)
{
	if (!Landscape)
	{
		return TEXT("{\"error\":\"Missing Landscape\"}");
	}

	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("actor"), Landscape->GetPathName());
	TArray<ULandscapeComponent*> Components;
	Landscape->GetComponents(Components);
	TArray<TSharedPtr<FJsonValue>> Records;
	for (ULandscapeComponent* Component : Components)
	{
		TSharedRef<FJsonObject> Record = MakeShared<FJsonObject>();
		Record->SetStringField(TEXT("component"), Component->GetName());
		TArray<TSharedPtr<FJsonValue>> Textures;
		for (UTexture2D* Texture : Component->GetWeightmapTextures())
		{
			Textures.Add(MakeShared<FJsonValueString>(GetPathNameSafe(Texture)));
		}
		Record->SetArrayField(TEXT("textures"), Textures);
		TArray<TSharedPtr<FJsonValue>> Allocations;
		for (const FWeightmapLayerAllocationInfo& Allocation : Component->GetWeightmapLayerAllocations())
		{
			TSharedRef<FJsonObject> Entry = MakeShared<FJsonObject>();
			Entry->SetStringField(TEXT("layer"), Allocation.LayerInfo ? Allocation.LayerInfo->LayerName.ToString() : TEXT("<null>"));
			Entry->SetNumberField(TEXT("texture_index"), Allocation.WeightmapTextureIndex);
			Entry->SetNumberField(TEXT("channel"), Allocation.WeightmapTextureChannel);
			Allocations.Add(MakeShared<FJsonValueObject>(Entry));
		}
		Record->SetArrayField(TEXT("allocations"), Allocations);
		Records.Add(MakeShared<FJsonValueObject>(Record));
	}
	Root->SetArrayField(TEXT("components"), Records);
	FString Result;
	FJsonSerializer::Serialize(Root, TJsonWriterFactory<>::Create(&Result));
	return Result;
}

FString UAGPAcceptanceLibrary::ShareFixtureWeightmaps(ALandscapeProxy* Source, ALandscapeProxy* Target)
{
	// Deliberately constrain this setup operation to the separate acceptance map.
	if (!Source || !Target || Source == Target || Source->GetWorld() != Target->GetWorld() ||
		!Target->GetWorld()->GetMapName().Contains(TEXT("Splatmap_ExporterFixture")))
	{
		return TEXT("{\"error\":\"Operation requires two proxies in the disposable exporter fixture\"}");
	}
	TArray<ULandscapeComponent*> SourceComponents;
	TArray<ULandscapeComponent*> TargetComponents;
	Source->GetComponents(SourceComponents);
	Target->GetComponents(TargetComponents);
	if (SourceComponents.IsEmpty() || SourceComponents.Num() != TargetComponents.Num())
	{
		return TEXT("{\"error\":\"Component layouts differ\"}");
	}
	for (int32 Index = 0; Index < SourceComponents.Num(); ++Index)
	{
		TargetComponents[Index]->Modify();
		TargetComponents[Index]->GetWeightmapTextures() = SourceComponents[Index]->GetWeightmapTextures();
		TargetComponents[Index]->GetWeightmapLayerAllocations() = SourceComponents[Index]->GetWeightmapLayerAllocations();
		TargetComponents[Index]->MarkPackageDirty();
	}
	return DescribeLandscape(Target);
}
