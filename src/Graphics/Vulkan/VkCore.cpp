#include "pch.h"
#include "VkCore.h"

#ifdef SUPPORT_VULKAN

namespace flaw {
    vk::Format ConvertToVkFormat(PixelFormat format) {
        switch (format) {
        case PixelFormat::BGRX8Unorm: return vk::Format::eB8G8R8A8Unorm;
        case PixelFormat::RGBA8Unorm: return vk::Format::eR8G8B8A8Unorm;
		case PixelFormat::RGBA8Srgb: return vk::Format::eR8G8B8A8Srgb;
        case PixelFormat::RGBA32F: return vk::Format::eR32G32B32A32Sfloat;
        case PixelFormat::RG8: return vk::Format::eR8G8Unorm;
        case PixelFormat::R8: return vk::Format::eR8Unorm;
        case PixelFormat::R8_UINT: return vk::Format::eR8Uint;
        case PixelFormat::R32F: return vk::Format::eR32Sfloat;
        case PixelFormat::R32_UINT: return vk::Format::eR32Uint;
        case PixelFormat::D24S8_UINT: return vk::Format::eD24UnormS8Uint;
        case PixelFormat::D32F_S8UI: return vk::Format::eD32SfloatS8Uint;
        case PixelFormat::D32F: return vk::Format::eD32Sfloat;
        case PixelFormat::BGRA8: return vk::Format::eB8G8R8A8Unorm;
        default:
            throw std::runtime_error("Unknown pixel format");
        }
    }

    PixelFormat ConvertToPixelFormat(vk::Format format) {
        switch (format) {
        case vk::Format::eB8G8R8A8Unorm: return PixelFormat::BGRX8Unorm;
        case vk::Format::eR8G8B8A8Unorm: return PixelFormat::RGBA8Unorm;
		case vk::Format::eR8G8B8A8Srgb: return PixelFormat::RGBA8Srgb;
        case vk::Format::eR32G32B32A32Sfloat: return PixelFormat::RGBA32F;
        case vk::Format::eR8G8Unorm: return PixelFormat::RG8;
        case vk::Format::eR8Unorm: return PixelFormat::R8;
        case vk::Format::eR8Uint: return PixelFormat::R8_UINT;
        case vk::Format::eR32Sfloat: return PixelFormat::R32F;
        case vk::Format::eR32Uint: return PixelFormat::R32_UINT;
        case vk::Format::eD24UnormS8Uint: return PixelFormat::D24S8_UINT;
        case vk::Format::eD32SfloatS8Uint: return PixelFormat::D32F_S8UI;
		case vk::Format::eD32Sfloat: return PixelFormat::D32F;
        default:
            throw std::runtime_error("Unknown Vulkan format");
        }
    }

    vk::PrimitiveTopology ConvertToVkPrimitiveTopology(PrimitiveTopology topology) {
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

    vk::PolygonMode ConvertToVkFillMode(FillMode fillMode) {
        switch (fillMode) {
        case FillMode::Solid: return vk::PolygonMode::eFill;
        case FillMode::Wireframe: return vk::PolygonMode::eLine;
        default:
            throw std::runtime_error("Unknown fill mode");
        }
    }

    vk::CullModeFlags ConvertToVkCullMode(CullMode cullMode) {
        switch (cullMode) {
        case CullMode::None: return vk::CullModeFlagBits::eNone;
        case CullMode::Front: return vk::CullModeFlagBits::eFront;
        case CullMode::Back: return vk::CullModeFlagBits::eBack;
        default:
            throw std::runtime_error("Unknown cull mode");
        }
    }

    vk::CompareOp ConvertToVkCompareOp(CompareOp compareOp) {
        switch (compareOp) {
        case CompareOp::Never: return vk::CompareOp::eNever;
        case CompareOp::Less: return vk::CompareOp::eLess;
        case CompareOp::Equal: return vk::CompareOp::eEqual;
        case CompareOp::LessEqual: return vk::CompareOp::eLessOrEqual;
        case CompareOp::Greater: return vk::CompareOp::eGreater;
        case CompareOp::NotEqual: return vk::CompareOp::eNotEqual;
        case CompareOp::GreaterEqual: return vk::CompareOp::eGreaterOrEqual;
        case CompareOp::Always: return vk::CompareOp::eAlways;
        default:
            throw std::runtime_error("Unknown depth test");
        }
    }

    vk::StencilOp ConvertToVkStencilOp(StencilOp stencilOp) {
        switch (stencilOp) {
        case StencilOp::Keep: return vk::StencilOp::eKeep;
        case StencilOp::Zero: return vk::StencilOp::eZero;
        case StencilOp::Replace: return vk::StencilOp::eReplace;
        case StencilOp::IncrementWrap: return vk::StencilOp::eIncrementAndWrap;
        case StencilOp::IncrementClamp: return vk::StencilOp::eIncrementAndClamp;
        case StencilOp::DecrementWrap: return vk::StencilOp::eDecrementAndWrap;
        case StencilOp::DecrementClamp: return vk::StencilOp::eDecrementAndClamp;
        case StencilOp::Invert: return vk::StencilOp::eInvert;
        default:
            throw std::runtime_error("Unknown stencil operation");
        }
    }

    vk::ShaderStageFlagBits ConvertToVkShaderStage(ShaderStage flag) {
        switch (flag) {
        case ShaderStage::Vertex: return vk::ShaderStageFlagBits::eVertex;
        case ShaderStage::Pixel: return vk::ShaderStageFlagBits::eFragment;
        case ShaderStage::Geometry: return vk::ShaderStageFlagBits::eGeometry;
        case ShaderStage::Hull: return vk::ShaderStageFlagBits::eTessellationControl;
        case ShaderStage::Domain: return vk::ShaderStageFlagBits::eTessellationEvaluation;
        default:
            throw std::runtime_error("Unknown shader compile flag");
        }
    }

    vk::VertexInputRate ConvertToVkVertexInputRate(VertexInputRate rate) {
        switch (rate) {
        case VertexInputRate::Vertex: return vk::VertexInputRate::eVertex;
        case VertexInputRate::Instance: return vk::VertexInputRate::eInstance;
        default:
            throw std::runtime_error("Unknown vertex input rate");
        }
    }

    vk::ImageLayout ConvertToVkImageLayout(TextureLayout layout) {
        switch (layout) {
        case TextureLayout::Undefined: return vk::ImageLayout::eUndefined;
        case TextureLayout::ColorAttachment: return vk::ImageLayout::eColorAttachmentOptimal;
        case TextureLayout::DepthStencilAttachment: return vk::ImageLayout::eDepthStencilAttachmentOptimal;
        case TextureLayout::PresentSource: return vk::ImageLayout::ePresentSrcKHR;
		case TextureLayout::ShaderReadOnly: return vk::ImageLayout::eShaderReadOnlyOptimal;
		case TextureLayout::General: return vk::ImageLayout::eGeneral;
        default:
            throw std::runtime_error("Unknown texture layout");
        }
    }

    vk::ImageAspectFlags GetVkImageAspectFlags(PixelFormat format) {
        vk::ImageAspectFlags aspectFlags = {};

        switch (format) {
        case PixelFormat::D24S8_UINT:
        case PixelFormat::D32F_S8UI:
            aspectFlags = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
            break;
        case PixelFormat::D32F:
			aspectFlags = vk::ImageAspectFlagBits::eDepth;
			break;
        default:
            aspectFlags = vk::ImageAspectFlagBits::eColor;
            break;
        }

		return aspectFlags;
    }

    vk::ImageAspectFlags GetVkImageAspectFlags(vk::Format format) {
		vk::ImageAspectFlags aspectFlags = {};

		switch (format) {
		case vk::Format::eD24UnormS8Uint:
		case vk::Format::eD32SfloatS8Uint:
			aspectFlags = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
			break;
		default:
			aspectFlags = vk::ImageAspectFlagBits::eColor;
			break;
		}

		return aspectFlags;
    }

    vk::ImageUsageFlags GetVkImageUsageFlags(TextureUsages texUsages) {
        vk::ImageUsageFlags usageFlags = {};

        if (texUsages & TextureUsage::ColorAttachment) {
            usageFlags |= vk::ImageUsageFlagBits::eColorAttachment;
        }

        if (texUsages & TextureUsage::DepthStencilAttachment) {
            usageFlags |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
        }

		if (texUsages & TextureUsage::InputAttachment) {
			usageFlags |= vk::ImageUsageFlagBits::eInputAttachment;
		}

        if (texUsages & TextureUsage::ShaderResource) {
            usageFlags |= vk::ImageUsageFlagBits::eSampled;
        }

        if (texUsages & TextureUsage::UnorderedAccess) {
            usageFlags |= vk::ImageUsageFlagBits::eStorage;
        }

        return usageFlags;
    }

    vk::Format ConvertToVkFormat(ElementType type, uint32_t count) {
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

    vk::DescriptorType ConvertToVkDescriptorType(ResourceType resourceType) {
        switch (resourceType) {
        case ResourceType::ConstantBuffer: return vk::DescriptorType::eUniformBuffer;
        case ResourceType::StructuredBuffer: return vk::DescriptorType::eStorageBuffer;
        case ResourceType::Texture2D: return vk::DescriptorType::eCombinedImageSampler;
        case ResourceType::TextureCube: return vk::DescriptorType::eCombinedImageSampler;
		case ResourceType::InputAttachment: return vk::DescriptorType::eInputAttachment;
        default:
            throw std::runtime_error("Unknown resource type");
        }
    }

    vk::ShaderStageFlags ConvertToVkShaderStages(ShaderStages shaderStages) {
        vk::ShaderStageFlags stages = {};
        if (shaderStages & ShaderStage::Vertex) stages |= vk::ShaderStageFlagBits::eVertex;
        if (shaderStages & ShaderStage::Pixel) stages |= vk::ShaderStageFlagBits::eFragment;
        if (shaderStages & ShaderStage::Geometry) stages |= vk::ShaderStageFlagBits::eGeometry;
        if (shaderStages & ShaderStage::Hull) stages |= vk::ShaderStageFlagBits::eTessellationControl;
        if (shaderStages & ShaderStage::Domain) stages |= vk::ShaderStageFlagBits::eTessellationEvaluation;
        return stages;
    }

    vk::PipelineBindPoint ConvertToVkPipelineBindPoint(PipelineType type) {
        switch (type) {
        case PipelineType::Graphics: return vk::PipelineBindPoint::eGraphics;
        case PipelineType::Compute: return vk::PipelineBindPoint::eCompute;
        default:
            throw std::runtime_error("Unknown pipeline type");
        }
    }

    vk::AttachmentLoadOp ConvertToVkAttachmentLoadOp(AttachmentLoadOp loadOp) {
        switch (loadOp) {
        case AttachmentLoadOp::Clear: return vk::AttachmentLoadOp::eClear;
        case AttachmentLoadOp::Load: return vk::AttachmentLoadOp::eLoad;
        case AttachmentLoadOp::DontCare: return vk::AttachmentLoadOp::eDontCare;
        default:
            throw std::runtime_error("Unknown attachment load operation");
        }
    }

    vk::AttachmentStoreOp ConvertToVkAttachmentStoreOp(AttachmentStoreOp storeOp) {
        switch (storeOp) {
        case AttachmentStoreOp::Store: return vk::AttachmentStoreOp::eStore;
        case AttachmentStoreOp::DontCare: return vk::AttachmentStoreOp::eDontCare;
        default:
            throw std::runtime_error("Unknown attachment store operation");
        }
    }

    vk::SampleCountFlagBits ConvertToVkSampleCount(uint32_t sampleCount) {
        switch (sampleCount) {
        case 1: return vk::SampleCountFlagBits::e1;
        case 2: return vk::SampleCountFlagBits::e2;
        case 4: return vk::SampleCountFlagBits::e4;
        case 8: return vk::SampleCountFlagBits::e8;
        case 16: return vk::SampleCountFlagBits::e16;
        case 32: return vk::SampleCountFlagBits::e32;
        case 64: return vk::SampleCountFlagBits::e64;
        default:
            throw std::runtime_error("Unsupported sample count");
        }
    }

    vk::AccessFlags ConvertToVkAccessFlags(AccessTypes access) {
        vk::AccessFlags accessFlags = {};

		if (access & AccessType::VertexElementRead) {
			accessFlags |= vk::AccessFlagBits::eVertexAttributeRead;
		}

        if (access & AccessType::ShaderRead) {
            accessFlags |= vk::AccessFlagBits::eShaderRead;
        }

        if (access & AccessType::ShaderWrite) {
            accessFlags |= vk::AccessFlagBits::eShaderWrite;
        }

        if (access & AccessType::ColorAttachmentRead) {
            accessFlags |= vk::AccessFlagBits::eColorAttachmentRead;
        }

        if (access & AccessType::ColorAttachmentWrite) {
            accessFlags |= vk::AccessFlagBits::eColorAttachmentWrite;
        }

        if (access & AccessType::DepthStencilAttachmentRead) {
            accessFlags |= vk::AccessFlagBits::eDepthStencilAttachmentRead;
        }

        if (access & AccessType::DepthStencilAttachmentWrite) {
            accessFlags |= vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        }

		if (access & AccessType::HostWrite) {
			accessFlags |= vk::AccessFlagBits::eHostWrite;
		}

		return accessFlags;
    }

    vk::PipelineStageFlags ConvertToVkPipelineStageFlags(PipelineStages stages) {
        vk::PipelineStageFlags stageFlags = {};

        if (stages & PipelineStage::TopOfPipe) {
            stageFlags |= vk::PipelineStageFlagBits::eTopOfPipe;
        }

		if (stages & PipelineStage::VertexInput) {
            stageFlags |= vk::PipelineStageFlagBits::eVertexInput;
		}

        if (stages & PipelineStage::VertexShader) {
            stageFlags |= vk::PipelineStageFlagBits::eVertexShader;
        }

		if (stages & PipelineStage::HullShader) {
			stageFlags |= vk::PipelineStageFlagBits::eTessellationControlShader;
		}

		if (stages & PipelineStage::DomainShader) {
			stageFlags |= vk::PipelineStageFlagBits::eTessellationEvaluationShader;
		}

		if (stages & PipelineStage::GeometryShader) {
			stageFlags |= vk::PipelineStageFlagBits::eGeometryShader;
		}

		if (stages & PipelineStage::EarlyPixelTests) {
			stageFlags |= vk::PipelineStageFlagBits::eEarlyFragmentTests;
		}

        if (stages & PipelineStage::PixelShader) {
            stageFlags |= vk::PipelineStageFlagBits::eFragmentShader;
        }

        if (stages & PipelineStage::ColorAttachmentOutput) {
            stageFlags |= vk::PipelineStageFlagBits::eColorAttachmentOutput;
        }

        if (stages & PipelineStage::BottomOfPipe) {
            stageFlags |= vk::PipelineStageFlagBits::eBottomOfPipe;
        }

		if (stages & PipelineStage::Host) {
			stageFlags |= vk::PipelineStageFlagBits::eHost;
		}

		return stageFlags;
    }

    void GetRequiredVkBufferUsageFlags(MemoryProperty usage, vk::BufferUsageFlags& usageFlags) {
        if (usage == MemoryProperty::Static) {
            usageFlags |= vk::BufferUsageFlagBits::eTransferDst;
        }
        else if (usage == MemoryProperty::Dynamic) {
            usageFlags |= vk::BufferUsageFlagBits::eTransferDst;
        }
        else if (usage == MemoryProperty::Staging) {
            usageFlags |= vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst;
        }
    }

    void GetRequiredVkMemoryPropertyFlags(MemoryProperty flags, vk::MemoryPropertyFlags& memoryFlags) {
        switch (flags) {
        case MemoryProperty::Static:
            memoryFlags |= vk::MemoryPropertyFlagBits::eDeviceLocal;
            break;
        case MemoryProperty::Dynamic:
            memoryFlags |= vk::MemoryPropertyFlagBits::eHostCoherent;
            break;
        case MemoryProperty::Staging:
            memoryFlags |= vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
            break;
        default:
            throw std::runtime_error("Unknown usage flag");
        }
    }

    bool CheckSupportedInstanceExtensions(const std::vector<const char*>& requiredExtensions) {
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

    bool CheckSupportedInstanceLayers(const std::vector<const char*>& requiredLayers) {
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

    bool CheckSupportedPhysicalDeviceExtensions(const vk::PhysicalDevice& device, const std::vector<const char*>& requiredExtensions) {
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

    VkSurfaceDetails GetVkSurfaceDetails(const vk::PhysicalDevice& physicalDevice, const vk::SurfaceKHR& surface) {
        VkSurfaceDetails details;
        details.capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface).value;
        details.formats = physicalDevice.getSurfaceFormatsKHR(surface).value;
        details.presentModes = physicalDevice.getSurfacePresentModesKHR(surface).value;
        return details;
    }

    VkQueueFamilyIndices GetVkQueueFamilyIndices(const vk::PhysicalDevice& physicalDevice, const vk::SurfaceKHR& surface) {
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

    uint32_t GetMaxUsableSampleCount(const vk::PhysicalDevice& physicalDevice) {
        vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();
        vk::SampleCountFlags counts = properties.limits.framebufferColorSampleCounts & properties.limits.framebufferDepthSampleCounts;

        if (counts & vk::SampleCountFlagBits::e64) return 64;
        if (counts & vk::SampleCountFlagBits::e32) return 32;
        if (counts & vk::SampleCountFlagBits::e16) return 16;
        if (counts & vk::SampleCountFlagBits::e8) return 8;
        if (counts & vk::SampleCountFlagBits::e4) return 4;
        if (counts & vk::SampleCountFlagBits::e2) return 2;

        return 1;
    }

    uint32_t GetMemoryTypeIndex(const vk::PhysicalDevice& physicalDevice, uint32_t typeBits, vk::MemoryPropertyFlags properties) {
        vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
            if ((typeBits & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("Failed to find suitable memory type");
    }
}

#endif