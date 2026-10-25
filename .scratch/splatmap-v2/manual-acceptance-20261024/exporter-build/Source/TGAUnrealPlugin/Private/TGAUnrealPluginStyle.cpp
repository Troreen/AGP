// Copyright Epic Games, Inc. All Rights Reserved.

#include "TGAUnrealPluginStyle.h"
#include "TGAUnrealPlugin.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/SlateStyleRegistry.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleMacros.h"

#define RootToContentDir Style->RootToContentDir

TSharedPtr<FSlateStyleSet> FTGAUnrealPluginStyle::StyleInstance = nullptr;

void FTGAUnrealPluginStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FTGAUnrealPluginStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FTGAUnrealPluginStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("TGAUnrealPluginStyle"));
	return StyleSetName;
}


const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);

TSharedRef< FSlateStyleSet > FTGAUnrealPluginStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("TGAUnrealPluginStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("TGAUnrealPlugin")->GetBaseDir() / TEXT("Resources"));
	Style->Set("Icons.TGA", new FSlateImageBrush(RootToContentDir(TEXT("TgaLogo64"), TEXT(".png")), Icon20x20));
	Style->Set("Icons.TGAExport", new FSlateImageBrush(RootToContentDir(TEXT("TGAExport"), TEXT(".png")), Icon20x20));

	Style->Set("TGAUnrealPlugin.TGAExportLevel", new FSlateImageBrush(RootToContentDir(TEXT("TGAExport"), TEXT(".png")), Icon20x20));
	return Style;
}

void FTGAUnrealPluginStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FTGAUnrealPluginStyle::Get()
{
	return *StyleInstance;
}
