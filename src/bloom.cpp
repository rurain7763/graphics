#include "pch.h"
#include "world.h"

static Ref<ShaderResourcesLayout> g_bloomSRL;
static Ref<GraphicsPipeline> g_bloomPipeline;

static Ref<GraphicsResourcesPool<ShaderResources>> g_bloomSRPool;

void Bloom_Init() {
	// NOTE: Create bloom shader resources layout
	ShaderResourcesLayout::Descriptor bloomSRLDesc;
	bloomSRLDesc.bindings = {
		{ 0, ResourceType::Texture2D, ShaderStage::Pixel, 1 },
	};

	g_bloomSRL = g_graphicsContext->CreateShaderResourcesLayout(bloomSRLDesc);

	g_bloomSRPool = CreateRef<GraphicsResourcesPool<ShaderResources>>(*g_graphicsContext, [](GraphicsContext& context) {
		ShaderResources::Descriptor desc;
		desc.layout = g_bloomSRL;
		return context.CreateShaderResources(desc);
	});

	// NOTE: Create bloom pipeline
	GraphicsShader::Descriptor bloomShaderDesc;
#if USE_VULKAN
	bloomShaderDesc.vertexShaderFile = "assets/shaders/fullscreen.vert.spv";
	bloomShaderDesc.vertexShaderEntry = "main";
	bloomShaderDesc.pixelShaderFile = "assets/shaders/bloom.frag.spv";
	bloomShaderDesc.pixelShaderEntry = "main";
#elif USE_DX11
	bloomShaderDesc.vertexShaderFile = "assets/shaders/fullscreen.fx";
	bloomShaderDesc.vertexShaderEntry = "VSMain";
	bloomShaderDesc.pixelShaderFile = "assets/shaders/bloom.fx";
	bloomShaderDesc.pixelShaderEntry = "PSMain";
#endif

	auto bloomShader = g_graphicsContext->CreateGraphicsShader(bloomShaderDesc);

	g_bloomPipeline = g_graphicsContext->CreateGraphicsPipeline();
	g_bloomPipeline->SetShader(bloomShader);
	g_bloomPipeline->SetRenderPass(g_bloomRenderPass, 0);
	g_bloomPipeline->EnableBlendMode(0, true);
	g_bloomPipeline->SetBlendMode(0, BlendMode::Default);
	g_bloomPipeline->SetVertexInputLayouts({ g_texturedVertexInputLayout });
	g_bloomPipeline->SetShaderResourcesLayouts({ g_bloomSRL });
	g_bloomPipeline->SetBehaviorStates(GraphicsPipeline::Behavior::AutoResizeViewport | GraphicsPipeline::Behavior::AutoResizeScissor);
}

void Bloom_Cleanup() {
	g_bloomSRL.reset();
	g_bloomPipeline.reset();
	g_bloomSRPool.reset();
}

void Bloom_Render() {
	g_bloomSRPool->Reset();

	auto& commandQueue = g_graphicsContext->GetCommandQueue();

	auto quad = GetMesh("quad");

	auto sceneFramebuffer = g_sceneFramebufferGroup->Get();
	auto brightTex = std::static_pointer_cast<Texture2D>(sceneFramebuffer->GetAttachment(2));

	auto shaderResources = g_bloomSRPool->Get();
	shaderResources->BindTexture2D(brightTex, 0);

	commandQueue.SetPipeline(g_bloomPipeline);
	commandQueue.SetVertexBuffers({ quad->vertexBuffer });
	commandQueue.SetShaderResources({ shaderResources });
	commandQueue.DrawIndexed(quad->indexBuffer, quad->indexBuffer->IndexCount());
}