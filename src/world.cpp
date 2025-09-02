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

#if USE_VULKAN
const uint32_t camersConstantsCBBinding = 0;
const uint32_t lightConstantsCBBinding = 1;
const uint32_t materialConstantsCBBinding = 1;
const uint32_t instanceDataSBBinding = 0;
const uint32_t directionalLightSBBinding = 4;
const uint32_t pointLightSBBinding = 5;
const uint32_t spotLightSBBinding = 6;
const uint32_t diffuseTextureBinding = 2;
const uint32_t specularTextureBinding = 3;
const uint32_t skyboxTextureBinding = 4;
#elif USE_DX11
const uint32_t camersConstantsCBBinding = 0;
const uint32_t lightConstantsCBBinding = 1;
const uint32_t materialConstantsCBBinding = 2;
const uint32_t instanceDataSBBinding = 0;
const uint32_t directionalLightSBBinding = 1;
const uint32_t pointLightSBBinding = 2;
const uint32_t spotLightSBBinding = 3;
const uint32_t diffuseTextureBinding = 4;
const uint32_t specularTextureBinding = 5;
const uint32_t skyboxTextureBinding = 6;
#endif

RenderQueue g_renderQueue;

std::vector<Ref<Framebuffer>> g_sceneFramebuffers;
Ref<RenderPassLayout> g_sceneRenderPassLayout;
Ref<RenderPass> g_sceneClearRenderPass;
Ref<RenderPass> g_sceneLoadRenderPass;

Ref<VertexInputLayout> g_texturedVertexInputLayout;

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

Ref<GraphicsResourcesPool<ShaderResources>> g_objDynamicShaderResourcesPool;
Ref<GraphicsResourcesPool<ConstantBuffer>> g_objMaterialCBPool;
Ref<GraphicsResourcesPool<ConstantBuffer>> g_objConstantsCBPool;
Ref<GraphicsResourcesPool<StructuredBuffer>> g_objInstanceSBPool;

Ref<GraphicsPipeline> g_finalizePipeline;

Ref<GraphicsResourcesPool<ShaderResources>> g_finalizeShaderResourcesPool;

CameraConstants g_cameraConstants;
LightConstants g_lightConstants;

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

    g_cameraConstants.view_matrix = ViewMatrix(vec3(0.f, 0.f, -5.f), vec3(0.f));
    g_cameraConstants.projection_matrix = Perspective(glm::radians(45.0f), static_cast<float>(windowWidth) / windowHeight, 0.1f, 100.0f);

    InitBaseResources();
    InitObjectBuffers();
    InitObjectShaderResources();
    InitObjectGraphicsPipeline();
}

void InitBaseResources() {
    int32_t width, height;
    g_graphicsContext->GetSize(width, height);

    // NOTE: Create render passes
    RenderPassLayout::Descriptor sceneRenderPassLayoutDesc;
	sceneRenderPassLayoutDesc.sampleCount = g_graphicsContext->GetMSAASampleCount();
    sceneRenderPassLayoutDesc.colorAttachments = { { g_graphicsContext->GetSurfaceFormat() } };
    sceneRenderPassLayoutDesc.depthStencilAttachment = { g_graphicsContext->GetDepthStencilFormat() };
	sceneRenderPassLayoutDesc.resolveAttachment = { { g_graphicsContext->GetSurfaceFormat() } };

    g_sceneRenderPassLayout = g_graphicsContext->CreateRenderPassLayout(sceneRenderPassLayoutDesc);

    RenderPass::Descriptor sceneClearRenderPassDesc;
    sceneClearRenderPassDesc.layout = g_sceneRenderPassLayout;
    sceneClearRenderPassDesc.colorAttachmentOps = {
        { TextureLayout::Undefined, TextureLayout::ColorAttachment, AttachmentLoadOp::Clear, AttachmentStoreOp::Store }
    };
    sceneClearRenderPassDesc.depthStencilAttachmentOp = {
        TextureLayout::DepthStencilAttachment, TextureLayout::DepthStencilAttachment, AttachmentLoadOp::Clear, AttachmentStoreOp::Store, AttachmentLoadOp::DontCare, AttachmentStoreOp::DontCare
    };
	sceneClearRenderPassDesc.resolveAttachmentOp = {
		TextureLayout::Undefined, TextureLayout::ColorAttachment, AttachmentLoadOp::Clear, AttachmentStoreOp::Store
	};

    g_sceneClearRenderPass = g_graphicsContext->CreateRenderPass(sceneClearRenderPassDesc);

    RenderPass::Descriptor sceneLoadRenderPassDesc;
    sceneLoadRenderPassDesc.layout = g_sceneRenderPassLayout;
    sceneLoadRenderPassDesc.colorAttachmentOps = {
        { TextureLayout::ColorAttachment, TextureLayout::ColorAttachment, AttachmentLoadOp::Load, AttachmentStoreOp::Store }
    };
    sceneLoadRenderPassDesc.depthStencilAttachmentOp = {
        TextureLayout::DepthStencilAttachment, TextureLayout::DepthStencilAttachment, AttachmentLoadOp::Load, AttachmentStoreOp::Store, AttachmentLoadOp::DontCare, AttachmentStoreOp::DontCare
    };
	sceneLoadRenderPassDesc.resolveAttachmentOp = {
		TextureLayout::ColorAttachment, TextureLayout::ColorAttachment, AttachmentLoadOp::Load, AttachmentStoreOp::Store
	};

	g_sceneLoadRenderPass = g_graphicsContext->CreateRenderPass(sceneLoadRenderPassDesc);

	// NOTE: Create framebuffers
	g_sceneFramebuffers.resize(g_graphicsContext->GetFrameCount());
	for (uint32_t i = 0; i < g_graphicsContext->GetFrameCount(); i++) {
	    Texture2D::Descriptor sceneColorAttachmentDesc;
	    sceneColorAttachmentDesc.width = width;
	    sceneColorAttachmentDesc.height = height;
	    sceneColorAttachmentDesc.format = g_graphicsContext->GetSurfaceFormat();
        sceneColorAttachmentDesc.memProperty = MemoryProperty::Static;
	    sceneColorAttachmentDesc.texUsages = TextureUsage::ColorAttachment;
		sceneColorAttachmentDesc.initialLayout = TextureLayout::ColorAttachment;
        sceneColorAttachmentDesc.sampleCount = g_graphicsContext->GetMSAASampleCount();

	    Ref<Texture2D> sceneColorAttachment = g_graphicsContext->CreateTexture2D(sceneColorAttachmentDesc);

	    Texture2D::Descriptor sceneDepthStencilAttachmentDesc;
	    sceneDepthStencilAttachmentDesc.width = width;
	    sceneDepthStencilAttachmentDesc.height = height;
	    sceneDepthStencilAttachmentDesc.format = g_graphicsContext->GetDepthStencilFormat();
	    sceneDepthStencilAttachmentDesc.memProperty = MemoryProperty::Static;
		sceneDepthStencilAttachmentDesc.initialLayout = TextureLayout::DepthStencilAttachment;
		sceneDepthStencilAttachmentDesc.texUsages = TextureUsage::DepthStencilAttachment;
		sceneDepthStencilAttachmentDesc.sampleCount = g_graphicsContext->GetMSAASampleCount();

	    Ref<Texture2D> sceneDepthStencilAttachment = g_graphicsContext->CreateTexture2D(sceneDepthStencilAttachmentDesc);

		Texture2D::Descriptor sceneResolveAttachmentDesc;
		sceneResolveAttachmentDesc.width = width;
		sceneResolveAttachmentDesc.height = height;
		sceneResolveAttachmentDesc.format = g_graphicsContext->GetSurfaceFormat();
		sceneResolveAttachmentDesc.memProperty = MemoryProperty::Static;
		sceneResolveAttachmentDesc.texUsages = TextureUsage::ColorAttachment | TextureUsage::ShaderResource;
		sceneResolveAttachmentDesc.initialLayout = TextureLayout::ColorAttachment;

		Ref<Texture2D> sceneResolveAttachment = g_graphicsContext->CreateTexture2D(sceneResolveAttachmentDesc);

	    Framebuffer::Descriptor sceneFramebufferDesc;
		sceneFramebufferDesc.renderPassLayout = g_sceneRenderPassLayout;
	    sceneFramebufferDesc.width = width;
	    sceneFramebufferDesc.height = height;
	    sceneFramebufferDesc.colorAttachments = { sceneColorAttachment };
		sceneFramebufferDesc.colorResizeHandler = [](Ref<Texture>& texture, int32_t width, int32_t height) -> bool {
			Texture2D::Descriptor desc;
			desc.width = width;
			desc.height = height;
			desc.format = g_graphicsContext->GetSurfaceFormat();
			desc.memProperty = MemoryProperty::Static;
			desc.texUsages = TextureUsage::ColorAttachment;
			desc.initialLayout = TextureLayout::ColorAttachment;
			desc.sampleCount = g_graphicsContext->GetMSAASampleCount();
			texture = g_graphicsContext->CreateTexture2D(desc);

            return true;
		};
	    sceneFramebufferDesc.depthStencilAttachment = sceneDepthStencilAttachment;
        sceneFramebufferDesc.depthStencilResizeHandler = [](Ref<Texture>& texture, int32_t width, int32_t height) -> bool {
            Texture2D::Descriptor desc;
            desc.width = width;
            desc.height = height;
            desc.format = g_graphicsContext->GetDepthStencilFormat();
            desc.memProperty = MemoryProperty::Static;
            desc.texUsages = TextureUsage::DepthStencilAttachment;
			desc.initialLayout = TextureLayout::DepthStencilAttachment;
			desc.sampleCount = g_graphicsContext->GetMSAASampleCount();
            texture = g_graphicsContext->CreateTexture2D(desc);
        
            return true;
        };

		sceneFramebufferDesc.resolveAttachment = sceneResolveAttachment;
		sceneFramebufferDesc.resolveResizeHandler = [](Ref<Texture>& texture, int32_t width, int32_t height) -> bool {
			Texture2D::Descriptor desc;
			desc.width = width;
			desc.height = height;
			desc.format = g_graphicsContext->GetSurfaceFormat();
			desc.memProperty = MemoryProperty::Static;
			desc.texUsages = TextureUsage::ColorAttachment | TextureUsage::ShaderResource;
			desc.initialLayout = TextureLayout::ColorAttachment;
			texture = g_graphicsContext->CreateTexture2D(desc);

			return true;
		};

	    g_sceneFramebuffers[i] = g_graphicsContext->CreateFramebuffer(sceneFramebufferDesc);
	}

	// NOTE: Vertex input layout
    VertexInputLayout::Descriptor vertexInputLayoutDesc;
    vertexInputLayoutDesc.binding = 0;
    vertexInputLayoutDesc.vertexInputRate = VertexInputRate::Vertex;
    vertexInputLayoutDesc.inputElements = {
        { "POSITION", ElementType::Float, 3 },
        { "COLOR", ElementType::Float, 4 },
        { "TEXCOORD", ElementType::Float, 2 },
        { "NORMAL", ElementType::Float, 3 }
    };

    g_texturedVertexInputLayout = g_graphicsContext->CreateVertexInputLayout(vertexInputLayoutDesc);

	// NOTE: Create buffers
    ConstantBuffer::Descriptor cameraConstantsDesc;
	cameraConstantsDesc.memProperty = MemoryProperty::Dynamic;
	cameraConstantsDesc.bufferSize = sizeof(CameraConstants);
    cameraConstantsDesc.initialData = &g_cameraConstants;
    
    g_cameraCB = g_graphicsContext->CreateConstantBuffer(cameraConstantsDesc);

	ConstantBuffer::Descriptor lightConstantsDesc;
	lightConstantsDesc.memProperty = MemoryProperty::Dynamic;
	lightConstantsDesc.bufferSize = sizeof(LightConstants);
    lightConstantsDesc.initialData = &g_lightConstants;

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
	ShaderResourcesLayout::Descriptor finalizeSRLDesc;
	finalizeSRLDesc.bindings = {
		{ 0, ResourceType::Texture2D, ShaderStage::Pixel, 1 },
	};

	auto finalizeSRL = g_graphicsContext->CreateShaderResourcesLayout(finalizeSRLDesc);

    g_finalizeShaderResourcesPool = CreateRef<GraphicsResourcesPool<ShaderResources>>(*g_graphicsContext, [finalizeSRL](GraphicsContext& context) { 
		ShaderResources::Descriptor desc;
		desc.layout = finalizeSRL;
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
	g_finalizePipeline->SetShaderResourcesLayouts({ finalizeSRL });
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

    g_objInstanceSBPool = CreateRef<GraphicsResourcesPool<StructuredBuffer>>(*g_graphicsContext, [](GraphicsContext& context) {
		StructuredBuffer::Descriptor desc;
		desc.memProperty = MemoryProperty::Dynamic;
		desc.elmSize = sizeof(InstanceData);
		desc.bufferSize = sizeof(InstanceData) * MaxInstancingCount;
		desc.bufferUsages = BufferUsage::ShaderResource;

		return context.CreateStructuredBuffer(desc);
    });
}

void InitObjectShaderResources() {
    ShaderResourcesLayout::Descriptor shaderResourceLayoutDesc;
    shaderResourceLayoutDesc.bindings = {
        { camersConstantsCBBinding, ResourceType::ConstantBuffer, ShaderStage::Vertex | ShaderStage::Pixel, 1 },
        { lightConstantsCBBinding, ResourceType::ConstantBuffer, ShaderStage::Pixel, 1 },
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
        { materialConstantsCBBinding, ResourceType::ConstantBuffer, ShaderStage::Pixel, 1 },
        { instanceDataSBBinding, ResourceType::StructuredBuffer, ShaderStage::Vertex, 1 },
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
    g_objPipeline->SetVertexInputLayouts({ g_texturedVertexInputLayout });
	g_objPipeline->SetRenderPassLayout(g_sceneRenderPassLayout);
    g_objPipeline->SetBehaviorStates(GraphicsPipeline::Behavior::AutoResizeViewport | GraphicsPipeline::Behavior::AutoResizeScissor);
}

void World_Cleanup() {
	g_renderQueue.Clear();
    g_objects.clear();
    g_finalizePipeline.reset();
    g_finalizeShaderResourcesPool.reset();
    g_objPipeline.reset();
    g_objShaderResources.reset();
    g_objShaderResourcesLayout.reset();
    g_objDynamicShaderResourcesPool.reset();
    g_objDynamicShaderResourcesLayout.reset();
    g_objShaderResources.reset();
    g_objShaderResourcesLayout.reset();
    g_objInstanceSBPool.reset();
	g_objConstantsCBPool.reset();
    g_objMaterialCBPool.reset();
    g_directionalLightSB.reset();
    g_pointLightSB.reset();
    g_spotLightSB.reset();
    g_lightCB.reset();
    g_cameraCB.reset();
	g_globalCB.reset();
    g_texturedVertexInputLayout.reset();
	g_sceneLoadRenderPass.reset();
	g_sceneClearRenderPass.reset();
	g_sceneRenderPassLayout.reset();
	g_sceneFramebuffers.clear();
    g_graphicsContext.reset();
    g_context.reset();

    Log::Info("World resources cleaned up successfully.");

    Log::Cleanup();
}

void World_Update() {
	g_objDynamicShaderResourcesPool->Reset();
	g_objMaterialCBPool->Reset();
	g_objConstantsCBPool->Reset();
    g_objInstanceSBPool->Reset();

    int32_t width, height;
    g_context->GetFrameBufferSize(width, height);

    // NOTE: Gather light datas
    DirectionalLight directionalLight;
    directionalLight.direction = Forward;
    directionalLight.ambient = glm::vec3(0.2f);
    directionalLight.diffuse = glm::vec3(0.8f);
    directionalLight.specular = glm::vec3(1.0f);

    std::vector<PointLight> pointLights(2);
    for (int32_t i = 0; i < pointLights.size(); ++i) {
        if (i == 0) {
            pointLights[i].position = vec3(cos(Time::GetTime()), sin(Time::GetTime()), 0) * 2.0f;
        }
        else if (i == 1) {
            pointLights[i].position = vec3(cos(Time::GetTime()), 0, sin(Time::GetTime())) * 2.0f;
        }
        pointLights[i].constant_attenuation = 1.0f;
        pointLights[i].linear_attenuation = 0.09f;
        pointLights[i].quadratic_attenuation = 0.032f;
        pointLights[i].ambient = glm::vec3(0.2f);
        pointLights[i].diffuse = glm::vec3(0.8f);
        pointLights[i].specular = glm::vec3(1.0f);
    }

    std::vector<SpotLight> spotLights(1);
    for (int32_t i = 0; i < spotLights.size(); ++i) {
        if (i == 0) {
            spotLights[i].position = vec3(0, 0, -5);
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

    g_lightConstants.directional_light_count = 1;
    g_lightConstants.point_light_count = pointLights.size();
    g_lightConstants.spot_light_count = spotLights.size();

    g_directionalLightSB->Update(&directionalLight, sizeof(DirectionalLight) * g_lightConstants.directional_light_count);
    g_pointLightSB->Update(pointLights.data(), sizeof(PointLight) * g_lightConstants.point_light_count);
    g_spotLightSB->Update(spotLights.data(), sizeof(SpotLight) * g_lightConstants.spot_light_count);
    g_lightCB->Update(&g_lightConstants, sizeof(LightConstants));

	// NOTE: Update camera
    const float aspectRatio = static_cast<float>(width) / height;
    g_camera->SetAspectRatio(aspectRatio);

    const vec2 nearFar = g_camera->GetNearFarClip();

    g_cameraConstants.near_plane = nearFar.x;
    g_cameraConstants.far_plane = nearFar.y;
    g_cameraConstants.view_matrix = g_camera->GetViewMatrix();
    g_cameraConstants.projection_matrix = g_camera->GetProjectionMatrix();
    g_cameraConstants.view_projection_matrix = g_cameraConstants.projection_matrix * g_cameraConstants.view_matrix;
    g_cameraConstants.world_position = g_camera->GetPosition();
    g_cameraCB->Update(&g_cameraConstants, sizeof(CameraConstants));

	// NOTE: Update global constants
	GlobalConstants globalConstants;
	globalConstants.time = Time::GetTime();
    globalConstants.delta_time = Time::DeltaTime();

	g_globalCB->Update(&globalConstants, sizeof(GlobalConstants));

    static bool initRender = false;

    if (!initRender) {
        g_outlineObjects.clear();
        g_viewNormalObjects.clear();
        g_spriteObjects.clear();

        g_renderQueue.Open();
        for (uint32_t i = 0; i < g_objects.size(); i++) {
            const auto& obj = g_objects[i];

            if (obj.HasComponent<StaticMeshComponent>()) {
                auto comp = obj.GetComponent<StaticMeshComponent>();

                if (comp->drawOutline) {
                    g_outlineObjects.push_back(i);
                }

                if (comp->drawOutline) {
                    g_viewNormalObjects.push_back(i);
                }

                if (comp->excludeFromRendering) {
                    continue;
                }

                for (uint32_t i = 0; i < comp->mesh->segments.size(); i++) {
                    g_renderQueue.Push(comp->mesh, i, ModelMatrix(obj.position, obj.rotation, obj.scale), comp->mesh->materials[i]);
                }
            }

            if (obj.HasComponent<SpriteComponent>()) {
                g_spriteObjects.push_back(i);
            }
        }
        g_renderQueue.Close();

		initRender = true;
    }
}

void World_Render() {
    auto& commandQueue = g_graphicsContext->GetCommandQueue();

    commandQueue.SetPipeline(g_objPipeline);

    auto objInstanceBuffer = g_objInstanceSBPool->Get();
	uint32_t instanceOffset = 0;

    objInstanceBuffer->Update(g_renderQueue.AllInstanceDatas().data(), sizeof(InstanceData)* g_renderQueue.AllInstanceDatas().size());

    g_renderQueue.Reset();
	while (!g_renderQueue.Empty()) {
		auto& entry = g_renderQueue.Front();
        auto objMaterialCB = g_objMaterialCBPool->Get();

        MaterialConstants materialConstants = GetMaterialConstants(entry.material);

        objMaterialCB->Update(&materialConstants, sizeof(MaterialConstants));

        for (const auto& instancingObj : entry.instancingObjects) {
			const auto& segment = instancingObj.mesh->segments[instancingObj.segmentIndex];

            auto objDynamicResources = g_objDynamicShaderResourcesPool->Get();

            objDynamicResources->BindStructuredBuffer(objInstanceBuffer, instanceDataSBBinding);
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

            objDynamicResources->BindTextureCube(GetTextureCube("skybox"), skyboxTextureBinding);

            commandQueue.SetVertexBuffers({ instancingObj.mesh->vertexBuffer });
            commandQueue.SetShaderResources({ g_objShaderResources, objDynamicResources });
            commandQueue.DrawIndexedInstanced(instancingObj.mesh->indexBuffer, segment.indexCount, instancingObj.instanceCount, segment.indexOffset, segment.vertexOffset, instanceOffset);

			instanceOffset += instancingObj.instanceCount;
        }

        for (const auto& skeltalObj : entry.skeletalInstancingObjects) {
			// TODO: Skeletal mesh rendering

			instanceOffset += skeltalObj.instanceCount;
        }

		g_renderQueue.Next();
	}
}

void World_FinalizeRender() {
    g_finalizeShaderResourcesPool->Reset();

    auto& commandQueue = g_graphicsContext->GetCommandQueue();
    auto& framebuffer = g_sceneFramebuffers[commandQueue.GetCurrentFrameIndex()];

    auto quad = GetMesh("quad");
    auto attachment = std::static_pointer_cast<Texture2D>(framebuffer->GetResolveAttachment());
    if (!attachment) {
        attachment = std::static_pointer_cast<Texture2D>(framebuffer->GetColorAttachment(0));
    }

    auto finalizeResources = g_finalizeShaderResourcesPool->Get();
    finalizeResources->BindTexture2D(attachment, 0);

	commandQueue.SetPipeline(g_finalizePipeline);
	commandQueue.SetVertexBuffers({ quad->vertexBuffer });
	commandQueue.SetShaderResources({ finalizeResources });
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




