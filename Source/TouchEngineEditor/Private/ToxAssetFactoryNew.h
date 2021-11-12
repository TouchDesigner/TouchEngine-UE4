// Copyright © Derivative Inc. 2021

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "ToxAssetFactoryNew.generated.h"

/**
 *
 */
UCLASS(hidecategories = (Object))
class UToxAssetFactoryNew : public UFactory
{
	GENERATED_BODY()

public:

	UToxAssetFactoryNew(const FObjectInitializer& ObjectInitializer);

	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual bool ShouldShowInNewMenu() const override;
	//virtual UObject* FactoryCreateBinary(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const uint8*& Buffer, const uint8* BufferEnd, FFeedbackContext* Warn) override;
};