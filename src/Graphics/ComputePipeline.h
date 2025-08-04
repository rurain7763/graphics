#pragma once

#include "Core.h"
#include "GraphicsBuffers.h"
#include "ComputeShader.h"
#include "GraphicsTextures.h"

#include <unordered_map>

namespace flaw {
	class ComputePipeline {
	public:
		ComputePipeline() = default;
		virtual ~ComputePipeline() = default;

		virtual void Bind() = 0;
		virtual void Dispatch(uint32_t x = 1, uint32_t y = 1, uint32_t z = 1) = 0;

		void SetShader(Ref<ComputeShader> shader) {
			_shader = shader;
		}

	protected:
		Ref<ComputeShader> _shader;
	};
}