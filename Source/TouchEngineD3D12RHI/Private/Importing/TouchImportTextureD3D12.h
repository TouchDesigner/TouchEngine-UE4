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
#include "Rendering/Importing/TouchImportTexture_AcquireOnRenderThread.h"
#include "Util/TouchFenceCache.h"

#include "Windows/AllowWindowsPlatformTypes.h"
THIRD_PARTY_INCLUDES_START
#include <d3d12.h>
THIRD_PARTY_INCLUDES_END
#include "Windows/HideWindowsPlatformTypes.h"

#include "TouchEngine/TouchObject.h"
#include "TouchEngine/TESemaphore.h"

namespace UE::TouchEngine::D3DX12
{
	class FTouchImportTextureD3D12 : public FTouchImportTexture_AcquireOnRenderThread
	{
		using Super = FTouchImportTexture_AcquireOnRenderThread;
	public:
		
		template<typename T>
		using TComPtr = Microsoft::WRL::ComPtr<T>;

		static TSharedPtr<FTouchImportTextureD3D12> CreateTexture_RenderThread(ID3D12Device* Device, const TED3DSharedTexture* Shared, TSharedRef<FTouchFenceCache> FenceCache);

		FTouchImportTextureD3D12(
			const FTextureRHIRef& TextureRHI,
			Microsoft::WRL::ComPtr<ID3D12Resource> SourceResource,
			TSharedRef<FTouchFenceCache> FenceCache,
			TSharedRef<FTouchFenceCache::FFenceData> ReleaseMutexSemaphore
			);
		
		//~ Begin ITouchPlatformTexture Interface
		virtual FTextureMetaData GetTextureMetaData() const override;
		virtual bool IsCurrentCopyDone() override;
		//~ End ITouchPlatformTexture Interface
		
	protected:

		//~ Begin FTouchPlatformTexture_AcquireOnRenderThread Interface
		virtual bool AcquireMutex(const FTouchCopyTextureArgs& CopyArgs, const TouchObject<TESemaphore>& Semaphore, uint64 WaitValue) override;
		virtual FTextureRHIRef ReadTextureDuringMutex() override { return DestTextureRHI; }
		virtual void ReleaseMutex_RenderThread(const FTouchCopyTextureArgs& CopyArgs, const TouchObject<TESemaphore>& Semaphore, FTextureRHIRef& SourceTexture) override;
		virtual void CopyTexture_RenderThread(FRHICommandListImmediate& RHICmdList, const FTextureRHIRef SrcTexture, const FTextureRHIRef DstTexture, TSharedRef<FTouchTextureImporter> Importer) override;
		//~ End FTouchPlatformTexture_AcquireOnRenderThread Interface

	private:

		FTextureRHIRef DestTextureRHI;
		Microsoft::WRL::ComPtr<ID3D12Resource> SourceResource;
		
		TSharedRef<FTouchFenceCache> FenceCache;
		TSharedRef<FTouchFenceCache::FFenceData> ReleaseMutexSemaphore;
	};
}