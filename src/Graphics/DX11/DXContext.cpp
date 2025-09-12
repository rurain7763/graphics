#include "pch.h"
#include "DXContext.h"

#ifdef SUPPORT_DX11

#include "Platform/PlatformContext.h"
#include "Log/Log.h"
#include "DXBuffers.h"
#include "DXShaders.h"
#include "DXPipelines.h"
#include "DXCommandQueue.h"
#include "DXTextures.h"
#include "DXRenderPass.h"
#include "DXFramebuffer.h"

namespace flaw {
	DXContext::DXContext(PlatformContext& context, int32_t width, int32_t height, bool msaa) {
		_hWnd = context.GetWindowHandle();
		_renderWidth = width;
		_renderHeight = height;
		_enableMSAA = msaa;

		UINT createDeviceFlags = 0;
#if _DEBUG
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

		D3D_FEATURE_LEVEL featureLevel;

		if (FAILED(D3D11CreateDevice(
			nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			createDeviceFlags,
			nullptr,
			0,
			D3D11_SDK_VERSION,
			&_device,
			&featureLevel,
			&_deviceContext))) 
		{
			LOG_FATAL("D3D11CreateDevice failed");
			return;
		}

		_msaaSampleCount = GetDXMaxMSAASampleCount(_device, ConvertToDXFormat(GetSurfaceFormat()));

		if (CreateSwapChain()) {
			return;
		}

		_samplerStates[0] = CreateSamplerState(D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP);
		_samplerStates[1] = CreateSamplerState(D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP);

		_deviceContext->PSSetSamplers(0, _samplerStates.size(), _samplerStates.data());
		_deviceContext->VSSetSamplers(0, _samplerStates.size(), _samplerStates.data());
		_deviceContext->GSSetSamplers(0, _samplerStates.size(), _samplerStates.data());
		_deviceContext->HSSetSamplers(0, _samplerStates.size(), _samplerStates.data());
		_deviceContext->DSSetSamplers(0, _samplerStates.size(), _samplerStates.data());
		_deviceContext->CSSetSamplers(0, _samplerStates.size(), _samplerStates.data());

		_commandQueue = CreateRef<DXCommandQueue>(*this);

		Log::Info("DirectX 11 Initialized");
	}

	int32_t DXContext::CreateSwapChain() {
		ComPtr<IDXGIDevice> dxgiDevice = nullptr;
		ComPtr<IDXGIAdapter> dxgiAdapter = nullptr;
		ComPtr<IDXGIFactory1> dxgiFactory = nullptr;

		_device->QueryInterface(__uuidof(IDXGIDevice), (void**)dxgiDevice.GetAddressOf());
		dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)dxgiAdapter.GetAddressOf());
		dxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void**)dxgiFactory.GetAddressOf());

		DXGI_SWAP_CHAIN_DESC swapchainDesc = {};
		swapchainDesc.BufferDesc.Format = ConvertToDXFormat(GetSurfaceFormat());
		swapchainDesc.BufferCount = 1;
		swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapchainDesc.BufferDesc.Width = _renderWidth;
		swapchainDesc.BufferDesc.Height = _renderHeight;
		swapchainDesc.OutputWindow = _hWnd;
		swapchainDesc.Windowed = TRUE; // TODO: 수정 가능하도록 변경
		swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		swapchainDesc.BufferDesc.RefreshRate.Denominator = 1;
		swapchainDesc.BufferDesc.RefreshRate.Numerator = 60;
		swapchainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		swapchainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		swapchainDesc.SampleDesc.Count = 1;
		swapchainDesc.SampleDesc.Quality = 0;

		if (FAILED(dxgiFactory->CreateSwapChain(_device.Get(), &swapchainDesc, _swapChain.GetAddressOf()))) {
			LOG_ERROR("CreateSwapChain failed");
			return -1;
		}

		ComPtr<ID3D11Texture2D> backBuffer;
		if (FAILED(_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)backBuffer.GetAddressOf()))) {
			throw std::runtime_error("GetBuffer failed");
		}

		_colorAttachment = CreateRef<DXTexture2D>(*this, backBuffer, GetSurfaceFormat(), 0);

		return 0;
	}

	ID3D11SamplerState* DXContext::CreateSamplerState(D3D11_FILTER filter, D3D11_TEXTURE_ADDRESS_MODE u, D3D11_TEXTURE_ADDRESS_MODE v, D3D11_TEXTURE_ADDRESS_MODE w) {
		D3D11_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = filter;
		samplerDesc.AddressU = u;
		samplerDesc.AddressV = v;
		samplerDesc.AddressW = w;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		samplerDesc.MinLOD = 0;
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

		ID3D11SamplerState* samplerState;
		if (FAILED(_device->CreateSamplerState(&samplerDesc, &samplerState))) {
			Log::Error("CreateSamplerState failed");
			return nullptr;
		}

		return samplerState;
	}

	DXContext::~DXContext() {
		_commandQueue.reset();

		for (auto samplerState : _samplerStates) {
			samplerState->Release();
		}
	}

	bool DXContext::Prepare() {
		// NOTE: DirectX 11에서는 별도의 준비 작업이 필요하지 않습니다.
		return true;
	}

	Ref<VertexInputLayout> DXContext::CreateVertexInputLayout(const VertexInputLayout::Descriptor& desc) {
		return CreateRef<DXVertexInputLayout>(*this, desc);
	}

	Ref<VertexBuffer> DXContext::CreateVertexBuffer(const VertexBuffer::Descriptor& descriptor) {
		return CreateRef<DXVertexBuffer>(*this, descriptor);
	}

	Ref<IndexBuffer> DXContext::CreateIndexBuffer(const IndexBuffer::Descriptor& descriptor) {
		return CreateRef<DXIndexBuffer>(*this, descriptor);
	}

	Ref<ShaderResourcesLayout> DXContext::CreateShaderResourcesLayout(const ShaderResourcesLayout::Descriptor& desc) {
		return CreateRef<DXShaderResourcesLayout>(*this, desc);
	}

	Ref<ShaderResources> DXContext::CreateShaderResources(const ShaderResources::Descriptor& desc) {
		return CreateRef<DXShaderResources>(*this, desc);
	}

	Ref<GraphicsShader> DXContext::CreateGraphicsShader(const GraphicsShader::Descriptor& desc) {
		return CreateRef<DXGraphicsShader>(*this, desc);
	}

	Ref<GraphicsPipeline> DXContext::CreateGraphicsPipeline() {
		return CreateRef<DXGraphicsPipeline>(*this);
	}

	Ref<ConstantBuffer> DXContext::CreateConstantBuffer(const ConstantBuffer::Descriptor& desc) {
		return CreateRef<DXConstantBuffer>(*this, desc);
	}

	Ref<StructuredBuffer> DXContext::CreateStructuredBuffer(const StructuredBuffer::Descriptor& desc) {
		return CreateRef<DXStructuredBuffer>(*this, desc);
	}

	Ref<Texture2D> DXContext::CreateTexture2D(const Texture2D::Descriptor& descriptor) {
		return CreateRef<DXTexture2D>(*this, descriptor);
	}

	Ref<Texture2DArray> DXContext::CreateTexture2DArray(const Texture2DArray::Descriptor& descriptor) {
		return CreateRef<DXTexture2DArray>(*this, descriptor);
	}

	Ref<TextureCube> DXContext::CreateTextureCube(const TextureCube::Descriptor& descriptor) {
		return CreateRef<DXTextureCube>(*this, descriptor);
	}

	Ref<TextureCubeArray> DXContext::CreateTextureCubeArray(const TextureCubeArray::Descriptor& descriptor) {
		// TODO: Implement DXTextureCubeArray
		return nullptr;
	}

	Ref<RenderPass> DXContext::CreateRenderPass(const RenderPass::Descriptor& desc) {
		return CreateRef<DXRenderPass>(*this, desc);
	}

	Ref<Framebuffer> DXContext::CreateFramebuffer(const Framebuffer::Descriptor& desc) {
		return CreateRef<DXFramebuffer>(*this, desc);
	}

	uint32_t DXContext::GetFrameCount() const {
		return 1;
	}

	uint32_t DXContext::GetCurrentFrameIndex() const {
		return 0;
	}

	Ref<Texture2D> DXContext::GetFrameColorAttachment(uint32_t frameIndex) const {
		return _colorAttachment;
	}

	GraphicsCommandQueue& DXContext::GetCommandQueue() {
		return *_commandQueue;
	}

	void DXContext::Resize(int32_t width, int32_t height) {
		if (!_swapChain) {
			return;
		}

		if (width == 0 || height == 0) {
			return;
		}

		_renderWidth = width;
		_renderHeight = height;
		_colorAttachment.reset();

		_swapChain.Reset();
		if (CreateSwapChain()) {
			return;
		}

		for (const auto& handler : _onResizeHandlers) {
			handler.second(width, height);
		}
	}

	void DXContext::GetSize(int32_t& width, int32_t& height) {
		width = _renderWidth;
		height = _renderHeight;
	}

	uint32_t DXContext::GetMSAASampleCount() const {
		return _msaaSampleCount;
	}

	PixelFormat DXContext::GetSurfaceFormat() const {
		return PixelFormat::RGBA8Unorm;
	}

	PixelFormat DXContext::GetDepthStencilFormat() const {
		return PixelFormat::D24S8_UINT;
	}

	Ref<ComputeShader> DXContext::CreateComputeShader(const ComputeShader::Descriptor& descriptor) {
		return CreateRef<DXComputeShader>(*this, descriptor);
	}

	Ref<ComputePipeline> DXContext::CreateComputePipeline() {
		return CreateRef<DXComputePipeline>(*this);
	}

	void DXContext::AddOnResizeHandler(uint32_t id, const std::function<void(int32_t, int32_t)>& handler) {
		_onResizeHandlers.push_back(handler);
		_onResizeHandlerIters[id] = --_onResizeHandlers.end();
	}

	void DXContext::RemoveOnResizeHandler(uint32_t id) {
		auto iter = _onResizeHandlerIters.find(id);
		if (iter != _onResizeHandlerIters.end()) {
			_onResizeHandlers.erase(iter->second);
			_onResizeHandlerIters.erase(iter);
		}
	}
}

#endif
