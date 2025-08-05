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

        inline uint32_t GetRenderTextureCount() const { return _renderTextures.size(); }
        inline Ref<VkTexture2D> GetRenderTexture(uint32_t index) const { return _renderTextures[index]; }
        inline vk::SwapchainKHR& GetNativeVkSwapchain() { return _swapchain; }
        inline const std::vector<Ref<VkFramebuffer>>& GetFramebuffers() const { return _frameBuffers; }
        inline Ref<VkFramebuffer> GetFramebuffer(uint32_t index) const { return _frameBuffers[index]; }
        inline Ref<VkRenderPassLayout> GetRenderPassLayout() const { return _renderPassLayout; }
        inline Ref<VkRenderPass> GetLoadOpRenderPass() const { return _loadOpRenderPass; }
        inline Ref<VkRenderPass> GetClearOpRenderPass() const { return _clearOpRenderPass; }

    private:
        bool CreateSwapchain(uint32_t renderTexCount, uint32_t width, uint32_t height);
        vk::SurfaceFormatKHR ChooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& formats) const;
        vk::PresentModeKHR ChoosePresentMode(const std::vector<vk::PresentModeKHR>& presentModes) const;
        vk::Extent2D ChooseExtent(const vk::SurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height) const;
        vk::Format ChooseDepthFormat(const std::vector<vk::Format>& candidates) const;

        bool CreateRenderTextures();

        bool CreateDepthStencilTextures();

        bool CreateMSAAColorTextures();

        bool CreateRenderPasses();

        bool CreateFramebuffers();

    private:  
        VkContext& _context;

        vk::SurfaceFormatKHR _surfaceFormat;
        vk::PresentModeKHR _presentMode;
        vk::Extent2D _extent;
        vk::SurfaceTransformFlagBitsKHR _transform;

        PixelFormat _depthStencilFormat;

        vk::SwapchainKHR _swapchain;
        std::vector<Ref<VkTexture2D>> _renderTextures;
        std::vector<Ref<VkTexture2D>> _depthStencilTextures;
        std::vector<Ref<VkTexture2D>> _msaaColorTextures;
        std::vector<Ref<VkFramebuffer>> _frameBuffers;
        Ref<VkRenderPassLayout> _renderPassLayout;
        Ref<VkRenderPass> _clearOpRenderPass;
        Ref<VkRenderPass> _loadOpRenderPass;
    };
}

#endif
