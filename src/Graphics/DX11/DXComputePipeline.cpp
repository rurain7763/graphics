#include "pch.h"
#include "DXPipelines.h"

#ifdef SUPPORT_DX11

#include "DXContext.h"
#include "DXShaders.h"
#include "Log/Log.h"

namespace flaw {
	DXComputePipeline::DXComputePipeline(DXContext& context)
		: _context(context)
	{
	}

	void DXComputePipeline::SetShader(const Ref<ComputeShader>& shader) {
		auto dxShader = std::static_pointer_cast<DXComputeShader>(shader);
		FASSERT(dxShader, "Shader is not a DXComputeShader");

		_shader = dxShader;
	}

	void DXComputePipeline::AddShaderResourcesLayout(const Ref<ShaderResourcesLayout>& shaderResourceLayout) {
	}
}

#endif