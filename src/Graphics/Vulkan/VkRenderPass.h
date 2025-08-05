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
        virtual const DepthStencilAttachment& GetDepthStencilAttachment() const override;

        virtual bool HasResolveAttachment() const override;
        virtual const ResolveAttachment& GetResolveAttachment() const override;

        inline vk::PipelineBindPoint GetVkPipelineBindPoint() const { return _pipelineBindPoint; }
        inline std::vector<vk::AttachmentDescription> GetVkAttachments() { return _vkAttachments; }
        inline std::vector<vk::AttachmentReference> GetVkColorAttachmentRefs() { return _colorAttachmentRefs; }
        inline vk::AttachmentReference GetVkDepthAttachmentRef() { return _depthAttachmentRef.value(); }
        inline vk::AttachmentReference GetVkResolveAttachmentRef() { return _resolveAttachmentRef.value(); }

    private:
        VkContext& _context;

        vk::PipelineBindPoint _pipelineBindPoint;
        std::vector<ColorAttachment> _colorAttachments;
        std::optional<DepthStencilAttachment> _depthStencilAttachment;
        std::optional<ResolveAttachment> _resolveAttachment;

        std::vector<vk::AttachmentDescription> _vkAttachments;

        std::vector<vk::AttachmentReference> _colorAttachmentRefs;

        std::optional<vk::AttachmentReference> _depthAttachmentRef;

        std::optional<vk::AttachmentReference> _resolveAttachmentRef;
    };
    
    class VkRenderPass : public GraphicsRenderPass {
    public:
        VkRenderPass(VkContext& context, const Descriptor& descriptor);
        ~VkRenderPass();

        virtual uint32_t GetColorAttachmentOpCount() const override;
        virtual const ColorAttachmentOperation& GetColorAttachmentOp(uint32_t index) const override;

        virtual bool HasDepthStencilAttachmentOp() const override;
        virtual const DepthStencilAttachmentOperation& GetDepthStencilAttachmentOp() const override;

        virtual bool HasResolveAttachmentOp() const override;
        virtual const ResolveAttachmentOperation& GetResolveAttachmentOp() const override;

        inline vk::RenderPass& GetNativeVkRenderPass() { return _renderPass; }

    private:
        bool CreateRenderPass(const Descriptor& descriptor);

    private:
        VkContext& _context;

        vk::RenderPass _renderPass;
        std::vector<ColorAttachmentOperation> _colorOperations;
        std::optional<DepthStencilAttachmentOperation> _depthStencilOperation;
        std::optional<ResolveAttachmentOperation> _resolveAttachmentOperation;
    };
}

#endif