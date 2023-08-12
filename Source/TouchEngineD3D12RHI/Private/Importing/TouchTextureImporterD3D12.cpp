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

#include "TouchTextureImporterD3D12.h"

#include "D3D12TouchUtils.h"
#include "ID3D12DynamicRHI.h"
#include "TouchImportTextureD3D12.h"
#include "TouchEngine/TED3D.h"
#include "Util/TouchEngineStatsGroup.h"
#include "Logging.h"
#include "Exporting/TextureShareD3D12PlatformWindows.h"

// macro to deal with COM calls inside a function that returns on failure
#define CHECK_HR_DEFAULT(COM_call)\
	{\
		HRESULT Res = COM_call;\
		if (FAILED(Res))\
		{\
			UE_LOG(LogTouchEngineD3D12RHI, Error, TEXT("`" #COM_call "` failed: 0x%X - %s"), Res, *GetComErrorDescription(Res)); \
			return;\
		}\
	}

namespace UE::TouchEngine::D3DX12
{
	FTouchTextureImporterD3D12::FTouchTextureImporterD3D12(ID3D12Device* Device, TSharedRef<FTouchFenceCache> FenceCache)
		: Device(Device)
		, FenceCache(MoveTemp(FenceCache))
	{
		constexpr D3D12_COMMAND_LIST_TYPE QueueType = D3D12_COMMAND_LIST_TYPE_COPY;
		ID3D12DynamicRHI* RHI = GetID3D12DynamicRHI();
		ID3D12CommandQueue* DefaultCommandQueue = RHI->RHIGetCommandQueue();
		
		// inspired by FD3D12Queue::FD3D12Queue
		D3D12_COMMAND_QUEUE_DESC CommandQueueDesc = {};
		CommandQueueDesc.Type = QueueType; // GetD3DCommandListType((ED3D12QueueType)QueueType);
		CommandQueueDesc.Priority = 0;
		CommandQueueDesc.NodeMask = DefaultCommandQueue->GetDesc().NodeMask;;
		CommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		CHECK_HR_DEFAULT(Device->CreateCommandQueue(&CommandQueueDesc, IID_PPV_ARGS(D3DCommandQueue.GetInitReference())));
		CHECK_HR_DEFAULT(D3DCommandQueue->SetName(*FString::Printf(TEXT("FTouchTextureImporterD3D12 (GPU %d)"), CommandQueueDesc.NodeMask)))

		CommandQueueFence = FenceCache->GetOrCreateOwnedFence_AnyThread(true);
	}

	FTouchTextureImporterD3D12::~FTouchTextureImporterD3D12()
	{
		D3DCommandQueue = nullptr; // this should release the command queue
		CommandQueueFence.Reset(); // this will be destroyed or reused by the FenceCache.
	}
	

	TSharedPtr<ITouchImportTexture> FTouchTextureImporterD3D12::CreatePlatformTexture_RenderThread(const TouchObject<TEInstance>& Instance, const TouchObject<TETexture>& SharedTexture)
	{
		check(TETextureGetType(SharedTexture) == TETextureTypeD3DShared);
		TED3DSharedTexture* Shared = static_cast<TED3DSharedTexture*>(SharedTexture.get());
		const HANDLE Handle = TED3DSharedTextureGetHandle(Shared);
		if (const TSharedPtr<FTouchImportTextureD3D12> Existing = GetSharedTexture(Handle))
		{
			return Existing;
		}

		const TSharedPtr<FTouchImportTextureD3D12> NewTexture = FTouchImportTextureD3D12::CreateTexture_RenderThread(Device, Shared, FenceCache);
		if (!NewTexture)
		{
			return nullptr;
		}

		TED3DSharedTextureSetCallback(Shared, TextureCallback, this);
		CachedTextures.Add(Handle, NewTexture.ToSharedRef());

		return StaticCastSharedPtr<ITouchImportTexture>(NewTexture);
	}

	FTextureMetaData FTouchTextureImporterD3D12::GetTextureMetaData(const TouchObject<TETexture>& Texture) const
	{
		const TED3DSharedTexture* Source = static_cast<TED3DSharedTexture*>(Texture.get());
		const DXGI_FORMAT Format = TED3DSharedTextureGetFormat(Source);
		FTextureMetaData Result;
		Result.SizeX = TED3DSharedTextureGetWidth(Source);
		Result.SizeY = TED3DSharedTextureGetHeight(Source);
		Result.PixelFormat = ConvertD3FormatToPixelFormat(Format, Result.IsSRGB);
		return Result;
	}

	void FTouchTextureImporterD3D12::CopyNativeToUnreal_RenderThread(const TSharedPtr<ITouchImportTexture>& TETexture, const FTouchCopyTextureArgs& CopyArgs)
	{
		FTouchTextureImporter::CopyNativeToUnreal_RenderThread(TETexture, CopyArgs);
	}

	TSharedPtr<FTouchImportTextureD3D12> FTouchTextureImporterD3D12::GetSharedTexture(HANDLE Handle) const
	{
		const TSharedRef<FTouchImportTextureD3D12>* Result = CachedTextures.Find(Handle);
		return Result
			? *Result
			: TSharedPtr<FTouchImportTextureD3D12>{ nullptr };
	}

	void FTouchTextureImporterD3D12::TextureCallback(HANDLE Handle, TEObjectEvent Event, void* Info)
	{
		if (Event == TEObjectEventRelease)
		{
			FTouchTextureImporterD3D12* This = static_cast<FTouchTextureImporterD3D12*>(Info);
			This->CachedTextures.Remove(Handle);
		}
	}
}

