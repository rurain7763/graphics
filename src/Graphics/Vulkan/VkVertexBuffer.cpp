#include "pch.h"
#include "VkBuffers.h"

#ifdef SUPPORT_VULKAN

#include "VkContext.h"
#include "VkCommandQueue.h"
#include "Log/Log.h"

namespace flaw {
    VkVertexBuffer::VkVertexBuffer(VkContext& context, const Descriptor& descriptor)
        : _context(context)
        , _memProperty(descriptor.memProperty)
        , _elmSize(descriptor.elmSize)
        , _size(descriptor.bufferSize)
    {
        vk::BufferUsageFlags usage = vk::BufferUsageFlagBits::eVertexBuffer;
        GetRequiredVkBufferUsageFlags(_memProperty, usage);

        vk::MemoryPropertyFlags memoryFlags;
        GetRequiredVkMemoryPropertyFlags(_memProperty, memoryFlags);

        _nativeBuffer = VkNativeBuffer::Create(_context, _size, usage, memoryFlags);

		if (_memProperty != MemoryProperty::Static) {
			auto mappedDataWrapper = _context.GetVkDevice().mapMemory(_nativeBuffer.memory, 0, _size, vk::MemoryMapFlags());
			if (mappedDataWrapper.result != vk::Result::eSuccess) {
				Log::Error("Failed to map vertex buffer memory.");
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
            stagingDesc.elmSize = descriptor.elmSize;
            stagingDesc.bufferSize = descriptor.bufferSize;
            stagingDesc.initialData = descriptor.initialData;
    
            auto stagingBuffer = CreateRef<VkVertexBuffer>(context, stagingDesc);
               
            vk::CommandBuffer commandBuffer = vkCommandQueue.BeginOneTimeCommands();

			vk::BufferCopy copyRegion;
			copyRegion.size = _size;
			copyRegion.srcOffset = 0;
			copyRegion.dstOffset = 0;

			commandBuffer.copyBuffer(stagingBuffer->_nativeBuffer.buffer, _nativeBuffer.buffer, 1, &copyRegion);

            vkCommandQueue.EndOneTimeCommands(commandBuffer);
        }
        else {
            Update(descriptor.initialData, _size);
        }
    }

    VkVertexBuffer::~VkVertexBuffer() {
        if (_mappedData) {
            _context.GetVkDevice().unmapMemory(_nativeBuffer.memory);
            _mappedData = nullptr;
        }

        _context.AddDelayedDeletionTasks([&context = _context, nativeBuffer = _nativeBuffer]() {
            context.GetVkDevice().destroyBuffer(nativeBuffer.buffer);
            context.GetVkDevice().freeMemory(nativeBuffer.memory);
        });
    }

    void VkVertexBuffer::Update(const void* data, uint32_t size) {
        if (size > _size) {
            Log::Error("Data size exceeds vertex buffer size.");
            return;
        }

        if (_mappedData) {
            memcpy(_mappedData, data, size);
        }
        else {
            Log::Error("Vertex buffer is not mapped for updating.");
            return;
        }
    }

    void VkVertexBuffer::CopyTo(Ref<VertexBuffer> dstBuffer, uint32_t srcOffset, uint32_t dstOffset) {
        auto vkDstBuffer = std::dynamic_pointer_cast<VkVertexBuffer>(dstBuffer);
        FASSERT(vkDstBuffer, "Invalid vertex buffer type for Vulkan command queue");

        auto& vkCommandQueue = static_cast<VkCommandQueue&>(_context.GetCommandQueue());

		vk::CommandBuffer commandBuffer = vkCommandQueue.BeginOneTimeCommands();

		vk::BufferCopy copyRegion;
		copyRegion.size = _size;
		copyRegion.srcOffset = srcOffset;
		copyRegion.dstOffset = dstOffset;

		commandBuffer.copyBuffer(_nativeBuffer.buffer, vkDstBuffer->_nativeBuffer.buffer, 1, &copyRegion);

		vkCommandQueue.EndOneTimeCommands(commandBuffer);
    }
}

#endif