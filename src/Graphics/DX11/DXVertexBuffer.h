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

	class DXVertexBuffer : public VertexBuffer {
	public:
		DXVertexBuffer(DXContext& context, const Descriptor& descriptor);
		~DXVertexBuffer() override = default;

		virtual void Update(const void* data, uint32_t elmSize, uint32_t count) override;
		virtual void Bind() override;

		virtual uint32_t Size() const override { return _size; }

	private:
		void CreateBuffer(const Descriptor& desc);

	private:
		DXContext& _context;

		ComPtr<ID3D11Buffer> _buffer;

		UsageFlag _usage;
		uint32_t _elmSize;
		uint32_t _size;
	};
}