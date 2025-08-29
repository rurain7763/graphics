#include "pch.h"
#include "VkSwapchain.h"

#ifdef SUPPORT_VULKAN

#include "VkContext.h"
#include "Log/Log.h"

namespace flaw {
    VkSwapchain::VkSwapchain(VkContext& context)
        : _context(context)
    {
    }

    VkSwapchain::~VkSwapchain() {
        Destroy();
    }

    int32_t VkSwapchain::Create(uint32_t width, uint32_t height) {
        auto surfaceDetails = GetVkSurfaceDetails(_context.GetVkPhysicalDevice(), _context.GetVkSurface());

        _surfaceFormat = ChooseSurfaceFormat(surfaceDetails.formats);
        _presentMode = ChoosePresentMode(surfaceDetails.presentModes);
        _extent = ChooseExtent(surfaceDetails.capabilities, width, height);
        _transform = surfaceDetails.capabilities.currentTransform;

        vk::Format vkDSFormat = ChooseDepthFormat({ vk::Format::eD24UnormS8Uint, vk::Format::eD32SfloatS8Uint, vk::Format::eD32Sfloat });
        _depthStencilFormat = ConvertToPixelFormat(vkDSFormat);

        if (!CreateSwapchain()) {
            Log::Error("Failed to create Vulkan swapchain.");
            return -1;
        }

        if (!CreateRenderTextures()) {
            Log::Error("Failed to create Vulkan swapchain render textures.");
            return -1;
        }

        if (!CreateDepthStencilTextures()) {
            Log::Error("Failed to create Vulkan swapchain depth stencil textures.");
            return -1;
        }

        if (!CreateMSAAColorTextures()) {
            Log::Error("Failed to create Vulkan swapchain MSAA color textures.");
            return -1;
        }

        if (!CreateRenderPasses()) {
            Log::Error("Failed to create Vulkan swapchain render pass.");
            return -1;
        }

        if (!CreateFramebuffers()) {
            Log::Error("Failed to create Vulkan swapchain framebuffer.");
            return -1;
        }

        Log::Info("Vulkan swapchain created successfully");

        return 0;
    }

    int32_t VkSwapchain::Resize(uint32_t width, uint32_t height) {
        if(_swapchain) {
            _frameBuffers.clear();
            _renderTextures.clear();
            _depthStencilTextures.clear();
			_msaaColorTextures.clear();
            _context.GetVkDevice().destroySwapchainKHR(_swapchain, nullptr);
        }

        auto surfaceDetails = GetVkSurfaceDetails(_context.GetVkPhysicalDevice(), _context.GetVkSurface());

        _extent = ChooseExtent(surfaceDetails.capabilities, width, height);
        _transform = surfaceDetails.capabilities.currentTransform;

        if (!CreateSwapchain()) {
            Log::Error("Failed to create Vulkan swapchain.");
            return -1;
        }

        if (!CreateRenderTextures()) {
            Log::Error("Failed to create Vulkan swapchain render textures.");
            return -1;
        }

        if (!CreateDepthStencilTextures()) {
            Log::Error("Failed to create Vulkan swapchain depth stencil textures.");
            return -1;
        }

        if (!CreateMSAAColorTextures()) {
            Log::Error("Failed to create Vulkan swapchain MSAA color textures.");
            return -1;
        }

        if (!CreateFramebuffers()) {
            Log::Error("Failed to create Vulkan swapchain framebuffer.");
            return -1;
        }

        Log::Info("Vulkan swapchain resized successfully to %dx%d", width, height);

        return 0;
    }

    bool VkSwapchain::CreateSwapchain() {
        vk::SwapchainCreateInfoKHR createInfo;
        createInfo.surface = _context.GetVkSurface();
        createInfo.minImageCount = _context.GetFrameCount();
        createInfo.imageFormat = _surfaceFormat.format;
        createInfo.imageColorSpace = _surfaceFormat.colorSpace;
        createInfo.imageExtent = _extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst;
        createInfo.preTransform = _transform;
        createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
        createInfo.presentMode = _presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = vk::SwapchainKHR();

        std::array<uint32_t, 2> queueFamilyIndices = {
            _context.GetVkGraphicsQueueFamilyIndex(),
            _context.GetVkPresentQueueFamilyIndex()
        };

        if (queueFamilyIndices[0] != queueFamilyIndices[1]) {
            createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
            createInfo.queueFamilyIndexCount = queueFamilyIndices.size();
            createInfo.pQueueFamilyIndices = queueFamilyIndices.data();
        } else {
            createInfo.imageSharingMode = vk::SharingMode::eExclusive;
            createInfo.queueFamilyIndexCount = 0;
        }

        auto swapchainWrapper = _context.GetVkDevice().createSwapchainKHR(createInfo);
        if (swapchainWrapper.result != vk::Result::eSuccess) {
            Log::Error("Failed to create Vulkan swapchain: %s", vk::to_string(swapchainWrapper.result).c_str());
            return false;
        }

        _swapchain = swapchainWrapper.value;

        return true;
    }

    vk::SurfaceFormatKHR VkSwapchain::ChooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& formats) const {
        for (const auto& format : formats) {
            if (format.format == vk::Format::eB8G8R8A8Unorm && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
                return format; // Prefer sRGB format
            }
        }
        return formats[0]; // Fallback to the first available format
    }

    vk::PresentModeKHR VkSwapchain::ChoosePresentMode(const std::vector<vk::PresentModeKHR>& presentModes) const {
        for (const auto& mode : presentModes) {
            if (mode == vk::PresentModeKHR::eMailbox) {
                return mode; // Prefer mailbox for lower latency
            }
        }
        return vk::PresentModeKHR::eFifo; // Fallback to FIFO
    }

    vk::Extent2D VkSwapchain::ChooseExtent(const vk::SurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height) const {
        if (capabilities.currentExtent.width != UINT32_MAX) {
            return capabilities.currentExtent;
        } 

        vk::Extent2D extent;
        extent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, extent.width));
        extent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, extent.height));
        return extent;
    }

    vk::Format VkSwapchain::ChooseDepthFormat(const std::vector<vk::Format>& candidates) const {
        for (const auto& format : candidates) {
            vk::FormatProperties props = _context.GetVkPhysicalDevice().getFormatProperties(format);

            if ((props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) == vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
                return format;
            }

            if ((props.linearTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) == vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
                return format;
            }
        }

        throw std::runtime_error("No suitable depth format found");
    }

    bool VkSwapchain::CreateRenderTextures() {
        auto images = _context.GetVkDevice().getSwapchainImagesKHR(_swapchain).value;

        _renderTextures.resize(images.size());
        for (uint32_t i = 0; i < images.size(); ++i) {
            vk::Image image = images[i];

            uint32_t bindFlags = TextureUsage::ColorAttachment | TextureUsage::ShaderResource;

            _renderTextures[i] = CreateRef<VkTexture2D>(
                _context, 
                image, 
                _extent.width, _extent.height, GetSurfaceFormat(), 
                MemoryProperty::Static,
                TextureUsage::ColorAttachment | TextureUsage::ShaderResource,
                1,
                1
            );
        }

        return true;
    }

    bool VkSwapchain::CreateDepthStencilTextures() {
        Texture2D::Descriptor desc;
        desc.width = _extent.width;
        desc.height = _extent.height;
        desc.format = _depthStencilFormat;
        desc.memProperty = MemoryProperty::Static;
        desc.texUsages = TextureUsage::DepthStencilAttachment;
		desc.initialLayout = TextureLayout::DepthStencilAttachment;
        desc.sampleCount = _context.GetMSAAState() ? _context.GetMSAASampleCount() : 1;

        _depthStencilTextures.reserve(_renderTextures.size());
        for (size_t i = 0; i < _renderTextures.size(); ++i) {
            _depthStencilTextures.push_back(CreateRef<VkTexture2D>(_context, desc));
        }

        return true;
    }

    bool VkSwapchain::CreateMSAAColorTextures() {
        if (!_context.GetMSAAState()) {
            return true;
        }

        _msaaColorTextures.reserve(_renderTextures.size());
        for (uint32_t i = 0; i < _renderTextures.size(); ++i) {
            Texture2D::Descriptor desc;
            desc.width = _extent.width;
            desc.height = _extent.height;
            desc.format = GetSurfaceFormat();
            desc.memProperty = MemoryProperty::Static;
            desc.texUsages = TextureUsage::ColorAttachment;
			desc.initialLayout = TextureLayout::ColorAttachment;
            desc.sampleCount = _context.GetMSAASampleCount();

            _msaaColorTextures.push_back(CreateRef<VkTexture2D>(_context, desc));
        }

        return true;
    }

    bool VkSwapchain::CreateRenderPasses() {
        RenderPassLayout::Descriptor renderPassLayoutDesc;
        if (!_context.GetMSAAState()) {
            renderPassLayoutDesc.sampleCount = 1;
            renderPassLayoutDesc.colorAttachments = { { _context.GetSurfaceFormat() } };
            renderPassLayoutDesc.depthStencilAttachment = { _depthStencilFormat };
        } else {
            renderPassLayoutDesc.sampleCount = _context.GetMSAASampleCount();
            renderPassLayoutDesc.colorAttachments = { { _context.GetSurfaceFormat() } };
            renderPassLayoutDesc.depthStencilAttachment = { _depthStencilFormat };
            renderPassLayoutDesc.resolveAttachment = { _context.GetSurfaceFormat() };
        }

        _renderPassLayout = CreateRef<VkRenderPassLayout>(_context, renderPassLayoutDesc);

        RenderPass::Descriptor renderPassDesc;
        renderPassDesc.layout = _renderPassLayout;

        renderPassDesc.depthStencilAttachmentOp = {
            { TextureLayout::Undefined, TextureLayout::DepthStencilAttachment, AttachmentLoadOp::Clear, AttachmentStoreOp::Store, AttachmentLoadOp::DontCare, AttachmentStoreOp::DontCare }
        };

        if (_context.GetMSAAState()) {
            renderPassDesc.colorAttachmentOps = {
                { TextureLayout::Undefined, TextureLayout::ColorAttachment, AttachmentLoadOp::Clear, AttachmentStoreOp::Store }
            };
            renderPassDesc.resolveAttachmentOp = {
                { TextureLayout::Undefined, TextureLayout::PresentSource, AttachmentLoadOp::Clear, AttachmentStoreOp::Store }
            };
        } else {
            renderPassDesc.colorAttachmentOps = {
                { TextureLayout::Undefined, TextureLayout::PresentSource, AttachmentLoadOp::Clear, AttachmentStoreOp::Store }
            };
        }

        _clearOpRenderPass = CreateRef<VkRenderPass>(_context, renderPassDesc);

        renderPassDesc.depthStencilAttachmentOp = {
            { TextureLayout::DepthStencilAttachment, TextureLayout::DepthStencilAttachment, AttachmentLoadOp::Load, AttachmentStoreOp::Store, AttachmentLoadOp::DontCare, AttachmentStoreOp::DontCare }
        };

        if (_context.GetMSAAState()) {
            renderPassDesc.colorAttachmentOps = {
                { TextureLayout::ColorAttachment, TextureLayout::ColorAttachment, AttachmentLoadOp::Load, AttachmentStoreOp::Store }
            };
            renderPassDesc.resolveAttachmentOp = {
                { TextureLayout::PresentSource, TextureLayout::PresentSource, AttachmentLoadOp::Load, AttachmentStoreOp::Store }
            };
        } else {
            renderPassDesc.colorAttachmentOps = {
                { TextureLayout::PresentSource, TextureLayout::PresentSource, AttachmentLoadOp::Load, AttachmentStoreOp::Store }
            };
        }

        _loadOpRenderPass = CreateRef<VkRenderPass>(_context, renderPassDesc);

        return true;
    }

    bool VkSwapchain::CreateFramebuffers() {
        _frameBuffers.reserve(_renderTextures.size());
        for (uint32_t i = 0; i < _renderTextures.size(); ++i) {
            auto renderTexture = _renderTextures[i];
            auto depthTexture = _depthStencilTextures[i];
            
            Framebuffer::Descriptor desc;
            desc.width = _extent.width;
            desc.height = _extent.height;
            desc.renderPassLayout = _renderPassLayout;

            desc.depthStencilAttachment = depthTexture;

            if (_context.GetMSAAState()) {
                desc.colorAttachments.push_back(_msaaColorTextures[i]);
                desc.resolveAttachment = renderTexture;
            } else {
                desc.colorAttachments.push_back(renderTexture);
            }

            _frameBuffers.push_back(CreateRef<VkFramebuffer>(_context, desc));
        }

        return true;
    }

    void VkSwapchain::Destroy() {
        if(!_swapchain) {
            return;
        }

        _clearOpRenderPass.reset();
        _loadOpRenderPass.reset();
        _renderPassLayout.reset();
        _frameBuffers.clear();
        _renderTextures.clear();
        _depthStencilTextures.clear();
        _msaaColorTextures.clear();
        _context.GetVkDevice().destroySwapchainKHR(_swapchain, nullptr);
        _swapchain = VK_NULL_HANDLE;
    }
}

#endif