#include "pch.h"
#include "VkBuffers.h"

#ifdef SUPPORT_VULKAN

#include "VkContext.h"
#include "VkCommandQueue.h"
#include "Log/Log.h"

namespace flaw {
    VkVertexBuffer::VkVertexBuffer(VkContext& context, const Descriptor& descriptor)
        : _context(context)
        , _usage(descriptor.usage)
        , _elmSize(descriptor.elmSize)
        , _size(descriptor.bufferSize)
    {
        if(!CreateBuffer()) {
            Log::Fatal("Failed to create vertex buffer.");
            return;
        }

        if (!AllocateMemory()) {
            Log::Fatal("Failed to allocate vertex buffer memory.");
            return;
        }

        if (descriptor.initialData) {
            if (descriptor.usage == UsageFlag::Static) {
                auto& vkCommandQueue = static_cast<VkCommandQueue&>(context.GetCommandQueue());
    
                Descriptor stagingDesc;
                stagingDesc.usage = UsageFlag::Staging;
                stagingDesc.elmSize = descriptor.elmSize;
                stagingDesc.bufferSize = descriptor.bufferSize;
                stagingDesc.initialData = descriptor.initialData;
    
                auto stagingBuffer = CreateRef<VkVertexBuffer>(context, stagingDesc);
                
                vkCommandQueue.CopyBuffer(stagingBuffer->GetVkBuffer(), _buffer, _size, 0, 0);
            }
            else {
                auto mappedDataWrapper = _context.GetVkDevice().mapMemory(_memory, 0, descriptor.bufferSize, vk::MemoryMapFlags(), _context.GetVkDispatchLoader());
                if (mappedDataWrapper.result != vk::Result::eSuccess) {
                    Log::Error("Failed to map vertex buffer memory.");
                    return;
                }

                _mappedData = mappedDataWrapper.value;

                Update(descriptor.initialData, _elmSize, _size / _elmSize);
            }
        }
    }

    VkVertexBuffer::~VkVertexBuffer() {
        if (_mappedData) {
            _context.GetVkDevice().unmapMemory(_memory, _context.GetVkDispatchLoader());
            _mappedData = nullptr;
        }

        _context.AddDelayedDeletionTasks([&context = _context, buffer = _buffer, memory = _memory]() {
            context.GetVkDevice().destroyBuffer(buffer, nullptr, context.GetVkDispatchLoader());
            context.GetVkDevice().freeMemory(memory, nullptr, context.GetVkDispatchLoader());
        });
    }

    void VkVertexBuffer::Update(const void* data, uint32_t elmSize, uint32_t count) {
        if (elmSize != _elmSize) {
            Log::Error("Element size mismatch in vertex buffer update.");
            return;
        }

        uint32_t requiredSize = count * elmSize;
        if (requiredSize > _size) {
            Log::Error("Data size exceeds vertex buffer size.");
            return;
        }

        if (_mappedData) {
            memcpy(_mappedData, data, requiredSize);
        }
        else {
            Log::Error("Vertex buffer is not mapped for updating.");
            return;
        }
    }

    void VkVertexBuffer::Bind() {
        // Bind buffer logic here
    }

    void VkVertexBuffer::CopyTo(Ref<VertexBuffer> dstBuffer, uint32_t srcOffset, uint32_t dstOffset) {
        auto vkDstBuffer = std::dynamic_pointer_cast<VkVertexBuffer>(dstBuffer);
        FASSERT(vkDstBuffer, "Invalid vertex buffer type for Vulkan command queue");

        auto& vkCommandQueue = static_cast<VkCommandQueue&>(_context.GetCommandQueue());

        vkCommandQueue.CopyBuffer(_buffer, vkDstBuffer->GetVkBuffer(), _size, srcOffset, dstOffset);
    }
    
    bool VkVertexBuffer::CreateBuffer() {
        vk::BufferCreateInfo bufferInfo;
        bufferInfo.size = _size;
        bufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer;
        GetRequiredVkBufferUsageFlags(_usage, bufferInfo.usage);
        bufferInfo.sharingMode = vk::SharingMode::eExclusive;

        auto bufferWrapper = _context.GetVkDevice().createBuffer(bufferInfo, nullptr, _context.GetVkDispatchLoader());
        if (bufferWrapper.result != vk::Result::eSuccess) {
            Log::Error("Failed to create vertex buffer: %s", vk::to_string(bufferWrapper.result).c_str());
            return false;
        }

        _buffer = bufferWrapper.value;

        return true;
    }

    bool VkVertexBuffer::AllocateMemory() {
        vk::MemoryRequirements memRequirements = _context.GetVkDevice().getBufferMemoryRequirements(_buffer, _context.GetVkDispatchLoader());

        vk::MemoryPropertyFlags memoryFlags;
        GetRequiredVkMemoryPropertyFlags(_usage, memoryFlags);

        vk::MemoryAllocateInfo allocInfo;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = GetMemoryTypeIndex(_context.GetVkPhysicalDevice(), memRequirements.memoryTypeBits, memoryFlags);

        auto memoryWrapper = _context.GetVkDevice().allocateMemory(allocInfo, nullptr, _context.GetVkDispatchLoader());
        if (memoryWrapper.result != vk::Result::eSuccess) {
            Log::Error("Failed to allocate vertex buffer memory: %s", vk::to_string(memoryWrapper.result).c_str());
            return false;
        }

        _memory = memoryWrapper.value;

        _context.GetVkDevice().bindBufferMemory(_buffer, _memory, 0, _context.GetVkDispatchLoader());

        return true;
    }
}

#endif