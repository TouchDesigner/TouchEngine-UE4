// Copyright © Derivative Inc. 2021

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "EditorReimportHandler.h"
#include "ToxAssetFactory.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogToxFactory, Log, All);

UCLASS()
class UToxAssetFactory : public UFactory, public FReimportHandler
{
	GENERATED_BODY()

public:
	UToxAssetFactory(const FObjectInitializer& ObjectInitializer);

	//~ Begin UFactory Interface
	virtual UObject* FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled) override;
	virtual bool FactoryCanImport(const FString& Filename) override { return true; }
	//~ End UFactory Interface

	//~ Begin FReimportHandler Interface
	virtual bool CanReimport(UObject* Obj, TArray<FString>& OutFilenames) override;
	virtual void SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths) override;
	virtual EReimportResult::Type Reimport(UObject* Obj) override;
	virtual int32 GetPriority() const override;
	//~ End FReimportHandler Interface
};


/*
UCLASS()
class UReimportToxAssetFactory : public UToxAssetFactory, public FReimportHandler
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	class UToxAsset* pOriginalTox;


};
*/