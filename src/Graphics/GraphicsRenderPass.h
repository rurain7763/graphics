#pragma once

#include "Core.h"
#include "Graphics/GraphicsType.h"

namespace flaw {
	class GraphicsRenderPass;

	class GraphicsRenderPassLayout {
	public:
		struct ColorAttachment {
			PixelFormat format = PixelFormat::UNDEFINED;
			BlendMode blendMode = BlendMode::Default;
		};

		struct DepthStencilAttachment {
			PixelFormat format = PixelFormat::UNDEFINED;
		};

		struct ResolveAttachment {
			PixelFormat format = PixelFormat::UNDEFINED;
		};

		struct Descriptor {
			PipelineType type;
			uint32_t sampleCount = 1;
			bool alphaToCoverage = false;
			std::vector<ColorAttachment> colorAttachments;
			std::optional<DepthStencilAttachment> depthStencilAttachment;
			std::optional<ResolveAttachment> resolveAttachment;
		};

		virtual ~GraphicsRenderPassLayout() = default;

		virtual uint32_t GetColorAttachmentCount() const = 0;
		virtual const ColorAttachment& GetColorAttachment(uint32_t index) const = 0;

		virtual bool HasDepthStencilAttachment() const = 0;
		virtual const DepthStencilAttachment& GetDepthStencilAttachment() const = 0;

		virtual bool HasResolveAttachment() const = 0;
		virtual const ResolveAttachment& GetResolveAttachment() const = 0;

		virtual uint32_t GetSampleCount() const = 0;
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

		struct ResolveAttachmentOperation {
			TextureLayout initialLayout;
			TextureLayout finalLayout;
			AttachmentLoadOp loadOp;
			AttachmentStoreOp storeOp;
		};

		struct Descriptor {
			Ref<GraphicsRenderPassLayout> layout;

			std::vector<ColorAttachmentOperation> colorAttachmentOps;
			std::optional<DepthStencilAttachmentOperation> depthStencilAttachmentOp;
			std::optional<ResolveAttachmentOperation> resolveAttachmentOp;
		};

		virtual ~GraphicsRenderPass() = default;

		virtual uint32_t GetColorAttachmentOpCount() const = 0;
		virtual const ColorAttachmentOperation& GetColorAttachmentOp(uint32_t index) const = 0;

		virtual bool HasDepthStencilAttachmentOp() const = 0;
		virtual const DepthStencilAttachmentOperation& GetDepthStencilAttachmentOp() const = 0;

		virtual bool HasResolveAttachmentOp() const = 0;
		virtual const ResolveAttachmentOperation& GetResolveAttachmentOp() const = 0;
	};
}
