// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TGAExportSettingsComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TGAUNREALPLUGIN_API UTGAExportSettingsComponent : public UActorComponent
{
	GENERATED_BODY()
public:

	UPROPERTY(EditAnywhere, Category = "The Game Assembly Exporter", meta = (ToolTip = "If True the owning Actor and its components will not be exported."))
	bool bDoNotExport;

public:	
	// Sets default values for this component's properties
	UTGAExportSettingsComponent();
};
