// Copyright © Derivative Inc. 2022

#include "TouchEngineD3D11RHI.h"

#include "ITouchEngineModule.h"
#include "Logging.h"
#include "TouchEngineD3X11ResourceProvider.h"

#include "Modules/ModuleManager.h"

namespace UE::TouchEngine::D3DX11
{
	void FTouchEngineD3D11RHI::StartupModule()
	{
		ITouchEngineModule::Get().BindResourceProvider(
			TEXT("D3D11"),
			FResourceProviderFactory::CreateLambda([](const FResourceProviderInitArgs& Args)
			{
				return MakeD3DX11ResourceProvider(Args);
			})
		);
	}

	void FTouchEngineD3D11RHI::ShutdownModule()
	{
		ITouchEngineModule::Get().UnbindResourceProvider(TEXT("D3D11"));
	}
}
	
IMPLEMENT_MODULE(UE::TouchEngine::D3DX11::FTouchEngineD3D11RHI, TouchEngineD3D11RHI);
