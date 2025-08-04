#pragma once

#include "Core.h"

#ifdef SUPPORT_VULKAN

#include "VkCore.h"
#include "Graphics/GraphicsFramebuffer.h"

namespace flaw {
    class VkContext;
    class VkRenderPassLayout;
    class VkRenderPass;

    class VkFramebuffer : public GraphicsFramebuffer
    {
    public:
        VkFramebuffer(VkContext &context, const Descriptor &descriptor);
        ~VkFramebuffer();

        void Resize(uint32_t width, uint32_t height) override;

        inline Ref<Texture> GetAttachment(uint32_t index) const override { return _colorAttachments[index]; }
        inline Ref<Texture> GetDepthStencilAttachment() const override { return _depthStencilAttachment; }

        const Ref<GraphicsRenderPassLayout>& GetRenderPassLayout() const override;

        inline uint32_t GetWidth() const override { return _extent.width; }
        inline uint32_t GetHeight() const override { return _extent.height; }

        inline vk::Framebuffer &GetNativeVkFramebuffer() { return _framebuffer; }

    private:
        bool CreateRenderPass();
        bool CreateFramebuffer();

    private:
        VkContext &_context;

        vk::Framebuffer _framebuffer;
        vk::Extent2D _extent;

        std::vector<Ref<Texture>> _colorAttachments;
        Ref<Texture> _depthStencilAttachment;

        Ref<VkRenderPassLayout> _renderPassLayout;
        Ref<VkRenderPass> _renderpass;

        std::function<Ref<Texture>(uint32_t, uint32_t, uint32_t)> _colorResizeHandler;
        std::function<Ref<Texture>(uint32_t, uint32_t)> _depthStencilResizeHandler;
    };
}

#endif
