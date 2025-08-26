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
		, _colorAttachments(desc.colorAttachments)
		, _depthStencilAttachment(desc.depthStencilAttachment)
		, _resolveAttachment(desc.resolveAttachment)
	{
	}

	uint32_t DXRenderPassLayout::GetColorAttachmentCount() const {
		return _colorAttachments.size();
	}

	DXRenderPassLayout::Attachment DXRenderPassLayout::GetColorAttachment(uint32_t index) const {
		FASSERT(index < _colorAttachments.size(), "Color attachment index out of bounds");
		return _colorAttachments[index];
	}

	bool DXRenderPassLayout::HasDepthStencilAttachment() const {
		return _depthStencilAttachment.has_value();
	}

	DXRenderPassLayout::Attachment DXRenderPassLayout::GetDepthStencilAttachment() const {
		FASSERT(_depthStencilAttachment.has_value(), "No depth-stencil attachment in this render pass layout");
		return _depthStencilAttachment.value();
	}

	bool DXRenderPassLayout::HasResolveAttachment() const {
		return _resolveAttachment.has_value();
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

	const RenderPass::ColorAttachmentOperation& DXRenderPass::GetColorAttachmentOp(uint32_t index) const {
		return _colorAttachmentOps[index];
	}

	bool DXRenderPass::HasDepthStencilAttachmentOp() const {
		return _depthStencilAttachmentOp.has_value();
	}

	const RenderPass::DepthStencilAttachmentOperation& DXRenderPass::GetDepthStencilAttachmentOp() const {
		return _depthStencilAttachmentOp.value();
	}

	bool DXRenderPass::HasResolveAttachmentOp() const {
		return _resolveAttachmentOp.has_value();
	}

	const RenderPass::ResolveAttachmentOperation& DXRenderPass::GetResolveAttachmentOp() const {
		return _resolveAttachmentOp.value();
	}
}

#endif