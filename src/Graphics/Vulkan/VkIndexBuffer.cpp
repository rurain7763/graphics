#include "pch.h"
#include "VkBuffers.h"

#ifdef SUPPORT_VULKAN

#include "VkContext.h"
#include "Log/Log.h"
#include "VkCommandQueue.h"

namespace flaw {
    VkIndexBuffer::VkIndexBuffer(VkContext& context, const Descriptor& descriptor)
        : _context(context)
        , _usage(descriptor.usage)
        , _size(descriptor.bufferSize)
        , _indexCount(descriptor.bufferSize / sizeof(uint32_t))
    {
        if (!CreateBuffer()) {
            return;
        }

        if (descriptor.initialData) {
            if (_usage == MemoryProperty::Static) {
                auto& vkCommandQueue = static_cast<VkCommandQueue&>(context.GetCommandQueue());

                Descriptor stagingDesc;
                stagingDesc.usage = MemoryProperty::Staging;
                stagingDesc.bufferSize = descriptor.bufferSize;
                stagingDesc.initialData = descriptor.initialData;

                auto stagingBuffer = CreateRef<VkIndexBuffer>(context, stagingDesc);

                vkCommandQueue.BeginOneTimeCommands();
                vkCommandQueue.CopyBuffer(stagingBuffer->GetVkBuffer(), _buffer, _size, 0, 0);
                vkCommandQueue.EndOneTimeCommands();
            } else {
                auto mappedDataWrapper = _context.GetVkDevice().mapMemory(_memory, 0, descriptor.bufferSize, vk::MemoryMapFlags());
                if (mappedDataWrapper.result != vk::Result::eSuccess) {
                    Log::Error("Failed to map index buffer memory.");
                    return;
                }

                _mappedData = mappedDataWrapper.value;

                Update(descriptor.initialData, _indexCount);
            }
        }
    }

    VkIndexBuffer::~VkIndexBuffer() {
        if (_mappedData) {
            _context.GetVkDevice().unmapMemory(_memory);
            _mappedData = nullptr;
        }

        _context.AddDelayedDeletionTasks([&context = _context, buffer = _buffer, memory = _memory]() {
            context.GetVkDevice().destroyBuffer(buffer, nullptr);
            context.GetVkDevice().freeMemory(memory, nullptr);
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

    void VkIndexBuffer::Bind() {
    }

    bool VkIndexBuffer::CreateBuffer() {
        vk::BufferUsageFlags usageFlags = vk::BufferUsageFlagBits::eIndexBuffer;
        GetRequiredVkBufferUsageFlags(_usage, usageFlags);

        vk::MemoryPropertyFlags memoryFlags;
        GetRequiredVkMemoryPropertyFlags(_usage, memoryFlags);

        VkBufferWrapper buffer = CreateVkBuffer(
            _context.GetVkPhysicalDevice(),
            _context.GetVkDevice(),
            _size,
            usageFlags,
            memoryFlags
        );

        if (buffer.result != vk::Result::eSuccess) {
            Log::Error("Failed to create Vulkan index buffer: %s", vk::to_string(buffer.result).c_str());
            return false;
        }

        _buffer = buffer.buffer;
        _memory = buffer.memory;

        return true;
    }
}

#endif