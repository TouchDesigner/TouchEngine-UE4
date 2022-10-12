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
#include "Windows/PreWindowsApi.h"
#include "d3d11.h"
#include "Windows/PostWindowsApi.h"

namespace UE::TouchEngine::D3DX11
{
	class FTouchTextureLinkerD3D11 : public FTouchTextureLinker
	{
	public:

		FTouchTextureLinkerD3D11(TouchObject<TED3D11Context> Context, ID3D11DeviceContext& DeviceContext);

	protected:

		//~ Begin FTouchTextureLinker Interface
		virtual TFuture<TSharedPtr<ITouchPlatformTexture>> CreatePlatformTexture(const TouchObject<TEInstance>& Instance, const TouchObject<TETexture>& SharedTexture) override;
		//~ End FTouchTextureLinker Interface

	private:

		TouchObject<TED3D11Context> Context;
		ID3D11DeviceContext* DeviceContext;
		
		TouchObject<TED3D11Texture> CreatePlatformTexture(const TouchObject<TETexture>& SharedTexture) const;
	};
}
