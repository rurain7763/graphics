#include "pch.h"
#include "VkTextures.h"

#ifdef SUPPORT_VULKAN

#include "VkContext.h"
#include "Log/Log.h"
#include "Graphics/GraphicsFunc.h"
#include "VkCommandQueue.h"
#include "VkBuffers.h"

namespace flaw {
    VkTexture2D::VkTexture2D(VkContext& context, const Descriptor& descriptor)
        : _context(context)
        , _isExternalImage(false)
        , _format(descriptor.format)
        , _memProperty(descriptor.memProperty)
        , _texUsages(descriptor.texUsages)
        , _mipLevels(descriptor.mipLevels)
        , _sampleCount(descriptor.sampleCount)
        , _width(descriptor.width)
        , _height(descriptor.height)
    {
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
        imageInfo.usage = GetVkImageUsageFlags(_texUsages) | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc;
        imageInfo.sharingMode = vk::SharingMode::eExclusive;
        imageInfo.initialLayout = vk::ImageLayout::eUndefined;

		vk::MemoryPropertyFlags memProperties;
		GetRequiredVkMemoryPropertyFlags(_memProperty, memProperties);

		_nativeTexture = VkNativeTexture::Create(_context, imageInfo, memProperties);

        if (!CreateImageView()) {
            return;
        }

        if (!CreateSampler()) {
            return;
        }

        auto& vkCmdQueue = static_cast<VkCommandQueue&>(_context.GetCommandQueue());

        vk::CommandBuffer commandBuffer;
        vkCmdQueue.BeginOneTimeCommands(commandBuffer);

        if (descriptor.data) {
            if (!PullMemory(commandBuffer, descriptor.data)) {
                return;
            }

            if (descriptor.mipLevels > 1) {
                if (!GenerateMipmaps(commandBuffer)) {
                    return;
                }
            }
        }

        if (!TransitionFinalImageLayout(commandBuffer, descriptor.initialLayout)) {
            return;
        }

        vkCmdQueue.EndOneTimeCommands(commandBuffer);
    }

    VkTexture2D::VkTexture2D(VkContext& context, vk::Image image, uint32_t width, uint32_t height, PixelFormat format, MemoryProperty usage, uint32_t bindFlags, uint32_t sampleCount, uint32_t mipLevels)
        : _context(context)
        , _isExternalImage(true)
        , _width(width)
        , _height(height)
        , _format(format)
        , _memProperty(usage)
        , _texUsages(bindFlags)
        , _sampleCount(sampleCount)
        , _mipLevels(mipLevels)
    {
		_nativeTexture.image = image;

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
        _context.AddDelayedDeletionTasks([&context = _context, nativeTex = _nativeTexture, view = _view, sampler = _sampler, isExternalImage = _isExternalImage]() {
            if (!isExternalImage) {
                context.GetVkDevice().destroyImage(nativeTex.image);
                context.GetVkDevice().freeMemory(nativeTex.memory);
            }
            context.GetVkDevice().destroyImageView(view);
            context.GetVkDevice().destroySampler(sampler);
        });
    }

    bool VkTexture2D::PullMemory(vk::CommandBuffer& commandBuffer, const uint8_t* data) {
        int32_t bufferSize = _width * _height * GetSizePerPixel(_format);

		VkNativeBuffer stagingBuffer = VkNativeBuffer::CreateAsStaging(_context, bufferSize, data);

        vk::ImageMemoryBarrier barrier;
		barrier.image = _nativeTexture.image;
		barrier.oldLayout = vk::ImageLayout::eUndefined;
        barrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
		barrier.srcAccessMask = vk::AccessFlagBits::eNone;
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.aspectMask = GetVkImageAspectFlags(_format);
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

        commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlags(), nullptr, nullptr, barrier);

		vk::BufferImageCopy copyRegion;
		copyRegion.bufferOffset = 0;
		copyRegion.bufferRowLength = 0;
		copyRegion.bufferImageHeight = 0;
		copyRegion.imageSubresource.aspectMask = GetVkImageAspectFlags(_format);
        copyRegion.imageSubresource.mipLevel = 0;
		copyRegion.imageSubresource.baseArrayLayer = 0;
        copyRegion.imageSubresource.layerCount = 1;
		copyRegion.imageOffset = vk::Offset3D{ 0, 0, 0 };
		copyRegion.imageExtent = vk::Extent3D{ _width, _height, 1 };

		commandBuffer.copyBufferToImage(stagingBuffer.buffer, _nativeTexture.image, vk::ImageLayout::eTransferDstOptimal, 1, &copyRegion);

        _context.AddDelayedDeletionTasks([&context = _context, stagingBuffer]() {
            context.GetVkDevice().destroyBuffer(stagingBuffer.buffer);
            context.GetVkDevice().freeMemory(stagingBuffer.memory);
        });

        return true;
    }

    bool VkTexture2D::GenerateMipmaps(vk::CommandBuffer& commandBuffer) {
        vk::FormatProperties formatProperties;
        _context.GetVkPhysicalDevice().getFormatProperties(ConvertToVkFormat(_format), &formatProperties);
        if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
            std::runtime_error("Texture format does not support linear blitting for mipmap generation.");
        }

        vk::ImageMemoryBarrier barrier;
        barrier.image = _nativeTexture.image;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.aspectMask = GetVkImageAspectFlags(_format);
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

        // 초기 상태: 밉 레벨 0을 TRANSFER_DST_OPTIMAL에서 TRANSFER_SRC_OPTIMAL로 전환
        // 이 상태는 밉 레벨 0이 첫 번째 블리팅의 소스가 되기 위함입니다.
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
        commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {}, nullptr, nullptr, barrier);

        int32_t mipWidth = _width;
        int32_t mipHeight = _height;

        for (uint32_t i = 1; i < _mipLevels; ++i) {
            vk::ImageBlit blitRegion;
            blitRegion.srcOffsets[0] = vk::Offset3D{ 0, 0, 0 };
            blitRegion.srcOffsets[1] = vk::Offset3D{ mipWidth, mipHeight, 1 };
			blitRegion.srcSubresource.aspectMask = GetVkImageAspectFlags(_format);
            blitRegion.srcSubresource.mipLevel = i - 1;
            blitRegion.srcSubresource.baseArrayLayer = 0;
            blitRegion.srcSubresource.layerCount = 1;

            // 다음 밉 레벨의 크기를 계산합니다.
            mipWidth = (mipWidth > 1) ? mipWidth / 2 : 1;
            mipHeight = (mipHeight > 1) ? mipHeight / 2 : 1;

            blitRegion.dstOffsets[0] = vk::Offset3D{ 0, 0, 0 };
            blitRegion.dstOffsets[1] = vk::Offset3D{ mipWidth, mipHeight, 1 };
            blitRegion.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
            blitRegion.dstSubresource.mipLevel = i;
            blitRegion.dstSubresource.baseArrayLayer = 0;
            blitRegion.dstSubresource.layerCount = 1;

            commandBuffer.blitImage(_nativeTexture.image, vk::ImageLayout::eTransferSrcOptimal, _nativeTexture.image, vk::ImageLayout::eTransferDstOptimal, 1, &blitRegion, vk::Filter::eLinear);

            // 현재 밉 레벨(i)을 다음 반복의 소스로 사용하기 위해 TRANSFER_SRC_OPTIMAL로 전환합니다.
            barrier.subresourceRange.baseMipLevel = i;
            barrier.subresourceRange.levelCount = 1;
            barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
            barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
            commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {}, nullptr, nullptr, barrier);
        }

        return true;
    }

    bool VkTexture2D::TransitionFinalImageLayout(vk::CommandBuffer& commandBuffer, TextureLayout layout) {
        auto& vkCmdQueue = static_cast<VkCommandQueue&>(_context.GetCommandQueue());

        vk::ImageLayout finalLayout = ConvertToVkImageLayout(layout);

        vk::ImageMemoryBarrier barrier;
        barrier.image = _nativeTexture.image;
        barrier.oldLayout = vk::ImageLayout::eUndefined;
        barrier.newLayout = finalLayout;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
        barrier.dstAccessMask = vk::AccessFlagBits::eNone;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = GetVkImageAspectFlags(_format);
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

        commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eAllCommands, vk::DependencyFlags(), nullptr, nullptr, barrier);

        return true;
    }

    bool VkTexture2D::CreateImageView() {
        vk::ImageViewCreateInfo createInfo;
        createInfo.image = _nativeTexture.image;
        createInfo.viewType = vk::ImageViewType::e2D;
        createInfo.format = ConvertToVkFormat(_format);
        createInfo.components.r = vk::ComponentSwizzle::eIdentity;
        createInfo.components.g = vk::ComponentSwizzle::eIdentity;
        createInfo.components.b = vk::ComponentSwizzle::eIdentity;
        createInfo.components.a = vk::ComponentSwizzle::eIdentity;
        createInfo.subresourceRange.aspectMask = GetVkImageAspectFlags(_format);
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = _mipLevels;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        auto imageViewWrapper = _context.GetVkDevice().createImageView(createInfo);
        if (imageViewWrapper.result != vk::Result::eSuccess) {
            Log::Fatal("Failed to create Vulkan image view: %s", vk::to_string(imageViewWrapper.result).c_str());
            return false;
        }

        _view = imageViewWrapper.value;

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