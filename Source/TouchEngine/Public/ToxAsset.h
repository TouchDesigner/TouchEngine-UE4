// Copyright © Derivative Inc. 2021

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Factories/Factory.h"
#include "ToxAsset.generated.h"

class UAssetImportData;

/**
 *
 */
UCLASS(BlueprintType, hidecategories = (Object))
class TOUCHENGINE_API UToxAsset : public UObject
{
	GENERATED_BODY()

public:

	UPROPERTY(VisibleAnywhere, Instanced, Category = ImportSettings)
	UAssetImportData* FilePath;
};

