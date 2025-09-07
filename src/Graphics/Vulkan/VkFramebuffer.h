#pragma once

#include "Core.h"

#ifdef SUPPORT_VULKAN

#include "VkCore.h"
#include "Graphics/GraphicsFramebuffer.h"

namespace flaw {
    class VkContext;
    class VkRenderPassLayout;
    class VkRenderPass;

    class VkFramebuffer : public Framebuffer {
    public:
        VkFramebuffer(VkContext &context, const Descriptor &descriptor);
        ~VkFramebuffer();

        void Resize(uint32_t width, uint32_t height) override;

		inline uint32_t GetAttachmentCount() const override { return _attachments.size(); }
		inline Ref<Texture> GetAttachment(uint32_t index) const override { return _attachments.at(index); }
        
        inline uint32_t GetWidth() const override { return _width; }
        inline uint32_t GetHeight() const override { return _height; }
		inline uint32_t GetLayers() const override { return _layers; }

        inline vk::Framebuffer& GetNativeVkFramebuffer() { return _framebuffer; }

    private:
        bool CreateFramebuffer();

    private:
        VkContext &_context;

        vk::Framebuffer _framebuffer;
        uint32_t _width;
		uint32_t _height;
        uint32_t _layers;

        Ref<VkRenderPass> _renderPass;

		std::vector<Ref<Texture>> _attachments;
		std::function<void(uint32_t, uint32_t, std::vector<Ref<Texture>>&)> _resizeHandler;
    };
}

#endif
