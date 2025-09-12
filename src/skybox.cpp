#include "pch.h"
#include "world.h"
#include "asset.h"

static Ref<ShaderResourcesLayout> g_staticShaderResourcesLayout;
static Ref<ShaderResourcesLayout> g_dynamicShaderResourcesLayout;
static Ref<ShaderResources> g_staticShaderResources;
static Ref<GraphicsResourcesPool<ShaderResources>> g_dynamicShaderResourcesPool;

static Ref<GraphicsPipeline> g_skyboxPipeline;

void Skybox_Init() {
	// NOTE: Create assets
	LoadTextureCube(
		{
			"./assets/textures/sky/left.jpg", 
			"./assets/textures/sky/right.jpg", 
			"./assets/textures/sky/top.jpg", 
			"./assets/textures/sky/bottom.jpg", 
			"./assets/textures/sky/front.jpg", 
			"./assets/textures/sky/back.jpg" 
		}, 
		"skybox"
	);
	
	// NOTE: Create shader resources
	ShaderResourcesLayout::Descriptor staticSRLDesc;
	staticSRLDesc.bindings = {
		{ 0, ResourceType::ConstantBuffer, ShaderStage::Vertex, 1 },
	};

	g_staticShaderResourcesLayout = g_graphicsContext->CreateShaderResourcesLayout(staticSRLDesc);
	
	ShaderResources::Descriptor staticSRDesc;
	staticSRDesc.layout = g_staticShaderResourcesLayout;

	g_staticShaderResources = g_graphicsContext->CreateShaderResources(staticSRDesc);
	g_staticShaderResources->BindConstantBuffer(g_cameraCB, 0);

	ShaderResourcesLayout::Descriptor dynamicSRLDesc;
	dynamicSRLDesc.bindings = {
		{ 0, ResourceType::TextureCube, ShaderStage::Pixel, 1 },
	};

	g_dynamicShaderResourcesLayout = g_graphicsContext->CreateShaderResourcesLayout(dynamicSRLDesc);

	g_dynamicShaderResourcesPool = CreateRef<GraphicsResourcesPool<ShaderResources>>(*g_graphicsContext, [](GraphicsContext& context) {
		ShaderResources::Descriptor desc = { g_dynamicShaderResourcesLayout };
		return context.CreateShaderResources(desc);
	});

	// NOTE: Create pipeline
	GraphicsShader::Descriptor skyboxShaderDesc;
#if USE_VULKAN
	skyboxShaderDesc.vertexShaderFile = "assets/shaders/sky.vert.spv";
	skyboxShaderDesc.vertexShaderEntry = "main";
	skyboxShaderDesc.pixelShaderFile = "assets/shaders/sky.frag.spv";
	skyboxShaderDesc.pixelShaderEntry = "main";
#elif USE_DX11
	skyboxShaderDesc.vertexShaderFile = "assets/shaders/sky.fx";
	skyboxShaderDesc.vertexShaderEntry = "VSMain";
	skyboxShaderDesc.pixelShaderFile = "assets/shaders/sky.fx";
	skyboxShaderDesc.pixelShaderEntry = "PSMain";
#endif

	auto skyboxShader = g_graphicsContext->CreateGraphicsShader(skyboxShaderDesc);

	g_skyboxPipeline = g_graphicsContext->CreateGraphicsPipeline();
	g_skyboxPipeline->SetShader(skyboxShader);
	g_skyboxPipeline->SetCullMode(CullMode::Front);
	g_skyboxPipeline->SetDepthTest(CompareOp::LessEqual, false);
	g_skyboxPipeline->SetShaderResourcesLayouts({ g_staticShaderResourcesLayout, g_dynamicShaderResourcesLayout });
	g_skyboxPipeline->SetRenderPass(g_sceneRenderPass, 0);
	g_skyboxPipeline->EnableBlendMode(0, true);
	g_skyboxPipeline->SetBlendMode(0, BlendMode::Default);
	g_skyboxPipeline->SetVertexInputLayouts({ g_texturedVertexInputLayout });
	g_skyboxPipeline->SetBehaviorStates(GraphicsPipeline::Behavior::AutoResizeViewport | GraphicsPipeline::Behavior::AutoResizeScissor);
}

void Skybox_Cleanup() {
	g_staticShaderResourcesLayout.reset();
	g_dynamicShaderResourcesLayout.reset();
	g_staticShaderResources.reset();
	g_dynamicShaderResourcesPool.reset();
	g_skyboxPipeline.reset();
}

void Skybox_Render() {
	auto& commandQueue = g_graphicsContext->GetCommandQueue();
	auto cubeMesh = GetMesh("cube");
	auto cubemapTex = GetTextureCube("skybox");

	g_dynamicShaderResourcesPool->Reset();

	auto dynmicShaderResources = g_dynamicShaderResourcesPool->Get();
	dynmicShaderResources->BindTextureCube(cubemapTex, 0);

	commandQueue.SetVertexBuffers({ cubeMesh->vertexBuffer });
	commandQueue.SetPipeline(g_skyboxPipeline);
	commandQueue.SetShaderResources({ g_staticShaderResources, dynmicShaderResources });
	for (const auto& subMesh : cubeMesh->segments) {
		commandQueue.DrawIndexed(cubeMesh->indexBuffer, subMesh.indexCount, subMesh.indexOffset, subMesh.vertexOffset);
	}
}

