#pragma once

#include "Core.h"

#ifdef SUPPORT_DX11

#include "DXCore.h"
#include "Graphics/GraphicsRenderPass.h"

namespace flaw {
	class DXContext;
	class DXTexture2D;

	class DXRenderPassLayout : public RenderPassLayout {
	public:
		DXRenderPassLayout(DXContext& context, const Descriptor& desc);
		~DXRenderPassLayout() = default;

		uint32_t GetColorAttachmentCount() const override;
		Attachment GetColorAttachment(uint32_t index) const override;
		bool HasDepthStencilAttachment() const override;
		Attachment GetDepthStencilAttachment() const override;
		bool HasResolveAttachment() const override;

		uint32_t GetSampleCount() const override;

	private:
		DXContext& _context;

		uint32_t _sampleCount;
		
		std::vector<Attachment> _colorAttachments;

		std::optional<Attachment> _depthStencilAttachment;
		std::optional<Attachment> _resolveAttachment;
	};

	class DXRenderPass : public RenderPass {
	public:
		DXRenderPass(DXContext& context, const Descriptor& desc);
		~DXRenderPass() = default;

		uint32_t GetColorAttachmentOpCount() const override;
		const ColorAttachmentOperation& GetColorAttachmentOp(uint32_t index) const override;

		bool HasDepthStencilAttachmentOp() const override;
		const DepthStencilAttachmentOperation& GetDepthStencilAttachmentOp() const override;

		bool HasResolveAttachmentOp() const override;
		const ResolveAttachmentOperation& GetResolveAttachmentOp() const override;

	private:
		DXContext& _context;

		std::vector<ColorAttachmentOperation> _colorAttachmentOps;
		std::optional<DepthStencilAttachmentOperation> _depthStencilAttachmentOp;
		std::optional<ResolveAttachmentOperation> _resolveAttachmentOp;
	};
}

#endif