#pragma once

#include <cstdint>

namespace flaw {
	using ShaderResourceView = void*;
	using UnorderedAccessView = void*;
	using RenderTargetView = void*;
	using DepthStencilView = void*;

	enum class MemoryProperty {
		Static,  // GPU only, no CPU access 
		Dynamic, // GPU and CPU access, data can change frequently
		Staging, // CPU only, used for transferring data to GPU
	};

	enum class TextureUsage {
		ShaderResource = 0x1,
		UnorderedAccess = 0x2,
		RenderTarget = 0x4,
		DepthStencil = 0x8,
	};

	struct TextureUsages {
		uint32_t value;

		TextureUsages() : value(0) {}
		TextureUsages(TextureUsage usage) : value(static_cast<uint32_t>(usage)) {}
		TextureUsages(uint32_t v) : value(v) {}

		TextureUsages operator|(TextureUsage usage) const {
			return TextureUsages(value | static_cast<uint32_t>(usage));
		}

		TextureUsages operator&(TextureUsage usage) const {
			return TextureUsages(value & static_cast<uint32_t>(usage));
		}

		TextureUsages& operator|=(TextureUsage usage) {
			value |= static_cast<uint32_t>(usage);
			return *this;
		}

		TextureUsages& operator&=(TextureUsage usage) {
			value &= static_cast<uint32_t>(usage);
			return *this;
		}

		TextureUsages& operator=(TextureUsage usage) {
			value = static_cast<uint32_t>(usage);
			return *this;
		}

		operator uint32_t() const {
			return value;
		}
	};

	inline TextureUsages operator|(TextureUsage a, TextureUsage b) {
		return TextureUsages(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
	}

	enum class BufferUsage {
		ShaderResource = 0x1,
		UnorderedAccess = 0x2,
	};

	struct BufferUsages {
		uint32_t value;

		BufferUsages() : value(0) {}
		BufferUsages(BufferUsage usage) : value(static_cast<uint32_t>(usage)) {}
		BufferUsages(uint32_t v) : value(v) {}

		BufferUsages operator|(BufferUsage usage) const {
			return BufferUsages(value | static_cast<uint32_t>(usage));
		}

		BufferUsages operator&(BufferUsage usage) const {
			return BufferUsages(value & static_cast<uint32_t>(usage));
		}

		BufferUsages& operator|=(BufferUsage usage) {
			value |= static_cast<uint32_t>(usage);
			return *this;
		}

		BufferUsages& operator&=(BufferUsage usage) {
			value &= static_cast<uint32_t>(usage);
			return *this;
		}

		BufferUsages& operator=(BufferUsage usage) {
			value = static_cast<uint32_t>(usage);
			return *this;
		}

		operator uint32_t() const {
			return value;
		}
	};

	inline BufferUsages operator|(BufferUsage a, BufferUsage b) {
		return BufferUsages(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
	}

	enum class PrimitiveTopology {
		TriangleList,
		TriangleStrip,
		LineList,
		LineStrip,
		PointList,
		ControlPoint3_PatchList,
		ControlPoint4_PatchList,
		Count
	};

	enum class CullMode {
		None,
		Front,
		Back
	};

	enum class FillMode {
		Solid,
		Wireframe
	};

	enum class DepthTest {
		Less,
		LessEqual,
		Greater,
		GreaterEqual,
		Equal,
		NotEqual,
		Always,
		Never,
		Disabled,
	};

	enum class BlendMode {
		Default,  // SRC : 1, DST : 0
		Alpha,    // SRC : SRC_ALPHA, DST : 1 - SRC_ALPHA
		Additive, // SRC : SRC_ALPHA, DST : 1
		Disabled,
	};

	enum class PixelFormat {
		UNDEFINED,
		RGBA8,
		RGBA32F,
		RGB8,
		BGRX8,
		RG8,
		R8,
		R8_UINT,
		R32F,
		R32_UINT,
		D24S8_UINT,
		D32F_S8UI,
		BGRA8,
	};

	enum class ElementType {
		Float,
		Uint32,
		Int,
	};

	enum class TextureType {
		Texture2D,
		TextureCube,
	};

	enum TextureSlot {
		_0 = 0,
		_1,
		_2,
		_3,
		_4,

		Cube0,
		Cube1,
		Cube2,
		Cube3,

		Array0,
		Array1,
		Array2,
		Array3,

		Count
	};

	enum class ShaderStage {
		Vertex = 0x1,
		Pixel = 0x2,
		Geometry = 0x4,
		Hull = 0x8,
		Domain = 0x10,
		Compute = 0x20,
	};

	struct ShaderStages {
		uint32_t value;

		ShaderStages() : value(0) {}
		ShaderStages(ShaderStage stage) : value(static_cast<uint32_t>(stage)) {}
		ShaderStages(uint32_t v) : value(v) {}

		ShaderStages operator|(ShaderStage stage) const {
			return ShaderStages(value | static_cast<uint32_t>(stage));
		}

		ShaderStages operator&(ShaderStage stage) const {
			return ShaderStages(value & static_cast<uint32_t>(stage));
		}

		ShaderStages& operator|=(ShaderStage stage) {
			value |= static_cast<uint32_t>(stage);
			return *this;
		}

		ShaderStages& operator&=(ShaderStage stage) {
			value &= static_cast<uint32_t>(stage);
			return *this;
		}

		ShaderStages& operator=(ShaderStage stage) {
			value = static_cast<uint32_t>(stage);
			return *this;
		}

		operator uint32_t() const {
			return value;
		}
	};

	inline ShaderStages operator|(ShaderStage a, ShaderStage b) {
		return ShaderStages(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
	}

	enum class PipelineType {
		Graphics,
		Compute
	};

	enum class VertexInputRate {
		Vertex,  // Vertex input is per vertex
		Instance // Vertex input is per instance
	};

	enum class TextureLayout {
		Undefined,
		Color,
		DepthStencil,
		Present,
	};

	enum class ResourceType {
		ConstantBuffer,
		StructuredBuffer,
		Texture2D,
		TextureCube,
	};

	enum class AttachmentLoadOp {
		Load,  
		Clear,
		DontCare
	};

	enum class AttachmentStoreOp {
		Store,
		DontCare
	};
}


