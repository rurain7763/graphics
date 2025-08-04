#include "pch.h"
#include "DXComputePipeline.h"
#include "DXContext.h"

namespace flaw {
	void DXComputePipeline::Bind() {
		_shader->Bind();
	}

	void DXComputePipeline::Dispatch(uint32_t x, uint32_t y, uint32_t z) {
		_shader->Dispatch(x, y, z);
	}
}