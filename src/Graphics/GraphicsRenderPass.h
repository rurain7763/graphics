#pragma once

#include "Core.h"
#include "Graphics/GraphicsType.h"

namespace flaw {
	class GraphicsRenderPass;

	class GraphicsRenderPassLayout {
	public:
		struct ColorAttachment {
			PixelFormat format;
			BlendMode blendMode = BlendMode::Default;
			bool alphaToCoverage = false;
		};

		struct DepthStencilAttachment {
			PixelFormat format;
		};

		struct Descriptor {
			PipelineType type;
			std::vector<ColorAttachment> colorAttachments;
			std::optional<DepthStencilAttachment> depthStencilAttachment;
		};

		virtual ~GraphicsRenderPassLayout() = default;

		virtual uint32_t GetColorAttachmentCount() const = 0;
		virtual const ColorAttachment& GetColorAttachment(uint32_t index) const = 0;

		virtual bool HasDepthStencilAttachment() const = 0;
	};

	class GraphicsRenderPass {
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

		struct Descriptor {
			Ref<GraphicsRenderPassLayout> layout;

			std::vector<ColorAttachmentOperation> colorAttachmentOperations;
			std::optional<DepthStencilAttachmentOperation> depthStencilAttachmentOperation;
		};

		virtual ~GraphicsRenderPass() = default;

		virtual uint32_t GetColorAttachmentCount() const = 0;
		virtual AttachmentLoadOp GetColorAttachmentLoadOp(uint32_t index) const = 0;

		virtual bool HasDepthStencilAttachment() const = 0;
		virtual AttachmentLoadOp GetDepthStencilAttachmentLoadOp() const = 0;
	};
}
