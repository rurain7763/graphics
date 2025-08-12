#include "pch.h"
#include "VkTextures.h"

#ifdef SUPPORT_VULKAN

#include "VkContext.h"
#include "Log/Log.h"
#include "VkCommandQueue.h"
#include "Graphics/GraphicsFunc.h"

namespace flaw {
    VkTextureCube::VkTextureCube(VkContext& context, const Descriptor& descriptor)
        : _context(context) 
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

        if (!TransitionFinalImageLayout()) {
            return;
        }

        vkCmdQueue.EndOneTimeCommands();

        if (!CreateImageView()) {
            return;
        }

        if (!CreateSampler()) {
            return;
        }

        _imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        _imageInfo.imageView = _imageView;
        _imageInfo.sampler = _sampler;
    }

    VkTextureCube::~VkTextureCube() {
        _context.AddDelayedDeletionTasks([&context = _context, image = _image, imageMemory = _imageMemory, imageView = _imageView, sampler = _sampler]() {
            context.GetVkDevice().destroyImage(image);
            context.GetVkDevice().freeMemory(imageMemory);
            context.GetVkDevice().destroyImageView(imageView);
            context.GetVkDevice().destroySampler(sampler);
        });
    }

    bool VkTextureCube::CreateImage(bool hasData) {
        vk::ImageCreateInfo imageInfo;
        imageInfo.flags = vk::ImageCreateFlagBits::eCubeCompatible;
        imageInfo.imageType = vk::ImageType::e2D;
        imageInfo.format = ConvertToVkFormat(_format);
        imageInfo.extent.width = _width;
        imageInfo.extent.height = _height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = _mipLevels;
        imageInfo.arrayLayers = 6;
        imageInfo.samples = ConvertToVkSampleCount(_sampleCount);
        imageInfo.tiling = vk::ImageTiling::eOptimal;
        imageInfo.usage = ConvertToVkImageUsageFlags(_texUsages);
        if (hasData) {
            imageInfo.usage |= vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc;
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

    bool VkTextureCube::AllocateMemory() {
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

    bool VkTextureCube::PullMemory(const uint8_t* data) {
        auto& vkCmdQueue = static_cast<VkCommandQueue&>(_context.GetCommandQueue());
        uint32_t bufferSizePerImage = _width * _height * GetSizePerPixel(_format);
        uint32_t bufferSize = bufferSizePerImage * 6;

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
            _context.GetVkDevice().destroyBuffer(stagingBuffer.buffer);
            _context.GetVkDevice().freeMemory(stagingBuffer.memory);
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
        vkCmdQueue.CopyBuffer(stagingBuffer.buffer, _image, _width, _height, 0, 0, 6);

		_currentLayout = vk::ImageLayout::eTransferDstOptimal;
		_currentAccessFlags = vk::AccessFlagBits::eTransferWrite;
		_currentPipelineStage = vk::PipelineStageFlagBits::eTransfer;

        _context.AddDelayedDeletionTasks([&context = _context, stagingBuffer]() {
            context.GetVkDevice().destroyBuffer(stagingBuffer.buffer);
            context.GetVkDevice().freeMemory(stagingBuffer.memory);
        });

        return true;
    }

    bool VkTextureCube::GenerateMipmaps() {
        auto& vkCmdQueue = static_cast<VkCommandQueue&>(_context.GetCommandQueue());

        vkCmdQueue.GenerateMipmaps(
            _image, 
            ConvertToVkImageAspectFlags(_format), 
            ConvertToVkFormat(_format), _width, _height, 
            6, 
            _mipLevels,
			_currentLayout,
			_currentAccessFlags,
			_currentPipelineStage
          );

        return true;
    }

    bool VkTextureCube::TransitionFinalImageLayout() {
        auto& vkCmdQueue = static_cast<VkCommandQueue&>(_context.GetCommandQueue());

		vk::ImageLayout finalLayout = ConvertToVkImageLayout(_texUsages);
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

    bool VkTextureCube::CreateImageView() {
        vk::ImageViewCreateInfo viewInfo;
        viewInfo.image = _image;
        viewInfo.viewType = vk::ImageViewType::eCube;
        viewInfo.format = ConvertToVkFormat(_format);
        viewInfo.subresourceRange.aspectMask = ConvertToVkImageAspectFlags(_format);
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = _mipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 6;

        auto imageViewWrapper = _context.GetVkDevice().createImageView(viewInfo);
        if (imageViewWrapper.result != vk::Result::eSuccess) {
            Log::Fatal("Failed to create Vulkan image view: %s", vk::to_string(imageViewWrapper.result).c_str());
            return false;
        }

        _imageView = imageViewWrapper.value;

        return true;
    }

    bool VkTextureCube::CreateSampler() {
        vk::SamplerCreateInfo samplerInfo;
        samplerInfo.magFilter = vk::Filter::eLinear;
        samplerInfo.minFilter = vk::Filter::eLinear;
        samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
        samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
        samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
        samplerInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.maxAnisotropy = 1.0f; // No anisotropy
        samplerInfo.compareOp = vk::CompareOp::eNever; // No comparison
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = static_cast<float>(_mipLevels);
        samplerInfo.borderColor = vk::BorderColor::eFloatOpaqueWhite;

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