#include "pch.h"
#include "VkBuffers.h"

#ifdef SUPPORT_VULKAN

#include "VkContext.h"
#include "Log/Log.h"
#include "VkCommandQueue.h"

namespace flaw {
    VkIndexBuffer::VkIndexBuffer(VkContext& context, const Descriptor& descriptor)
        : _context(context)
        , _memProperty(descriptor.memProperty)
        , _size(descriptor.bufferSize)
        , _indexCount(descriptor.bufferSize / sizeof(uint32_t))
    {
        vk::BufferUsageFlags usage = vk::BufferUsageFlagBits::eIndexBuffer;
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

        if (!descriptor.initialData) {
            return;
        }

        if (_memProperty == MemoryProperty::Static) {
            auto& vkCommandQueue = static_cast<VkCommandQueue&>(context.GetCommandQueue());

            Descriptor stagingDesc;
            stagingDesc.memProperty = MemoryProperty::Staging;
            stagingDesc.bufferSize = descriptor.bufferSize;
            stagingDesc.initialData = descriptor.initialData;

            auto stagingBuffer = CreateRef<VkIndexBuffer>(context, stagingDesc);

            vk::CommandBuffer commandBuffer = vkCommandQueue.BeginOneTimeCommands();

			vk::BufferCopy copyRegion;
			copyRegion.size = _size;
			copyRegion.srcOffset = 0;
			copyRegion.dstOffset = 0;

			commandBuffer.copyBuffer(stagingBuffer->_nativeBuffer.buffer, _nativeBuffer.buffer, 1, &copyRegion);

            vkCommandQueue.EndOneTimeCommands(commandBuffer);
        } else {
            Update(descriptor.initialData, _indexCount);
        }
    }

    VkIndexBuffer::~VkIndexBuffer() {
        if (_mappedData) {
            _context.GetVkDevice().unmapMemory(_nativeBuffer.memory);
            _mappedData = nullptr;
        }

        _context.AddDelayedDeletionTasks([&context = _context, nativeBuffer = _nativeBuffer]() {
            context.GetVkDevice().destroyBuffer(nativeBuffer.buffer);
            context.GetVkDevice().freeMemory(nativeBuffer.memory);
        });
    }

    void VkIndexBuffer::Update(const uint32_t* indices, uint32_t count) {
        if (count * sizeof(uint32_t) > _size) {
            Log::Error("Data size exceeds index buffer size.");
            return;
        }

        if (_mappedData) {
            memcpy(_mappedData, indices, count * sizeof(uint32_t));
            _indexCount = count;
        } else {
            Log::Error("Index buffer is not mapped for updating.");
        }
    }
}

#endif