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
		virtual Attachment GetColorAttachment(uint32_t index) const override;

        virtual bool HasDepthStencilAttachment() const override;
		virtual Attachment GetDepthStencilAttachment() const override;

        virtual uint32_t GetResolveAttachmentCount() const override;
        virtual Attachment GetResolveAttachment(uint32_t index) const override;

        virtual uint32_t GetSampleCount() const override;

        inline vk::PipelineBindPoint GetVkPipelineBindPoint() const { return _pipelineBindPoint; }
        inline std::vector<vk::AttachmentDescription> GetVkAttachments() { return _vkAttachments; }
        inline std::vector<vk::AttachmentReference> GetVkColorAttachmentRefs() { return _colorAttachmentRefs; }
        inline vk::AttachmentReference GetVkDepthAttachmentRef() { return _depthAttachmentRef.value(); }
        inline std::vector<vk::AttachmentReference> GetVkResolveAttachmentRefs() { return _resolveAttachmentRefs; }

    private:
        VkContext& _context;

        vk::PipelineBindPoint _pipelineBindPoint;

        uint32_t _sampleCount;

        std::vector<vk::AttachmentDescription> _vkAttachments;
        std::vector<vk::AttachmentReference> _colorAttachmentRefs;
        std::optional<vk::AttachmentReference> _depthAttachmentRef;
        std::vector<vk::AttachmentReference> _resolveAttachmentRefs;
    };
    
    class VkRenderPass : public RenderPass {
    public:
        VkRenderPass(VkContext& context, const Descriptor& descriptor);
        ~VkRenderPass();

        virtual const ColorAttachmentOperation& GetColorAttachmentOp(uint32_t index) const override;
        virtual const DepthStencilAttachmentOperation& GetDepthStencilAttachmentOp() const override;
        virtual const ResolveAttachmentOperation& GetResolveAttachmentOp(uint32_t index) const override;

		virtual Ref<RenderPassLayout> GetLayout() const override { return _layout; }

        inline vk::RenderPass& GetNativeVkRenderPass() { return _renderPass; }

    private:
        bool CreateRenderPass(const Descriptor& descriptor);

    private:
        VkContext& _context;

		Ref<VkRenderPassLayout> _layout;

        vk::RenderPass _renderPass;
        std::vector<ColorAttachmentOperation> _colorAttachmentOps;
        std::optional<DepthStencilAttachmentOperation> _depthStencilOp;
        std::vector<ResolveAttachmentOperation> _resolveAttachmentOps;
    };
}

#endif