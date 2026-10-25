// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "TGAPluginDeveloperSettings.generated.h"

/**
 * 
 */
UCLASS(Config = Editor, DefaultConfig, DisplayName = "TGA Plugin Settings")
class TGAUNREALPLUGIN_API UTGAPluginDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:

	UTGAPluginDeveloperSettings();

	virtual FName GetCategoryName() const override { return FName("Plugins"); }
	virtual FName GetSectionName() const override { return FName("TGA"); }

	UPROPERTY(EditAnywhere, Config, Category = "Level Export", meta = (ToolTip = "Folder relative to the UE project root where export should default."))
	FString RelativeExportPath;

	UPROPERTY(EditAnywhere, Config, Category = "Level Export", meta = (ToolTip = "Allow the Export Save As dialog to remember the last used path."))
	bool RememberExportPath;

	UPROPERTY(EditAnywhere, Config, Category = "Level Export", meta = (ToolTip = "A list of Classes which will be ignored when exporting the level even if they are an acceptable archetype or have components that should otherwise export."))
	TArray<UClass*> TypeExclusionList;

	UPROPERTY(EditAnywhere, Config, DisplayName="Relative Transform No Scale", Category = "Level Export|Extra Component Transforms", meta = (ToolTip = ""))
	bool ExportComponentRelativeTransformNoScale;
	
	UPROPERTY(EditAnywhere, Config, DisplayName="World Transform", Category = "Level Export|Extra Component Transforms", meta = (ToolTip = ""))
	bool ExportComponentWorldTransform;

	UPROPERTY(EditAnywhere, Config, DisplayName="World Transform No Scale", Category = "Level Export|Extra Component Transforms", meta = (ToolTip = ""))
	bool ExportComponentWorldTransformNoScale;

	UPROPERTY(EditAnywhere, Config, DisplayName="Relative Scale", Category = "Level Export|Extra Component Transforms", meta = (ToolTip = ""))
	bool ExportComponentRelativeScale;

	UPROPERTY(EditAnywhere, Config, DisplayName="World Scale", Category = "Level Export|Extra Component Transforms", meta = (ToolTip = ""))
	bool ExportComponentWorldScale;

	UPROPERTY(EditAnywhere, Config, DisplayName="Relative Rotation", Category = "Level Export|Extra Component Transforms", meta = (ToolTip = ""))
	bool ExportComponentRelativeRotation;

	UPROPERTY(EditAnywhere, Config, DisplayName="World Rotation", Category = "Level Export|Extra Component Transforms", meta = (ToolTip = ""))
	bool ExportComponentWorldRotation;
};
