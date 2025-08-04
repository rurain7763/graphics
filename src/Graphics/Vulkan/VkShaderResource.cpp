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

        auto descriptorWrapper = _context.GetVkDevice().createDescriptorSetLayout(layoutInfo, nullptr, _context.GetVkDispatchLoader());
        if (descriptorWrapper.result != vk::Result::eSuccess) {
            Log::Error("Failed to create descriptor set layout: %s", vk::to_string(descriptorWrapper.result).c_str());
            throw std::runtime_error("Failed to create descriptor set layout");
        }
        
        _descriptorSetLayout = descriptorWrapper.value;
    }

	VkShaderResourcesLayout::~VkShaderResourcesLayout() {
        _context.AddDelayedDeletionTasks([&context = _context, layout = _descriptorSetLayout]() {
            context.GetVkDevice().destroyDescriptorSetLayout(layout, nullptr, context.GetVkDispatchLoader());
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
    }

    VkShaderResources::~VkShaderResources() {
        _context.AddDelayedDeletionTasks([&context = _context, descriptorSet = _descriptorSet]() {
            context.GetVkDevice().freeDescriptorSets(context.GetVkDescriptorPool(), { descriptorSet }, context.GetVkDispatchLoader());
        });
    }

    void VkShaderResources::BindTexture2D(const Ref<Texture2D>& texture, uint32_t binding) {
        auto vkTexture = std::dynamic_pointer_cast<VkTexture2D>(texture);
        FASSERT(vkTexture, "Invalid texture type for Vulkan shader resources");

        vk::WriteDescriptorSet writeDescSet;
        writeDescSet.dstSet = _descriptorSet;
        writeDescSet.dstBinding = binding;
        writeDescSet.dstArrayElement = 0;
        writeDescSet.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        writeDescSet.descriptorCount = 1;
        writeDescSet.pImageInfo = &vkTexture->GetVkDescriptorImageInfo();

        _context.GetVkDevice().updateDescriptorSets(1, &writeDescSet, 0, nullptr, _context.GetVkDispatchLoader());
    }

    void VkShaderResources::BindTextureCube(const Ref<TextureCube>& texture, uint32_t binding) {
        auto vkTexture = std::dynamic_pointer_cast<VkTextureCube>(texture);
        FASSERT(vkTexture, "Invalid texture cube type for Vulkan shader resources");

        vk::WriteDescriptorSet writeDescSet;
        writeDescSet.dstSet = _descriptorSet;
        writeDescSet.dstBinding = binding;
        writeDescSet.dstArrayElement = 0;
        writeDescSet.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        writeDescSet.descriptorCount = 1;
        writeDescSet.pImageInfo = &vkTexture->GetVkDescriptorImageInfo();

        _context.GetVkDevice().updateDescriptorSets(1, &writeDescSet, 0, nullptr, _context.GetVkDispatchLoader());
    }

    void VkShaderResources::BindConstantBuffer(const Ref<ConstantBuffer>& constantBuffer, uint32_t binding) {
        auto vkConstantBuffer = std::dynamic_pointer_cast<VkConstantBuffer>(constantBuffer);
        FASSERT(vkConstantBuffer, "Invalid constant buffer type for Vulkan shader resources");

        vk::WriteDescriptorSet writeDescSet;
        writeDescSet.dstSet = _descriptorSet;
        writeDescSet.dstBinding = binding;
        writeDescSet.dstArrayElement = 0;
        writeDescSet.descriptorType = vk::DescriptorType::eUniformBuffer;
        writeDescSet.descriptorCount = 1;
        writeDescSet.pBufferInfo = &vkConstantBuffer->GetVkDescriptorBufferInfo();

        _context.GetVkDevice().updateDescriptorSets(1, &writeDescSet, 0, nullptr, _context.GetVkDispatchLoader());
    }

    void VkShaderResources::BindStructuredBuffer(const Ref<StructuredBuffer>& structuredBuffer, uint32_t binding) {
        auto vkStructuredBuffer = std::dynamic_pointer_cast<VkStructuredBuffer>(structuredBuffer);
        FASSERT(vkStructuredBuffer, "Invalid structured buffer type for Vulkan shader resources");

        vk::WriteDescriptorSet writeDescSet;
        writeDescSet.dstSet = _descriptorSet;
        writeDescSet.dstBinding = binding;
        writeDescSet.dstArrayElement = 0;
        writeDescSet.descriptorType = vk::DescriptorType::eStorageBuffer;
        writeDescSet.descriptorCount = 1;
        writeDescSet.pBufferInfo = &vkStructuredBuffer->GetVkDescriptorBufferInfo();

        _context.GetVkDevice().updateDescriptorSets(1, &writeDescSet, 0, nullptr, _context.GetVkDispatchLoader());
    }
}

#endif