#pragma once

namespace flaw {
	using ShaderResourceView = void*;
	using UnorderedAccessView = void*;
	using RenderTargetView = void*;
	using DepthStencilView = void*;

	enum class RenderDomain {
		Opaque,
		Masked,
		Transparent,
		PostProcess,
		Debug,
		Count
	};

	enum class UsageFlag {
		Static,  // �⺻ �뵵. GPU�� �б�� ���⸦ ����ϸ� CPU ������ �ź��Ѵ�
		Dynamic, // CPU�� ���Ⱑ �����ϴ�. GPU�� �б⸸ �����ϴ�
		Staging, // GPU���� �ڷḦ ���, �����ϰ� �װ��� �����ϰų� �߰� ���縦 ���� CPU�� �о� �鿩�� �ϴ� ���
	};

	enum AccessFlag {
		Write = 0x1,
		Read = 0x2
	};

	enum BindFlag {
		ShaderResource = 0x1,
		UnorderedAccess = 0x2,
		RenderTarget = 0x4,
		DepthStencil = 0x8,
		DepthOnly = 0x10,
		StencilOnly = 0x20,
	};

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

	enum ShaderCompileFlag { // TODO: Change name to ShaderStage
		Vertex = 0x1,
		Pixel = 0x2,
		Geometry = 0x4,
		Hull = 0x8,
		Domain = 0x10
	};

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


