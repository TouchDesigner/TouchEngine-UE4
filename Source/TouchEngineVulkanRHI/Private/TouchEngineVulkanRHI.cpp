// Copyright © Derivative Inc. 2022

#include "TouchEngineVulkanRHI.h"

#include "ITouchEngineModule.h"
#include "Logging.h"
#include "TouchEngineVulkanResourceProvider.h"

#include "Modules/ModuleManager.h"
#include "Util/VulkanWindowsFunctions.h"

#define LOCTEXT_NAMESPACE "FTouchEngineVulkanRHI"

namespace UE::TouchEngine::Vulkan
{
	void FTouchEngineVulkanRHI::StartupModule()
	{
		// We are loaded in PostConfigInit so that we can hook up to Vulkan RHI and add some custom extensions
		// This is required so we can load the functions later in ConditionallyLoadVulkanFunctionsForWindows
		ConditionallySetupVulkanExtensions();

		// The rest of the setup needs to be done once the RHI has been initialised
		FCoreDelegates::OnPostEngineInit.AddRaw(this, &FTouchEngineVulkanRHI::LoadWindowsFunctions);
	}

	void FTouchEngineVulkanRHI::ShutdownModule()
	{
		if (ITouchEngineModule* Module = ITouchEngineModule::GetSafe())
		{
			Module->UnbindResourceProvider(TEXT("Vulkan"));
		}
	}

#if PLATFORM_WINDOWS
	void FTouchEngineVulkanRHI::LoadWindowsFunctions()
	{
		// Bypass graphics API check while cooking
		if (!FApp::CanEverRender())
		{
			return;
		}

		ITouchEngineModule::Get().BindResourceProvider(
			TEXT("Vulkan"),
			FResourceProviderFactory::CreateLambda([](const FResourceProviderInitArgs& Args)
			{
				return MakeVulkanResourceProvider(Args);
			})
		);
		
		ConditionallyLoadVulkanFunctionsForWindows();
		UE_CLOG(!AreVulkanFunctionsForWindowsLoaded(), LogTouchEngineVulkanRHI, Error, TEXT("Failed to load Vulkan Windows functions."));
	}
#endif
}


#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(UE::TouchEngine::Vulkan::FTouchEngineVulkanRHI, TouchEngineVulkanRHI);