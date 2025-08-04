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

	class DXIndexBuffer : public IndexBuffer {
	public:
		DXIndexBuffer(DXContext& context, const Descriptor& descriptor);
		~DXIndexBuffer() override = default;

		void Update(const uint32_t* indices, uint32_t count) override;
		void Bind() override;

		uint32_t IndexCount() const override { return _indexCount; }

	private:
		void CreateBuffer(const Descriptor& desc);

	private:
		DXContext& _context;
		ComPtr<ID3D11Buffer> _buffer;

		UsageFlag _usage;
		uint32_t _size;
		uint32_t _indexCount;
	};
}
