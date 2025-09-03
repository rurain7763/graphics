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
		uint32_t GetResolveAttachmentCount() const override;
		Attachment GetResolveAttachment(uint32_t index) const override;

		uint32_t GetSampleCount() const override;

	private:
		DXContext& _context;

		uint32_t _sampleCount;
		
		std::vector<Attachment> _colorAttachments;
		std::optional<Attachment> _depthStencilAttachment;
		std::vector<Attachment> _resolveAttachments;
	};

	class DXRenderPass : public RenderPass {
	public:
		DXRenderPass(DXContext& context, const Descriptor& desc);
		~DXRenderPass() = default;

		const ColorAttachmentOperation& GetColorAttachmentOp(uint32_t index) const override;
		const DepthStencilAttachmentOperation& GetDepthStencilAttachmentOp() const override;
		const ResolveAttachmentOperation& GetResolveAttachmentOp(uint32_t index) const override;

		Ref<RenderPassLayout> GetLayout() const override { return _layout; }

	private:
		DXContext& _context;

		Ref<DXRenderPassLayout> _layout;

		std::vector<ColorAttachmentOperation> _colorAttachmentOps;
		std::optional<DepthStencilAttachmentOperation> _depthStencilAttachmentOp;
		std::vector<ResolveAttachmentOperation> _resolveAttachmentOps;
	};
}

#endif