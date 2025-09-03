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
		, _resolveAttachments(desc.resolveAttachments)
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

	uint32_t DXRenderPassLayout::GetResolveAttachmentCount() const {
		return _resolveAttachments.size();
	}

	DXRenderPassLayout::Attachment DXRenderPassLayout::GetResolveAttachment(uint32_t index) const {
		FASSERT(index < _resolveAttachments.size(), "Resolve attachment index out of bounds");
		return _resolveAttachments[index];
	}

	uint32_t DXRenderPassLayout::GetSampleCount() const {
		return _sampleCount;
	}

	DXRenderPass::DXRenderPass(DXContext& context, const Descriptor& desc)
		: _context(context)
		, _layout(std::static_pointer_cast<DXRenderPassLayout>(desc.layout))
		, _colorAttachmentOps(desc.colorAttachmentOps)
		, _depthStencilAttachmentOp(desc.depthStencilAttachmentOp)
		, _resolveAttachmentOps(desc.resolveAttachmentOps)
	{
		FASSERT(_layout, "Render pass layout is not a DXRenderPassLayout or null");
	}

	const RenderPass::ColorAttachmentOperation& DXRenderPass::GetColorAttachmentOp(uint32_t index) const {
		return _colorAttachmentOps[index];
	}

	const RenderPass::DepthStencilAttachmentOperation& DXRenderPass::GetDepthStencilAttachmentOp() const {
		return _depthStencilAttachmentOp.value();
	}

	const RenderPass::ResolveAttachmentOperation& DXRenderPass::GetResolveAttachmentOp(uint32_t index) const {
		return _resolveAttachmentOps[index];
	}
}

#endif