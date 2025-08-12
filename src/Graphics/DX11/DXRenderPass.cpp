#include "pch.h"
#include "DXRenderPass.h"

#ifdef SUPPORT_DX11

#include "DXContext.h"
#include "DXTextures.h"
#include "Log/Log.h"

namespace flaw {
	DXRenderPassLayout::DXRenderPassLayout(DXContext& context, const Descriptor& desc)
		: _context(context)
		, _sampleCount(desc.sampleCount)
		, _alphaToCoverage(desc.alphaToCoverage)
		, _colorAttachments(desc.colorAttachments)
		, _depthStencilAttachment(desc.depthStencilAttachment)
		, _resolveAttachment(desc.resolveAttachment)
	{
		if (!CreateBlendState()) {
			return;
		}
	}

	bool DXRenderPassLayout::CreateBlendState() {
		D3D11_BLEND_DESC blendDesc = {};
		blendDesc.IndependentBlendEnable = TRUE;
		blendDesc.AlphaToCoverageEnable = _alphaToCoverage;

		for (int32_t i = 0; i < _colorAttachments.size(); ++i) {
			auto& renderTarget = _colorAttachments[i];
			auto& renderTargetDesc = blendDesc.RenderTarget[i];

			if (renderTarget.blendMode == BlendMode::Disabled) {
				renderTargetDesc.BlendEnable = FALSE;
				renderTargetDesc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
				continue;
			}

			renderTargetDesc.BlendEnable = TRUE;
			ConvertDXBlend(renderTarget.blendMode, renderTargetDesc.SrcBlend, renderTargetDesc.DestBlend);
			renderTargetDesc.BlendOp = D3D11_BLEND_OP_ADD;
			renderTargetDesc.SrcBlendAlpha = D3D11_BLEND_ONE;
			renderTargetDesc.DestBlendAlpha = D3D11_BLEND_ZERO;
			renderTargetDesc.BlendOpAlpha = D3D11_BLEND_OP_ADD;
			renderTargetDesc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		}

		if (FAILED(_context.Device()->CreateBlendState(&blendDesc, &_blendState))) {
			LOG_ERROR("Failed to create blend state.");
			return false;
		}

		return true;
	}

	uint32_t DXRenderPassLayout::GetColorAttachmentCount() const {
		return _colorAttachments.size();
	}

	const GraphicsRenderPassLayout::ColorAttachment& DXRenderPassLayout::GetColorAttachment(uint32_t index) const {
		return _colorAttachments[index];
	}

	bool DXRenderPassLayout::HasDepthStencilAttachment() const {
		return _depthStencilAttachment.has_value();
	}

	const GraphicsRenderPassLayout::DepthStencilAttachment& DXRenderPassLayout::GetDepthStencilAttachment() const {
		return _depthStencilAttachment.value();
	}

	bool DXRenderPassLayout::HasResolveAttachment() const {
		return _resolveAttachment.has_value();
	}

	const GraphicsRenderPassLayout::ResolveAttachment& DXRenderPassLayout::GetResolveAttachment() const {
		return _resolveAttachment.value();
	}

	uint32_t DXRenderPassLayout::GetSampleCount() const {
		return _sampleCount;
	}

	DXRenderPass::DXRenderPass(DXContext& context, const Descriptor& desc)
		: _context(context)
		, _colorAttachmentOps(desc.colorAttachmentOps)
		, _depthStencilAttachmentOp(desc.depthStencilAttachmentOp)
		, _resolveAttachmentOp(desc.resolveAttachmentOp)
	{
	}

	uint32_t DXRenderPass::GetColorAttachmentOpCount() const {
		return _colorAttachmentOps.size();
	}

	const GraphicsRenderPass::ColorAttachmentOperation& DXRenderPass::GetColorAttachmentOp(uint32_t index) const {
		return _colorAttachmentOps[index];
	}

	bool DXRenderPass::HasDepthStencilAttachmentOp() const {
		return _depthStencilAttachmentOp.has_value();
	}

	const GraphicsRenderPass::DepthStencilAttachmentOperation& DXRenderPass::GetDepthStencilAttachmentOp() const {
		return _depthStencilAttachmentOp.value();
	}

	bool DXRenderPass::HasResolveAttachmentOp() const {
		return _resolveAttachmentOp.has_value();
	}

	const GraphicsRenderPass::ResolveAttachmentOperation& DXRenderPass::GetResolveAttachmentOp() const {
		return _resolveAttachmentOp.value();
	}
}

#endif