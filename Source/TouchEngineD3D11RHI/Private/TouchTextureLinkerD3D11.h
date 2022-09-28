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
#include "Rendering/TouchTextureLinker.h"
#include "TouchEngine/TED3D11.h"

namespace UE::TouchEngine::D3DX11
{
	class FTouchTextureLinkerD3D11 : public FTouchTextureLinker
	{
	public:

		FTouchTextureLinkerD3D11(TED3D11Context& Context);

	protected:

		//~ Begin FTouchTextureLinker Interface
		virtual TouchObject<TETexture> CreatePlatformTextureFromShared(TETexture* SharedTexture) const override;
		virtual int32 GetPlatformTextureWidth(TETexture* Texture) const override;
		virtual int32 GetPlatformTextureHeight(TETexture* Texture) const override;
		virtual EPixelFormat GetPlatformTexturePixelFormat(TETexture* Texture) const override;
		virtual bool CopyNativeResources(TETexture* SourcePlatformTexture, UTexture2D* Target) const override;
		//~ End FTouchTextureLinker Interface

	private:

		TED3D11Context* Context;

		D3D11_TEXTURE2D_DESC GetDescriptor(TETexture* Texture) const;
	};
}
