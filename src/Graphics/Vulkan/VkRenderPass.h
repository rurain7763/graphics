#pragma once

#include "Core.h"

#ifdef SUPPORT_VULKAN

#include "VkCore.h"
#include "Graphics/GraphicsRenderPass.h"

namespace flaw {
    class VkContext;

    class VkRenderPassLayout : public RenderPassLayout {
    public:
        VkRenderPassLayout(VkContext& context, const Descriptor& descriptor);
        ~VkRenderPassLayout() = default;

        virtual uint32_t GetColorAttachmentCount() const override;

        virtual bool HasDepthStencilAttachment() const override;

        virtual bool HasResolveAttachment() const override;

        virtual uint32_t GetSampleCount() const override;

        inline vk::PipelineBindPoint GetVkPipelineBindPoint() const { return _pipelineBindPoint; }
        inline std::vector<vk::AttachmentDescription> GetVkAttachments() { return _vkAttachments; }
        inline std::vector<vk::AttachmentReference> GetVkColorAttachmentRefs() { return _colorAttachmentRefs; }
        inline vk::AttachmentReference GetVkDepthAttachmentRef() { return _depthAttachmentRef.value(); }
        inline vk::AttachmentReference GetVkResolveAttachmentRef() { return _resolveAttachmentRef.value(); }

    private:
        VkContext& _context;

        vk::PipelineBindPoint _pipelineBindPoint;

        uint32_t _sampleCount;

        std::vector<vk::AttachmentDescription> _vkAttachments;
        std::vector<vk::AttachmentReference> _colorAttachmentRefs;
        std::optional<vk::AttachmentReference> _depthAttachmentRef;
        std::optional<vk::AttachmentReference> _resolveAttachmentRef;
    };
    
    class VkRenderPass : public RenderPass {
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
        std::vector<ColorAttachmentOperation> _colorAttachmentOp;
        std::optional<DepthStencilAttachmentOperation> _depthStencilOp;
        std::optional<ResolveAttachmentOperation> _resolveAttachmentOp;
    };
}

#endif