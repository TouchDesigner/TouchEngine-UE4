// Copyright © Derivative Inc. 2021


#include "ToxAssetFactoryNew.h"

#include "ToxAsset.h"


UToxAssetFactoryNew::UToxAssetFactoryNew(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UToxAsset::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UToxAssetFactoryNew::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UToxAsset* NewAsset = NewObject<UToxAsset>(InParent, InClass, InName, Flags);
	
	return NewAsset;
}

bool UToxAssetFactoryNew::ShouldShowInNewMenu() const
{
	return false;
}