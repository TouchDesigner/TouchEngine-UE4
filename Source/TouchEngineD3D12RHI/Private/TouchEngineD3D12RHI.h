// Copyright © Derivative Inc. 2022

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

namespace UE::TouchEngine::D3DX12
{
	class FTouchEngineD3D12RHI : public IModuleInterface
	{
	public:

		//~ Begin IModuleInterface Interface
		void StartupModule() override;
		void ShutdownModule() override;
		//~ End IModuleInterface Interface
	};
}

