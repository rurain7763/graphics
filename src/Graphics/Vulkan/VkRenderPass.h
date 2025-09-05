#pragma once

#include "Core.h"

#ifdef SUPPORT_VULKAN

#include "VkCore.h"
#include "Graphics/GraphicsRenderPass.h"

namespace flaw {
    class VkContext;

    class VkRenderPass : public RenderPass {
	public:
		VkRenderPass(VkContext& context, const Descriptor& descriptor);
		~VkRenderPass();

		const Attachment& GetAttachment(uint32_t index) const override { return _attachments.at(index); }

        uint32_t GetInputAttachmentRefCount(uint32_t subpass) const override {
			return _subpasses.at(subpass).inputAttachmentRefs.size();
        }

        const AttachmentRef& GetInputAttachmentRef(uint32_t subpass, uint32_t index) const override {
			return _subpasses.at(subpass).inputAttachmentRefs.at(index);
        }

        uint32_t GetColorAttachmentRefsCount(uint32_t subpass) const override {
			return _subpasses.at(subpass).colorAttachmentRefs.size();
        }

        const AttachmentRef& GetColorAttachmentRef(uint32_t subpass, uint32_t index) const override {
			return _subpasses.at(subpass).colorAttachmentRefs.at(index);
        }

        bool HasDepthStencilAttachmentRef(uint32_t subpass) const override {
			return _subpasses.at(subpass).depthStencilAttachmentRef.has_value();
        }

        const AttachmentRef& GetDepthStencilAttachmentRef(uint32_t subpass) const override {
			return _subpasses.at(subpass).depthStencilAttachmentRef.value();
        }

        uint32_t GetResolveAttachmentRefCount(uint32_t subpass) const override {
			return _subpasses.at(subpass).resolveAttachmentRefs.size();
        }

        const AttachmentRef& GetResolveAttachmentRef(uint32_t subpass, uint32_t index) const override {
			return _subpasses.at(subpass).resolveAttachmentRefs.at(index);
        }

		inline vk::RenderPass GetNativeVkRenderPass() const { return _renderPass; }
		inline const std::vector<vk::ClearValue>& GetClearValues() const { return _clearValues; }

    private:
        VkContext& _context;

        vk::RenderPass _renderPass;

        std::vector<Attachment> _attachments;
		std::vector<vk::ClearValue> _clearValues;
        std::vector<SubPass> _subpasses;
		std::vector<SubPassDependency> _dependencies;
    };
}

#endif