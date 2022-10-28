// Copyright © Derivative Inc. 2021


#include "ToxAssetFactory.h"

#include "ToxAsset.h"
#include "Misc/Paths.h"
#include "EditorFramework/AssetImportData.h"
//#include "AssetRegistryModule.h"

DEFINE_LOG_CATEGORY(LogToxFactory);

#define LOCTEXT_NAMESPACE "ToxFactory"

UToxAssetFactory::UToxAssetFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Formats.Add(FString(TEXT("tox;")) + LOCTEXT("ToxFile", "Tox File").ToString());
	SupportedClass = UToxAsset::StaticClass();
	bCreateNew = false;
	bEditorImport = true;
}

UObject* UToxAssetFactory::FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled)
{
	UToxAsset* ToxAsset = NewObject<UToxAsset>(InParent, InClass, InName, Flags);

	/*
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked< FAssetRegistryModule >(FName("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	TArray<FAssetData> OutAssetData;
	AssetRegistry.GetAssetsByClass(InParent->GetClass()->GetFName(), OutAssetData);
	UObject* InParentBlueprint = OutAssetData.GetAsset();
	*/

	FString Path = Filename;
	FPaths::MakePathRelativeTo(Path, *FPaths::ProjectContentDir());
	ToxAsset->FilePath = Path;
	ToxAsset->AssetImportData->AddFileName(Path, 0);

	bOutOperationCanceled = false;
	return ToxAsset;
}

bool UToxAssetFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
	UToxAsset* ToxAsset = Cast<UToxAsset>(Obj);
	if (ToxAsset && ToxAsset->AssetImportData)
	{
		ToxAsset->AssetImportData->ExtractFilenames(OutFilenames);
		return true;
	}
	return false;
}

void UToxAssetFactory::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths)
{
	UToxAsset* ToxAsset = Cast<UToxAsset>(Obj);
	if (ToxAsset && ToxAsset->AssetImportData && ensure(NewReimportPaths.Num() == 1))
	{
		ToxAsset->Modify();

		FString Path = NewReimportPaths[0];
		ToxAsset->AssetImportData->UpdateFilenameOnly(Path);

		FPaths::MakePathRelativeTo(Path, *FPaths::ProjectContentDir());
		ToxAsset->FilePath = Path;
	}
}

EReimportResult::Type UToxAssetFactory::Reimport(UObject* Obj)
{
	if (!Obj || !Obj->IsA(UToxAsset::StaticClass()))
	{
		return EReimportResult::Failed;
	}

	UToxAsset* ToxAsset = Cast<UToxAsset>(Obj);

	//TGuardValue<UToxAsset*> OriginalToxGuardValue(pOriginalTox, pTox);

	const FString ResolvedSourceFilePath = ToxAsset->AssetImportData->GetFirstFilename();
	if (!ResolvedSourceFilePath.Len())
	{
		return EReimportResult::Failed;
	}

	bool OutCanceled = false;
	if (ImportObject(ToxAsset->GetClass(), ToxAsset->GetOuter(), *ToxAsset->GetName(), RF_Public | RF_Standalone, ResolvedSourceFilePath, nullptr, OutCanceled) != nullptr)
	{
		UE_LOG(LogToxFactory, Log, TEXT("-- import success"));
	}
	else if (OutCanceled)
	{
		UE_LOG(LogToxFactory, Warning, TEXT("-- import canceled"));
		return EReimportResult::Cancelled;
	}
	else
	{
		UE_LOG(LogToxFactory, Warning, TEXT("-- import failed"));
		return EReimportResult::Failed;
	}
	return EReimportResult::Succeeded;
}

int32 UToxAssetFactory::GetPriority() const
{
	return ImportPriority;
}

#undef LOCTEXT_NAMESPACE