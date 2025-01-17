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

#pragma once

#include "CoreMinimal.h"
#include "Rendering/Importing/TouchTextureImporter.h"
#include "Util/TouchFenceCache.h"

#include "Windows/AllowWindowsPlatformTypes.h"
THIRD_PARTY_INCLUDES_START
#include "d3d12.h"
THIRD_PARTY_INCLUDES_END
#include "Windows/HideWindowsPlatformTypes.h"

namespace UE::TouchEngine::D3DX12
{
	class FTouchImportTextureD3D12;

	class FTouchTextureImporterD3D12 : public FTouchTextureImporter
	{
	public:

		FTouchTextureImporterD3D12(ID3D12Device* Device, TSharedRef<FTouchFenceCache> FenceCache);
	
	protected:

		//~ Begin FTouchTextureImporter Interface
		virtual TSharedPtr<ITouchImportTexture> CreatePlatformTexture_RenderThread(const TouchObject<TEInstance>& Instance, const TouchObject<TETexture>& SharedTexture) override;
		virtual FTextureMetaData GetTextureMetaData(const TouchObject<TETexture>& Texture) const override;
		//~ End FTouchTextureImporter Interface

	private:
		
		template<typename T>
		using TComPtr = Microsoft::WRL::ComPtr<T>;
		
		ID3D12Device* Device;
		TMap<HANDLE, TSharedRef<FTouchImportTextureD3D12>> CachedTextures;
		TSharedRef<FTouchFenceCache> FenceCache;

		TSharedPtr<FTouchImportTextureD3D12> GetSharedTexture(HANDLE Handle) const;
		
		static void TextureCallback(HANDLE Handle, TEObjectEvent Event, void* TE_NULLABLE Info);
	};
}

