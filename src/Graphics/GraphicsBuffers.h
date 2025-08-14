#pragma once

#include "Core.h"
#include "GraphicsType.h"

namespace flaw {	
 	class VertexInputLayout {
    public:
        struct InputElement {
            std::string name;
            ElementType type;
            uint32_t count;
        };

        struct Descriptor {
			uint32_t binding;
            VertexInputRate vertexInputRate;
            std::vector<InputElement> inputElements;
        }; 
    
        virtual ~VertexInputLayout() = default;        
    };

	class VertexBuffer {
	public:
		struct Descriptor {
			MemoryProperty memProperty;
			uint32_t elmSize;
			uint32_t bufferSize;
			const void* initialData;
		};

		VertexBuffer() = default;
		virtual ~VertexBuffer() = default;

		virtual void Update(const void* data, uint32_t size) = 0;

		virtual void CopyTo(Ref<VertexBuffer> dstBuffer, uint32_t srcOffset = 0, uint32_t dstOffset = 0) = 0;

		virtual uint32_t Size() const = 0;
	};

	class IndexBuffer {
	public:
		struct Descriptor {
			MemoryProperty memProperty; 
			uint32_t bufferSize;
			const uint32_t* initialData;
		};

		IndexBuffer() = default;
		virtual ~IndexBuffer() = default;

		virtual void Update(const uint32_t* indices, uint32_t count) = 0;

		virtual uint32_t IndexCount() const = 0;
	};

	// what is constant buffer?
	// limit size
	// allighn size with 16
	class ConstantBuffer {
	public:
		struct Descriptor {
			MemoryProperty memProperty;
			uint32_t bufferSize;
			const void* initialData;
		};

		ConstantBuffer() = default;
		virtual ~ConstantBuffer() = default;

		virtual void Update(const void* data, int32_t size) = 0;

		virtual uint32_t Size() const = 0;
	};

	// what is structured buffer?
	// no limit size
	// no allighn size
	class StructuredBuffer {
	public:
		struct Descriptor {
			MemoryProperty memProperty;
			BufferUsages bufferUsages;
			uint32_t elmSize;
			uint32_t bufferSize;
			const void* initialData;
		};

		StructuredBuffer() = default;
		virtual ~StructuredBuffer() = default;

		virtual void Update(const void* data, uint32_t size) = 0;
		virtual void Fetch(void* data, uint32_t size) = 0;

		virtual void CopyTo(Ref<StructuredBuffer> dstBuffer, uint32_t srcOffset = 0, uint32_t dstOffset = 0) = 0;
		virtual void CopyFrom(Ref<StructuredBuffer> srcBuffer, uint32_t srcOffset = 0, uint32_t dstOffset = 0) = 0;

		virtual uint32_t Size() const = 0;
	};
}
