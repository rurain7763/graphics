#pragma once

#include "Core.h"

#ifdef SUPPORT_DX11

#include "DXCore.h"
#include "Graphics/GraphicsBuffers.h"

namespace flaw {
	class DXContext;

	struct DXNativeBuffer : public GraphicsNativeBuffer {
		ComPtr<ID3D11Buffer> buffer;
		ComPtr<ID3D11ShaderResourceView> srv;
		ComPtr<ID3D11UnorderedAccessView> uav;

		static DXNativeBuffer Create(DXContext& context, const D3D11_BUFFER_DESC& descriptor, const void* initialData = nullptr);
	};

	class DXVertexInputLayout : public VertexInputLayout {
	public:
		DXVertexInputLayout(DXContext& context, const Descriptor& descriptor);
		~DXVertexInputLayout() override;

		inline const std::vector<D3D11_INPUT_ELEMENT_DESC>& GetInputElements() const { return _inputElements; }

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

		const GraphicsNativeBuffer& GetNativeBuffer() const override { return _nativeBuffer; }

		inline uint32_t ElementSize() const { return _elmSize; }

	private:
		DXContext& _context;

		DXNativeBuffer _nativeBuffer;

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

		const GraphicsNativeBuffer& GetNativeBuffer() const override { return _nativeBuffer; }

	private:
		DXContext& _context;
	
		DXNativeBuffer _nativeBuffer;

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

		const GraphicsNativeBuffer& GetNativeBuffer() const override { return _nativeBuffer; }

	private:
		DXContext& _context;

		DXNativeBuffer _nativeBuffer;

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

		inline uint32_t Size() const override { return _bufferByteSize; }

		const GraphicsNativeBuffer& GetNativeBuffer() const override { return _nativeBuffer; }

	private:
		void CreateShaderResourceView();
		void CreateUnorderedAccessView();

	private:
		DXContext& _context;

		DXNativeBuffer _nativeBuffer;

		MemoryProperty _memProperty;
		BufferUsages _usages;
		uint32_t _elmSize;
		uint32_t _bufferByteSize;
	};
}

#endif