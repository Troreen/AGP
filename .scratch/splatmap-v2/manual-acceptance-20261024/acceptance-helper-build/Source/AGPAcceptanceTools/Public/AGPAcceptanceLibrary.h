#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "AGPAcceptanceLibrary.generated.h"

class ALandscapeProxy;

UCLASS()
class AGPACCEPTANCETOOLS_API UAGPAcceptanceLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "AGP Acceptance")
	static FString DescribeLandscape(ALandscapeProxy* Landscape);

	UFUNCTION(BlueprintCallable, Category = "AGP Acceptance")
	static FString ShareFixtureWeightmaps(ALandscapeProxy* Source, ALandscapeProxy* Target);
};
