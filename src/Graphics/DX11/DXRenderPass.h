#pragma once

#include "Core.h"

#ifdef SUPPORT_DX11

#include "DXCore.h"
#include "Graphics/GraphicsRenderPass.h"

namespace flaw {
	class DXContext;
	class DXTexture2D;

	class DXRenderPassLayout : public GraphicsRenderPassLayout {
	public:
		DXRenderPassLayout(DXContext& context, const Descriptor& desc);
		~DXRenderPassLayout() = default;

		uint32_t GetColorAttachmentCount() const override;
		const ColorAttachment& GetColorAttachment(uint32_t index) const override;

		bool HasDepthStencilAttachment() const override;
		const DepthStencilAttachment& GetDepthStencilAttachment() const override;

		bool HasResolveAttachment() const override;
		const ResolveAttachment& GetResolveAttachment() const override;

		uint32_t GetSampleCount() const override;

		inline ComPtr<ID3D11BlendState> GetDXBlendState() const { return _blendState; }

	private:
		bool CreateBlendState();

	private:
		DXContext& _context;

		uint32_t _sampleCount;
		bool _alphaToCoverage;
		
		std::vector<ColorAttachment> _colorAttachments;
		ComPtr<ID3D11BlendState> _blendState;

		std::optional<DepthStencilAttachment> _depthStencilAttachment;
		std::optional<ResolveAttachment> _resolveAttachment;
	};

	class DXRenderPass : public GraphicsRenderPass {
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