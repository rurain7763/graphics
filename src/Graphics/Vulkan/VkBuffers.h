#pragma once

#include "Core.h"

#ifdef SUPPORT_VULKAN

#include "VkCore.h"
#include "Graphics/GraphicsBuffers.h"

namespace flaw {
	class VkContext;

	class VkVertexInputLayout : public VertexInputLayout {
    public:
        VkVertexInputLayout(VkContext& context, const Descriptor& descriptor);
        ~VkVertexInputLayout() override = default;

        inline vk::VertexInputBindingDescription& GetVkVertexInputBindingDescription() { return _bindingDescription; }
        inline const std::vector<vk::VertexInputAttributeDescription>& GetVkVertexInputAttributeDescriptions() const { return _attributeDescriptions; }

    private:
        VkContext& _context;
        
        vk::VertexInputBindingDescription _bindingDescription;
		std::vector<vk::VertexInputAttributeDescription> _attributeDescriptions;
    };

	class VkVertexBuffer : public VertexBuffer {
	public:
		VkVertexBuffer(VkContext& context, const Descriptor& descriptor);
		~VkVertexBuffer();

		virtual void Update(const void* data, uint32_t size) override;

		virtual void CopyTo(Ref<VertexBuffer> dstBuffer, uint32_t srcOffset = 0, uint32_t dstOffset = 0) override;

		virtual uint32_t Size() const override { return _size; }

		inline vk::Buffer GetVkBuffer() const { return _buffer; }

	private:
		bool CreateBuffer();
		
		bool AllocateMemory();

	private:
		VkContext& _context;

		MemoryProperty _memProperty;
		uint32_t _elmSize;
		uint32_t _size;

		vk::Buffer _buffer;
		vk::DeviceMemory _memory;

		void* _mappedData = nullptr;
	};

	class VkIndexBuffer : public IndexBuffer {
	public:
		VkIndexBuffer(VkContext& context, const Descriptor& descriptor);
		~VkIndexBuffer() override;

		void Update(const uint32_t* indices, uint32_t count) override;

		uint32_t IndexCount() const override { return _indexCount; }

		inline vk::Buffer GetVkBuffer() const { return _buffer; }

	private:
		bool CreateBuffer();

	private:
		VkContext& _context;

		vk::Buffer _buffer;
		vk::DeviceMemory _memory;

		void* _mappedData = nullptr;

		MemoryProperty _memProperty;
		uint32_t _size;
		uint32_t _indexCount;
	};

	class VkConstantBuffer : public ConstantBuffer {
	public:
		VkConstantBuffer(VkContext& context, const Descriptor& descriptor);

		~VkConstantBuffer() override;

		void Update(const void* data, int32_t size) override;

		uint32_t Size() const override { return _size; }

		inline const vk::Buffer& GetVkBuffer() const { return _buffer; }

	private:
		bool CreateBuffer();
		bool AllocateMemory();

	private:
		VkContext& _context;

		vk::Buffer _buffer;
		vk::DeviceMemory _memory;

		void* _mappedData = nullptr;

		MemoryProperty _memProperty;
		uint32_t _size;
	};

	class VkStructuredBuffer : public StructuredBuffer {
	public:
		VkStructuredBuffer(VkContext& context, const Descriptor& desc);
		~VkStructuredBuffer();

		void Update(const void* data, uint32_t size) override;
		void Fetch(void* data, uint32_t size) override;

		void CopyTo(Ref<StructuredBuffer> dstBuffer, uint32_t srcOffset = 0, uint32_t dstOffset = 0) override {}
		void CopyFrom(Ref<StructuredBuffer> srcBuffer, uint32_t srcOffset = 0, uint32_t dstOffset = 0) override {}

		uint32_t Size() const override { return _size; }

		inline const vk::Buffer& GetVkBuffer() const { return _buffer; }

	private:
		bool CreateBuffer();
		bool AllocateMemory();

	private:
		VkContext& _context;

		vk::Buffer _buffer;
		vk::DeviceMemory _memory;

		void* _mappedData = nullptr;

		MemoryProperty _memProperty;
		uint32_t _elmSize;
		uint32_t _size;
	};
}

#endif