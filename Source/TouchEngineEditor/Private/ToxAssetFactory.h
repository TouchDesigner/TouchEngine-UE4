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
	virtual UObject* FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled) override;

};

UCLASS()
class UReimportToxAssetFactory : public UToxAssetFactory, public FReimportHandler
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	class UToxAsset* pOriginalTox;

	//~ Begin FReimportHandler Interface
	virtual bool CanReimport(UObject* Obj, TArray<FString>& OutFilenames) override;
	virtual void SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths) override;
	virtual EReimportResult::Type Reimport(UObject* Obj) override;
	virtual int32 GetPriority() const override;
	//~ End FReimportHandler Interface

	//~ Begin UFactory Interface
	virtual bool IsAutomatedImport() const override;
	//~ End UFactory Interface
};