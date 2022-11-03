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

#include "TouchFenceCache.h"

#include "TouchEngine/TED3D.h"

namespace UE::TouchEngine::D3DX12
{
	FTouchFenceCache::FTouchFenceCache(ID3D12Device* Device)
		: Device(Device)
	{}

	FTouchFenceCache::~FTouchFenceCache()
	{
		FScopeLock Lock(&CachedFencesMutex);
		for(auto Pair : CachedFences)
		{
			TED3DSharedFenceSetCallback(Pair.Value.TouchResource.get(), nullptr, nullptr);
		}
	}

	FTouchFenceCache::TComPtr<ID3D12Fence> FTouchFenceCache::GetOrCreateSharedFence(const TouchObject<TESemaphore>& Semaphore)
	{
		check(TESemaphoreGetType(Semaphore) == TESemaphoreTypeD3DFence);
		FScopeLock Lock(&CachedFencesMutex);
		
		const HANDLE Handle = TED3DSharedFenceGetHandle(static_cast<TED3DSharedFence*>(Semaphore.get()));
		if (const TComPtr<ID3D12Fence> Existing = GetSharedFence(Handle))
		{
			return Existing;
		}
		
		TComPtr<ID3D12Fence> Fence;
		const HRESULT Result = Device->OpenSharedHandle(Handle, IID_PPV_ARGS(&Fence));
		if (FAILED(Result))
		{
			return nullptr;
		}
		
		TED3DSharedFenceSetCallback(static_cast<TED3DSharedFence*>(Semaphore.get()), FenceCallback, this);
		TouchObject<TED3DSharedFence> FenceObject;
		FenceObject.set(static_cast<TED3DSharedFence*>(Semaphore.get()));
		CachedFences.Add(Handle, { Fence, FenceObject });
		return Fence;
	}

	FTouchFenceCache::TComPtr<ID3D12Fence> FTouchFenceCache::GetSharedFence(HANDLE Handle) const
	{
		const FFenceData* FenceData = CachedFences.Find(Handle);
		return FenceData && ensure(FenceData->Fence)
			? FenceData->Fence
			: TComPtr<ID3D12Fence>{ nullptr };
	}
	
	void FTouchFenceCache::FenceCallback(HANDLE Handle, TEObjectEvent Event, void* Info)
	{
		if (Event == TEObjectEventRelease)
		{
			FTouchFenceCache* This = static_cast<FTouchFenceCache*>(Info);
			FScopeLock Lock(&This->CachedFencesMutex);
			This->CachedFences.Remove(Handle);
		}
	}
}
