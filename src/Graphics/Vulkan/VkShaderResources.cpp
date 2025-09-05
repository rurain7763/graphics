#include "pch.h"
#include "VkShaders.h"

#ifdef SUPPORT_VULKAN

#include "VkContext.h"
#include "Log/Log.h"
#include "VkTextures.h"
#include "VkBuffers.h"

namespace flaw {
	VkShaderResourcesLayout::VkShaderResourcesLayout(VkContext& context, const Descriptor& descriptor)
		: _context(context) 
    {
        std::vector<vk::DescriptorSetLayoutBinding> bindings(descriptor.bindings.size());
        for (int32_t i = 0; i < descriptor.bindings.size(); ++i) {
            const auto& binding = descriptor.bindings[i];
            auto& vkBinding = bindings[i];

            vkBinding.binding = binding.binding;
            vkBinding.descriptorType = ConvertToVkDescriptorType(binding.resourceType);
            vkBinding.descriptorCount = binding.count;
            vkBinding.stageFlags = ConvertToVkShaderStages(binding.shaderStages);
        }

        vk::DescriptorSetLayoutCreateInfo layoutInfo;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        auto descriptorWrapper = _context.GetVkDevice().createDescriptorSetLayout(layoutInfo, nullptr);
        if (descriptorWrapper.result != vk::Result::eSuccess) {
            Log::Error("Failed to create descriptor set layout: %s", vk::to_string(descriptorWrapper.result).c_str());
            throw std::runtime_error("Failed to create descriptor set layout");
        }
        
        _descriptorSetLayout = descriptorWrapper.value;
    }

	VkShaderResourcesLayout::~VkShaderResourcesLayout() {
        _context.AddDelayedDeletionTasks([&context = _context, layout = _descriptorSetLayout]() {
            context.GetVkDevice().destroyDescriptorSetLayout(layout, nullptr);
        });
	}

    VkShaderResources::VkShaderResources(VkContext& context, const Descriptor& descriptor)
        : _context(context) 
    {
        auto vkShaderResourcesLayout = std::dynamic_pointer_cast<VkShaderResourcesLayout>(descriptor.layout);
        FASSERT(vkShaderResourcesLayout, "Invalid shader resources layout type for Vulkan");

        vk::DescriptorSetAllocateInfo allocInfo;
        allocInfo.descriptorPool = _context.GetVkDescriptorPool();
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &vkShaderResourcesLayout->GetVkDescriptorSetLayout();

        auto descriptorSetWrapper = _context.GetVkDevice().allocateDescriptorSets(allocInfo);
        if (descriptorSetWrapper.result != vk::Result::eSuccess) {
            Log::Error("Failed to allocate Vulkan descriptor set: %s", vk::to_string(descriptorSetWrapper.result).c_str());
            throw std::runtime_error("Failed to allocate Vulkan descriptor set");
        }

        _descriptorSet = descriptorSetWrapper.value[0];
		_layout = vkShaderResourcesLayout;
    }

    VkShaderResources::~VkShaderResources() {
        _context.AddDelayedDeletionTasks([&context = _context, descriptorSet = _descriptorSet]() {
            context.GetVkDevice().freeDescriptorSets(context.GetVkDescriptorPool(), { descriptorSet });
        });
    }

    void VkShaderResources::BindTexture2D(const Ref<Texture2D>& texture, uint32_t binding) {
        auto vkTexture = std::dynamic_pointer_cast<VkTexture2D>(texture);
        FASSERT(vkTexture, "Invalid texture type for Vulkan shader resources");

		const auto& vkNativeTexView = static_cast<const VkNativeTextureView&>(texture->GetNativeTextureView());

		vk::DescriptorImageInfo imageInfo;
		imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		imageInfo.imageView = vkNativeTexView.imageView;
		imageInfo.sampler = vkTexture->GetVkSampler();

        vk::WriteDescriptorSet writeDescSet;
        writeDescSet.dstSet = _descriptorSet;
        writeDescSet.dstBinding = binding;
        writeDescSet.dstArrayElement = 0;
        writeDescSet.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        writeDescSet.descriptorCount = 1;
		writeDescSet.pImageInfo = &imageInfo;

        _context.GetVkDevice().updateDescriptorSets(1, &writeDescSet, 0, nullptr);
    }

    void VkShaderResources::BindTextureCube(const Ref<TextureCube>& texture, uint32_t binding) {
        auto vkTexture = std::dynamic_pointer_cast<VkTextureCube>(texture);
        FASSERT(vkTexture, "Invalid texture cube type for Vulkan shader resources");

        const auto& vkNativeTexView = static_cast<const VkNativeTextureView&>(texture->GetNativeTextureView());

        vk::DescriptorImageInfo imageInfo;
		imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		imageInfo.imageView = vkNativeTexView.imageView;
		imageInfo.sampler = vkTexture->GetVkSampler();

        vk::WriteDescriptorSet writeDescSet;
        writeDescSet.dstSet = _descriptorSet;
        writeDescSet.dstBinding = binding;
        writeDescSet.dstArrayElement = 0;
        writeDescSet.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        writeDescSet.descriptorCount = 1;
        writeDescSet.pImageInfo = &imageInfo;

        _context.GetVkDevice().updateDescriptorSets(1, &writeDescSet, 0, nullptr);
    }

    void VkShaderResources::BindConstantBuffer(const Ref<ConstantBuffer>& constantBuffer, uint32_t binding) {
        auto vkConstantBuffer = std::dynamic_pointer_cast<VkConstantBuffer>(constantBuffer);
        FASSERT(vkConstantBuffer, "Invalid constant buffer type for Vulkan shader resources");

		const auto& vkNativeBuff = static_cast<const VkNativeBuffer&>(vkConstantBuffer->GetNativeBuffer());

        vk::DescriptorBufferInfo bufferInfo;
		bufferInfo.buffer = vkNativeBuff.buffer;
		bufferInfo.offset = 0;
		bufferInfo.range = vkConstantBuffer->Size();

        vk::WriteDescriptorSet writeDescSet;
        writeDescSet.dstSet = _descriptorSet;
        writeDescSet.dstBinding = binding;
        writeDescSet.dstArrayElement = 0;
        writeDescSet.descriptorType = vk::DescriptorType::eUniformBuffer;
        writeDescSet.descriptorCount = 1;
        writeDescSet.pBufferInfo = &bufferInfo;

        _context.GetVkDevice().updateDescriptorSets(1, &writeDescSet, 0, nullptr);
    }

    void VkShaderResources::BindStructuredBuffer(const Ref<StructuredBuffer>& structuredBuffer, uint32_t binding) {
        auto vkStructuredBuffer = std::dynamic_pointer_cast<VkStructuredBuffer>(structuredBuffer);
        FASSERT(vkStructuredBuffer, "Invalid structured buffer type for Vulkan shader resources");

		const auto& vkNativeBuff = static_cast<const VkNativeBuffer&>(vkStructuredBuffer->GetNativeBuffer());

		vk::DescriptorBufferInfo bufferInfo;
		bufferInfo.buffer = vkNativeBuff.buffer;
		bufferInfo.offset = 0;
		bufferInfo.range = vkStructuredBuffer->Size();

        vk::WriteDescriptorSet writeDescSet;
        writeDescSet.dstSet = _descriptorSet;
        writeDescSet.dstBinding = binding;
        writeDescSet.dstArrayElement = 0;
        writeDescSet.descriptorType = vk::DescriptorType::eStorageBuffer;
        writeDescSet.descriptorCount = 1;
        writeDescSet.pBufferInfo = &bufferInfo;

        _context.GetVkDevice().updateDescriptorSets(1, &writeDescSet, 0, nullptr);
    }

    void VkShaderResources::BindInputAttachment(const Ref<Texture>& texture, uint32_t binding) {
        auto vkTexture = std::dynamic_pointer_cast<VkTexture2D>(texture);
        FASSERT(vkTexture, "Invalid texture type for Vulkan shader resources");

        const auto& vkNativeTexView = static_cast<const VkNativeTextureView&>(texture->GetNativeTextureView());

        vk::DescriptorImageInfo imageInfo;
        imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		imageInfo.imageView = vkNativeTexView.imageView;
        imageInfo.sampler = nullptr;

        vk::WriteDescriptorSet writeDescSet;
        writeDescSet.dstSet = _descriptorSet;
        writeDescSet.dstBinding = binding;
        writeDescSet.dstArrayElement = 0;
        writeDescSet.descriptorType = vk::DescriptorType::eInputAttachment;
        writeDescSet.descriptorCount = 1;
        writeDescSet.pImageInfo = &imageInfo;

        _context.GetVkDevice().updateDescriptorSets(1, &writeDescSet, 0, nullptr);
    }
}

#endif