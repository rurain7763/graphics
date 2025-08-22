#pragma once

#include "Core.h"
#include "Graphics/GraphicsType.h"

namespace flaw {
	class RenderPass;

	class RenderPassLayout {
	public:
		struct Attachment {
			PixelFormat format = PixelFormat::UNDEFINED;
		};

		struct Descriptor {
			PipelineType type;
			uint32_t sampleCount = 1;
			std::vector<Attachment> colorAttachments;
			std::optional<Attachment> depthStencilAttachment;
			std::optional<Attachment> resolveAttachment;
		};

		virtual ~RenderPassLayout() = default;

		virtual uint32_t GetColorAttachmentCount() const = 0;
		virtual bool HasDepthStencilAttachment() const = 0;
		virtual bool HasResolveAttachment() const = 0;

		virtual uint32_t GetSampleCount() const = 0;
	};

	class RenderPass {
	public:
		struct ColorAttachmentOperation {
			TextureLayout initialLayout;
			TextureLayout finalLayout;
			AttachmentLoadOp loadOp;
			AttachmentStoreOp storeOp;
		};

		struct DepthStencilAttachmentOperation {
			TextureLayout initialLayout;
			TextureLayout finalLayout;
			AttachmentLoadOp loadOp;
			AttachmentStoreOp storeOp;
			AttachmentLoadOp stencilLoadOp;
			AttachmentStoreOp stencilStoreOp;
		};

		struct ResolveAttachmentOperation {
			TextureLayout initialLayout;
			TextureLayout finalLayout;
			AttachmentLoadOp loadOp;
			AttachmentStoreOp storeOp;
		};

		struct Descriptor {
			Ref<RenderPassLayout> layout;

			std::vector<ColorAttachmentOperation> colorAttachmentOps;
			std::optional<DepthStencilAttachmentOperation> depthStencilAttachmentOp;
			std::optional<ResolveAttachmentOperation> resolveAttachmentOp;
		};

		virtual ~RenderPass() = default;

		virtual uint32_t GetColorAttachmentOpCount() const = 0;
		virtual const ColorAttachmentOperation& GetColorAttachmentOp(uint32_t index) const = 0;

		virtual bool HasDepthStencilAttachmentOp() const = 0;
		virtual const DepthStencilAttachmentOperation& GetDepthStencilAttachmentOp() const = 0;

		virtual bool HasResolveAttachmentOp() const = 0;
		virtual const ResolveAttachmentOperation& GetResolveAttachmentOp() const = 0;
	};
}
