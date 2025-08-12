#include "pch.h"
#include "VkBuffers.h"

#ifdef SUPPORT_VULKAN

#include "VkContext.h"
#include "Log/Log.h"

namespace flaw {
    VkStructuredBuffer::VkStructuredBuffer(VkContext& context, const Descriptor& desc)
        : _context(context)
		, _memProperty(desc.memProperty)
        , _elmSize(desc.elmSize)
        , _size(desc.bufferSize)
    {
        if (!CreateBuffer()) {
            Log::Fatal("Failed to create structured buffer.");
            return;
        }

        if (!AllocateMemory()) {
            Log::Fatal("Failed to allocate structured buffer memory.");
            return;
        }

        _bufferInfo.buffer = _buffer;
        _bufferInfo.offset = 0;
        _bufferInfo.range = _size;

        auto mappedDataWrapper = _context.GetVkDevice().mapMemory(_memory, 0, _size, vk::MemoryMapFlags());
        if (mappedDataWrapper.result != vk::Result::eSuccess) {
            Log::Fatal("Failed to map structured buffer memory.");
            return;
        }

        _mappedData = mappedDataWrapper.value;

        if (desc.initialData) {
            Update(desc.initialData, _size);
        }
    }

    VkStructuredBuffer::~VkStructuredBuffer() {
        if (_mappedData) {
            _context.GetVkDevice().unmapMemory(_memory);
            _mappedData = nullptr;
        }

        _context.AddDelayedDeletionTasks([&context = _context, buffer = _buffer, memory = _memory]() {
            context.GetVkDevice().destroyBuffer(buffer, nullptr);
            context.GetVkDevice().freeMemory(memory, nullptr);
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

    bool VkStructuredBuffer::CreateBuffer() {
        vk::BufferCreateInfo bufferInfo;
        bufferInfo.size = _size;
        bufferInfo.usage = vk::BufferUsageFlagBits::eStorageBuffer;
        GetRequiredVkBufferUsageFlags(_memProperty, bufferInfo.usage);
        bufferInfo.sharingMode = vk::SharingMode::eExclusive;

        auto bufferWrapper = _context.GetVkDevice().createBuffer(bufferInfo, nullptr);
        if (bufferWrapper.result != vk::Result::eSuccess) {
            Log::Error("Failed to create structured buffer: %s", vk::to_string(bufferWrapper.result).c_str());
            return false;
        }

        _buffer = bufferWrapper.value;

        return true;
    }

    bool VkStructuredBuffer::AllocateMemory() {
        vk::MemoryRequirements memRequirements = _context.GetVkDevice().getBufferMemoryRequirements(_buffer);

        vk::MemoryAllocateInfo allocInfo;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = GetMemoryTypeIndex(_context.GetVkPhysicalDevice(), memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        if (allocInfo.memoryTypeIndex == UINT32_MAX) {
            Log::Error("Failed to find suitable memory type for structured buffer.");
            return false;
        }

        auto memoryWrapper = _context.GetVkDevice().allocateMemory(allocInfo, nullptr);
        if (memoryWrapper.result != vk::Result::eSuccess) {
            Log::Error("Failed to allocate structured buffer memory: %s", vk::to_string(memoryWrapper.result).c_str());
            return false;
        }

        _memory = memoryWrapper.value;

        _context.GetVkDevice().bindBufferMemory(_buffer, _memory, 0);

        return true;
    }
}

#endif