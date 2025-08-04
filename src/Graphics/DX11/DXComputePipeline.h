#pragma once

#include "Graphics/ComputePipeline.h"

namespace flaw {
	class DXContext;

	class DXComputePipeline : public ComputePipeline {
	public:
		DXComputePipeline(DXContext& context) : _context(context) {}
		~DXComputePipeline() override = default;

		void Bind() override;
		void Dispatch(uint32_t x = 1, uint32_t y = 1, uint32_t z = 1) override;

	private:
		DXContext& _context;
	};
}