#include "pch.h"
#include "DXRenderPass.h"

#ifdef SUPPORT_DX11

#include "DXContext.h"
#include "DXTextures.h"

namespace flaw {
	DXRenderPass::DXRenderPass(DXContext& context, const Descriptor& desc)
		: _context(context)
		, _attachments(desc.attachments)
		, _subpasses(desc.subpasses)
		, _dependencies(desc.dependencies)
	{
	}
}

#endif