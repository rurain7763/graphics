#pragma once

#include "Graphics/GraphicsType.h"

#include <Windows.h>
#include <wrl.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <stdexcept>

using namespace Microsoft::WRL;
using namespace DirectX;
using namespace DirectX::PackedVector;

namespace flaw {
	static D3D11_PRIMITIVE_TOPOLOGY ConvertToD3D11Topology(PrimitiveTopology topology) {
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
		default:
			throw std::runtime_error("Unknown primitive topology");
		}
	}

	static DXGI_FORMAT ConvertToDXGIFormat(PixelFormat format) {
		switch (format) {
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
		default:
			throw std::runtime_error("Unknown pixel format");
		}
	}

	static D3D11_USAGE ConvertD3D11Usage(const UsageFlag usage) {
		switch (usage)
		{
		case UsageFlag::Static:
			return D3D11_USAGE_DEFAULT;
		case UsageFlag::Dynamic:
			return D3D11_USAGE_DYNAMIC;
		case UsageFlag::Staging:
			return D3D11_USAGE_STAGING;
		}

		throw std::runtime_error("Unknown usage flag");
	}

	static uint32_t ConvertD3D11Access(const uint32_t access) {
		uint32_t accessFlag = 0;
		if (access & AccessFlag::Write) {
			accessFlag |= D3D11_CPU_ACCESS_WRITE;
		}
		if (access & AccessFlag::Read) {
			accessFlag |= D3D11_CPU_ACCESS_READ;
		}
		return accessFlag;
	}

	static uint32_t ConvertD3D11Bind(uint32_t flags) {
		uint32_t bindFlags = 0;

		if (flags & BindFlag::ShaderResource) {
			bindFlags |= D3D11_BIND_SHADER_RESOURCE;
		}

		if (flags & BindFlag::RenderTarget) {
			bindFlags |= D3D11_BIND_RENDER_TARGET;
		}

		if (flags & BindFlag::DepthStencil) {
			bindFlags |= D3D11_BIND_DEPTH_STENCIL;
		}

		if (flags & BindFlag::UnorderedAccess) {
			bindFlags |= D3D11_BIND_UNORDERED_ACCESS;
		}

		return bindFlags;
	}

	static void ConvertD3D11Blend(BlendMode blendMode, D3D11_BLEND& outSrcBlend, D3D11_BLEND& outDestBlend) {
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

	static D3D11_COMPARISON_FUNC ConvertD3D11DepthTest(DepthTest depthTest) {
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
}