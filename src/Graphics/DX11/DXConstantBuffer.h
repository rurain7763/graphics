#pragma once

#include "Graphics/GraphicsBuffers.h"

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

	class DXConstantBuffer : public ConstantBuffer {
	public:
		DXConstantBuffer(DXContext& context, uint32_t size);

		~DXConstantBuffer() = default;

		void Update(const void* data, int32_t size) override;

		void BindToGraphicsShader(const uint32_t slot) override;
		void BindToComputeShader(const uint32_t slot) override;

		void Unbind() override;

		uint32_t Size() const override { return _size; }

	private:
		void Initialize(uint32_t size);

	private:
		DXContext& _context;

		ComPtr<ID3D11Buffer> _buffer;
		uint32_t _size;

		std::function<void()> _unbindFunc;
	};
}
