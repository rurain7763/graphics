#include "pch.h"

#ifdef _WIN32

#include "DXContext.h"
#include "Platform/PlatformContext.h"
#include "Log/Log.h"
#include "DXVertexBuffer.h"
#include "DXIndexBuffer.h"
#include "DXGraphicsShader.h"
#include "DXGraphicsPipeline.h"
#include "DXCommandQueue.h"
#include "DXConstantBuffer.h"
#include "DXStructuredBuffer.h"
#include "DXTextures.h"
#include "DXComputeShader.h"
#include "DXComputePipeline.h"
#include "DXRenderPass.h"

namespace flaw {
	DXContext::DXContext(PlatformContext& context, int32_t width, int32_t height) {
		_hWnd = wndContext->GetWindowHandle();
		_renderWidth = width;
		_renderHeight = height;

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
			Log::Fatal("D3D11CreateDevice failed");
			return;
		}

		if (CreateSwapChain()) {
			return;
		}

		if (CreateMainRenderPass()) {
			return;
		}

		_currentRenderPass = _mainRenderPass.get();

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

		SetVSync(false);
	}

	DXContext::~DXContext() {
		for (auto samplerState : _samplerStates) {
			samplerState->Release();
		}
	}

	void DXContext::Prepare() {
		_currentRenderPass = _mainRenderPass.get();
		_currentRenderPass->Bind();
	}

	void DXContext::Present() {
		_swapChain->Present(_swapInterval, 0);
	}

	Ref<VertexBuffer> DXContext::CreateVertexBuffer(const VertexBuffer::Descriptor& descriptor) {
		return CreateRef<DXVertexBuffer>(*this, descriptor);
	}

	Ref<IndexBuffer> DXContext::CreateIndexBuffer(const IndexBuffer::Descriptor& descriptor) {
		return CreateRef<DXIndexBuffer>(*this, descriptor);
	}

	Ref<GraphicsShader> DXContext::CreateGraphicsShader(const char* filePath, const uint32_t compileFlag) {
		return CreateRef<DXGraphicsShader>(*this, filePath, compileFlag);
	}

	Ref<GraphicsPipeline> DXContext::CreateGraphicsPipeline() {
		return CreateRef<DXGraphicsPipeline>(*this);
	}

	Ref<ConstantBuffer> DXContext::CreateConstantBuffer(uint32_t size) {
		return CreateRef<DXConstantBuffer>(*this, size);
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

	Ref<GraphicsRenderPass> DXContext::GetMainRenderPass() {
		return _mainRenderPass;
	}

	GraphicsCommandQueue& DXContext::GetCommandQueue() {
		return *_commandQueue;
	}

	Ref<GraphicsRenderPass> DXContext::CreateRenderPass(const GraphicsRenderPass::Descriptor& desc) {
		return CreateRef<DXRenderPass>(*this, desc);
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
	}

	void DXContext::GetSize(int32_t& width, int32_t& height) {
		width = _renderWidth;
		height = _renderHeight;
	}

	int32_t DXContext::CreateSwapChain() {
		ComPtr<IDXGIDevice> dxgiDevice = nullptr;
		ComPtr<IDXGIAdapter> dxgiAdapter = nullptr;
		ComPtr<IDXGIFactory1> dxgiFactory = nullptr;

		_device->QueryInterface(__uuidof(IDXGIDevice), (void**)dxgiDevice.GetAddressOf());
		dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)dxgiAdapter.GetAddressOf());
		dxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void**)dxgiFactory.GetAddressOf());

		DXGI_SWAP_CHAIN_DESC swapchainDesc = {};
		swapchainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapchainDesc.BufferCount = 1;
		swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapchainDesc.BufferDesc.Width = _renderWidth;
		swapchainDesc.BufferDesc.Height = _renderHeight;
		swapchainDesc.OutputWindow = _hWnd;
		swapchainDesc.Windowed = TRUE; // TODO: ���� �����ϵ��� ����
		swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		swapchainDesc.BufferDesc.RefreshRate.Denominator = 1;
		swapchainDesc.BufferDesc.RefreshRate.Numerator = 60;
		swapchainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		swapchainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		swapchainDesc.SampleDesc.Count = 1;
		swapchainDesc.SampleDesc.Quality = 0;

		if (FAILED(dxgiFactory->CreateSwapChain(_device.Get(), &swapchainDesc, _swapChain.GetAddressOf()))) {
			Log::Error("CreateSwapChain failed");
			return -1;
		}

		return 0;
	}

	int32_t DXContext::CreateMainRenderPass() {
		ComPtr<ID3D11Texture2D> backBuffer;
		if (FAILED(_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)backBuffer.GetAddressOf()))) {
			Log::Error("GetBuffer failed");
			return -1;
		}

		GraphicsRenderPass::Descriptor mainRenderPass = {};
		mainRenderPass.renderTargets.resize(1);
		mainRenderPass.renderTargets[0].texture = CreateRef<DXTexture2D>(*this, backBuffer, PixelFormat::RGBA8, BindFlag::RenderTarget);
		mainRenderPass.renderTargets[0].viewportX = 0;
		mainRenderPass.renderTargets[0].viewportY = 0;
		mainRenderPass.renderTargets[0].viewportWidth = _renderWidth;
		mainRenderPass.renderTargets[0].viewportHeight = _renderHeight;
		mainRenderPass.renderTargets[0].clearValue = { 0.0f, 0.0f, 0.0f, 1.0f };
		mainRenderPass.renderTargets[0].resizeFunc = [this](GraphicsRenderTarget& current, int32_t width, int32_t height) {
			current.texture.reset();

			if (FAILED(_swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0))) {
				throw std::runtime_error("ResizeBuffers failed");
			}

			ComPtr<ID3D11Texture2D> backBuffer;
			if (FAILED(_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)backBuffer.GetAddressOf()))) {
				throw std::runtime_error("GetBuffer failed");
			}

			current.texture = CreateRef<DXTexture2D>(*this, backBuffer, PixelFormat::RGBA8, BindFlag::RenderTarget);
			current.viewportWidth = width;
			current.viewportHeight = height;
		};

		Texture2D::Descriptor descDepth = {};
		descDepth.format = PixelFormat::D24S8_UINT;
		descDepth.width = _renderWidth;
		descDepth.height = _renderHeight;
		descDepth.usage = UsageFlag::Static;
		descDepth.bindFlags = BindFlag::DepthStencil;

		mainRenderPass.depthStencil.texture = CreateRef<DXTexture2D>(*this, descDepth);
		mainRenderPass.depthStencil.resizeFunc = [this](GraphicsDepthStencil& current, int32_t width, int32_t height) {
			Texture2D::Descriptor desc = {};
			desc.format = PixelFormat::D24S8_UINT;
			desc.width = width;
			desc.height = height;
			desc.usage = UsageFlag::Static;
			desc.bindFlags = BindFlag::DepthStencil;

			current.texture = CreateTexture2D(desc);
		};

		_mainRenderPass = CreateRef<DXRenderPass>(*this, mainRenderPass);

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

	void DXContext::SetVSync(bool enable) {
		_swapInterval = enable ? 1 : 0;
	}

	Ref<ComputeShader> DXContext::CreateComputeShader(const char* filename) {
		return CreateRef<DXComputeShader>(*this, filename);
	}

	Ref<ComputePipeline> DXContext::CreateComputePipeline() {
		return CreateRef<DXComputePipeline>(*this);
	}
}

#endif
