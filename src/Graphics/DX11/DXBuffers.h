#pragma once

#include "Core.h"

#ifdef SUPPORT_DX11

#include "DXCore.h"
#include "Graphics/GraphicsBuffers.h"

namespace flaw {
	class DXContext;
	class GraphicsShader;

	class DXVertexInputLayout : public VertexInputLayout {
	public:
		DXVertexInputLayout(DXContext& context, const Descriptor& descriptor);
		~DXVertexInputLayout() override;

		ComPtr<ID3D11InputLayout> GetDXInputLayout(Ref<GraphicsShader> shader) const;

	private:
		DXContext& _context;

		std::vector<D3D11_INPUT_ELEMENT_DESC> _inputElements;
	};

	class DXVertexBuffer : public VertexBuffer {
	public:
		DXVertexBuffer(DXContext& context, const Descriptor& descriptor);
		~DXVertexBuffer() override = default;

		virtual void Update(const void* data, uint32_t size) override;

		virtual void CopyTo(Ref<VertexBuffer> dstBuffer, uint32_t srcOffset = 0, uint32_t dstOffset = 0) override;

		virtual uint32_t Size() const override { return _bufferByteSize; }

		inline ComPtr<ID3D11Buffer> GetNativeDXBuffer() const { return _buffer; }
		inline uint32_t ElementSize() const { return _elmSize; }

	private:
		void CreateBuffer(const void* data);

	private:
		DXContext& _context;

		ComPtr<ID3D11Buffer> _buffer;

		MemoryProperty _memProperty;
		uint32_t _elmSize;
		uint32_t _bufferByteSize;
	};

	class DXIndexBuffer : public IndexBuffer {
	public:
		DXIndexBuffer(DXContext& context, const Descriptor& descriptor);
		~DXIndexBuffer() override = default;

		void Update(const uint32_t* indices, uint32_t count) override;

		uint32_t IndexCount() const override { return _indexCount; }

		inline ComPtr<ID3D11Buffer> GetNativeDXBuffer() const { return _buffer; }

	private:
		void CreateBuffer(const uint* data);

	private:
		DXContext& _context;
		ComPtr<ID3D11Buffer> _buffer;

		MemoryProperty _memProperty;
		uint32_t _bufferByteSize;
		uint32_t _indexCount;
	};

	class DXConstantBuffer : public ConstantBuffer {
	public:
		DXConstantBuffer(DXContext& context, const Descriptor& descriptor);
		~DXConstantBuffer() = default;

		void Update(const void* data, int32_t size) override;

		uint32_t Size() const override { return _bufferByteSize; }

		inline ComPtr<ID3D11Buffer> GetNativeBuffer() const { return _buffer; }

	private:
		bool CreateBuffer(const void* data);

	private:
		DXContext& _context;

		ComPtr<ID3D11Buffer> _buffer;

		MemoryProperty _memProperty;
		uint32_t _bufferByteSize;
	};

	class DXStructuredBuffer : public StructuredBuffer {
	public:
		DXStructuredBuffer(DXContext& context, const Descriptor& desc);
		~DXStructuredBuffer() = default;

		void Update(const void* data, uint32_t size) override;
		void Fetch(void* data, uint32_t size) override;

		void CopyTo(Ref<StructuredBuffer> dstBuffer, uint32_t srcOffset = 0, uint32_t dstOffset = 0) override;
		void CopyFrom(Ref<StructuredBuffer> srcBuffer, uint32_t srcOffset = 0, uint32_t dstOffset = 0) override;

		inline ComPtr<ID3D11Buffer> GetNativeBuffer() const { return _buffer; }
		inline ComPtr<ID3D11ShaderResourceView> GetNativeSRV() const { return _srv; }
		inline ComPtr<ID3D11UnorderedAccessView> GetNativeUAV() const { return _uav; }
		inline uint32_t Size() const override { return _bufferByteSize; }

	private:
		bool CreateBuffer(const void* data);

		void CreateShaderResourceView();
		void CreateUnorderedAccessView();

	private:
		DXContext& _context;

		ComPtr<ID3D11Buffer> _buffer;

		ComPtr<ID3D11ShaderResourceView> _srv;
		ComPtr<ID3D11UnorderedAccessView> _uav;

		MemoryProperty _memProperty;
		BufferUsages _usages;
		uint32_t _elmSize;
		uint32_t _bufferByteSize;
	};
}

#endif