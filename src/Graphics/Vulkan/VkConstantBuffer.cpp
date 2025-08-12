#include "pch.h"
#include "VkBuffers.h"

#ifdef SUPPORT_VULKAN

#include "VkContext.h"
#include "Log/Log.h"

namespace flaw {
	VkConstantBuffer::VkConstantBuffer(VkContext& context, const Descriptor& descriptor)
		: _context(context)
		, _size(descriptor.bufferSize)
    {
        if (!CreateBuffer()) {
            return;
        }

        if (!AllocateMemory()) {
            return;
        }

        _bufferInfo.buffer = _buffer;
        _bufferInfo.offset = 0;
        _bufferInfo.range = _size;

        auto mappedDataWrapper = _context.GetVkDevice().mapMemory(_memory, 0, _size, vk::MemoryMapFlags());
        if (mappedDataWrapper.result != vk::Result::eSuccess) {
            Log::Error("Failed to map constant buffer memory.");
            return;
        }

        _mappedData = mappedDataWrapper.value;
	}

	VkConstantBuffer::~VkConstantBuffer() {
        if (_mappedData) {
            _context.GetVkDevice().unmapMemory(_memory);
            _mappedData = nullptr;
        }

        _context.AddDelayedDeletionTasks([&context = _context, buffer = _buffer, memory = _memory]() {
            context.GetVkDevice().destroyBuffer(buffer, nullptr);
            context.GetVkDevice().freeMemory(memory, nullptr);
        });
	}

	void VkConstantBuffer::Update(const void* data, int32_t size) {
		if (size > _size) {
            Log::Error("Data size exceeds constant buffer size.");
            return;
        }

        if (_mappedData) {
            std::memcpy(_mappedData, data, size);
        } else {
            Log::Error("Constant buffer memory is not mapped.");
        }
	}

    bool VkConstantBuffer::CreateBuffer() {
        vk::BufferCreateInfo bufferInfo;
        bufferInfo.size = _size;
        bufferInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst;
        bufferInfo.sharingMode = vk::SharingMode::eExclusive;

        auto bufferWrapper = _context.GetVkDevice().createBuffer(bufferInfo, nullptr);
        if (bufferWrapper.result != vk::Result::eSuccess) {
            Log::Error("Failed to create vertex buffer: %s", vk::to_string(bufferWrapper.result).c_str());
            return false;
        }

        _buffer = bufferWrapper.value;

        return true;
    }

    bool VkConstantBuffer::AllocateMemory() {
        vk::MemoryRequirements memRequirements = _context.GetVkDevice().getBufferMemoryRequirements(_buffer);

        vk::MemoryAllocateInfo allocInfo;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = GetMemoryTypeIndex(_context.GetVkPhysicalDevice(), memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        auto memoryWrapper = _context.GetVkDevice().allocateMemory(allocInfo, nullptr);
        if (memoryWrapper.result != vk::Result::eSuccess) {
            Log::Error("Failed to allocate vertex buffer memory: %s", vk::to_string(memoryWrapper.result).c_str());
            return false;
        }

        _memory = memoryWrapper.value;

        _context.GetVkDevice().bindBufferMemory(_buffer, _memory, 0);

        return true;
    }
} // namespace flaw

#endif
