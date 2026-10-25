// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "TGAUnrealPluginStyle.h"

class FTGAUnrealPluginCommands : public TCommands<FTGAUnrealPluginCommands>
{
public:

	FTGAUnrealPluginCommands()
		: TCommands<FTGAUnrealPluginCommands>(TEXT("TGAUnrealPlugin"), NSLOCTEXT("Contexts", "TGAUnrealPlugin", "TGAUnrealPlugin Plugin"), NAME_None, FTGAUnrealPluginStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > TGAExportLevel;
};
