/* Shared Use License: This file is owned by Derivative Inc. (Derivative)
* and can only be used, and/or modified for use, in conjunction with
* Derivative's TouchDesigner software, and only if you are a licensee who has
* accepted Derivative's TouchDesigner license or assignment agreement
* (which also govern the use of this file). You may share or redistribute
* a modified version of this file provided the following conditions are met:
*
* 1. The shared file or redistribution must retain the information set out
* above and this list of conditions.
* 2. Derivative's name (Derivative Inc.) or its trademarks may not be used
* to endorse or promote products derived from this file without specific
* prior written permission from Derivative.
*/

#include "TouchEngineEditorModule.h"
#include "Core.h"
#include "Modules/ModuleManager.h"
#include "DetailCustomizations.h"
#include "TouchEngineDynVarDetsCust.h"
#include "TouchEngineIntVector4StructCust.h"
#include "TouchNodeFactory.h"
#include "EdGraphUtilities.h"
#include "TouchEngineDynamicVariableStruct.h"
#include "TouchEngineIntVector4.h"

#define LOCTEXT_NAMESPACE "FTouchEngineEditorModule"




void FTouchEngineEditorModule::StartupModule()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomPropertyTypeLayout(FTouchEngineDynamicVariableContainer::StaticStruct()->GetFName(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FTouchEngineDynamicVariableStructDetailsCustomization::MakeInstance));
	PropertyModule.RegisterCustomPropertyTypeLayout(FTouchEngineIntVector4::StaticStruct()->GetFName(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FTouchEngineIntVector4StructCust::MakeInstance));

	TouchNodeFactory = MakeShareable(new FTouchNodeFactory());
	FEdGraphUtilities::RegisterVisualNodeFactory(TouchNodeFactory);

	PropertyModule.NotifyCustomizationModuleChanged();
}

void FTouchEngineEditorModule::ShutdownModule()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.UnregisterCustomPropertyTypeLayout(FTouchEngineDynamicVariableContainer::StaticStruct()->GetFName());
	PropertyModule.UnregisterCustomPropertyTypeLayout(FTouchEngineIntVector4::StaticStruct()->GetFName());

	if (TouchNodeFactory.IsValid())
	{
		FEdGraphUtilities::UnregisterVisualNodeFactory(TouchNodeFactory);
		TouchNodeFactory.Reset();
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FTouchEngineEditorModule, TouchEngineEditor)
