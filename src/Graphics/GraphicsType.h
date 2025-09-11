#pragma once

#include "Core.h"

#include <cstdint>

namespace flaw {
	using ShaderResourceView = void*;
	using UnorderedAccessView = void*;
	using RenderTargetView = void*;
	using DepthStencilView = void*;

	enum class MemoryProperty {
		Static,  // GPU only, no CPU access 
		Dynamic, // CPU write access, data can change frequently
		Staging, // CPU read write access, used for transferring data to GPU
	};

	enum class TextureUsage {
		ShaderResource = 0x1,
		UnorderedAccess = 0x2,
		ColorAttachment = 0x4,
		DepthStencilAttachment = 0x8,
		InputAttachment = 0x10,
	};

	using TextureUsages = Flags<TextureUsage>;

	inline TextureUsages operator|(TextureUsage a, TextureUsage b) {
		return TextureUsages(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
	}

	enum class BufferUsage {
		ShaderResource = 0x1,
		UnorderedAccess = 0x2,
	};

	using BufferUsages = Flags<BufferUsage>;

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

	enum class CompareOp {
		Less,
		LessEqual,
		Greater,
		GreaterEqual,
		Equal,
		NotEqual,
		Always,
		Never
	};

	enum class StencilOp {
		Keep, // Keep the current stencil value
		Zero, // Set the stencil value to zero
		Replace, // Replace the stencil value with the reference value
		IncrementWrap, // Increment the stencil value, but wraps it back to zero as soon as it exceeds the maximum value
		IncrementClamp, // Increment the stencil value, but clamps it to the maximum value
		DecrementWrap, // Decrement the stencil value with wrap, but wraps it back to the maximum value as soon as it goes below zero
		DecrementClamp, // Decrement the stencil value, but clamps it to zero
		Invert // Bitwise invert the stencil value
	};

	enum class BlendMode {
		Default,  // SRC : 1, DST : 0
		Alpha,    // SRC : SRC_ALPHA, DST : 1 - SRC_ALPHA
		Additive, // SRC : SRC_ALPHA, DST : 1
	};

	enum class PixelFormat {
		Undefined,
		RGBA8Unorm,
		RGBA8Srgb,
		RGBA32F,
		RGBA16F,
		RGB8,
		BGRX8Unorm,
		RG8,
		R8,
		R8_UINT,
		R32F,
		R32_UINT,
		D24S8_UINT,
		D32F_S8UI,
		D32F,
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

	using ShaderStages = Flags<ShaderStage>;

	inline ShaderStages operator|(ShaderStage a, ShaderStage b) {
		return ShaderStages(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
	}

	enum class PipelineType {
		Graphics,
		Compute
	};

	enum class PipelineStage {
		TopOfPipe = 0x1,
		VertexInput = 0x400,
		VertexShader = 0x2,
		HullShader = 0x4,
		DomainShader = 0x8,
		GeometryShader = 0x10,
		EarlyPixelTests = 0x20,
		PixelShader = 0x40,
		ColorAttachmentOutput = 0x80,
		AllGraphics = 0x800,
		BottomOfPipe = 0x100,
		Host = 0x200,
	};

	using PipelineStages = Flags<PipelineStage>;

	inline PipelineStages operator|(PipelineStage a, PipelineStage b) {
		return PipelineStages(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
	}

	enum class VertexInputRate {
		Vertex,  // Vertex input is per vertex
		Instance // Vertex input is per instance
	};

	enum class TextureLayout {
		Undefined,
		ColorAttachment,
		DepthStencilAttachment,
		PresentSource,
		General,
		ShaderReadOnly,
	};

	enum class ResourceType {
		ConstantBuffer,
		StructuredBuffer,
		Texture2D,
		TextureCube,
		InputAttachment,
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

	enum class AccessType {
		VertexElementRead = 0x80,
		ShaderRead = 0x1,
		ShaderWrite = 0x2,
		ColorAttachmentRead = 0x4,
		ColorAttachmentWrite = 0x8,
		InputAttachmentRead = 0x200,
		DepthStencilAttachmentRead = 0x10,
		DepthStencilAttachmentWrite = 0x20,
		HostWrite = 0x40,
		None = 0x100,
	};

	using AccessTypes = Flags<AccessType>;

	inline AccessTypes operator|(AccessType a, AccessType b) {
		return AccessTypes(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
	}
}


