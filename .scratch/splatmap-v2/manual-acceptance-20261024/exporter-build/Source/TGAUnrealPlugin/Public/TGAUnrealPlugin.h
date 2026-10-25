// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LandscapeProxy.h"
#include "Modules/ModuleManager.h"
#include "NavMesh/RecastNavMesh.h"
#include "TGALevelSerializer.h"

DECLARE_LOG_CATEGORY_EXTERN(LogTGA, Log, All);


class FToolBarBuilder;
class FMenuBuilder;

class FTGAUnrealPluginModule : public IModuleInterface
{
	TGALevelSerializer LevelSerializer;

public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	/** This function will be bound to Command. */
	void Command_ExportLevel();
	bool ExportLevelToJSON(const UWorld* World, const FString& OutputFile);
	bool ExportLandscapeWeightmaps(FString LevelName, ALandscapeProxy* LandscapeProxy, const FString& SavePath);
	bool ExportNavMeshToFBX(const UWorld* World, const FString& OutputFile);

private:

	const ARecastNavMesh* TryGetNavMesh(const UWorld* World) const; 
	void RegisterMenus();

	bool IsActorAllowedToExport(const AActor* Actor) const;

	FString LastLevelExportFolder;
	FString DefaultExportFolder;
	TSharedPtr<class FUICommandList> PluginCommands;
};