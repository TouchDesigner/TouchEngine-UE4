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

#include "TouchEngineModule.h"

#include "Logging.h"
#include "Interfaces/IPluginManager.h"
#include "Rendering/TouchResourceProvider.h"
#include "TouchEngine/TEResult.h"

namespace UE::TouchEngine
{
	void FTouchEngineModule::StartupModule()
	{
		LoadTouchEngineLib();
	}

	void FTouchEngineModule::ShutdownModule()
	{
		ResourceFactories.Reset();
		UnloadTouchEngineLib();
	}
	
	void FTouchEngineModule::BindResourceProvider(const FString& NameOfRHI, FResourceProviderFactory FactoryDelegate)
	{
		if (ensure(!ResourceFactories.Contains(NameOfRHI) && FactoryDelegate.IsBound()))
		{
			ResourceFactories.Add(NameOfRHI, { FactoryDelegate });
		}
	}

	bool FTouchEngineModule::IsTouchEngineLibInitialized() const
	{
		return TouchEngineLibHandle != nullptr;
	}

	void FTouchEngineModule::UnbindResourceProvider(const FString& NameOfRHI)
	{
		ResourceFactories.Remove(NameOfRHI);
	}

	TSharedPtr<FTouchResourceProvider> FTouchEngineModule::CreateResourceProvider(const FString& NameOfRHI)
	{
		if (FResourceProviderFactory* Factory = ResourceFactories.Find(NameOfRHI)
			; ensure(IsTouchEngineLibInitialized()) && ensure(Factory))
		{
			auto OnLoadError = [](const FString& Error)
			{
				UE_LOG(LogTouchEngine, Error, TEXT("ResourceProvider load error: %s"), *Error)
			};
			auto OnResultError = [](const TEResult Result, const FString& Error)
			{
				if (Result != TEResultSuccess)
				{
					UE_LOG(LogTouchEngine, Error, TEXT("ResourceProvider failed to create context: %s"), *Error)
				}
			};
			
			const FResourceProviderInitArgs Args{ OnLoadError, OnResultError };
			return Factory->Execute(Args);
		}

		UE_LOG(LogTouchEngine, Error, TEXT("RHI %s is unsupported"), *NameOfRHI);
		return nullptr;
	}

	void FTouchEngineModule::LoadTouchEngineLib()
	{
		const FString BasePath = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("TouchEngine"))->GetBaseDir(), TEXT("/Binaries/ThirdParty/Win64"));
		const FString FullPathToDLL = FPaths::Combine(BasePath, TEXT("TouchEngine.dll"));
		if (!FPaths::FileExists(FullPathToDLL))
		{
			UE_LOG(LogTouchEngine, Error, TEXT("Invalid path to TouchEngine.dll: %s"), *FullPathToDLL);
			return;
		}
		
		FPlatformProcess::PushDllDirectory(*BasePath);
		TouchEngineLibHandle = FPlatformProcess::GetDllHandle(*FullPathToDLL);
		FPlatformProcess::PopDllDirectory(*BasePath);
		
		UE_CLOG(!IsTouchEngineLibInitialized(), LogTouchEngine, Error, TEXT("Failed to load TouchEngine library: %s"), *FullPathToDLL);
	}

	void FTouchEngineModule::UnloadTouchEngineLib()
	{
		if (IsTouchEngineLibInitialized())
		{
			FPlatformProcess::FreeDllHandle(TouchEngineLibHandle);
			TouchEngineLibHandle = nullptr;
		}
	}
}

IMPLEMENT_MODULE(UE::TouchEngine::FTouchEngineModule, TouchEngine)
