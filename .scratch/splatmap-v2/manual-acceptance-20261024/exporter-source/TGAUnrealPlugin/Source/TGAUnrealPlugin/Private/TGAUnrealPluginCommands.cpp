// Copyright Epic Games, Inc. All Rights Reserved.

#include "TGAUnrealPluginCommands.h"

#define LOCTEXT_NAMESPACE "FTGAUnrealPluginModule"

void FTGAUnrealPluginCommands::RegisterCommands()
{
	UI_COMMAND(TGAExportLevel, "TGAUnrealPlugin", "Export current Level to a folder.", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
