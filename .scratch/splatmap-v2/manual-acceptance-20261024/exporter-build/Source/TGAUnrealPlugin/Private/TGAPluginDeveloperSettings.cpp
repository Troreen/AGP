// Fill out your copyright notice in the Description page of Project Settings.


#include "TGAPluginDeveloperSettings.h"

#include "AbstractNavData.h"

UTGAPluginDeveloperSettings::UTGAPluginDeveloperSettings()
{
	TypeExclusionList.Add(AAbstractNavData::StaticClass());
	RememberExportPath = true;
	RelativeExportPath = "Exported";
}
