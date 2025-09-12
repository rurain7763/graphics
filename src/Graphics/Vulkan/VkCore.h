#pragma once

#include "Core.h"

#ifdef SUPPORT_VULKAN

#include "Graphics/GraphicsType.h"

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#define VULKAN_HPP_NO_EXCEPTIONS
#include <vulkan/vulkan.hpp>

#include <optional>

namespace flaw {
    struct VkSurfaceDetails {
        vk::SurfaceCapabilitiesKHR capabilities;
        std::vector<vk::SurfaceFormatKHR> formats;
        std::vector<vk::PresentModeKHR> presentModes;
    };

    struct VkQueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;
        std::optional<uint32_t> transferFamily;

        inline bool IsComplete() const {
            return graphicsFamily.has_value() && presentFamily.has_value() && transferFamily.has_value();
        }
    };

    struct VkBufferWrapper {
        vk::Buffer buffer;
        vk::DeviceMemory memory;
        vk::Result result;
    };

    vk::Format ConvertToVkFormat(PixelFormat format);
    PixelFormat ConvertToPixelFormat(vk::Format format);
    vk::PrimitiveTopology ConvertToVkPrimitiveTopology(PrimitiveTopology topology);
    vk::PolygonMode ConvertToVkFillMode(FillMode fillMode);
    vk::CullModeFlags ConvertToVkCullMode(CullMode cullMode);
    vk::CompareOp ConvertToVkCompareOp(CompareOp compareOp);
    vk::StencilOp ConvertToVkStencilOp(StencilOp stencilOp);
    vk::ShaderStageFlagBits ConvertToVkShaderStage(ShaderStage flag);
    vk::VertexInputRate ConvertToVkVertexInputRate(VertexInputRate rate);
    vk::ImageLayout ConvertToVkImageLayout(TextureLayout layout);
    vk::Format ConvertToVkFormat(ElementType type, uint32_t count);
    vk::DescriptorType ConvertToVkDescriptorType(ResourceType resourceType);
    vk::ShaderStageFlags ConvertToVkShaderStages(ShaderStages shaderStages);
    vk::PipelineBindPoint ConvertToVkPipelineBindPoint(PipelineType type);
    vk::AttachmentLoadOp ConvertToVkAttachmentLoadOp(AttachmentLoadOp loadOp);
    vk::AttachmentStoreOp ConvertToVkAttachmentStoreOp(AttachmentStoreOp storeOp);
    vk::SampleCountFlagBits ConvertToVkSampleCount(uint32_t sampleCount);
	vk::AccessFlags ConvertToVkAccessFlags(AccessTypes access);
	vk::PipelineStageFlags ConvertToVkPipelineStageFlags(PipelineStages stages);
	vk::ImageAspectFlags GetVkImageAspectFlags(PixelFormat format);
	vk::ImageAspectFlags GetVkImageAspectFlags(vk::Format format);
    vk::ImageUsageFlags GetVkImageUsageFlags(TextureUsages texUsages);
    vk::ColorComponentFlags GetVkColorComponentFlags(PixelFormat format);
	vk::Filter ConvertToVkFilter(FilterMode filterMode);
	vk::SamplerAddressMode ConvertToVkSamplerAddressMode(WrapMode wrapMode);
    void GetRequiredVkBufferUsageFlags(MemoryProperty usage, vk::BufferUsageFlags& usageFlags);
    void GetRequiredVkMemoryPropertyFlags(MemoryProperty flags, vk::MemoryPropertyFlags& memoryFlags);
    bool CheckSupportedInstanceExtensions(const std::vector<const char*>& requiredExtensions);
    bool CheckSupportedInstanceLayers(const std::vector<const char*>& requiredLayers);
    bool CheckSupportedPhysicalDeviceExtensions(const vk::PhysicalDevice& device, const std::vector<const char*>& requiredExtensions);
    VkSurfaceDetails GetVkSurfaceDetails(const vk::PhysicalDevice& physicalDevice, const vk::SurfaceKHR& surface);
    VkQueueFamilyIndices GetVkQueueFamilyIndices(const vk::PhysicalDevice& physicalDevice, const vk::SurfaceKHR& surface);
    uint32_t GetMaxUsableSampleCount(const vk::PhysicalDevice& physicalDevice);
    uint32_t GetMemoryTypeIndex(const vk::PhysicalDevice& physicalDevice, uint32_t typeBits, vk::MemoryPropertyFlags properties);
}

#endif