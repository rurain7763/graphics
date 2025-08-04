#pragma once

#include "Core.h"

namespace flaw {
	class ComputeShader {
	public:
		ComputeShader() = default;
		virtual ~ComputeShader() = default;

		virtual void Bind() = 0;
		virtual void Dispatch(uint32_t x = 1, uint32_t y = 1, uint32_t z = 1) = 0;
	};
}