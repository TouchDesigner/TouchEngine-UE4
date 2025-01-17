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

#include "Exporting/ExportedTextureVulkan.h"
#include "RHI.h"
#include "TextureResource.h"

#include "Logging.h"
#include "VulkanTouchUtils.h"
THIRD_PARTY_INCLUDES_START
#include "vulkan_core.h"
THIRD_PARTY_INCLUDES_END

#include "Engine/Texture.h"
#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include "vulkan_win32.h"
// Very hacky way of getting access to DXGI_SHARED_RESOURCE_READ and DXGI_SHARED_RESOURCE_WRITE without curios dependency in build.cs file
#include "ThirdParty/Windows/DirectX/include/dxgi1_2.h"
#include "Windows/HideWindowsPlatformTypes.h"
#include "Util/VulkanWindowsFunctions.h"
#endif

#include "Engine/TEDebug.h"
#include "Importing/VulkanImportUtils.h"
#include "TEVulkanInclude.h"
#include "Util/TextureShareVulkanPlatformWindows.h"
#include "Util/TouchEngineStatsGroup.h"
#include "Util/TouchHelpers.h"
#include "Util/VulkanGetterUtils.h"


DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Export - No Vulkan Textures"), STAT_TENoVulkanTextures, STATGROUP_TouchEngine);

namespace UE::TouchEngine::Vulkan
{
	namespace Private
	{
		struct FInputVulkanTextureData
		{
			TSharedPtr<VkImage> ImageOwnership;
			TSharedPtr<VkDeviceMemory> TextureMemoryOwnership;
			
			HANDLE VulkanSharedHandle;
			VkExternalMemoryHandleTypeFlagBits MemoryHandleFlags;
		};
		
		static TOptional<FInputVulkanTextureData> CreateSharedVulkanTexture(const FIntPoint& Resolution, const VkFormat VulkanFormat, const TSharedRef<FVulkanSharedResourceSecurityAttributes>& SecurityAttributes)
		{
#if !PLATFORM_WINDOWS
			static_assert(false, "You must implement a different sharing method. On windows we used NT handles.");
#endif

			const FVulkanPointers Vulkan;
			FInputVulkanTextureData Result;
			
			VkExternalMemoryImageCreateInfo ExternalMemoryImageCreateInfo = { VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO };
			constexpr bool bUseWindowNT = true;
			Result.MemoryHandleFlags = bUseWindowNT ? VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT : VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT;
			ExternalMemoryImageCreateInfo.handleTypes = Result.MemoryHandleFlags;
			VkImageCreateInfo TexCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, &ExternalMemoryImageCreateInfo };
			{
				TexCreateInfo.imageType = VK_IMAGE_TYPE_2D;
				TexCreateInfo.format = VulkanFormat;
				TexCreateInfo.extent.width = Resolution.X;
				TexCreateInfo.extent.height = Resolution.Y;
				TexCreateInfo.extent.depth = 1;
				TexCreateInfo.mipLevels = 1;
				TexCreateInfo.arrayLayers = 1;
				TexCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
				TexCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
				TexCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
				TexCreateInfo.flags = 0;
				TexCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
				TexCreateInfo.initialLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				
				VkImage NakedImageHandle;
				VERIFYVULKANRESULT(VulkanRHI::vkCreateImage(Vulkan.VulkanDeviceHandle, &TexCreateInfo, NULL, &NakedImageHandle));
				INC_DWORD_STAT(STAT_TENoVulkanTextures)
				Result.ImageOwnership = MakeShareable<VkImage>(new VkImage(NakedImageHandle), [Device = Vulkan.VulkanDeviceHandle](const VkImage* Memory)
				{
					VulkanRHI::vkDestroyImage(Device, *Memory, nullptr);
					DEC_DWORD_STAT(STAT_TENoVulkanTextures)
					delete Memory;
				});
			}
			
	        VkMemoryRequirements ImageMemoryRequirements = { };
	        VulkanRHI::vkGetImageMemoryRequirements(Vulkan.VulkanDeviceHandle, *Result.ImageOwnership.Get(), &ImageMemoryRequirements);

			VkPhysicalDeviceMemoryBudgetPropertiesEXT MemoryBudget { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT };
			VkPhysicalDeviceMemoryProperties2 MemoryProperties { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2, &MemoryBudget };
			VulkanRHI::vkGetPhysicalDeviceMemoryProperties2(Vulkan.VulkanPhysicalDeviceHandle, &MemoryProperties);
	        const uint32 MemoryTypeIndex = GetMemoryTypeIndex(MemoryProperties, ImageMemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	        if (MemoryTypeIndex >= MemoryProperties.memoryProperties.memoryTypeCount)
	        {
	        	UE_LOG(LogTouchEngineVulkanRHI, Error, TEXT("Vulkan: Memory doesn't support sharing"));
	        	return {};
	        }

	        VkExportMemoryAllocateInfo ExportMemInfo = { VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO };
	        ExportMemInfo.handleTypes = ExternalMemoryImageCreateInfo.handleTypes;
	        VkExportMemoryWin32HandleInfoKHR ExportMemWin32Info = { VK_STRUCTURE_TYPE_EXPORT_MEMORY_WIN32_HANDLE_INFO_KHR };
	        if (ExternalMemoryImageCreateInfo.handleTypes == VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT)
	        {
	            ExportMemWin32Info.dwAccess = DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE;
	            ExportMemWin32Info.pAttributes = SecurityAttributes->Get();
	            ExportMemInfo.pNext = &ExportMemWin32Info;
	        }

	        VkMemoryAllocateInfo MemInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, &ExportMemInfo };
			{
				MemInfo.allocationSize = ImageMemoryRequirements.size;
		        MemInfo.memoryTypeIndex = MemoryTypeIndex;

				// Is this needed?
				/*VkPhysicalDeviceExternalImageFormatInfo ExternalImageFormatInfo = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_IMAGE_FORMAT_INFO };
				ExternalImageFormatInfo.handleType = Result.MemoryHandleFlags;
		        VkMemoryDedicatedAllocateInfo memoryDedicatedAllocateInfo = { VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO };
				VkPhysicalDeviceImageFormatInfo2 ImageFormatInfo = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2, &ExternalImageFormatInfo };
				ImageFormatInfo.format  = VulkanFormat;
				ImageFormatInfo.type    = TexCreateInfo.imageType;
				ImageFormatInfo.tiling  = TexCreateInfo.tiling;
				ImageFormatInfo.usage   = TexCreateInfo.usage;
				ImageFormatInfo.flags   = TexCreateInfo.flags;
				VkMemoryDedicatedAllocateInfo DedicatedAllocationMemoryAllocationInfo = { VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO };
				VkExternalImageFormatProperties ExternalImageFormatProperties = { VK_STRUCTURE_TYPE_EXTERNAL_IMAGE_FORMAT_PROPERTIES };
				VkImageFormatProperties2 ImageFormatProperties = { VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2, &ExternalImageFormatProperties };
				VERIFYVULKANRESULT(vkGetPhysicalDeviceImageFormatProperties2(Vulkan.VulkanPhysicalDeviceHandle, &ImageFormatInfo, &ImageFormatProperties));
				bool bUseDedicatedMemory = ExternalImageFormatProperties.externalMemoryProperties.externalMemoryFeatures & VK_EXTERNAL_MEMORY_FEATURE_DEDICATED_ONLY_BIT;
		        if (bUseDedicatedMemory)
	        	{
		            memoryDedicatedAllocateInfo.image = *Result.ImageOwnership.Get();
		            memoryDedicatedAllocateInfo.pNext = MemInfo.pNext;
		            MemInfo.pNext = &memoryDedicatedAllocateInfo;
		        }*/

				VkDeviceMemory NakedMemoryHandle;
		        VERIFYVULKANRESULT(VulkanRHI::vkAllocateMemory(Vulkan.VulkanDeviceHandle, &MemInfo, nullptr, &NakedMemoryHandle));
				Result.TextureMemoryOwnership = MakeShareable<VkDeviceMemory>(new VkDeviceMemory(NakedMemoryHandle), [Device = Vulkan.VulkanDeviceHandle](const VkDeviceMemory* Memory)
				{
					VulkanRHI::vkFreeMemory(Device, *Memory, nullptr);
					UE_LOG(LogTouchEngineVulkanRHI, VeryVerbose, TEXT("[Result.TextureMemoryOwnership DELETER]"))
					delete Memory;
				});
			}
			
	        VkMemoryGetWin32HandleInfoKHR memoryGetWin32HandleInfo = { VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR };
	        memoryGetWin32HandleInfo.memory = *Result.TextureMemoryOwnership.Get();
	        memoryGetWin32HandleInfo.handleType = static_cast<VkExternalMemoryHandleTypeFlagBits>(ExternalMemoryImageCreateInfo.handleTypes);
	        VERIFYVULKANRESULT(Vulkan::vkGetMemoryWin32HandleKHR(Vulkan.VulkanDeviceHandle, &memoryGetWin32HandleInfo, &Result.VulkanSharedHandle));

	        VERIFYVULKANRESULT(VulkanRHI::vkBindImageMemory(Vulkan.VulkanDeviceHandle, *Result.ImageOwnership.Get(), *Result.TextureMemoryOwnership.Get(), 0));
			return Result;
		}
	}
	
	TSharedPtr<FExportedTextureVulkan> FExportedTextureVulkan::Create(const FRHITexture& SourceRHI, const TSharedRef<FVulkanSharedResourceSecurityAttributes>& SecurityAttributes)
	{
		DECLARE_SCOPE_CYCLE_COUNTER(TEXT("      I.B.1.a [GT] Cook Frame - Vulkan::CreateTexture"), STAT_TE_I_B_1_a_Vulkan, STATGROUP_TouchEngine);
		const EPixelFormat PixelFormat = SourceRHI.GetFormat();
		const FIntPoint Resolution = SourceRHI.GetSizeXY();
		const bool bIsSRGB = EnumHasAnyFlags(SourceRHI.GetDesc().Flags, ETextureCreateFlags::SRGB);

		VkComponentMapping Mapping;
		const VkFormat VulkanFormat = UnrealToVulkanTextureFormat(PixelFormat, bIsSRGB, Mapping);
		if (VulkanFormat == VK_FORMAT_UNDEFINED)
		{
			UE_LOG(LogTouchEngineVulkanRHI, Error, TEXT("Failed to import because PixelFormat %s could not be mapped"), GPixelFormats[PixelFormat].Name);
			return nullptr;
		}

		TOptional<Private::FInputVulkanTextureData> SharedTextureInfo;
		{
			DECLARE_SCOPE_CYCLE_COUNTER(TEXT("      I.B.1.b [GT] Cook Frame - Vulkan::CreateTexture - CreateSharedVulkanTexture"), STAT_TE_I_B_1_b_Vulkan, STATGROUP_TouchEngine);
			SharedTextureInfo = Private::CreateSharedVulkanTexture(Resolution, VulkanFormat, SecurityAttributes);
			if (!SharedTextureInfo)
			{
				UE_LOG(LogTouchEngineVulkanRHI, Error, TEXT("Failed to import because the shared Vulkan texutre could not be created"));
				return nullptr;
			}
		}
		
		TouchObject<TEVulkanTexture> SharedTouchTexture = TouchObject<TEVulkanTexture>::make_take(TEVulkanTextureCreate(SharedTextureInfo->VulkanSharedHandle, SharedTextureInfo->MemoryHandleFlags, VulkanFormat, Resolution.X, Resolution.Y, TETextureOriginTopLeft, Mapping, nullptr, nullptr));
		if(SharedTextureInfo->MemoryHandleFlags == VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT)
		{
			CloseHandle(SharedTextureInfo->VulkanSharedHandle); // we need to release the vulkan handle
		}
		
		if (!SharedTouchTexture)
		{
			UE_LOG(LogTouchEngineVulkanRHI, Error, TEXT("TEVulkanTextureCreate failed"));
			return nullptr;
		}
		
		return MakeShared<FExportedTextureVulkan>(SharedTouchTexture, PixelFormat, Resolution, bIsSRGB, SharedTextureInfo->ImageOwnership.ToSharedRef(), SharedTextureInfo->TextureMemoryOwnership.ToSharedRef());
	}

	FExportedTextureVulkan::FExportedTextureVulkan(
		TouchObject<TEVulkanTexture> SharedTexture,
		EPixelFormat PixelFormat,
		const FIntPoint& TextureBounds,
		bool bInIsSRGB,
		TSharedRef<VkImage> ImageOwnership,
		TSharedRef<VkDeviceMemory> TextureMemoryOwnership
		)
		: FExportedTouchTexture(MoveTemp(SharedTexture), [this](const TouchObject<TETexture>& Texture)
		{
			TEVulkanTexture* VulkanTexture = static_cast<TEVulkanTexture*>(Texture.get());
			TEVulkanTextureSetCallback(VulkanTexture, TouchTextureCallback, this);
		})
		, PixelFormat(PixelFormat)
		, Resolution(TextureBounds)
		, bIsSRGB(bInIsSRGB)
		, ImageOwnership(MoveTemp(ImageOwnership))
		, TextureMemoryOwnership(MoveTemp(TextureMemoryOwnership))
	{}

	bool FExportedTextureVulkan::CanFitTexture(const FRHITexture* TextureToFit) const
	{
		return TextureToFit
			&& TextureToFit->GetSizeXY() == Resolution
			&& TextureToFit->GetFormat() == PixelFormat
			&& EnumHasAnyFlags(TextureToFit->GetFlags(), ETextureCreateFlags::SRGB) == bIsSRGB;
	}

	const TSharedPtr<VkCommandBuffer>& FExportedTextureVulkan::EnsureCommandBufferInitialized(FRHICommandListBase& RHICmdList)
	{
		if (!CommandBuffer)
		{
			CommandBuffer = CreateCommandBuffer(RHICmdList);
		}
		return CommandBuffer;
	}

	void FExportedTextureVulkan::RemoveTextureCallback()
	{
		TEVulkanTexture* Casted = static_cast<TEVulkanTexture*>(GetTouchRepresentation().get()); //todo: this returns null so this does not cancel the callback
		TEVulkanTextureSetCallback(Casted, nullptr, nullptr);
	}

	void FExportedTextureVulkan::TouchTextureCallback(void* Handle, TEObjectEvent Event, void* Info)
	{
		FExportedTextureVulkan* This = static_cast<FExportedTextureVulkan*>(Info);
		UE_LOG(LogTouchEngineVulkanRHI, Verbose, TEXT("[FExportedTextureVulkan::TouchTextureCallback[%s]] Received FExportedTextureVulkan Event `%s` for `%s`"), *GetCurrentThreadStr(), *TEObjectEventToString(Event), *This->DebugName)
		This->OnTouchTextureUseUpdate(Event);
	}

	void FExportedTextureVulkan::OnWaitVulkanSemaphoreUsageChanged(void* Semaphore, TEObjectEvent Event, void* Info)
	{
		//todo: this sometimes gets called after the semaphore has been destroyed
		// FExportedTextureVulkan* This = static_cast<FExportedTextureVulkan*>(Info);
		// UE_LOG(LogTouchEngineVulkanRHI, Warning, TEXT("[FExportedTextureVulkan::OnWaitVulkanSemaphoreUsageChanged[%s]] Event `%s` for `%s`"), *GetCurrentThreadStr(), *TEObjectEventToString(Event), *(This ? This->DebugName : TEXT("")))
		// I think if it stops being used it is ok to just keep the semaphore alive and reuse in the future ... not need to destroy it, right?
	}
}

// Do not pollute other cpp files in unity builds
#if PLATFORM_WINDOWS
#endif