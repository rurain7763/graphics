#pragma once

#include "Core.h"

#ifdef SUPPORT_DX11

#include "Graphics/GraphicsType.h"
#include "Graphics/GraphicsFunc.h"

#include <wrl.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>

using namespace Microsoft::WRL;
using namespace DirectX;
using namespace DirectX::PackedVector;

namespace flaw {
	inline DXGI_FORMAT ConvertToDXFormat(ElementType type, uint32_t count) {
		switch (type) {
		case ElementType::Float:
			switch (count) {
			case 1: return DXGI_FORMAT_R32_FLOAT;
			case 2: return DXGI_FORMAT_R32G32_FLOAT;
			case 3: return DXGI_FORMAT_R32G32B32_FLOAT;
			case 4: return DXGI_FORMAT_R32G32B32A32_FLOAT;
			}
			break;
		case ElementType::Uint32:
			switch (count) {
			case 1: return DXGI_FORMAT_R32_UINT;
			case 2: return DXGI_FORMAT_R32G32_UINT;
			case 3: return DXGI_FORMAT_R32G32B32_UINT;
			case 4: return DXGI_FORMAT_R32G32B32A32_UINT;
			}
			break;
		case ElementType::Int:
			switch (count) {
			case 1: return DXGI_FORMAT_R32_SINT;
			case 2: return DXGI_FORMAT_R32G32_SINT;
			case 3: return DXGI_FORMAT_R32G32B32_SINT;
			case 4: return DXGI_FORMAT_R32G32B32A32_SINT;
			}
			break;
		}
		return DXGI_FORMAT_UNKNOWN;
	}

	static DXGI_FORMAT ConvertToDXFormat(PixelFormat format) {
		switch (format) {
		case PixelFormat::UNDEFINED:
			return DXGI_FORMAT_UNKNOWN;
		case PixelFormat::BGRX8:
			return DXGI_FORMAT_B8G8R8X8_UNORM;
		case PixelFormat::RGBA8:
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		case PixelFormat::RGBA32F:
			return DXGI_FORMAT_R32G32B32A32_FLOAT;
		case PixelFormat::RG8:
			return DXGI_FORMAT_R8G8_UNORM;
		case PixelFormat::R8:
			return DXGI_FORMAT_R8_UNORM;
		case PixelFormat::R8_UINT:
			return DXGI_FORMAT_R8_UINT;
		case PixelFormat::R32F:
			return DXGI_FORMAT_R32_FLOAT;
		case PixelFormat::R32_UINT:
			return DXGI_FORMAT_R32_UINT;
		case PixelFormat::D24S8_UINT:
			return DXGI_FORMAT_D24_UNORM_S8_UINT;
		case PixelFormat::D32F_S8UI:
			return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
		case PixelFormat::BGRA8:
			return DXGI_FORMAT_B8G8R8A8_UNORM;
		}

		throw std::runtime_error("Unsupported PixelFormat");	
	}

	static PixelFormat ConvertToPixelFormat(DXGI_FORMAT format) {
		switch (format) {
		case DXGI_FORMAT_UNKNOWN:
			return PixelFormat::UNDEFINED;
		case DXGI_FORMAT_B8G8R8X8_UNORM:
			return PixelFormat::BGRX8;
		case DXGI_FORMAT_R8G8B8A8_UNORM:
			return PixelFormat::RGBA8;
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
			return PixelFormat::RGBA32F;
		case DXGI_FORMAT_R8G8_UNORM:
			return PixelFormat::RG8;
		case DXGI_FORMAT_R8_UNORM:
			return PixelFormat::R8;
		case DXGI_FORMAT_R8_UINT:
			return PixelFormat::R8_UINT;
		case DXGI_FORMAT_R32_FLOAT:
			return PixelFormat::R32F;
		case DXGI_FORMAT_R32_UINT:
			return PixelFormat::R32_UINT;
		case DXGI_FORMAT_D24_UNORM_S8_UINT:
			return PixelFormat::D24S8_UINT;
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
			return PixelFormat::D32F_S8UI;
		case DXGI_FORMAT_B8G8R8A8_UNORM:
			return PixelFormat::BGRA8;
		}

		throw std::runtime_error("Unsupported DXGI_FORMAT");
	}

	inline D3D11_INPUT_CLASSIFICATION ConvertToDXInputClassification(VertexInputRate rate) {
		switch (rate) {
		case VertexInputRate::Vertex:
			return D3D11_INPUT_PER_VERTEX_DATA;
		case VertexInputRate::Instance:
			return D3D11_INPUT_PER_INSTANCE_DATA;
		}

		throw std::runtime_error("Unsupported VertexInputRate");
	}

	inline UINT ConvertToDXCPUAccessFlags(MemoryProperty memProperty) {
		switch (memProperty) {
		case MemoryProperty::Static:
			return 0;
		case MemoryProperty::Dynamic:
			return D3D11_CPU_ACCESS_WRITE;
		case MemoryProperty::Staging:
			return D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
		default:
			throw std::runtime_error("Unsupported MemoryProperty");
		}
	}

	inline D3D11_USAGE ConvertToDXUsage(MemoryProperty memProperty) {
		switch (memProperty) {
		case MemoryProperty::Static:
			return D3D11_USAGE_DEFAULT;
		case MemoryProperty::Dynamic:
			return D3D11_USAGE_DYNAMIC;
		case MemoryProperty::Staging:
			return D3D11_USAGE_STAGING;
		default:
			throw std::runtime_error("Unsupported MemoryProperty");
		}
	}

	static MemoryProperty ConvertToMemoryProperty(D3D11_USAGE usage) {
		switch (usage) {
		case D3D11_USAGE_DEFAULT:
			return MemoryProperty::Static;
		case D3D11_USAGE_DYNAMIC:
			return MemoryProperty::Dynamic;
		case D3D11_USAGE_STAGING:
			return MemoryProperty::Staging;
		default:
			throw std::runtime_error("Unsupported D3D11_USAGE");
		}
	}

	inline D3D11_PRIMITIVE_TOPOLOGY ConvertToDXPrimitiveTopology(PrimitiveTopology topology) {
		switch (topology) {
		case PrimitiveTopology::PointList:
			return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
		case PrimitiveTopology::LineList:
			return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
		case PrimitiveTopology::LineStrip:
			return D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
		case PrimitiveTopology::TriangleList:
			return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		case PrimitiveTopology::TriangleStrip:
			return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
		case PrimitiveTopology::ControlPoint3_PatchList:
			return D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
		case PrimitiveTopology::ControlPoint4_PatchList:
			return D3D11_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST;
		}

		throw std::runtime_error("Unknown primitive topology");
	}

	inline D3D11_COMPARISON_FUNC ConvertToDXComparisonFunc(DepthTest depthTest) {
		switch (depthTest)
		{
		case DepthTest::Less:
			return D3D11_COMPARISON_LESS;
		case DepthTest::LessEqual:
			return D3D11_COMPARISON_LESS_EQUAL;
		case DepthTest::Greater:
			return D3D11_COMPARISON_GREATER;
		case DepthTest::GreaterEqual:
			return D3D11_COMPARISON_GREATER_EQUAL;
		case DepthTest::Equal:
			return D3D11_COMPARISON_EQUAL;
		case DepthTest::NotEqual:
			return D3D11_COMPARISON_NOT_EQUAL;
		case DepthTest::Always:
			return D3D11_COMPARISON_ALWAYS;
		case DepthTest::Never:
			return D3D11_COMPARISON_NEVER;
		}

		return D3D11_COMPARISON_LESS;
	}

	inline D3D11_DEPTH_WRITE_MASK ConvertToDXDepthWriteMask(bool depthWrite) {
		return depthWrite ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
	}

	inline D3D11_FILL_MODE ConverToDXFillMode(FillMode fillMode) {
		switch (fillMode)
		{
		case FillMode::Solid:
			return D3D11_FILL_SOLID;
		case FillMode::Wireframe:
			return D3D11_FILL_WIREFRAME;
		}

		return D3D11_FILL_SOLID;
	}

	inline D3D11_CULL_MODE ConvertToDXCullMode(CullMode cullMode) {
		switch (cullMode)
		{
		case CullMode::None:
			return D3D11_CULL_NONE;
		case CullMode::Front:
			return D3D11_CULL_FRONT;
		case CullMode::Back:
			return D3D11_CULL_BACK;
		}

		return D3D11_CULL_BACK;
	}

	inline UINT ConvertToDXBufferBindFlags(BufferUsages bufferUsages) {
		UINT flags = 0;

		if (bufferUsages & BufferUsage::ShaderResource) {
			flags |= D3D11_BIND_SHADER_RESOURCE;
		}

		if (bufferUsages & BufferUsage::UnorderedAccess) {
			flags |= D3D11_BIND_UNORDERED_ACCESS;
		}

		return flags;
	}

	static uint32_t ConvertToDXTexBindFlags(TextureUsages texUsages) {
		uint32_t bindFlags = 0;

		if (texUsages & TextureUsage::ShaderResource) {
			bindFlags |= D3D11_BIND_SHADER_RESOURCE;
		}

		if (texUsages & TextureUsage::UnorderedAccess) {
			bindFlags |= D3D11_BIND_UNORDERED_ACCESS;
		}

		if (texUsages & TextureUsage::RenderTarget) {
			bindFlags |= D3D11_BIND_RENDER_TARGET;
		}

		if (texUsages & TextureUsage::DepthStencil) {
			bindFlags |= D3D11_BIND_DEPTH_STENCIL;
		}

		return bindFlags;
	}

	static TextureUsages ConvertToTexUsages(TextureUsages texUsages) {
		TextureUsages usages = 0;
		if (texUsages & D3D11_BIND_SHADER_RESOURCE) {
			usages |= TextureUsage::ShaderResource;
		}

		if (texUsages & D3D11_BIND_UNORDERED_ACCESS) {
			usages |= TextureUsage::UnorderedAccess;
		}

		if (texUsages & D3D11_BIND_RENDER_TARGET) {
			usages |= TextureUsage::RenderTarget;
		}

		if (texUsages & D3D11_BIND_DEPTH_STENCIL) {
			usages |= TextureUsage::DepthStencil;
		}

		return usages;
	}

	static void ConvertDXBlend(BlendMode blendMode, D3D11_BLEND& outSrcBlend, D3D11_BLEND& outDestBlend) {
		switch (blendMode)
		{
		case BlendMode::Default:
			outSrcBlend = D3D11_BLEND_ONE;
			outDestBlend = D3D11_BLEND_ZERO;
			break;
		case BlendMode::Alpha:
			outSrcBlend = D3D11_BLEND_SRC_ALPHA;
			outDestBlend = D3D11_BLEND_INV_SRC_ALPHA;
			break;
		case BlendMode::Additive:
			outSrcBlend = D3D11_BLEND_SRC_ALPHA;
			outDestBlend = D3D11_BLEND_ONE;
			break;
		default:
			outSrcBlend = D3D11_BLEND_ONE;
			outDestBlend = D3D11_BLEND_ZERO;
			break;
		}
	}
}

#endif