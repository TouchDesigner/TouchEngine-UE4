// Copyright © Derivative Inc. 2021

#include "ToxAssetFactory.h"

#include "ToxAsset.h"
#include "Misc/Paths.h"

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

	FString Path = Filename;
	FPaths::MakePathRelativeTo(Path, *FPaths::ProjectContentDir());
	ToxAsset->FilePath = Path;

	bOutOperationCanceled = false;
	return ToxAsset;
}

#undef LOCTEXT_NAMESPACE