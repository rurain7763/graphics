#include "pch.h"
#include "VkBuffers.h"

#ifdef SUPPORT_VULKAN

#include "VkContext.h"
#include "VkCommandQueue.h"
#include "Log/Log.h"

namespace flaw {
    VkStructuredBuffer::VkStructuredBuffer(VkContext& context, const Descriptor& desc)
        : _context(context)
		, _memProperty(desc.memProperty)
        , _elmSize(desc.elmSize)
        , _size(desc.bufferSize)
    {
        vk::BufferUsageFlags usage = vk::BufferUsageFlagBits::eStorageBuffer;
        GetRequiredVkBufferUsageFlags(_memProperty, usage);

        vk::MemoryPropertyFlags memoryFlags;
        GetRequiredVkMemoryPropertyFlags(_memProperty, memoryFlags);

        _nativeBuffer = VkNativeBuffer::Create(_context, _size, usage, memoryFlags);

        if (_memProperty != MemoryProperty::Static) {
            auto mappedDataWrapper = _context.GetVkDevice().mapMemory(_nativeBuffer.memory, 0, _size, vk::MemoryMapFlags());
            if (mappedDataWrapper.result != vk::Result::eSuccess) {
                Log::Error("Failed to map index buffer memory.");
                return;
            }
            _mappedData = mappedDataWrapper.value;
        }

		if (!desc.initialData) {
			return;
		}

        if (_memProperty == MemoryProperty::Static) {
            auto& vkCommandQueue = static_cast<VkCommandQueue&>(context.GetCommandQueue());

            Descriptor stagingDesc;
            stagingDesc.memProperty = MemoryProperty::Staging;
            stagingDesc.bufferSize = desc.bufferSize;
            stagingDesc.initialData = desc.initialData;

            auto stagingBuffer = CreateRef<VkStructuredBuffer>(context, stagingDesc);

            vk::CommandBuffer commandBuffer;
            vkCommandQueue.BeginOneTimeCommands(commandBuffer);

            vk::BufferCopy copyRegion;
            copyRegion.size = _size;
            copyRegion.srcOffset = 0;
            copyRegion.dstOffset = 0;

            commandBuffer.copyBuffer(stagingBuffer->_nativeBuffer.buffer, _nativeBuffer.buffer, 1, &copyRegion);

            vkCommandQueue.EndOneTimeCommands(commandBuffer);
        }
        else {
            Update(desc.initialData, _size);
        }
    }

    VkStructuredBuffer::~VkStructuredBuffer() {
        if (_mappedData) {
            _context.GetVkDevice().unmapMemory(_nativeBuffer.memory);
            _mappedData = nullptr;
        }

        _context.AddDelayedDeletionTasks([&context = _context, nativeBuffer = _nativeBuffer]() {
            context.GetVkDevice().destroyBuffer(nativeBuffer.buffer);
            context.GetVkDevice().freeMemory(nativeBuffer.memory);
        });
    }

    void VkStructuredBuffer::Update(const void* data, uint32_t size) {
        if (_size < size) {
            Log::Error("Update size exceeds structured buffer size.");
            return;
        }

        if (!_mappedData) {
            Log::Error("Structured buffer is not mapped.");
            return;
        }

        std::memcpy(_mappedData, data, size);
    }

    void VkStructuredBuffer::Fetch(void* data, uint32_t size) {
        // TODO: Implement Fetch method if needed
    }
}

#endif