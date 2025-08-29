#include "pch.h"
#include "VkBuffers.h"

#ifdef SUPPORT_VULKAN

#include "VkContext.h"

namespace flaw {
	VkNativeBuffer VkNativeBuffer::Create(VkContext& context, uint64_t size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties) {
		VkNativeBuffer result;

		vk::BufferCreateInfo bufferInfo;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = vk::SharingMode::eExclusive;

		auto bufferWrapper = context.GetVkDevice().createBuffer(bufferInfo, nullptr);
		if (bufferWrapper.result != vk::Result::eSuccess) {
			throw std::runtime_error("Failed to create buffer");
		}

		result.buffer = bufferWrapper.value;

		vk::MemoryRequirements memRequirements = context.GetVkDevice().getBufferMemoryRequirements(result.buffer);

		vk::MemoryAllocateInfo allocInfo;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = GetMemoryTypeIndex(context.GetVkPhysicalDevice(), memRequirements.memoryTypeBits, properties);

		auto memoryWrapper = context.GetVkDevice().allocateMemory(allocInfo, nullptr);
		if (memoryWrapper.result != vk::Result::eSuccess) {
			throw std::runtime_error("Failed to allocate buffer memory");
		}

		result.memory = memoryWrapper.value;

		context.GetVkDevice().bindBufferMemory(result.buffer, result.memory, 0);

		return result;
	}

	VkNativeBuffer VkNativeBuffer::CreateAsStaging(VkContext& context, uint64_t size, const void* initialData) {
		auto stagingBuffer = Create(context, size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

		auto mappedDataWrapper = context.GetVkDevice().mapMemory(stagingBuffer.memory, 0, size, vk::MemoryMapFlags());
		if (mappedDataWrapper.result != vk::Result::eSuccess) {
			throw std::runtime_error("Failed to map staging buffer memory");
		}

		memcpy(mappedDataWrapper.value, initialData, size);

		context.GetVkDevice().unmapMemory(stagingBuffer.memory);

		return stagingBuffer;
	}
}

#endif