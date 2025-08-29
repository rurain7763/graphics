#include "pch.h"
#include "VkTextures.h"

#ifdef SUPPORT_VULKAN

#include "VkContext.h"

namespace flaw {
	VkNativeTexture VkNativeTexture::Create(VkContext& context, const vk::ImageCreateInfo& imageCreateInfo, vk::MemoryPropertyFlags memoryProperties) {
		VkNativeTexture nativeTex = {};

		auto imageWrapper = context.GetVkDevice().createImage(imageCreateInfo);
		if (imageWrapper.result != vk::Result::eSuccess) {
			throw std::runtime_error("Failed to create image for texture");
		}

		nativeTex.image = imageWrapper.value;

		vk::MemoryRequirements memRequirements;
		context.GetVkDevice().getImageMemoryRequirements(nativeTex.image, &memRequirements);

		vk::MemoryAllocateInfo allocInfo;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = GetMemoryTypeIndex(context.GetVkPhysicalDevice(), memRequirements.memoryTypeBits, memoryProperties);

		auto memoryWrapper = context.GetVkDevice().allocateMemory(allocInfo);
		if (memoryWrapper.result != vk::Result::eSuccess) {
			throw std::runtime_error("Failed to allocate memory for texture");
		}

		nativeTex.memory = memoryWrapper.value;

		if (context.GetVkDevice().bindImageMemory(nativeTex.image, nativeTex.memory, 0) != vk::Result::eSuccess) {
			throw std::runtime_error("Failed to bind image memory for texture");
		}

		return nativeTex;
	}
}

#endif