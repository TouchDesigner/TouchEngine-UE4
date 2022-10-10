// Copyright © Derivative Inc. 2022

#include "TouchEngineD3D12RHI.h"

#include "ITouchEngineModule.h"
#include "TouchEngineD3D12ResourceProvider.h"

#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "FTouchEngineD3D12RHI"

namespace UE::TouchEngine::D3DX12
{
	void FTouchEngineD3D12RHI::StartupModule()
	{
		ITouchEngineModule::Get().BindResourceProvider(
			TEXT("D3D12"),
			FResourceProviderFactory::CreateLambda([](const FResourceProviderInitArgs& Args)
			{
				return MakeD3DX12ResourceProvider(Args);
			})
		);
	}

	void FTouchEngineD3D12RHI::ShutdownModule()
	{
		ITouchEngineModule::Get().UnbindResourceProvider(TEXT("D3D12"));
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(UE::TouchEngine::D3DX12::FTouchEngineD3D12RHI, TouchEngineD3D12RHI);