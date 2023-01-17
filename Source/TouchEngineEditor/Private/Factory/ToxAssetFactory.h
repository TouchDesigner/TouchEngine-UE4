// Copyright © Derivative Inc. 2021

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "EditorReimportHandler.h"
#include "ToxAssetFactory.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogToxFactory, Log, All);

UCLASS()
class UToxAssetFactory : public UFactory
{
	GENERATED_BODY()

public:

	UToxAssetFactory(const FObjectInitializer& ObjectInitializer);

	/* Handles import of .tox files via drag-drop */
	virtual UObject* FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled) override;

	/* Handles creation of a new UToxAsset for tox files copied into the Content folder (this occurs on startup)
	* as well as for new asset creation action via Content Browser -> New*/
	//virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};