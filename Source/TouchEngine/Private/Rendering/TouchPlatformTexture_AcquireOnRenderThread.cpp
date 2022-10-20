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

#include "Rendering/TouchPlatformTexture_AcquireOnRenderThread.h"

#include "TouchEngine/TEInstance.h"

namespace UE::TouchEngine
{
	bool FTouchPlatformTexture_AcquireOnRenderThread::CopyNativeToUnreal(const FTouchCopyTextureArgs& CopyArgs)
	{
		const TouchObject<TEInstance> Instance = CopyArgs.RequestParams.Instance;
		const TouchObject<TETexture> SharedTexture = CopyArgs.RequestParams.Texture;
		
		if (SharedTexture && TEInstanceHasTextureTransfer(Instance, SharedTexture))
		{
			TouchObject<TESemaphore> Semaphore;
			uint64 WaitValue;
			const TEResult ResultCode = TEInstanceGetTextureTransfer(Instance, SharedTexture, Semaphore.take(), &WaitValue);
			if (ResultCode != TEResultSuccess)
			{
				return false;
			}

			const bool bSuccess = AcquireMutex(CopyArgs, Semaphore, WaitValue);
			if (!bSuccess)
			{
				return false;
			}
				
			CopyArgs.RHICmdList.CopyTexture(ReadTextureDuringMutex(), CopyArgs.Target->GetResource()->TextureRHI->GetTexture2D(), FRHICopyTextureInfo());
			ReleaseMutex(CopyArgs, Semaphore, WaitValue);
		}

		return false;
	}
}
