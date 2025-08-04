#pragma once

#include "Core.h"

#ifdef SUPPORT_VULKAN

#include "VkCore.h"
#include "Graphics/GraphicsRenderPass.h"

namespace flaw {
    class VkContext;

    class VkRenderPassLayout : public GraphicsRenderPassLayout {
    public:
        VkRenderPassLayout(VkContext& context, const Descriptor& descriptor);
        ~VkRenderPassLayout() = default;

        virtual uint32_t GetColorAttachmentCount() const override;
        virtual const ColorAttachment& GetColorAttachment(uint32_t index) const override;

        virtual bool HasDepthStencilAttachment() const override;

        inline vk::PipelineBindPoint GetVkPipelineBindPoint() const { return _pipelineBindPoint; }
        inline std::vector<vk::AttachmentDescription> GetVkAttachments() { return _vkColorAttachments; }
        inline std::vector<vk::AttachmentReference> GetVkColorAttachmentRefs() { return _colorAttachmentRefs; }
        inline std::optional<vk::AttachmentReference> GetVkDepthAttachmentRef() { return _depthAttachmentRef; }

    private:
        VkContext& _context;

        vk::PipelineBindPoint _pipelineBindPoint;
        std::vector<ColorAttachment> _colorAttachments;
        std::vector<vk::AttachmentDescription> _vkColorAttachments;
        std::vector<vk::AttachmentReference> _colorAttachmentRefs;

        std::optional<vk::AttachmentReference> _depthAttachmentRef;
    };
    
    class VkRenderPass : public GraphicsRenderPass {
    public:
        VkRenderPass(VkContext& context, const Descriptor& descriptor);
        ~VkRenderPass();

        virtual uint32_t GetColorAttachmentCount() const override;
        virtual AttachmentLoadOp GetColorAttachmentLoadOp(uint32_t index) const override;

        virtual bool HasDepthStencilAttachment() const override;
        virtual AttachmentLoadOp GetDepthStencilAttachmentLoadOp() const override;

        inline vk::RenderPass& GetNativeVkRenderPass() { return _renderPass; }

    private:
        bool CreateRenderPass(const Descriptor& descriptor);

    private:
        VkContext& _context;

        vk::RenderPass _renderPass;
        std::vector<ColorAttachmentOperation> _colorOperations;
        std::optional<DepthStencilAttachmentOperation> _depthStencilOperation;
    };
}

#endif