// Copyright © Derivative Inc. 2021

#pragma once

#include "AssetTypeActions_Base.h"
#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "ToxAssetFactoryNew.generated.h"

class FToxAssetTypeActions : public FAssetTypeActions_Base
{
public:
	UClass* GetSupportedClass() const override;
	FText GetName() const override;
	FColor GetTypeColor() const override;
	uint32 GetCategories() override;
};

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
	//virtual UObject* FactoryCreateBinary(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const uint8*& Buffer, const uint8* BufferEnd, FFeedbackContext* Warn) override;

	/* Whether the new menu should support creation of a new Tox Asset*/
	virtual bool ShouldShowInNewMenu() const
	{
		return true;
	}

	/* The New asset menu category under which creation of a Tox Asset should be shown */
	uint32 GetMenuCategories() const override;
};