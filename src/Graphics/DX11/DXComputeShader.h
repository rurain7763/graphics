#pragma once

#include "Graphics/ComputeShader.h"

#include <string>

#include <Windows.h>
#include <wrl.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>

using namespace Microsoft::WRL;
using namespace DirectX;
using namespace DirectX::PackedVector;

namespace flaw {
	class DXContext;

	class DXComputeShader : public ComputeShader {
	public:
		DXComputeShader(DXContext& context, const char* filepath);
		~DXComputeShader() override = default;

		void Bind() override;
		void Dispatch(uint32_t x = 1, uint32_t y = 1, uint32_t z = 1) override;

	private:
		bool CreateComputeShader(const char* filepath);

	private:
		DXContext& _context;

		ComPtr<ID3DBlob> _csBlob;
		ComPtr<ID3D11ComputeShader> _computeShader;
	};
}