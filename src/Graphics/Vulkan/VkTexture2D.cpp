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
        , _memProperty(descriptor.memProperty)
        , _texUsages(descriptor.texUsages)
        , _mipLevels(descriptor.mipLevels)
        , _sampleCount(descriptor.sampleCount)
		, _shaderStages(descriptor.shaderStages)
        , _width(descriptor.width)
        , _height(descriptor.height)
		, _currentLayout(vk::ImageLayout::eUndefined)
		, _currentAccessFlags(vk::AccessFlagBits::eNone)
		, _currentPipelineStage(vk::PipelineStageFlagBits::eTopOfPipe)
    {
        if (!CreateImage(descriptor.data)) {
            return;
        }

        if (!AllocateMemory()) {
            return;
        }

        auto& vkCmdQueue = static_cast<VkCommandQueue&>(_context.GetCommandQueue());

        vkCmdQueue.BeginOneTimeCommands();

        if (descriptor.data) {
            if (!PullMemory(descriptor.data)) {
                return;
            }
        }

        if (descriptor.mipLevels > 1) {
            if (!GenerateMipmaps()) {
                return;
            }
        }

        if (!TransitionFinalImageLayout(descriptor.initialLayout)) {
            return;
        }

        vkCmdQueue.EndOneTimeCommands();

        if (!CreateImageView()) {
            return;
        }

        if (!CreateSampler()) {
            return;
        }
    }

    VkTexture2D::VkTexture2D(VkContext& context, vk::Image image, uint32_t width, uint32_t height, PixelFormat format, MemoryProperty usage, uint32_t bindFlags, uint32_t sampleCount, uint32_t mipLevels, uint32_t shaderStages)
        : _context(context)
        , _isExternalImage(true)
        , _image(image)
        , _width(width)
        , _height(height)
        , _format(format)
        , _memProperty(usage)
        , _texUsages(bindFlags)
        , _sampleCount(sampleCount)
        , _mipLevels(mipLevels)
		, _shaderStages(shaderStages)
		, _currentLayout(vk::ImageLayout::eUndefined)
		, _currentAccessFlags(vk::AccessFlagBits::eNone)
		, _currentPipelineStage(vk::PipelineStageFlagBits::eTopOfPipe)
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
        imageInfo.mipLevels = _mipLevels;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = ConvertToVkSampleCount(_sampleCount);
        imageInfo.tiling = vk::ImageTiling::eOptimal;
        imageInfo.usage = ConvertToVkImageUsageFlags(_texUsages) | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc;
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
        GetRequiredVkMemoryPropertyFlags(_memProperty, properties);

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

        vkCmdQueue.TransitionImageLayout(
            _image,
            ConvertToVkImageAspectFlags(_format),
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eTransferDstOptimal,
            vk::AccessFlagBits::eNone,
            vk::AccessFlagBits::eTransferWrite,
            vk::PipelineStageFlagBits::eTopOfPipe,
            vk::PipelineStageFlagBits::eTransfer
        );
        vkCmdQueue.CopyBuffer(stagingBuffer.buffer, _image, _width, _height, 0, 0);

		_currentLayout = vk::ImageLayout::eTransferDstOptimal;
		_currentAccessFlags = vk::AccessFlagBits::eTransferWrite;
		_currentPipelineStage = vk::PipelineStageFlagBits::eTransfer;

        _context.AddDelayedDeletionTasks([&context = _context, stagingBuffer]() {
            context.GetVkDevice().destroyBuffer(stagingBuffer.buffer);
            context.GetVkDevice().freeMemory(stagingBuffer.memory);
        });

        return true;
    }

    bool VkTexture2D::GenerateMipmaps() {
        auto& vkCmdQueue = static_cast<VkCommandQueue&>(_context.GetCommandQueue());

        vkCmdQueue.GenerateMipmaps(
            _image, 
            ConvertToVkImageAspectFlags(_format), 
            ConvertToVkFormat(_format), 
            _width, _height, 
            1, 
            _mipLevels,
			_currentLayout,
			_currentAccessFlags,
			_currentPipelineStage
        );

        return true;
    }

    bool VkTexture2D::TransitionFinalImageLayout(TextureLayout layout) {
        auto& vkCmdQueue = static_cast<VkCommandQueue&>(_context.GetCommandQueue());

        vk::ImageLayout finalLayout = ConvertToVkImageLayout(layout);
		vk::PipelineStageFlags finalPipelineStage = ConvertToVkPipelineStageFlags(_texUsages, _shaderStages);

        vk::AccessFlags finalAccessFlags;
        if (_texUsages & TextureUsage::ShaderResource) {
            finalAccessFlags |= vk::AccessFlagBits::eShaderRead;
        } 
        
        if (_texUsages & TextureUsage::RenderTarget) {
            finalAccessFlags |= vk::AccessFlagBits::eColorAttachmentWrite;
        }
        
        if (_texUsages & TextureUsage::DepthStencil) {
            finalAccessFlags |= vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        }

        if (_texUsages & TextureUsage::UnorderedAccess) {
            finalAccessFlags |= vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead;
        }

        vkCmdQueue.TransitionImageLayout(
            _image,
            ConvertToVkImageAspectFlags(_format),
            _currentLayout,
            finalLayout,
            _currentAccessFlags,
            finalAccessFlags,
            _currentPipelineStage,
            finalPipelineStage
        );

		_currentLayout = finalLayout;
		_currentAccessFlags = finalAccessFlags;
		_currentPipelineStage = finalPipelineStage;

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
        createInfo.subresourceRange.aspectMask = ConvertToVkImageAspectFlags(_format);
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = _mipLevels;
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
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = static_cast<float>(_mipLevels);

        auto samplerWrapper = _context.GetVkDevice().createSampler(samplerInfo);
        if (samplerWrapper.result != vk::Result::eSuccess) {
            Log::Fatal("Failed to create Vulkan sampler: %s", vk::to_string(samplerWrapper.result).c_str());
            return false;
        }

        _sampler = samplerWrapper.value;

        return true;
    }

    void VkTexture2D::Fetch(void* outData, const uint32_t size) const {
        if (_memProperty == MemoryProperty::Static) {
            Log::Error("Cannot fetch data from a static texture");
            return;
        }

        uint32_t copySize = min(size, _width * _height * GetSizePerPixel(_format));

        return;
    }
}

#endif