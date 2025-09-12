#include "pch.h"
#include "VkTextures.h"

#ifdef SUPPORT_VULKAN

#include "VkContext.h"
#include "Log/Log.h"
#include "VkCommandQueue.h"
#include "Graphics/GraphicsFunc.h"
#include "VkBuffers.h"

namespace flaw {
	VkTextureCubeArray::VkTextureCubeArray(VkContext& context, const Descriptor& descriptor)
		: _context(context)
		, _format(descriptor.format)
		, _memProperty(descriptor.memProperty)
		, _texUsages(descriptor.texUsages)
		, _mipLevels(descriptor.mipLevels)
		, _layers(descriptor.arraySize)
		, _sampleCount(descriptor.sampleCount)
		, _width(descriptor.width)
		, _height(descriptor.height)
		, _minFilter(descriptor.minFilter)
		, _magFilter(descriptor.magFilter)
		, _wrapModeU(descriptor.wrapModeU)
		, _wrapModeV(descriptor.wrapModeV)
		, _wrapModeW(descriptor.wrapModeW)
	{
		vk::ImageCreateInfo imageInfo;
		imageInfo.imageType = vk::ImageType::e2D;
		imageInfo.format = ConvertToVkFormat(_format);
		imageInfo.extent.width = _width;
		imageInfo.extent.height = _height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = _mipLevels;
		imageInfo.arrayLayers = _layers * 6; // 6 layers per cube
		imageInfo.samples = ConvertToVkSampleCount(_sampleCount);
		imageInfo.tiling = vk::ImageTiling::eOptimal;
		imageInfo.usage = GetVkImageUsageFlags(_texUsages) | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc;
		imageInfo.sharingMode = vk::SharingMode::eExclusive;
		imageInfo.initialLayout = vk::ImageLayout::eUndefined;
		imageInfo.flags = vk::ImageCreateFlagBits::eCubeCompatible;

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

		vk::CommandBuffer commandBuffer = vkCmdQueue.BeginOneTimeCommands();
		if (descriptor.data) {
			if (!PullMemory(commandBuffer, descriptor.data)) {
				return;
			}
		}

		if (descriptor.mipLevels > 1) {
			if (!GenerateMipmaps(commandBuffer)) {
				return;
			}
		}

		if (!TransitionFinalImageLayout(commandBuffer, descriptor.initialLayout)) {
			return;
		}

		vkCmdQueue.EndOneTimeCommands(commandBuffer);
	}

	VkTextureCubeArray::~VkTextureCubeArray() {
		_context.AddDelayedDeletionTasks([&context = _context, nativeTexture = _nativeTexture, nativeTexView = _nativeTextureView, sampler = _sampler]() {
			context.GetVkDevice().destroyImage(nativeTexture.image);
			context.GetVkDevice().freeMemory(nativeTexture.memory);
			context.GetVkDevice().destroyImageView(nativeTexView.imageView);
			context.GetVkDevice().destroySampler(sampler);
		});
	}

	bool VkTextureCubeArray::PullMemory(vk::CommandBuffer& commandBuffer, const uint8_t* data) {
		vk::ImageMemoryBarrier barrier;
		barrier.oldLayout = vk::ImageLayout::eUndefined;
		barrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = _nativeTexture.image;
		barrier.subresourceRange.aspectMask = GetVkImageAspectFlags(_format);
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
		barrier.srcAccessMask = vk::AccessFlagBits::eNone;
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

		commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlags(), nullptr, nullptr, barrier);

		uint32_t bufferSizePerImage = _width * _height * GetSizePerPixel(_format);
		uint32_t bufferSize = bufferSizePerImage * _layers * 6; // 6 layers per cube

		auto stagingBuffer = VkNativeBuffer::CreateAsStaging(_context, bufferSize, data);

		std::vector<vk::BufferImageCopy> copyRegions(_layers * 6);
		for (uint32_t i = 0; i < _layers * 6; ++i) {
			copyRegions[i].bufferOffset = bufferSizePerImage * i;
			copyRegions[i].bufferRowLength = 0;
			copyRegions[i].bufferImageHeight = 0;
			copyRegions[i].imageSubresource.aspectMask = GetVkImageAspectFlags(_format);
			copyRegions[i].imageSubresource.mipLevel = 0;
			copyRegions[i].imageSubresource.baseArrayLayer = i;
			copyRegions[i].imageSubresource.layerCount = 1;
			copyRegions[i].imageOffset = vk::Offset3D{ 0, 0, 0 };
			copyRegions[i].imageExtent = vk::Extent3D{ _width, _height, 1 };
		}

		commandBuffer.copyBufferToImage(stagingBuffer.buffer, _nativeTexture.image, vk::ImageLayout::eTransferDstOptimal, copyRegions.size(), copyRegions.data());

		_context.AddDelayedDeletionTasks([&context = _context, stagingBuffer]() {
			context.GetVkDevice().destroyBuffer(stagingBuffer.buffer);
			context.GetVkDevice().freeMemory(stagingBuffer.memory);
		});

		return true;
	}

	bool VkTextureCubeArray::GenerateMipmaps(vk::CommandBuffer& commandBuffer) {
		// TODO: Implement mipmap generation for texture cube arrays
		return true;
	}

	bool VkTextureCubeArray::TransitionFinalImageLayout(vk::CommandBuffer& commandBuffer, TextureLayout layout) {
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

	bool VkTextureCubeArray::CreateImageView() {
		vk::ImageViewCreateInfo viewInfo;
		viewInfo.image = _nativeTexture.image;
		viewInfo.viewType = vk::ImageViewType::eCubeArray;
		viewInfo.format = ConvertToVkFormat(_format);
		viewInfo.subresourceRange.aspectMask = GetVkImageAspectFlags(_format);
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = _mipLevels;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = _layers * 6; // 6 layers per cube

		auto imageViewWrapper = _context.GetVkDevice().createImageView(viewInfo);
		if (imageViewWrapper.result != vk::Result::eSuccess) {
			Log::Fatal("Failed to create Vulkan image view: %s", vk::to_string(imageViewWrapper.result).c_str());
			return false;
		}

		_nativeTextureView.imageView = imageViewWrapper.value;

		return true;
	}

	bool VkTextureCubeArray::CreateSampler() {
		vk::SamplerCreateInfo samplerInfo;
		samplerInfo.magFilter = ConvertToVkFilter(_magFilter);
		samplerInfo.minFilter = ConvertToVkFilter(_minFilter);
		samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
		samplerInfo.addressModeU = ConvertToVkSamplerAddressMode(_wrapModeU);
		samplerInfo.addressModeV = ConvertToVkSamplerAddressMode(_wrapModeV);
		samplerInfo.addressModeW = ConvertToVkSamplerAddressMode(_wrapModeW);
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