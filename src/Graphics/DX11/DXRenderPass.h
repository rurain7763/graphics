#pragma once

#include "Core.h"

#ifdef SUPPORT_DX11

#include "DXCore.h"
#include "Graphics/GraphicsRenderPass.h"

namespace flaw {
	class DXContext;

	class DXRenderPass : public RenderPass {
	public:
		DXRenderPass(DXContext& context, const Descriptor& desc);
		~DXRenderPass() = default;

		const Attachment& GetAttachment(uint32_t index) const override {
			return _attachments.at(index);
		}

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

	private:
		DXContext& _context;

		std::vector<Attachment> _attachments;
		std::vector<SubPass> _subpasses;
		std::vector<SubPassDependency> _dependencies;
	};
}

#endif