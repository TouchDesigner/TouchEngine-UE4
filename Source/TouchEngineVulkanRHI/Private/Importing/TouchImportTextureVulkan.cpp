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

#include "TouchImportTextureVulkan.h"

#include "Logging.h"
#include "RHICommandCopyTouchToUnreal.h"
#include "VulkanImportUtils.h"
#include "Util/TextureShareVulkanPlatformWindows.h"
#include "Util/VulkanGetterUtils.h"
#include "Util/VulkanWindowsFunctions.h"
#include "VulkanTouchUtils.h"

#include "TouchEngine/TEVulkan.h"
#include "Util/TouchEngineStatsGroup.h"

namespace UE::TouchEngine::Vulkan
{
	TSharedPtr<FTouchImportTextureVulkan> FTouchImportTextureVulkan::CreateTexture(const TouchObject<TEVulkanTexture_>& SharedOutputTexture, TSharedRef<FVulkanSharedResourceSecurityAttributes> SecurityAttributes)
	{
		DECLARE_SCOPE_CYCLE_COUNTER(TEXT("      III.A.2.a [RT] Link Texture Import - CreateTexture"), STAT_TE_III_A_2_a_Vulkan, STATGROUP_TouchEngine);
		if (!AreVulkanFunctionsForWindowsLoaded())
		{
			UE_LOG(LogTouchEngineVulkanRHI, Error, TEXT("Failed to import because Vulkan Windows functions are not loaded"));
			return nullptr;
		}
		
		// Fail early if the pixel format is not known since we'll do some more construction on the render thread later
		const VkFormat FormatVk = TEVulkanTextureGetFormat(SharedOutputTexture);
		const EPixelFormat FormatUnreal = VulkanToUnrealTextureFormat(FormatVk);
		if (FormatUnreal == PF_Unknown)
		{
			UE_LOG(LogTouchEngineVulkanRHI, Error, TEXT("Failed to import because VkFormat %d could not be mapped"), FormatVk);
			return nullptr;
		}

		FTextureCreationResult TextureCreationResult = CreateSharedTouchVulkanTexture(SharedOutputTexture);
		return MakeShared<FTouchImportTextureVulkan>(TextureCreationResult.ImageHandleOwnership, TextureCreationResult.ImportedTextureMemoryOwnership, SharedOutputTexture, MoveTemp(SecurityAttributes));
	}

	FTouchImportTextureVulkan::FTouchImportTextureVulkan(
		TSharedPtr<VkImage> ImageHandle,
		TSharedPtr<VkDeviceMemory> ImportedTextureMemoryOwnership,
		TouchObject<TEVulkanTexture_> InSharedOutputTexture,
		TSharedRef<FVulkanSharedResourceSecurityAttributes> SecurityAttributes)
		: ImageHandle(MoveTemp(ImageHandle))
		, ImportedTextureMemoryOwnership(MoveTemp(ImportedTextureMemoryOwnership))
		, WeakSharedOutputTextureReference(MoveTemp(InSharedOutputTexture))
		, SecurityAttributes(MoveTemp(SecurityAttributes))
	{}

	FTouchImportTextureVulkan::~FTouchImportTextureVulkan()
	{
		if (WaitSemaphoreData.IsSet())
		{
			TEVulkanSemaphoreSetCallback(WaitSemaphoreData->TouchSemaphore, nullptr, nullptr);
		}
	}

	FTextureMetaData FTouchImportTextureVulkan::GetTextureMetaData() const
	{
		const uint32 Width = TEVulkanTextureGetWidth(WeakSharedOutputTextureReference);
		const uint32 Height = TEVulkanTextureGetHeight(WeakSharedOutputTextureReference);
		const VkFormat FormatVk = TEVulkanTextureGetFormat(WeakSharedOutputTextureReference);
		const EPixelFormat FormatUnreal = VulkanToUnrealTextureFormat(FormatVk);
		return FTextureMetaData{ Width, Height, FormatUnreal };
	}

	TFuture<ECopyTouchToUnrealResult> FTouchImportTextureVulkan::CopyNativeToUnreal_RenderThread(const FTouchCopyTextureArgs& CopyArgs, TSharedRef<FTouchTextureImporter> Importer)
	{
		return DispatchCopyTouchToUnrealRHICommand(CopyArgs, SharedThis(this));
	}

	ECopyTouchToUnrealResult FTouchImportTextureVulkan::CopyNativeToUnrealRHI_RenderThread(const FTouchCopyTextureArgs& CopyArgs, TSharedRef<FTouchTextureImporter> Importer)
	{
		return CopyTouchToUnrealRHICommand(CopyArgs, SharedThis(this));
	}

	const TSharedPtr<VkCommandBuffer>& FTouchImportTextureVulkan::EnsureCommandBufferInitialized(FRHICommandListBase& RHICmdList)
	{
		if (!CommandBuffer)
		{
			CommandBuffer = CreateCommandBuffer(RHICmdList);
		}
		return CommandBuffer;
	}

	void FTouchImportTextureVulkan::OnWaitVulkanSemaphoreUsageChanged(void* Semaphore, TEObjectEvent Event, void* Info)
	{
		// I think if it stops being used it is ok to just keep the semaphore alive and reuse in the future ... not need to destroy it, right?
	}
}
