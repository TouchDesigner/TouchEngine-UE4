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
#include "Rendering/Exporting/ExportedTouchTexture.h"

#include "TouchEngine/TouchObject.h"

namespace UE::TouchEngine::D3DX12
{
	class FTextureShareD3D12SharedResourceSecurityAttributes;
	
	class FExportedTextureD3D12 : public FExportedTouchTexture
	{
	public:
		
		static TSharedPtr<FExportedTextureD3D12> Create(const FRHITexture& SourceRHI, const FTextureShareD3D12SharedResourceSecurityAttributes& SharedResourceSecurityAttributes);
		
		FExportedTextureD3D12(FTextureRHIRef SharedTextureRHI, const FGuid& ResourceId, void* ResourceSharingHandle, const TouchObject<TED3DSharedTexture>& TouchRepresentation);
		//~ Begin FExportedTouchTexture Interface
		virtual bool CanFitTexture(const FRHITexture* TextureToFit) const override;
		//~ End FExportedTouchTexture Interface

		const FTextureRHIRef& GetSharedTextureRHI() const { return SharedTextureRHI; }

	protected:
		virtual void RemoveTextureCallback() override;

	private:

		/** Shared between Unreal and TE. Access must be synchronized. */
		FTextureRHIRef SharedTextureRHI;

		/** Used to handle the ID of the resource */
		FGuid ResourceId;
		/** Handle to the shared resource */
		void* ResourceSharingHandle;

		static void TouchTextureCallback(void* Handle, TEObjectEvent Event, void* Info);
	};

}

