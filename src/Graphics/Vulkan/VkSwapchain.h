#pragma once

#include "Core.h"

#ifdef SUPPORT_VULKAN

#include "VkCore.h"
#include "VkTextures.h"
#include "VkFramebuffer.h"
#include "VkRenderPass.h"

namespace flaw {
    class VkContext;

    class VkSwapchain {
    public:
        VkSwapchain(VkContext& context);
        ~VkSwapchain();
 
        int32_t Create(uint32_t width, uint32_t height);
        int32_t Resize(uint32_t width, uint32_t height);

        void Destroy();

        explicit operator bool() const { return _swapchain; }

        inline uint32_t GetColorAttachmentCount() const { return _colorAttachments.size(); }
        inline Ref<VkTexture2D> GetColorAttachment(uint32_t index) const { return _colorAttachments[index]; }
        inline vk::SwapchainKHR& GetNativeVkSwapchain() { return _swapchain; }
        inline PixelFormat GetSurfaceFormat() const { return ConvertToPixelFormat(_surfaceFormat.format); }
        inline PixelFormat GetDepthStencilFormat() const { return _depthStencilFormat; }

    private:
        bool CreateSwapchain();
        vk::SurfaceFormatKHR ChooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& formats) const;
        vk::PresentModeKHR ChoosePresentMode(const std::vector<vk::PresentModeKHR>& presentModes) const;
        vk::Extent2D ChooseExtent(const vk::SurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height) const;
        vk::Format ChooseDepthFormat(const std::vector<vk::Format>& candidates) const;

        bool CreateColorAttachments();

    private:  
        VkContext& _context;

        vk::SurfaceFormatKHR _surfaceFormat;
        vk::PresentModeKHR _presentMode;
        vk::Extent2D _extent;
        vk::SurfaceTransformFlagBitsKHR _transform;

        PixelFormat _depthStencilFormat;

        vk::SwapchainKHR _swapchain;
        std::vector<Ref<VkTexture2D>> _colorAttachments;
    };
}

#endif
