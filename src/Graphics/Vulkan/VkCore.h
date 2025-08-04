#pragma once

#include "Graphics/GraphicsType.h"

#define VULKAN_HPP_NO_EXCEPTIONS
#include <vulkan/vulkan.hpp>

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

    struct VkBuffer {
        vk::Buffer buffer;
        vk::DeviceMemory memory;
        vk::Result result;
    };

    static vk::Format ConvertToVkFormat(PixelFormat format) {
        switch (format) {
            case PixelFormat::BGRX8: return vk::Format::eB8G8R8A8Unorm;
            case PixelFormat::RGBA8: return vk::Format::eR8G8B8A8Unorm;
            case PixelFormat::RGBA32F: return vk::Format::eR32G32B32A32Sfloat;
            case PixelFormat::RG8: return vk::Format::eR8G8Unorm;
            case PixelFormat::R8: return vk::Format::eR8Unorm;
            case PixelFormat::R8_UINT: return vk::Format::eR8Uint;
            case PixelFormat::R32F: return vk::Format::eR32Sfloat;
            case PixelFormat::R32_UINT: return vk::Format::eR32Uint;
            case PixelFormat::D24S8_UINT: return vk::Format::eD24UnormS8Uint;
            case PixelFormat::D32F_S8UI: return vk::Format::eD32SfloatS8Uint;
            case PixelFormat::BGRA8: return vk::Format::eB8G8R8A8Unorm;
            default:
                throw std::runtime_error("Unknown pixel format");
        }
    } 

    static PixelFormat ConvertToPixelFormat(vk::Format format) {
        switch (format) {
            case vk::Format::eB8G8R8A8Unorm: return PixelFormat::BGRX8;
            case vk::Format::eR8G8B8A8Unorm: return PixelFormat::RGBA8;
            case vk::Format::eR32G32B32A32Sfloat: return PixelFormat::RGBA32F;
            case vk::Format::eR8G8Unorm: return PixelFormat::RG8;
            case vk::Format::eR8Unorm: return PixelFormat::R8;
            case vk::Format::eR8Uint: return PixelFormat::R8_UINT;
            case vk::Format::eR32Sfloat: return PixelFormat::R32F;
            case vk::Format::eR32Uint: return PixelFormat::R32_UINT;
            case vk::Format::eD24UnormS8Uint: return PixelFormat::D24S8_UINT;
            case vk::Format::eD32SfloatS8Uint: return PixelFormat::D32F_S8UI;
            default:
                throw std::runtime_error("Unknown Vulkan format");
        }
    }

    static vk::PrimitiveTopology ConvertToVkPrimitiveTopology(PrimitiveTopology topology) {
        switch (topology) {
            case PrimitiveTopology::PointList: return vk::PrimitiveTopology::ePointList;
            case PrimitiveTopology::LineList: return vk::PrimitiveTopology::eLineList;
            case PrimitiveTopology::LineStrip: return vk::PrimitiveTopology::eLineStrip;
            case PrimitiveTopology::TriangleList: return vk::PrimitiveTopology::eTriangleList;
            case PrimitiveTopology::TriangleStrip: return vk::PrimitiveTopology::eTriangleStrip;
            default:
                throw std::runtime_error("Unknown primitive topology");
        }
    }

    static vk::PolygonMode ConvertToVkFillMode(FillMode fillMode) {
        switch (fillMode) {
            case FillMode::Solid: return vk::PolygonMode::eFill;
            case FillMode::Wireframe: return vk::PolygonMode::eLine;
            default:
                throw std::runtime_error("Unknown fill mode");
        }
    }

    static vk::CullModeFlags ConvertToVkCullMode(CullMode cullMode) {
        switch (cullMode) {
            case CullMode::None: return vk::CullModeFlagBits::eNone;
            case CullMode::Front: return vk::CullModeFlagBits::eFront;
            case CullMode::Back: return vk::CullModeFlagBits::eBack;
            default:
                throw std::runtime_error("Unknown cull mode");
        }
    }

    static vk::CompareOp ConvertToVkDepthTest(DepthTest depthTest) {
        switch (depthTest) {
            case DepthTest::Never: return vk::CompareOp::eNever;
            case DepthTest::Less: return vk::CompareOp::eLess;
            case DepthTest::Equal: return vk::CompareOp::eEqual;
            case DepthTest::LessEqual: return vk::CompareOp::eLessOrEqual;
            case DepthTest::Greater: return vk::CompareOp::eGreater;
            case DepthTest::NotEqual: return vk::CompareOp::eNotEqual;
            case DepthTest::GreaterEqual: return vk::CompareOp::eGreaterOrEqual;
            case DepthTest::Always: return vk::CompareOp::eAlways;
            case DepthTest::Disabled: return vk::CompareOp::eNever;
            default:
                throw std::runtime_error("Unknown depth test");
        }
    }

    static vk::ShaderStageFlagBits ConvertToVkShaderStage(ShaderCompileFlag flag) {
        switch (flag) {
            case ShaderCompileFlag::Vertex: return vk::ShaderStageFlagBits::eVertex;
            case ShaderCompileFlag::Pixel: return vk::ShaderStageFlagBits::eFragment;
            case ShaderCompileFlag::Geometry: return vk::ShaderStageFlagBits::eGeometry;
            case ShaderCompileFlag::Hull: return vk::ShaderStageFlagBits::eTessellationControl;
            case ShaderCompileFlag::Domain: return vk::ShaderStageFlagBits::eTessellationEvaluation;
            default:
                throw std::runtime_error("Unknown shader compile flag");
        }
    }

    static vk::VertexInputRate ConvertToVkVertexInputRate(VertexInputRate rate) {
        switch (rate) {
            case VertexInputRate::Vertex: return vk::VertexInputRate::eVertex;
            case VertexInputRate::Instance: return vk::VertexInputRate::eInstance;
            default:
                throw std::runtime_error("Unknown vertex input rate");
        }
    }

    static vk::ImageLayout ConvertToVkImageLayout(TextureLayout layout) {
        switch (layout) {
            case TextureLayout::Undefined: return vk::ImageLayout::eUndefined;
            case TextureLayout::Color: return vk::ImageLayout::eColorAttachmentOptimal;
            case TextureLayout::DepthStencil: return vk::ImageLayout::eDepthStencilAttachmentOptimal;
            case TextureLayout::Present: return vk::ImageLayout::ePresentSrcKHR;
            default:
                throw std::runtime_error("Unknown texture layout");
        }
    }

    static vk::ImageAspectFlags ConvertToVkImageAspectFlags(uint32_t bindFlags) {
        vk::ImageAspectFlags aspectFlags = {};

        if (bindFlags & BindFlag::DepthStencil) {
            aspectFlags |= vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
        }

        if (bindFlags & BindFlag::DepthOnly) {
            aspectFlags |= vk::ImageAspectFlagBits::eDepth;
        }

        if (bindFlags & BindFlag::StencilOnly) {
            aspectFlags |= vk::ImageAspectFlagBits::eStencil;
        }
        
        if (bindFlags & (BindFlag::RenderTarget | BindFlag::ShaderResource | BindFlag::UnorderedAccess)) {
            aspectFlags |= vk::ImageAspectFlagBits::eColor;
        }

        return aspectFlags;
    }

    static vk::ImageUsageFlags ConvertToVkImageUsageFlags(uint32_t bindFlags) {
        vk::ImageUsageFlags usageFlags = {};

        if (bindFlags & BindFlag::RenderTarget) {
            usageFlags |= vk::ImageUsageFlagBits::eColorAttachment;
        }

        if (bindFlags & BindFlag::DepthStencil) {
            usageFlags |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
        }

        if (bindFlags & BindFlag::ShaderResource) {
            usageFlags |= vk::ImageUsageFlagBits::eSampled;
        }

        if (bindFlags & BindFlag::UnorderedAccess) {
            usageFlags |= vk::ImageUsageFlagBits::eStorage;
        }

        return usageFlags;
    }

    static vk::Format ConvertToVkFormat(ElementType type, uint32_t count) {
        switch (type) {
            case ElementType::Float:
                if (count == 1) return vk::Format::eR32Sfloat;
                if (count == 2) return vk::Format::eR32G32Sfloat;
                if (count == 3) return vk::Format::eR32G32B32Sfloat;
                if (count == 4) return vk::Format::eR32G32B32A32Sfloat;
                break;
            case ElementType::Uint32:
                if (count == 1) return vk::Format::eR32Uint;
                if (count == 2) return vk::Format::eR32G32Uint;
                if (count == 3) return vk::Format::eR32G32B32Uint;
                if (count == 4) return vk::Format::eR32G32B32A32Uint;
                break;
            case ElementType::Int:
                if (count == 1) return vk::Format::eR32Sint;
                if (count == 2) return vk::Format::eR32G32Sint;
                if (count == 3) return vk::Format::eR32G32B32Sint;
                if (count == 4) return vk::Format::eR32G32B32A32Sint;
                break;
            default:
                throw std::runtime_error("Unsupported input element type");
        }
        throw std::runtime_error("Unsupported element count");
    }

    static vk::DescriptorType ConvertToVkDescriptorType(ResourceType resourceType) {
        switch (resourceType) {
            case ResourceType::ConstantBuffer: return vk::DescriptorType::eUniformBuffer;
            case ResourceType::StructuredBuffer: return vk::DescriptorType::eStorageBuffer;
            case ResourceType::Texture2D: return vk::DescriptorType::eCombinedImageSampler;
            case ResourceType::TextureCube: return vk::DescriptorType::eCombinedImageSampler;
            default:
                throw std::runtime_error("Unknown resource type");
        }
    }

    static vk::ShaderStageFlags ConvertToVkShaderStages(uint32_t compileFlags) {
        vk::ShaderStageFlags stages = {};
        if (compileFlags & ShaderCompileFlag::Vertex) stages |= vk::ShaderStageFlagBits::eVertex;
        if (compileFlags & ShaderCompileFlag::Pixel) stages |= vk::ShaderStageFlagBits::eFragment;
        if (compileFlags & ShaderCompileFlag::Geometry) stages |= vk::ShaderStageFlagBits::eGeometry;
        if (compileFlags & ShaderCompileFlag::Hull) stages |= vk::ShaderStageFlagBits::eTessellationControl;
        if (compileFlags & ShaderCompileFlag::Domain) stages |= vk::ShaderStageFlagBits::eTessellationEvaluation;
        return stages;
    }

    static vk::PipelineBindPoint ConvertToVkPipelineBindPoint(PipelineType type) {
        switch (type) {
            case PipelineType::Graphics: return vk::PipelineBindPoint::eGraphics;
            case PipelineType::Compute: return vk::PipelineBindPoint::eCompute;
            default:
                throw std::runtime_error("Unknown pipeline type");
        }
    }

    static vk::AttachmentLoadOp ConvertToVkAttachmentLoadOp(AttachmentLoadOp loadOp) {
        switch (loadOp) {
            case AttachmentLoadOp::Clear: return vk::AttachmentLoadOp::eClear;
            case AttachmentLoadOp::Load: return vk::AttachmentLoadOp::eLoad;
            case AttachmentLoadOp::DontCare: return vk::AttachmentLoadOp::eDontCare;
            default:
                throw std::runtime_error("Unknown attachment load operation");
        }
    }

    static vk::AttachmentStoreOp ConvertToVkAttachmentStoreOp(AttachmentStoreOp storeOp) {
        switch (storeOp) {
            case AttachmentStoreOp::Store: return vk::AttachmentStoreOp::eStore;
            case AttachmentStoreOp::DontCare: return vk::AttachmentStoreOp::eDontCare;
            default:
                throw std::runtime_error("Unknown attachment store operation");
        }
    }

    static void GetRequiredVkBufferUsageFlags(UsageFlag usage, vk::BufferUsageFlags& usageFlags) {
        if (usage == UsageFlag::Static) {
            usageFlags |= vk::BufferUsageFlagBits::eTransferDst;
        } else if (usage == UsageFlag::Dynamic) {
            usageFlags |= vk::BufferUsageFlagBits::eTransferDst;
        } else if (usage == UsageFlag::Staging) {
            usageFlags |= vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst;
        }
    }

    static void GetRequiredVkBufferUsageFlags(uint32_t accessFlags, vk::BufferUsageFlags& usageFlags) {
        if (accessFlags & AccessFlag::Read) {
            usageFlags |= vk::BufferUsageFlagBits::eTransferSrc;
        }
        if (accessFlags & AccessFlag::Write) {
            usageFlags |= vk::BufferUsageFlagBits::eTransferDst;
        }
    }

    static void GetRequiredVkMemoryPropertyFlags(UsageFlag flags, vk::MemoryPropertyFlags& memoryFlags) {
        switch (flags) {
            case UsageFlag::Static:
                memoryFlags |= vk::MemoryPropertyFlagBits::eDeviceLocal;
                break;
            case UsageFlag::Dynamic:
                memoryFlags |= vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
                break;
            case UsageFlag::Staging:
                memoryFlags |= vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
                break;
            default:
                throw std::runtime_error("Unknown usage flag");
        }
    }

    static bool CheckSupportedInstanceExtensions(const std::vector<const char*>& requiredExtensions) {
        auto availableExtensions = vk::enumerateInstanceExtensionProperties().value;
        for (const auto& ext : requiredExtensions) {
            bool found = false;
            for (const auto& available : availableExtensions) {
                if (std::strcmp(available.extensionName, ext) == 0) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                return false;
            }
        }
        return true;
    }

    static bool CheckSupportedInstanceLayers(const std::vector<const char*>& requiredLayers) {
        auto availableLayers = vk::enumerateInstanceLayerProperties().value;
        for (const auto& layer : requiredLayers) {
            bool found = false;
            for (const auto& available : availableLayers) {
                if (std::strcmp(available.layerName, layer) == 0) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                return false;
            }
        }
        return true;
    }

    static bool CheckSupportedPhysicalDeviceExtensions(const vk::PhysicalDevice& device, const std::vector<const char*>& requiredExtensions) {
        auto availableExtensions = device.enumerateDeviceExtensionProperties().value;
        for (const auto& ext : requiredExtensions) {
            bool found = false;
            for (const auto& available : availableExtensions) {
                if (std::strcmp(available.extensionName, ext) == 0) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                return false;
            }
        }
        return true;
    }

    static VkSurfaceDetails GetVkSurfaceDetails(const vk::PhysicalDevice& physicalDevice, const vk::SurfaceKHR& surface) {
        VkSurfaceDetails details;
        details.capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface).value;
        details.formats = physicalDevice.getSurfaceFormatsKHR(surface).value;
        details.presentModes = physicalDevice.getSurfacePresentModesKHR(surface).value;
        return details;
    }

    static VkQueueFamilyIndices GetVkQueueFamilyIndices(const vk::PhysicalDevice& physicalDevice, const vk::SurfaceKHR& surface) {
        VkQueueFamilyIndices indices;

        auto queueFamilies = physicalDevice.getQueueFamilyProperties();
        for (uint32_t i = 0; i < queueFamilies.size(); ++i) {
            if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics) {
                indices.graphicsFamily = i;
            }

            if (physicalDevice.getSurfaceSupportKHR(i, surface).value) {
                indices.presentFamily = i;
            }

            if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eTransfer) {
                indices.transferFamily = i;
            }

            if (indices.IsComplete()) {
                break;
            }
        }

        return indices;
    }

    static uint32_t GetMemoryTypeIndex(const vk::PhysicalDevice& physicalDevice, uint32_t typeBits, vk::MemoryPropertyFlags properties) {
        vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
            if ((typeBits & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("Failed to find suitable memory type");
    }

    static VkBuffer CreateVkBuffer(const vk::PhysicalDevice& physicalDevice, const vk::Device& device, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties) {
        VkBuffer buffer;

        vk::BufferCreateInfo bufferInfo;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = vk::SharingMode::eExclusive;

        auto bufferWrapper = device.createBuffer(bufferInfo, nullptr);
        if (bufferWrapper.result != vk::Result::eSuccess) {
            buffer.result = bufferWrapper.result;
            return buffer;
        }

        buffer.buffer = bufferWrapper.value;

        vk::MemoryRequirements memRequirements = device.getBufferMemoryRequirements(buffer.buffer);

        vk::MemoryAllocateInfo allocInfo;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = GetMemoryTypeIndex(physicalDevice, memRequirements.memoryTypeBits, properties);

        auto memoryWrapper = device.allocateMemory(allocInfo, nullptr);
        if (memoryWrapper.result != vk::Result::eSuccess) {
            buffer.result = memoryWrapper.result;
            return buffer;
        }

        buffer.memory = memoryWrapper.value;

        device.bindBufferMemory(buffer.buffer, buffer.memory, 0);

        buffer.result = vk::Result::eSuccess;
        return buffer;
    }
}