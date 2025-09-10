#include "pch.h"
#include "world.h"
#include "Platform/PlatformEvents.h"
#include "Event/EventDispatcher.h"
#include "Graphics/Vulkan/VkContext.h"
#include "Graphics/DX11/DXContext.h"
#include "Graphics/GraphicsFunc.h"
#include "Graphics/GraphicsHelper.h"
#include "Time/Time.h"
#include "Math/Math.h"
#include "Image/Image.h"
#include "Model/Model.h"
#include "Input/Input.h"
#include "Log/Log.h"

Ref<PlatformContext> g_context;
EventDispatcher g_eventDispatcher;
Ref<GraphicsContext> g_graphicsContext;
Ref<EngineCamera> g_camera;

std::vector<Object> g_objects;
std::vector<uint32_t> g_outlineObjects;
std::vector<uint32_t> g_viewNormalObjects;
std::vector<uint32_t> g_spriteObjects;

const uint32_t camersConstantsCBBinding = 0;
const uint32_t lightConstantsCBBinding = 1;
const uint32_t materialConstantsCBBinding = 1;
const uint32_t directionalLightSBBinding = 4;
const uint32_t pointLightSBBinding = 5;
const uint32_t spotLightSBBinding = 6;
const uint32_t diffuseTextureBinding = 2;
const uint32_t specularTextureBinding = 3;
const uint32_t skyboxTextureBinding = 4;
const uint32_t shadowMapTextureBinding = 5;
const uint32_t pointShadowMapTextureBinding = 6;
const uint32_t normalTextureBinding = 7;
const uint32_t displacementTextureBinding = 8;

RenderQueue g_meshOnlyRenderQueue;
RenderQueue g_renderQueue;

std::vector<PointLight> g_pointLights;

ShadowMap g_globalShadowMap;
PointLightShadowMap g_pointLightShadowMap;

Ref<RenderPass> g_sceneRenderPass;
Ref<FramebufferGroup> g_sceneFramebufferGroup;
Ref<RenderPass> g_bloomRenderPass;
Ref<FramebufferGroup> g_bloomFramebufferGroup;
Ref<RenderPass> g_postProcessRenderPass;
Ref<FramebufferGroup> g_postProcessFramebufferGroup;

Ref<VertexInputLayout> g_texturedVertexInputLayout;
Ref<VertexInputLayout> g_instanceVertexInputLayout;

Ref<ConstantBuffer> g_cameraCB;
Ref<ConstantBuffer> g_lightCB;
Ref<ConstantBuffer> g_globalCB;
Ref<StructuredBuffer> g_directionalLightSB;
Ref<StructuredBuffer> g_pointLightSB;
Ref<StructuredBuffer> g_spotLightSB;
Ref<GraphicsPipeline> g_objPipeline;
Ref<ShaderResourcesLayout> g_objShaderResourcesLayout;
Ref<ShaderResources> g_objShaderResources;
Ref<ShaderResourcesLayout> g_objDynamicShaderResourcesLayout;

Ref<ShaderResourcesLayout> g_finalizeShaderResourcesLayout;
Ref<GraphicsPipeline> g_finalizePipeline;

Ref<GraphicsResourcesPool<VertexBuffer>> g_instanceVBPool;
Ref<GraphicsResourcesPool<ShaderResources>> g_objDynamicShaderResourcesPool;
Ref<GraphicsResourcesPool<ConstantBuffer>> g_objMaterialCBPool;
Ref<GraphicsResourcesPool<ConstantBuffer>> g_objConstantsCBPool;

Ref<GraphicsResourcesPool<ShaderResources>> g_finalizeDynamicShaderResourcesPool;

void InitBaseResources();

void InitObjectBuffers();
void InitObjectShaderResources();
void InitObjectGraphicsPipeline();

void World_Init() {
    Log::Initialize();

    const int32_t windowWidth = 800;
    const int32_t windowHeight = 600;

    g_context = CreateRef<PlatformContext>("Flaw Application", windowWidth, windowHeight, g_eventDispatcher);
#if USE_VULKAN
    g_graphicsContext = CreateRef<VkContext>(*g_context, windowWidth, windowHeight);
    MathParams::InvertYAxis = true;
    ModelParams::LeftHanded = true;
#elif USE_DX11
	g_graphicsContext = CreateRef<DXContext>(*g_context, windowWidth, windowHeight);
	MathParams::InvertYAxis = false;
	ModelParams::LeftHanded = true;
#endif

	g_camera = CreateRef<EngineCamera>();
	g_camera->SetAspectRatio(static_cast<float>(windowWidth) / windowHeight);

    g_eventDispatcher.Register<WindowResizeEvent>([](const WindowResizeEvent& event) { g_graphicsContext->Resize(event.frameBufferWidth, event.frameBufferHeight); }, 0);
    g_eventDispatcher.Register<KeyPressEvent>([](const KeyPressEvent& event) { Input::OnKeyPress(event.key); }, 0);
    g_eventDispatcher.Register<KeyReleaseEvent>([](const KeyReleaseEvent& event) { Input::OnKeyRelease(event.key); }, 0);
    g_eventDispatcher.Register<MouseMoveEvent>([](const MouseMoveEvent& event) { Input::OnMouseMove(event.x, event.y); }, 0);
    g_eventDispatcher.Register<MousePressEvent>([](const MousePressEvent& event) { Input::OnMousePress(event.button); }, 0);
    g_eventDispatcher.Register<MouseReleaseEvent>([](const MouseReleaseEvent& event) { Input::OnMouseRelease(event.button); }, 0);
    g_eventDispatcher.Register<MouseScrollEvent>([](const MouseScrollEvent& event) { Input::OnMouseScroll(event.xOffset, event.yOffset); }, 0);

	g_globalShadowMap.lightSpaceView = ViewMatrix(vec3(-2.0f, 4.0f, -2.0f), vec3(0.0f));
	g_globalShadowMap.lightSpaceProj = Orthographic(-10.0f, 10.0f, -10.0f, 10.0f, 1.0f, 7.5f);

    InitBaseResources();
    InitObjectBuffers();
    InitObjectShaderResources();
    InitObjectGraphicsPipeline();
}

void InitBaseResources() {
    // NOTE: Create render passes
    RenderPass::Attachment gBufferAttachment;
	gBufferAttachment.format = PixelFormat::RGBA16F;
	gBufferAttachment.sampleCount = g_graphicsContext->GetMSAASampleCount();
	gBufferAttachment.loadOp = AttachmentLoadOp::Clear;
	gBufferAttachment.storeOp = AttachmentStoreOp::DontCare;
	gBufferAttachment.initialLayout = TextureLayout::Undefined;
	gBufferAttachment.finalLayout = TextureLayout::ColorAttachment;

	RenderPass::Attachment depthAttachment;
	depthAttachment.format = g_graphicsContext->GetDepthStencilFormat();
	depthAttachment.sampleCount = g_graphicsContext->GetMSAASampleCount();
	depthAttachment.loadOp = AttachmentLoadOp::Clear;
	depthAttachment.storeOp = AttachmentStoreOp::Store;
	depthAttachment.stencilLoadOp = AttachmentLoadOp::Clear;
	depthAttachment.stencilStoreOp = AttachmentStoreOp::DontCare;
	depthAttachment.initialLayout = TextureLayout::Undefined;
	depthAttachment.finalLayout = TextureLayout::DepthStencilAttachment;

	RenderPass::Attachment resolveAttachment;
	resolveAttachment.format = PixelFormat::RGBA16F;
	resolveAttachment.loadOp = AttachmentLoadOp::DontCare;
	resolveAttachment.storeOp = AttachmentStoreOp::Store;
	resolveAttachment.initialLayout = TextureLayout::Undefined;
	resolveAttachment.finalLayout = TextureLayout::ColorAttachment;

	RenderPass::SubPass sceneSubPass;
	sceneSubPass.colorAttachmentRefs = { {0, TextureLayout::ColorAttachment} };
	sceneSubPass.depthStencilAttachmentRef = { 1, TextureLayout::DepthStencilAttachment };
	sceneSubPass.resolveAttachmentRefs = { {2, TextureLayout::ColorAttachment} };

	RenderPass::Descriptor sceneRenderPassDesc;
	sceneRenderPassDesc.attachments = { gBufferAttachment, depthAttachment, resolveAttachment };
	sceneRenderPassDesc.subpasses = { sceneSubPass };

	g_sceneRenderPass = g_graphicsContext->CreateRenderPass(sceneRenderPassDesc);

	RenderPass::Attachment bloomAttachment;
	bloomAttachment.format = PixelFormat::RGBA16F;
	bloomAttachment.loadOp = AttachmentLoadOp::Clear;
	bloomAttachment.storeOp = AttachmentStoreOp::Store;
	bloomAttachment.initialLayout = TextureLayout::Undefined;
	bloomAttachment.finalLayout = TextureLayout::ColorAttachment;

	RenderPass::SubPass bloomSubPass;
	bloomSubPass.colorAttachmentRefs = { {0, TextureLayout::ColorAttachment} };

	RenderPass::Descriptor bloomRenderPassDesc;
	bloomRenderPassDesc.attachments = { bloomAttachment };
	bloomRenderPassDesc.subpasses = { bloomSubPass };

	g_bloomRenderPass = g_graphicsContext->CreateRenderPass(bloomRenderPassDesc);

	RenderPass::Attachment postProcessAttachment;
	postProcessAttachment.format = g_graphicsContext->GetSurfaceFormat();
	postProcessAttachment.loadOp = AttachmentLoadOp::Clear;
	postProcessAttachment.storeOp = AttachmentStoreOp::Store;
	postProcessAttachment.initialLayout = TextureLayout::Undefined;
	postProcessAttachment.finalLayout = TextureLayout::PresentSource;

	RenderPass::SubPass postProcessSubPass;
	postProcessSubPass.colorAttachmentRefs = { {0, TextureLayout::ColorAttachment} };

	RenderPass::Descriptor postProcessRenderPassDesc;
	postProcessRenderPassDesc.attachments = { postProcessAttachment };
	postProcessRenderPassDesc.subpasses = { postProcessSubPass };

	g_postProcessRenderPass = g_graphicsContext->CreateRenderPass(postProcessRenderPassDesc);

	// NOTE: Create framebuffers
    g_sceneFramebufferGroup = CreateRef<FramebufferGroup>(*g_graphicsContext, [](GraphicsContext& context, uint32_t frameIndex) {
		int32_t width, height;
		context.GetSize(width, height);
		
		Texture2D::Descriptor gBufferDesc;
		gBufferDesc.width = width;
		gBufferDesc.height = height;
		gBufferDesc.format = PixelFormat::RGBA16F;
		gBufferDesc.sampleCount = context.GetMSAASampleCount();
		gBufferDesc.memProperty = MemoryProperty::Static;
		gBufferDesc.texUsages = TextureUsage::ColorAttachment;
		gBufferDesc.initialLayout = TextureLayout::ColorAttachment;

		Ref<Texture2D> gBuffer = context.CreateTexture2D(gBufferDesc);

		Texture2D::Descriptor depthDesc;
		depthDesc.width = width;
		depthDesc.height = height;
		depthDesc.format = context.GetDepthStencilFormat();
		depthDesc.sampleCount = context.GetMSAASampleCount();
		depthDesc.memProperty = MemoryProperty::Static;
		depthDesc.texUsages = TextureUsage::DepthStencilAttachment;
		depthDesc.initialLayout = TextureLayout::DepthStencilAttachment;

		Ref<Texture2D> depth = context.CreateTexture2D(depthDesc);

		Texture2D::Descriptor resolveDesc;
		resolveDesc.width = width;
		resolveDesc.height = height;
		resolveDesc.format = PixelFormat::RGBA16F;
		resolveDesc.memProperty = MemoryProperty::Static;
		resolveDesc.texUsages = TextureUsage::ColorAttachment | TextureUsage::ShaderResource;
		resolveDesc.initialLayout = TextureLayout::ColorAttachment;

		Ref<Texture2D> resolve = context.CreateTexture2D(resolveDesc);

		Framebuffer::Descriptor desc;
		desc.renderPass = g_sceneRenderPass;
		desc.width = width;
		desc.height = height;

		desc.attachments = { gBuffer, depth, resolve };
		desc.resizeHandler = [&context, gBufferDesc, depthDesc, resolveDesc](uint32_t width, uint32_t height, std::vector<Ref<Texture>>& attachments) mutable {
			gBufferDesc.width = width;
			gBufferDesc.height = height;

			depthDesc.width = width;
			depthDesc.height = height;

			resolveDesc.width = width;
			resolveDesc.height = height;

			attachments[0] = context.CreateTexture2D(gBufferDesc);
			attachments[1] = context.CreateTexture2D(depthDesc);
			attachments[2] = context.CreateTexture2D(resolveDesc);
		};

		return context.CreateFramebuffer(desc);
	});

	g_bloomFramebufferGroup = CreateRef<FramebufferGroup>(*g_graphicsContext, [](GraphicsContext& context, uint32_t frameIndex) {
		int32_t width, height;
		context.GetSize(width, height);

		Texture2D::Descriptor bloomDesc;
		bloomDesc.width = width * 0.5;
		bloomDesc.height = height * 0.5;
		bloomDesc.format = PixelFormat::RGBA16F;
		bloomDesc.memProperty = MemoryProperty::Static;
		bloomDesc.texUsages = TextureUsage::ColorAttachment | TextureUsage::ShaderResource;
		bloomDesc.initialLayout = TextureLayout::ColorAttachment;

		Ref<Texture2D> bloom = context.CreateTexture2D(bloomDesc);

		Framebuffer::Descriptor desc;
		desc.renderPass = g_bloomRenderPass;
		desc.width = width * 0.5;
		desc.height = height * 0.5;
		desc.attachments = { bloom };
		desc.resizeHandler = [&context, bloomDesc](uint32_t width, uint32_t height, std::vector<Ref<Texture>>& attachments) mutable {
			bloomDesc.width = width * 0.5;
			bloomDesc.height = height * 0.5;
			
			attachments[0] = context.CreateTexture2D(bloomDesc);
		};

		return context.CreateFramebuffer(desc);
	});

	g_postProcessFramebufferGroup = CreateRef<FramebufferGroup>(*g_graphicsContext, [](GraphicsContext& context, uint32_t frameIndex) {
		int32_t width, height;
		context.GetSize(width, height);

		Framebuffer::Descriptor desc;
		desc.renderPass = g_postProcessRenderPass;
		desc.width = width;
		desc.height = height;
		desc.attachments = { context.GetFrameColorAttachment(frameIndex) };
		desc.resizeHandler = [&context, frameIndex](uint32_t width, uint32_t height, std::vector<Ref<Texture>>& attachments) {
			attachments[0] = context.GetFrameColorAttachment(frameIndex);
		};

		return context.CreateFramebuffer(desc);
	});

	// NOTE: Vertex input layout
    VertexInputLayout::Descriptor vertexInputLayoutDesc;
    vertexInputLayoutDesc.vertexInputRate = VertexInputRate::Vertex;
    vertexInputLayoutDesc.inputElements = {
        { "POSITION", ElementType::Float, 3 },
        { "COLOR", ElementType::Float, 4 },
        { "TEXCOORD", ElementType::Float, 2 },
        { "NORMAL", ElementType::Float, 3 },
		{ "TANGENT", ElementType::Float, 3 }
    };

    g_texturedVertexInputLayout = g_graphicsContext->CreateVertexInputLayout(vertexInputLayoutDesc);

	VertexInputLayout::Descriptor instanceInputLayoutDesc;
	instanceInputLayoutDesc.vertexInputRate = VertexInputRate::Instance;
    instanceInputLayoutDesc.inputElements = {
		{ "MODEL_MATRIX", ElementType::Float, 4 },
		{ "MODEL_MATRIX1", ElementType::Float, 4 },
		{ "MODEL_MATRIX2", ElementType::Float, 4 },
		{ "MODEL_MATRIX3", ElementType::Float, 4 },
		{ "INV_MODEL_MATRIX", ElementType::Float, 4 },
		{ "INV_MODEL_MATRIX1", ElementType::Float, 4 },
		{ "INV_MODEL_MATRIX2", ElementType::Float, 4 },
		{ "INV_MODEL_MATRIX3", ElementType::Float, 4 }
    };

	g_instanceVertexInputLayout = g_graphicsContext->CreateVertexInputLayout(instanceInputLayoutDesc);

	// NOTE: Create instance vertex buffer pool
	g_instanceVBPool = CreateRef<GraphicsResourcesPool<VertexBuffer>>(*g_graphicsContext, [](GraphicsContext& context) {
		VertexBuffer::Descriptor desc;
		desc.memProperty = MemoryProperty::Dynamic;
		desc.elmSize = sizeof(InstanceData);
		desc.bufferSize = sizeof(InstanceData) * MaxInstancingCount;

		return context.CreateVertexBuffer(desc);
	});

	// NOTE: Create buffers
    ConstantBuffer::Descriptor cameraConstantsDesc;
	cameraConstantsDesc.memProperty = MemoryProperty::Dynamic;
	cameraConstantsDesc.bufferSize = sizeof(CameraConstants);
    
    g_cameraCB = g_graphicsContext->CreateConstantBuffer(cameraConstantsDesc);

	ConstantBuffer::Descriptor lightConstantsDesc;
	lightConstantsDesc.memProperty = MemoryProperty::Dynamic;
	lightConstantsDesc.bufferSize = sizeof(LightConstants);

    g_lightCB = g_graphicsContext->CreateConstantBuffer(lightConstantsDesc);

	ConstantBuffer::Descriptor globalConstantsDesc;
	globalConstantsDesc.memProperty = MemoryProperty::Dynamic;
	globalConstantsDesc.bufferSize = sizeof(GlobalConstants);

	g_globalCB = g_graphicsContext->CreateConstantBuffer(globalConstantsDesc);

	StructuredBuffer::Descriptor directionalLightDesc;
	directionalLightDesc.memProperty = MemoryProperty::Dynamic;
	directionalLightDesc.elmSize = sizeof(DirectionalLight);
	directionalLightDesc.bufferSize = sizeof(DirectionalLight) * MAX_DIRECTIONAL_LIGHTS;
	directionalLightDesc.bufferUsages = BufferUsage::ShaderResource;    

	g_directionalLightSB = g_graphicsContext->CreateStructuredBuffer(directionalLightDesc);

	StructuredBuffer::Descriptor pointLightDesc;
	pointLightDesc.memProperty = MemoryProperty::Dynamic;
	pointLightDesc.elmSize = sizeof(PointLight);
	pointLightDesc.bufferSize = sizeof(PointLight) * MAX_POINT_LIGHTS;
	pointLightDesc.bufferUsages = BufferUsage::ShaderResource;

	g_pointLightSB = g_graphicsContext->CreateStructuredBuffer(pointLightDesc);

	StructuredBuffer::Descriptor spotLightDesc;
	spotLightDesc.memProperty = MemoryProperty::Dynamic;
	spotLightDesc.elmSize = sizeof(SpotLight);
	spotLightDesc.bufferSize = sizeof(SpotLight) * MAX_SPOT_LIGHTS;
	spotLightDesc.bufferUsages = BufferUsage::ShaderResource;

	g_spotLightSB = g_graphicsContext->CreateStructuredBuffer(spotLightDesc);

	// NOTE: Create finalize shader resources
	ShaderResourcesLayout::Descriptor finalizeShaderResourceLayoutDesc;
	finalizeShaderResourceLayoutDesc.bindings = {
		{ 0, ResourceType::Texture2D, ShaderStage::Pixel, 1 },
		{ 1, ResourceType::Texture2D, ShaderStage::Pixel, 1 },
	};

	g_finalizeShaderResourcesLayout = g_graphicsContext->CreateShaderResourcesLayout(finalizeShaderResourceLayoutDesc);

	g_finalizeDynamicShaderResourcesPool = CreateRef<GraphicsResourcesPool<ShaderResources>>(*g_graphicsContext, [](GraphicsContext& context) {
		ShaderResources::Descriptor desc;
		desc.layout = g_finalizeShaderResourcesLayout;

		return context.CreateShaderResources(desc);
	});

	// NOTE: Create finalize pipeline
	GraphicsShader::Descriptor finalizeShaderDesc;
#if USE_VULKAN
	finalizeShaderDesc.vertexShaderFile = "assets/shaders/fullscreen.vert.spv";
	finalizeShaderDesc.vertexShaderEntry = "main";
	finalizeShaderDesc.pixelShaderFile = "assets/shaders/finalize.frag.spv";
	finalizeShaderDesc.pixelShaderEntry = "main";
#else
	finalizeShaderDesc.vertexShaderFile = "assets/shaders/fullscreen.fx";
	finalizeShaderDesc.vertexShaderEntry = "VSMain";
	finalizeShaderDesc.pixelShaderFile = "assets/shaders/finalize.fx";
	finalizeShaderDesc.pixelShaderEntry = "PSMain";
#endif

	auto finalizeShader = g_graphicsContext->CreateGraphicsShader(finalizeShaderDesc);

	g_finalizePipeline = g_graphicsContext->CreateGraphicsPipeline();
	g_finalizePipeline->SetShader(finalizeShader);
	g_finalizePipeline->SetVertexInputLayouts({ g_texturedVertexInputLayout });
	g_finalizePipeline->SetRenderPass(g_postProcessRenderPass, 0);
	g_finalizePipeline->EnableBlendMode(0, true);
	g_finalizePipeline->SetBlendMode(0, BlendMode::Default);
	g_finalizePipeline->SetShaderResourcesLayouts({ g_finalizeShaderResourcesLayout });
    g_finalizePipeline->EnableDepthTest(false);
	g_finalizePipeline->SetBehaviorStates(GraphicsPipeline::Behavior::AutoResizeViewport | GraphicsPipeline::Behavior::AutoResizeScissor);
}

void InitObjectBuffers() {
    g_objMaterialCBPool = CreateRef<GraphicsResourcesPool<ConstantBuffer>>(*g_graphicsContext, [](GraphicsContext& context) {
		ConstantBuffer::Descriptor desc;
		desc.memProperty = MemoryProperty::Dynamic;
		desc.bufferSize = sizeof(MaterialConstants);

		return context.CreateConstantBuffer(desc);
    });

    g_objConstantsCBPool = CreateRef<GraphicsResourcesPool<ConstantBuffer>>(*g_graphicsContext, [](GraphicsContext& context) {
		ConstantBuffer::Descriptor desc;
		desc.memProperty = MemoryProperty::Dynamic;
		desc.bufferSize = sizeof(ObjectConstants);

		return context.CreateConstantBuffer(desc);
    });
}

void InitObjectShaderResources() {
    ShaderResourcesLayout::Descriptor shaderResourceLayoutDesc;
    shaderResourceLayoutDesc.bindings = {
        { camersConstantsCBBinding, ResourceType::ConstantBuffer, ShaderStage::Vertex | ShaderStage::Pixel, 1 },
        { lightConstantsCBBinding, ResourceType::ConstantBuffer, ShaderStage::Vertex | ShaderStage::Pixel, 1 },
		{ directionalLightSBBinding, ResourceType::StructuredBuffer, ShaderStage::Pixel, 1 },
		{ pointLightSBBinding, ResourceType::StructuredBuffer, ShaderStage::Pixel, 1 },
		{ spotLightSBBinding, ResourceType::StructuredBuffer, ShaderStage::Pixel, 1 },
    };

    g_objShaderResourcesLayout = g_graphicsContext->CreateShaderResourcesLayout(shaderResourceLayoutDesc);

	ShaderResources::Descriptor shaderResourcesDesc;
	shaderResourcesDesc.layout = g_objShaderResourcesLayout;

	g_objShaderResources = g_graphicsContext->CreateShaderResources(shaderResourcesDesc);
    g_objShaderResources->BindConstantBuffer(g_cameraCB, camersConstantsCBBinding);
    g_objShaderResources->BindConstantBuffer(g_lightCB, lightConstantsCBBinding);
	g_objShaderResources->BindStructuredBuffer(g_directionalLightSB, directionalLightSBBinding);
	g_objShaderResources->BindStructuredBuffer(g_pointLightSB, pointLightSBBinding);
	g_objShaderResources->BindStructuredBuffer(g_spotLightSB, spotLightSBBinding);

	shaderResourceLayoutDesc.bindings = {
        { diffuseTextureBinding, ResourceType::Texture2D, ShaderStage::Pixel, 1 },
        { specularTextureBinding, ResourceType::Texture2D, ShaderStage::Pixel, 1 },
		{ skyboxTextureBinding, ResourceType::TextureCube, ShaderStage::Pixel, 1 },
		{ pointShadowMapTextureBinding, ResourceType::TextureCube, ShaderStage::Pixel, 1 },
		{ shadowMapTextureBinding, ResourceType::Texture2D, ShaderStage::Pixel, 1 },
		{ normalTextureBinding, ResourceType::Texture2D, ShaderStage::Pixel, 1 },
		{ displacementTextureBinding, ResourceType::Texture2D, ShaderStage::Pixel, 1 },
        { materialConstantsCBBinding, ResourceType::ConstantBuffer, ShaderStage::Pixel, 1 },
	};

    g_objDynamicShaderResourcesLayout = g_graphicsContext->CreateShaderResourcesLayout(shaderResourceLayoutDesc);

    g_objDynamicShaderResourcesPool = CreateRef<GraphicsResourcesPool<ShaderResources>>(*g_graphicsContext, [](GraphicsContext& context) {
		ShaderResources::Descriptor desc;
		desc.layout = g_objDynamicShaderResourcesLayout;

		return context.CreateShaderResources(desc);
    });
}

void InitObjectGraphicsPipeline() {
    GraphicsShader::Descriptor shaderDesc;
#if USE_VULKAN
    shaderDesc.vertexShaderFile = "./assets/shaders/shader.vert.spv";
    shaderDesc.vertexShaderEntry = "main";
    shaderDesc.pixelShaderFile = "./assets/shaders/shader.frag.spv";
    shaderDesc.pixelShaderEntry = "main";
#elif USE_DX11
	shaderDesc.vertexShaderFile = "./assets/shaders/shader.fx";
	shaderDesc.vertexShaderEntry = "VSMain";
	shaderDesc.pixelShaderFile = "./assets/shaders/shader.fx";
	shaderDesc.pixelShaderEntry = "PSMain";
#endif

    auto graphicsShader = g_graphicsContext->CreateGraphicsShader(shaderDesc);

    g_objPipeline = g_graphicsContext->CreateGraphicsPipeline();
	g_objPipeline->SetShaderResourcesLayouts({ g_objShaderResourcesLayout, g_objDynamicShaderResourcesLayout });
    g_objPipeline->SetShader(graphicsShader);
    g_objPipeline->SetPrimitiveTopology(PrimitiveTopology::TriangleList);
    g_objPipeline->SetVertexInputLayouts({ g_texturedVertexInputLayout, g_instanceVertexInputLayout });
	g_objPipeline->SetRenderPass(g_sceneRenderPass, 0);
	g_objPipeline->EnableBlendMode(0, true);
	g_objPipeline->SetBlendMode(0, BlendMode::Default);
    g_objPipeline->SetBehaviorStates(GraphicsPipeline::Behavior::AutoResizeViewport | GraphicsPipeline::Behavior::AutoResizeScissor);
}

void World_Cleanup() {
	g_meshOnlyRenderQueue.Clear();
	g_renderQueue.Clear();
    g_objects.clear();
    g_finalizePipeline.reset();
	g_finalizeDynamicShaderResourcesPool.reset();
	g_finalizeShaderResourcesLayout.reset();
	g_instanceVBPool.reset();
    g_objPipeline.reset();
    g_objShaderResources.reset();
    g_objShaderResourcesLayout.reset();
    g_objDynamicShaderResourcesPool.reset();
    g_objDynamicShaderResourcesLayout.reset();
    g_objShaderResources.reset();
    g_objShaderResourcesLayout.reset();
	g_objConstantsCBPool.reset();
    g_objMaterialCBPool.reset();
    g_directionalLightSB.reset();
    g_pointLightSB.reset();
    g_spotLightSB.reset();
    g_lightCB.reset();
    g_cameraCB.reset();
	g_globalCB.reset();
	g_instanceVertexInputLayout.reset();
    g_texturedVertexInputLayout.reset();
	g_postProcessFramebufferGroup.reset();
	g_postProcessRenderPass.reset();
	g_bloomFramebufferGroup.reset();
	g_bloomRenderPass.reset();
	g_sceneRenderPass.reset();
	g_sceneFramebufferGroup.reset();
    g_graphicsContext.reset();
    g_context.reset();

    Log::Info("World resources cleaned up successfully.");

    Log::Cleanup();
}

void World_Update() {
	g_objDynamicShaderResourcesPool->Reset();
	g_objMaterialCBPool->Reset();
	g_instanceVBPool->Reset();
	g_objConstantsCBPool->Reset();
	g_finalizeDynamicShaderResourcesPool->Reset();

    int32_t width, height;
    g_context->GetFrameBufferSize(width, height);

    // NOTE: Gather light datas
    DirectionalLight directionalLight;
    directionalLight.direction = Forward;
    directionalLight.ambient = glm::vec3(0.2f);
    directionalLight.diffuse = glm::vec3(0.8f);
    directionalLight.specular = glm::vec3(1.0f);

	g_pointLights.resize(1);
    for (int32_t i = 0; i < g_pointLights.size(); ++i) {
		PointLight& pointLight = g_pointLights[i];

        if (i == 0) {
            g_pointLights[i].position = vec3(cos(Time::GetTime()), 0, sin(Time::GetTime())) * 2.0f;
			pointLight.diffuse = glm::vec3(10.0, 0.0, 0.0);
        }
        
        pointLight.constant_attenuation = 1.0f;
        pointLight.linear_attenuation = 0.09f;
        pointLight.quadratic_attenuation = 0.032f;
        pointLight.ambient = glm::vec3(0.2f);
        pointLight.specular = glm::vec3(1.0f);
    }

    std::vector<SpotLight> spotLights;
    for (int32_t i = 0; i < spotLights.size(); ++i) {
        if (i == 0) {
            spotLights[i].position = vec3(0, 0, 0);
            spotLights[i].direction = normalize(vec3(sin(Time::GetTime()), 0, abs(cos(Time::GetTime()))));
        }

        spotLights[i].cutoff_inner_cosine = glm::cos(glm::radians(15.f));
        spotLights[i].cutoff_outer_cosine = glm::cos(glm::radians(17.0f));
        spotLights[i].constant_attenuation = 1.0f;
        spotLights[i].linear_attenuation = 0.09f;
        spotLights[i].quadratic_attenuation = 0.032f;
        spotLights[i].ambient = glm::vec3(0.2f);
        spotLights[i].diffuse = glm::vec3(0.8f);
        spotLights[i].specular = glm::vec3(1.0f);
    }

	LightConstants lightConstants;
	lightConstants.light_space_view_matrix = g_globalShadowMap.lightSpaceView;
	lightConstants.light_space_proj_matrix = g_globalShadowMap.lightSpaceProj;
	lightConstants.directional_light_count = 1;
    lightConstants.point_light_count = g_pointLights.size();
    lightConstants.spot_light_count = spotLights.size();
	lightConstants.point_light_far_plane = g_pointLightShadowMap.farPlane;

    g_directionalLightSB->Update(&directionalLight, sizeof(DirectionalLight) * lightConstants.directional_light_count);
    g_pointLightSB->Update(g_pointLights.data(), sizeof(PointLight) * lightConstants.point_light_count);
    g_spotLightSB->Update(spotLights.data(), sizeof(SpotLight) * lightConstants.spot_light_count);
    g_lightCB->Update(&lightConstants, sizeof(LightConstants));

	// NOTE: Update camera
    const float aspectRatio = static_cast<float>(width) / height;
    g_camera->SetAspectRatio(aspectRatio);

    const vec2 nearFar = g_camera->GetNearFarClip();

	CameraConstants cameraConstants;
    cameraConstants.near_plane = nearFar.x;
    cameraConstants.far_plane = nearFar.y;
    cameraConstants.view_matrix = g_camera->GetViewMatrix();
    cameraConstants.projection_matrix = g_camera->GetProjectionMatrix();
    cameraConstants.view_projection_matrix = cameraConstants.projection_matrix * cameraConstants.view_matrix;
    cameraConstants.world_position = g_camera->GetPosition();
    g_cameraCB->Update(&cameraConstants, sizeof(CameraConstants));

	// NOTE: Update global constants
	GlobalConstants globalConstants;
	globalConstants.time = Time::GetTime();
    globalConstants.delta_time = Time::DeltaTime();

	g_globalCB->Update(&globalConstants, sizeof(GlobalConstants));

    static bool initRender = false;

    if (!initRender) {
		g_meshOnlyRenderQueue.Clear();
		g_renderQueue.Clear();
        g_outlineObjects.clear();
        g_viewNormalObjects.clear();
        g_spriteObjects.clear();

		g_meshOnlyRenderQueue.Open();
        g_renderQueue.Open();
        for (uint32_t i = 0; i < g_objects.size(); i++) {
            const auto& obj = g_objects[i];

            if (obj.HasComponent<StaticMeshComponent>()) {
                auto comp = obj.GetComponent<StaticMeshComponent>();

                if (comp->drawOutline) {
                    g_outlineObjects.push_back(i);
                }

                if (comp->drawNormal) {
                    g_viewNormalObjects.push_back(i);
                }

                if (comp->excludeFromRendering) {
                    continue;
                }

				mat4 modelMatrix = ModelMatrix(obj.position, obj.rotation, obj.scale);

				g_meshOnlyRenderQueue.Push(comp->mesh, modelMatrix);

                for (uint32_t i = 0; i < comp->mesh->segments.size(); i++) {
                    g_renderQueue.Push(comp->mesh, i, modelMatrix, comp->mesh->materials[i]);
                }
            }

            if (obj.HasComponent<SpriteComponent>()) {
                g_spriteObjects.push_back(i);
            }
        }
        g_renderQueue.Close();
		g_meshOnlyRenderQueue.Close();

		initRender = true;
    }
}

void World_Render() {
    auto& commandQueue = g_graphicsContext->GetCommandQueue();
	auto shadowMapFramebuffer = g_globalShadowMap.framebufferGroup->Get();
	auto pointShadowMapFramebuffer = g_pointLightShadowMap.framebufferGroup->Get();
	auto shadowMapTex = std::static_pointer_cast<Texture2D>(shadowMapFramebuffer->GetAttachment(0));
	auto pointShadowMapTex = std::static_pointer_cast<TextureCube>(pointShadowMapFramebuffer->GetAttachment(0));

    commandQueue.SetPipeline(g_objPipeline);

	uint32_t instanceOffset = 0;

	auto objInstanceVB = g_instanceVBPool->Get();

	objInstanceVB->Update(g_renderQueue.AllInstanceDatas().data(), sizeof(InstanceData) * g_renderQueue.AllInstanceDatas().size());

    g_renderQueue.Reset();
	while (!g_renderQueue.Empty()) {
		auto& entry = g_renderQueue.Front();
        auto objMaterialCB = g_objMaterialCBPool->Get();

        MaterialConstants materialConstants = GetMaterialConstants(entry.material);

        objMaterialCB->Update(&materialConstants, sizeof(MaterialConstants));

        for (const auto& instancingObj : entry.instancingObjects) {
			const auto& segment = instancingObj.mesh->segments[instancingObj.segmentIndex];

            auto objDynamicResources = g_objDynamicShaderResourcesPool->Get();

            objDynamicResources->BindConstantBuffer(objMaterialCB, materialConstantsCBBinding);

            if (entry.material->diffuseTexture) {
                objDynamicResources->BindTexture2D(entry.material->diffuseTexture, diffuseTextureBinding);
            }
            else {
                objDynamicResources->BindTexture2D(GetTexture2D("dummy"), diffuseTextureBinding);
            }

            if (entry.material->specularTexture) {
                objDynamicResources->BindTexture2D(entry.material->specularTexture, specularTextureBinding);
            }
            else {
                objDynamicResources->BindTexture2D(GetTexture2D("dummy"), specularTextureBinding);
            }

			if (entry.material->normalTexture) {
				objDynamicResources->BindTexture2D(entry.material->normalTexture, normalTextureBinding);
			}
			else {
				objDynamicResources->BindTexture2D(GetTexture2D("dummy"), normalTextureBinding);
			}

			if (entry.material->displacementTexture) {
				objDynamicResources->BindTexture2D(entry.material->displacementTexture, displacementTextureBinding);
			}
			else {
				objDynamicResources->BindTexture2D(GetTexture2D("dummy"), displacementTextureBinding);
			}

            objDynamicResources->BindTextureCube(GetTextureCube("skybox"), skyboxTextureBinding);
			objDynamicResources->BindTexture2D(shadowMapTex, shadowMapTextureBinding);
			objDynamicResources->BindTextureCube(pointShadowMapTex, pointShadowMapTextureBinding);

            commandQueue.SetVertexBuffers({ instancingObj.mesh->vertexBuffer, objInstanceVB });
            commandQueue.SetShaderResources({ g_objShaderResources, objDynamicResources });
            commandQueue.DrawIndexedInstanced(instancingObj.mesh->indexBuffer, segment.indexCount, instancingObj.instanceCount, segment.indexOffset, segment.vertexOffset, instanceOffset);

			instanceOffset += instancingObj.instanceCount;
        }

		g_renderQueue.Next();
	}
}

void World_FinalizeRender() {
    auto& commandQueue = g_graphicsContext->GetCommandQueue();

    auto quad = GetMesh("quad");
	auto sceneFramebuffer = g_sceneFramebufferGroup->Get();
	auto bloomFramebuffer = g_bloomFramebufferGroup->Get();

	auto finalTex = std::static_pointer_cast<Texture2D>(sceneFramebuffer->GetAttachment(2));
	auto bloomTex = std::static_pointer_cast<Texture2D>(bloomFramebuffer->GetAttachment(0));

	auto finalizeSR = g_finalizeDynamicShaderResourcesPool->Get();
	finalizeSR->BindTexture2D(finalTex, 0);
	finalizeSR->BindTexture2D(bloomTex, 1);

	commandQueue.SetPipeline(g_finalizePipeline);
	commandQueue.SetVertexBuffers({ quad->vertexBuffer });
	commandQueue.SetShaderResources({ finalizeSR });
	commandQueue.DrawIndexed(quad->indexBuffer, quad->indexBuffer->IndexCount());

	commandQueue.ResetShaderResources();
}

Object& AddObject() {
    Object object;
    object.position = glm::vec3(0.f);
    object.rotation = glm::vec3(0.f);
    object.scale = glm::vec3(1.f);

    g_objects.push_back(object);

	return g_objects.back();
}

Object& GetObjectWithName(const char* name) {
	for (auto& obj : g_objects) {
		if (obj.name == name) {
			return obj;
		}
	}
	throw std::runtime_error("Object with name " + std::string(name) + " not found.");
}

MaterialConstants GetMaterialConstants(Ref<Material> material) {
	MaterialConstants materialConstants;
	materialConstants.texture_binding_flags = 0;
	materialConstants.diffuseColor = material->diffuseColor;
	materialConstants.specularColor = material->specularColor;
	materialConstants.shininess = material->shininess;

	if (material->diffuseTexture) {
		materialConstants.texture_binding_flags |= static_cast<uint32_t>(TextureBindingFlag::Diffuse);
	}
	if (material->specularTexture) {
		materialConstants.texture_binding_flags |= static_cast<uint32_t>(TextureBindingFlag::Specular);
	}
	if (material->normalTexture) {
		materialConstants.texture_binding_flags |= static_cast<uint32_t>(TextureBindingFlag::Normal);
	}
	if (material->displacementTexture) {
		materialConstants.texture_binding_flags |= static_cast<uint32_t>(TextureBindingFlag::Displacement);
	}

	return materialConstants;
}

std::vector<uint8_t> GenerateTextureCubeData(Image& left, Image& right, Image& top, Image& bottom, Image& front, Image& back) {
    std::vector<uint8_t> textureData;

    uint32_t width = left.Width();
    uint32_t height = left.Height();
    uint32_t channels = left.Channels();

    Image* images[6];

    images[0] = &right; // +X (Right)
    images[1] = &left;  // -X (Left)
    images[2] = &top;   // +Y (Top)
    images[3] = &bottom;// -Y (Bottom)
    images[4] = &front; // +Z (Front)
    images[5] = &back;  // -Z (Back)

    textureData.resize(width * height * 6 * channels);

    uint32_t stride = width * channels * 6;
    for (int i = 0; i < 6; ++i) {
        Image& img = *images[i];
        const auto& imgData = img.Data();
        FASSERT(imgData.size() == width * height * channels, "Image data size does not match expected size for texture cube face.");
        std::copy(imgData.begin(), imgData.end(), textureData.begin() + (i * width * height * channels));
    }
    
    return textureData;
}




