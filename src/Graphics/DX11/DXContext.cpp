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
	DXContext::DXContext(PlatformContext& context, int32_t width, int32_t height) {
		_hWnd = context.GetWindowHandle();
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
			LOG_FATAL("D3D11CreateDevice failed");
			return;
		}

		if (CreateMainRenderPassLayout()) {
			return;
		}

		if (CreateMainRenderPass()) {
			return;
		}

		if (CreateSwapChain()) {
			return;
		}

		if (CreateMainFramebuffer()) {
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

	int32_t DXContext::CreateMainRenderPassLayout() {
		GraphicsRenderPassLayout::ColorAttachment colorAttachment;
		colorAttachment.format = PixelFormat::RGBA8;
		colorAttachment.blendMode = BlendMode::Default;

		GraphicsRenderPassLayout::DepthStencilAttachment depthStencilAttachment;
		depthStencilAttachment.format = PixelFormat::D24S8_UINT;

		GraphicsRenderPassLayout::Descriptor renderPassLayoutDesc;
		renderPassLayoutDesc.type = PipelineType::Graphics;
		renderPassLayoutDesc.sampleCount = 1; // TODO: MSAA 지원 시 변경
		renderPassLayoutDesc.alphaToCoverage = false;
		renderPassLayoutDesc.colorAttachments.resize(1);
		renderPassLayoutDesc.colorAttachments[0] = colorAttachment;
		renderPassLayoutDesc.depthStencilAttachment = depthStencilAttachment;

		_mainRenderPassLayout = CreateRef<DXRenderPassLayout>(*this, renderPassLayoutDesc);

		return 0;
	}

	int32_t DXContext::CreateMainRenderPass() {
		GraphicsRenderPass::ColorAttachmentOperation colorAttachmentOp;
		colorAttachmentOp.initialLayout = TextureLayout::Undefined;
		colorAttachmentOp.finalLayout = TextureLayout::Present;
		colorAttachmentOp.loadOp = AttachmentLoadOp::Clear;
		colorAttachmentOp.storeOp = AttachmentStoreOp::Store;

		GraphicsRenderPass::DepthStencilAttachmentOperation depthStencilAttachmentOp;
		depthStencilAttachmentOp.initialLayout = TextureLayout::Undefined;
		depthStencilAttachmentOp.finalLayout = TextureLayout::DepthStencil;
		depthStencilAttachmentOp.loadOp = AttachmentLoadOp::Clear;
		depthStencilAttachmentOp.storeOp = AttachmentStoreOp::Store;

		GraphicsRenderPass::Descriptor mainRenderPassDesc;
		mainRenderPassDesc.layout = _mainRenderPassLayout;
		mainRenderPassDesc.colorAttachmentOps.resize(1);
		mainRenderPassDesc.colorAttachmentOps[0] = colorAttachmentOp;
		mainRenderPassDesc.depthStencilAttachmentOp = depthStencilAttachmentOp;

		_mainClearRenderPass = CreateRef<DXRenderPass>(*this, mainRenderPassDesc);

		colorAttachmentOp.loadOp = AttachmentLoadOp::Load;
		
		depthStencilAttachmentOp.loadOp = AttachmentLoadOp::Load;

		_mainLoadRenderPass = CreateRef<DXRenderPass>(*this, mainRenderPassDesc);

		return 0;
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

		return 0;
	}

	int32_t DXContext::CreateMainFramebuffer() {
		ComPtr<ID3D11Texture2D> backBuffer;
		if (FAILED(_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)backBuffer.GetAddressOf()))) {
			LOG_ERROR("GetBuffer failed");
			return -1;
		}

		Texture2D::Descriptor descDepth = {};
		descDepth.format = PixelFormat::D24S8_UINT;
		descDepth.width = _renderWidth;
		descDepth.height = _renderHeight;
		descDepth.memProperty = MemoryProperty::Static;
		descDepth.texUsages = TextureUsage::DepthStencil;

		auto depthStencilTex = CreateRef<DXTexture2D>(*this, descDepth);

		GraphicsFramebuffer::Descriptor framebufferDesc;
		framebufferDesc.width = _renderWidth;
		framebufferDesc.height = _renderHeight;
		framebufferDesc.renderPassLayout = _mainRenderPassLayout;
		framebufferDesc.colorAttachments.resize(1);
		framebufferDesc.colorAttachments[0] = CreateRef<DXTexture2D>(*this, backBuffer, PixelFormat::RGBA8, 0);
		framebufferDesc.colorResizeHandler = [this](Ref<Texture>& tex, uint32_t width, uint32_t height) -> bool {
			tex.reset();

			if (FAILED(_swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0))) {
				throw std::runtime_error("ResizeBuffers failed");
			}

			ComPtr<ID3D11Texture2D> backBuffer;
			if (FAILED(_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)backBuffer.GetAddressOf()))) {
				throw std::runtime_error("GetBuffer failed");
			}

			tex = CreateRef<DXTexture2D>(*this, backBuffer, PixelFormat::RGBA8, 0);

			return true;
		};
		framebufferDesc.depthStencilAttachment = depthStencilTex;
		framebufferDesc.depthStencilResizeHandler = [this](Ref<Texture>& tex, uint32_t width, uint32_t height) -> bool {
			tex.reset();

			Texture2D::Descriptor desc = {};
			desc.format = PixelFormat::D24S8_UINT;
			desc.width = width;
			desc.height = height;
			desc.memProperty = MemoryProperty::Static;
			desc.texUsages = TextureUsage::DepthStencil;

			tex = CreateRef<DXTexture2D>(*this, desc);

			return true;
		};

		_mainFramebuffer = CreateRef<DXFramebuffer>(*this, framebufferDesc);

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

		_mainFramebuffer.reset();
		_mainLoadRenderPass.reset();
		_mainClearRenderPass.reset();
		_mainRenderPassLayout.reset();
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

	Ref<GraphicsRenderPassLayout> DXContext::CreateRenderPassLayout(const GraphicsRenderPassLayout::Descriptor& desc) {
		return CreateRef<DXRenderPassLayout>(*this, desc);
	}

	Ref<GraphicsRenderPass> DXContext::CreateRenderPass(const GraphicsRenderPass::Descriptor& desc) {
		return CreateRef<DXRenderPass>(*this, desc);
	}

	Ref<GraphicsFramebuffer> DXContext::CreateFramebuffer(const GraphicsFramebuffer::Descriptor& desc) {
		return CreateRef<DXFramebuffer>(*this, desc);
	}

	Ref<GraphicsRenderPassLayout> DXContext::GetMainRenderPassLayout() {
		return _mainRenderPassLayout;
	}

	uint32_t DXContext::GetMainFramebuffersCount() const {
		return 1;
	}

	Ref<GraphicsFramebuffer> DXContext::GetMainFramebuffer(uint32_t index) {
		if (index != 0) {
			LOG_ERROR("Invalid framebuffer index: %d", index);
			return nullptr;
		}

		return _mainFramebuffer;
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

	void DXContext::SetMSAAState(bool enable) {
		if (_enableMSAA == enable) {
			return;
		}

		_enableMSAA = enable;

		if (_enableMSAA) {
			// TODO: MSAA 설정 변경 로직 추가
		}
		else {
			// TODO: MSAA 비활성화 로직 추가
		}
	}

	bool DXContext::GetMSAAState() const {
		return _enableMSAA;
	}

	Ref<ComputeShader> DXContext::CreateComputeShader(const ComputeShader::Descriptor& descriptor) {
		return CreateRef<DXComputeShader>(*this, descriptor);
	}

	Ref<ComputePipeline> DXContext::CreateComputePipeline() {
		return CreateRef<DXComputePipeline>(*this);
	}

	void DXContext::AddOnResizeHandler(uint32_t id, const std::function<void(int32_t, int32_t)>& handler) {
		_onResizeHandlers[id] = handler;
	}

	void DXContext::RemoveOnResizeHandler(uint32_t id) {
		_onResizeHandlers.erase(id);
	}
}

#endif
