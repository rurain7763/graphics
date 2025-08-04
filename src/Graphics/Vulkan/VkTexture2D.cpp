#include "pch.h"
#include "VkTextures.h"

#ifdef SUPPORT_VULKAN

#include "VkContext.h"
#include "Log/Log.h"
#include "Graphics/GraphicsFunc.h"
#include "VkCommandQueue.h"

namespace flaw {
    VkTexture2D::VkTexture2D(VkContext& context, const Descriptor& descriptor)
        : _context(context)
        , _isExternalImage(false)
        , _format(descriptor.format)
        , _usage(descriptor.usage)
        , _acessFlags(descriptor.access)
        , _bindFlags(descriptor.bindFlags)
        , _width(descriptor.width)
        , _height(descriptor.height)
    {
        if (!CreateImage(descriptor.data)) {
            Log::Fatal("Failed to create image for texture");
            return;
        }

        if (!AllocateMemory()) {
            Log::Fatal("Failed to allocate memory for texture");
            return;
        }

        if (descriptor.data) {
            if (!PullMemory(descriptor.data)) {
                Log::Fatal("Failed to pull memory for texture");
                return;
            }
        }

        if (!CreateImageView()) {
            Log::Fatal("Failed to create image view for texture");
            return;
        }

        if (!CreateSampler()) {
            Log::Fatal("Failed to create sampler for texture");
            return;
        }

        _imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        _imageInfo.imageView = _imageView;
        _imageInfo.sampler = _sampler;
    }

    VkTexture2D::VkTexture2D(VkContext& context, vk::Image image, PixelFormat format, uint32_t bindFlags)
        : _context(context)
        , _isExternalImage(true)
        , _image(image)
        , _format(format)
        , _bindFlags(bindFlags)
    {
        if (!CreateImageView()) {
            Log::Fatal("Failed to create image view for texture");
            return;
        }

        if (!CreateSampler()) {
            Log::Fatal("Failed to create sampler for texture");
            return;
        }
    }

    VkTexture2D::~VkTexture2D() {
        _context.AddDelayedDeletionTasks([&context = _context, image = _image, imageView = _imageView, isExternalImage = _isExternalImage, imageMemory = _imageMemory, sampler = _sampler]() {
            if (!isExternalImage) {
                context.GetVkDevice().destroyImage(image);
                context.GetVkDevice().freeMemory(imageMemory);
            }
            context.GetVkDevice().destroyImageView(imageView);
            context.GetVkDevice().destroySampler(sampler);
        });
    }

    bool VkTexture2D::CreateImage(bool hasData) {
        vk::ImageCreateInfo imageInfo;
        imageInfo.imageType = vk::ImageType::e2D;
        imageInfo.format = ConvertToVkFormat(_format);
        imageInfo.extent.width = _width;
        imageInfo.extent.height = _height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1; // Assuming no mipmaps for now
        imageInfo.arrayLayers = 1;
        imageInfo.samples = vk::SampleCountFlagBits::e1;
        imageInfo.tiling = vk::ImageTiling::eOptimal;
        imageInfo.usage = ConvertToVkImageUsageFlags(_bindFlags);
        if (hasData) {
            imageInfo.usage |= vk::ImageUsageFlagBits::eTransferDst;
        }
        imageInfo.sharingMode = vk::SharingMode::eExclusive;
        imageInfo.initialLayout = vk::ImageLayout::eUndefined;

        auto imageWrapper = _context.GetVkDevice().createImage(imageInfo);
        if (imageWrapper.result != vk::Result::eSuccess) {
            Log::Fatal("Failed to create Vulkan image: %s", vk::to_string(imageWrapper.result).c_str());
            return false;
        }

        _image = imageWrapper.value;

        return true;
    }

    bool VkTexture2D::AllocateMemory() {
        vk::MemoryRequirements memRequirements = _context.GetVkDevice().getImageMemoryRequirements(_image);

        vk::MemoryPropertyFlags properties;
        GetRequiredVkMemoryPropertyFlags(_usage, properties);

        vk::MemoryAllocateInfo allocInfo;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = GetMemoryTypeIndex(_context.GetVkPhysicalDevice(), memRequirements.memoryTypeBits, properties);

        auto memoryWrapper = _context.GetVkDevice().allocateMemory(allocInfo, nullptr);
        if (memoryWrapper.result != vk::Result::eSuccess) {
            Log::Fatal("Failed to allocate memory for Vulkan image: %s", vk::to_string(memoryWrapper.result).c_str());
            return false;
        }

        _imageMemory = memoryWrapper.value;

        _context.GetVkDevice().bindImageMemory(_image, _imageMemory, 0);

        return true;
    }
    
    bool VkTexture2D::PullMemory(const uint8_t* data) {
        auto& vkCmdQueue = static_cast<VkCommandQueue&>(_context.GetCommandQueue());    
        int32_t bufferSize = _width * _height * GetSizePerPixel(_format);

        auto stagingBuffer = CreateVkBuffer(
            _context.GetVkPhysicalDevice(),
            _context.GetVkDevice(),
            bufferSize,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        );

        auto stagingBuffMapedDataWrapper = _context.GetVkDevice().mapMemory(stagingBuffer.memory, 0, bufferSize, vk::MemoryMapFlags());
        if (stagingBuffMapedDataWrapper.result != vk::Result::eSuccess) {
            Log::Fatal("Failed to map memory for staging buffer: %s", vk::to_string(stagingBuffMapedDataWrapper.result).c_str());
            return false;
        }

        memcpy(stagingBuffMapedDataWrapper.value, data, bufferSize);
        _context.GetVkDevice().unmapMemory(stagingBuffer.memory);

        vkCmdQueue.TransitionImageLayout(_image, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
        vkCmdQueue.CopyBuffer(stagingBuffer.buffer, _image, _width, _height, 0, 0);
        vkCmdQueue.TransitionImageLayout(_image, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

        _context.GetVkDevice().freeMemory(stagingBuffer.memory, nullptr);
        _context.GetVkDevice().destroyBuffer(stagingBuffer.buffer, nullptr);

        return true;
    }

    bool VkTexture2D::CreateImageView() {
        vk::ImageViewCreateInfo createInfo;
        createInfo.image = _image;
        createInfo.viewType = vk::ImageViewType::e2D;
        createInfo.format = ConvertToVkFormat(_format);
        createInfo.components.r = vk::ComponentSwizzle::eIdentity;
        createInfo.components.g = vk::ComponentSwizzle::eIdentity;
        createInfo.components.b = vk::ComponentSwizzle::eIdentity;
        createInfo.components.a = vk::ComponentSwizzle::eIdentity;
        createInfo.subresourceRange.aspectMask = ConvertToVkImageAspectFlags(_bindFlags);
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1; // Assuming no mipmaps for now
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        auto imageViewWrapper = _context.GetVkDevice().createImageView(createInfo);
        if (imageViewWrapper.result != vk::Result::eSuccess) {
            Log::Fatal("Failed to create Vulkan image view: %s", vk::to_string(imageViewWrapper.result).c_str());
            return false;
        }

        _imageView = imageViewWrapper.value;

        return true;
    }

    bool VkTexture2D::CreateSampler() {
        vk::SamplerCreateInfo samplerInfo;
        samplerInfo.minFilter = vk::Filter::eNearest;
        samplerInfo.magFilter = vk::Filter::eLinear;
        samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
        samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
        samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;
        samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = vk::CompareOp::eAlways;
        samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;

        auto samplerWrapper = _context.GetVkDevice().createSampler(samplerInfo);
        if (samplerWrapper.result != vk::Result::eSuccess) {
            Log::Fatal("Failed to create Vulkan sampler: %s", vk::to_string(samplerWrapper.result).c_str());
            return false;
        }

        _sampler = samplerWrapper.value;

        return true;
    }
}

#endif