#pragma once

#include "Core.h"
#include "Graphics/GraphicsType.h"

namespace flaw {
	class RenderPass {
	public:
		struct Attachment {
			PixelFormat format = PixelFormat::Undefined;
			uint32_t sampleCount = 1;
			TextureLayout initialLayout = TextureLayout::Undefined;
			TextureLayout finalLayout = TextureLayout::Undefined;
			AttachmentLoadOp loadOp = AttachmentLoadOp::DontCare;
			AttachmentStoreOp storeOp = AttachmentStoreOp::DontCare;
			AttachmentLoadOp stencilLoadOp = AttachmentLoadOp::DontCare;
			AttachmentStoreOp stencilStoreOp = AttachmentStoreOp::DontCare;
		};

		struct AttachmentRef {
			uint32_t attachmentIndex = 0;
			TextureLayout layout = TextureLayout::Undefined;
		};

		struct SubPass {
			std::vector<AttachmentRef> inputAttachmentRefs;
			std::vector<AttachmentRef> colorAttachmentRefs;
			std::vector<AttachmentRef> resolveAttachmentRefs;
			std::optional<AttachmentRef> depthStencilAttachmentRef;
		};

		struct SubPassDependency {
			uint32_t srcSubPass;
			uint32_t dstSubPass;

			AccessTypes srcAccessTypes;
			AccessTypes dstAccessTypes;
			PipelineStages srcPipeStages;
			PipelineStages dstPipeStages;
		};

		struct Descriptor {
			std::vector<Attachment> attachments;
			std::vector<SubPass> subpasses;
			std::vector<SubPassDependency> dependencies;
		};

		virtual ~RenderPass() = default;

		virtual const Attachment& GetAttachment(uint32_t index) const = 0;

		virtual uint32_t GetInputAttachmentRefCount(uint32_t subpass) const = 0;
		virtual const AttachmentRef& GetInputAttachmentRef(uint32_t subpass, uint32_t index) const = 0;

		virtual uint32_t GetColorAttachmentRefsCount(uint32_t subpass) const = 0;
		virtual const AttachmentRef& GetColorAttachmentRef(uint32_t subpass, uint32_t index) const = 0;

		virtual bool HasDepthStencilAttachmentRef(uint32_t subpass) const = 0;
		virtual const AttachmentRef& GetDepthStencilAttachmentRef(uint32_t subpass) const = 0;

		virtual uint32_t GetResolveAttachmentRefCount(uint32_t subpass) const = 0;
		virtual const AttachmentRef& GetResolveAttachmentRef(uint32_t subpass, uint32_t index) const = 0;
	};
}
