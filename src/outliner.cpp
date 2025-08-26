#include "pch.h"
#include "outliner.h"
#include "world.h"

static Ref<RenderPass> g_renderPass;
static std::vector<std::vector<Ref<ConstantBuffer>>> g_objectConstantsCBsPerFrame;
static uint32_t g_objectConstantsCBUsed = 0;
static Ref<ShaderResourcesLayout> g_staticShaderResourcesLayout;
static Ref<ShaderResources> g_staticShaderResources;
static Ref<ShaderResourcesLayout> g_dynamicShaderResourcesLayout;
static std::vector<std::vector<Ref<ShaderResources>>> g_dynamicShaderResourcesPerFrame;
static uint32_t g_dynamicShaderResourcesUsed = 0;
static Ref<GraphicsPipeline> g_writeStencilPipeline;
static Ref<GraphicsPipeline> g_outlinePipeline;

#if USE_VULKAN
const uint32_t cameraConstantsCBBinding = 0;
const uint32_t objectConstantsCBBinding = 0;
#elif USE_DX11
const uint32_t cameraConstantsCBBinding = 0;
const uint32_t objectConstantsCBBinding = 1;
#endif

void Outliner_Init() {
	// NOTE: Create render pass
	auto mainRenderPassLayout = g_graphicsContext->GetMainRenderPassLayout();

	RenderPass::Descriptor renderPassDesc;
	renderPassDesc.layout = mainRenderPassLayout;

	RenderPass::ColorAttachmentOperation colorOp;
	colorOp.initialLayout = TextureLayout::Present;
	colorOp.finalLayout = TextureLayout::Present;
	colorOp.loadOp = AttachmentLoadOp::Load;
	colorOp.storeOp = AttachmentStoreOp::Store;
	renderPassDesc.colorAttachmentOps.push_back(colorOp);

	RenderPass::DepthStencilAttachmentOperation depthOp;
	depthOp.initialLayout = TextureLayout::DepthStencil;
	depthOp.finalLayout = TextureLayout::DepthStencil;
	depthOp.loadOp = AttachmentLoadOp::Load;
	depthOp.storeOp = AttachmentStoreOp::Store;
	depthOp.stencilLoadOp = AttachmentLoadOp::Clear;
	depthOp.stencilStoreOp = AttachmentStoreOp::Store;
	renderPassDesc.depthStencilAttachmentOp = depthOp;

	g_renderPass = g_graphicsContext->CreateRenderPass(renderPassDesc);

	// NOTE: Create buffers
	g_objectConstantsCBsPerFrame.resize(g_graphicsContext->GetMainFramebuffersCount());

	// NOTE: Create shader resources
	ShaderResourcesLayout::Descriptor staticSRLDesc;
	staticSRLDesc.bindings = {
		{ cameraConstantsCBBinding, ResourceType::ConstantBuffer, ShaderStage::Vertex, 1 },
	};

	g_staticShaderResourcesLayout = g_graphicsContext->CreateShaderResourcesLayout(staticSRLDesc);

	ShaderResources::Descriptor staticSRDesc;
	staticSRDesc.layout = g_staticShaderResourcesLayout;

	g_staticShaderResources = g_graphicsContext->CreateShaderResources(staticSRDesc);
	g_staticShaderResources->BindConstantBuffer(g_cameraCB, cameraConstantsCBBinding);

	ShaderResourcesLayout::Descriptor dynamicSRLDesc;
	dynamicSRLDesc.bindings = {
		{ objectConstantsCBBinding, ResourceType::ConstantBuffer, ShaderStage::Vertex, 1 },
	};

	g_dynamicShaderResourcesLayout = g_graphicsContext->CreateShaderResourcesLayout(dynamicSRLDesc);

	g_dynamicShaderResourcesPerFrame.resize(g_graphicsContext->GetMainFramebuffersCount());

	// NOTE: Create pipelines
	GraphicsShader::Descriptor objectShaderDesc;
#if USE_VULKAN
	objectShaderDesc.vertexShaderFile = "assets/shaders/object.vert.spv";
	objectShaderDesc.vertexShaderEntry = "main";
#else USE_DX11
	objectShaderDesc.vertexShaderFile = "assets/shaders/object.fx";
	objectShaderDesc.vertexShaderEntry = "VSMain";
#endif

	auto objectShader = g_graphicsContext->CreateGraphicsShader(objectShaderDesc);

	GraphicsPipeline::StencilOperation stencilOp;
	stencilOp.failOp = StencilOp::Keep;
	stencilOp.depthFailOp = StencilOp::Keep;
	stencilOp.passOp = StencilOp::Replace;
	stencilOp.compareOp = CompareOp::Always;
	stencilOp.reference = 1;

	g_writeStencilPipeline = g_graphicsContext->CreateGraphicsPipeline();
	g_writeStencilPipeline->SetShader(objectShader);
	g_writeStencilPipeline->EnableDepthTest(false);
	g_writeStencilPipeline->EnableStencilTest(true);
	g_writeStencilPipeline->SetStencilTest(stencilOp, stencilOp);
	g_writeStencilPipeline->EnableBlendMode(0, false);
	g_writeStencilPipeline->SetShaderResourcesLayouts({ g_staticShaderResourcesLayout, g_dynamicShaderResourcesLayout });
	g_writeStencilPipeline->SetVertexInputLayouts({ g_texturedVertexInputLayout });
	g_writeStencilPipeline->SetBehaviorStates(GraphicsPipeline::Behavior::AutoResizeViewport | GraphicsPipeline::Behavior::AutoResizeScissor);

	GraphicsShader::Descriptor outlineShaderDesc;
#if USE_VULKAN
	outlineShaderDesc.vertexShaderFile = "assets/shaders/object.vert.spv";
	outlineShaderDesc.vertexShaderEntry = "main";
	outlineShaderDesc.pixelShaderFile = "assets/shaders/outline.frag.spv";
	outlineShaderDesc.pixelShaderEntry = "main";
#else USE_DX11
	outlineShaderDesc.vertexShaderFile = "assets/shaders/object.fx";
	outlineShaderDesc.vertexShaderEntry = "VSMain";
	outlineShaderDesc.pixelShaderFile = "assets/shaders/outline.fx";
	outlineShaderDesc.pixelShaderEntry = "PSMain";
#endif

	auto outlineShader = g_graphicsContext->CreateGraphicsShader(outlineShaderDesc);

	GraphicsPipeline::StencilOperation outlineStencilOp;
	outlineStencilOp.failOp = StencilOp::Keep;
	outlineStencilOp.depthFailOp = StencilOp::Keep;
	outlineStencilOp.passOp = StencilOp::Keep;
	outlineStencilOp.compareOp = CompareOp::NotEqual;
	outlineStencilOp.reference = 1;

	g_outlinePipeline = g_graphicsContext->CreateGraphicsPipeline();
	g_outlinePipeline->SetShader(outlineShader);
	g_outlinePipeline->EnableStencilTest(true);
	g_outlinePipeline->SetStencilTest(outlineStencilOp, outlineStencilOp);
	g_outlinePipeline->SetShaderResourcesLayouts({ g_staticShaderResourcesLayout, g_dynamicShaderResourcesLayout });
	g_outlinePipeline->SetVertexInputLayouts({ g_texturedVertexInputLayout });
	g_outlinePipeline->SetBehaviorStates(GraphicsPipeline::Behavior::AutoResizeViewport | GraphicsPipeline::Behavior::AutoResizeScissor);
}

void Outliner_Cleanup() {
	g_objectConstantsCBsPerFrame.clear();
	g_staticShaderResourcesLayout.reset();
	g_staticShaderResources.reset();
	g_dynamicShaderResourcesLayout.reset();
	g_dynamicShaderResourcesPerFrame.clear();
	g_writeStencilPipeline.reset();
	g_outlinePipeline.reset();
	g_renderPass.reset();
}

static Ref<ShaderResources> GetDynamicShaderResources(uint32_t frameIndex) {
	if (g_dynamicShaderResourcesUsed >= g_dynamicShaderResourcesPerFrame[frameIndex].size()) {
		ShaderResources::Descriptor shaderResourcesDesc;
		shaderResourcesDesc.layout = g_dynamicShaderResourcesLayout;
		auto shaderResources = g_graphicsContext->CreateShaderResources(shaderResourcesDesc);
		g_dynamicShaderResourcesPerFrame[frameIndex].push_back(shaderResources);
	}

	return g_dynamicShaderResourcesPerFrame[frameIndex][g_dynamicShaderResourcesUsed++];
}

static Ref<ConstantBuffer> GetObjectConstantsCB(uint32_t frameIndex) {
	if (g_objectConstantsCBUsed >= g_objectConstantsCBsPerFrame[frameIndex].size()) {
		ConstantBuffer::Descriptor constantBufferDesc;
		constantBufferDesc.memProperty = MemoryProperty::Dynamic;
		constantBufferDesc.bufferSize = sizeof(ObjectConstants);
		constantBufferDesc.initialData = nullptr;

		auto constantBuffer = g_graphicsContext->CreateConstantBuffer(constantBufferDesc);
		g_objectConstantsCBsPerFrame[frameIndex].push_back(constantBuffer);
	}

	return g_objectConstantsCBsPerFrame[frameIndex][g_objectConstantsCBUsed++];
}

void Outliner_Render() {
	auto& commandQueue = g_graphicsContext->GetCommandQueue();
	uint32_t frameIndex = commandQueue.GetCurrentFrameIndex();
	auto frameBuffer = g_graphicsContext->GetMainFramebuffer(frameIndex);

	g_objectConstantsCBUsed = 0;
	g_dynamicShaderResourcesUsed = 0;

	for (const auto& object : g_objects) {
		if (!object.HasComponent<StaticMeshComponent>()) {
			continue;
		}

		auto meshComp = object.GetComponent<StaticMeshComponent>();
		if (!meshComp->drawOutline) {
			continue;
		}
		
		auto dynamicResources = GetDynamicShaderResources(frameIndex);
		auto objectConstantsCB = GetObjectConstantsCB(frameIndex);

		dynamicResources->BindConstantBuffer(objectConstantsCB, objectConstantsCBBinding);

		ObjectConstants objectConstants;
		objectConstants.model_matrix = ModelMatrix(object.position, object.rotation, object.scale);
		objectConstants.inv_model_matrix = glm::inverse(objectConstants.model_matrix);

		objectConstantsCB->Update(&objectConstants, sizeof(ObjectConstants));

		commandQueue.BeginRenderPass(g_renderPass, g_renderPass, frameBuffer);

		commandQueue.SetPipeline(g_writeStencilPipeline);
		commandQueue.SetVertexBuffers({ meshComp->mesh->vertexBuffer });
		commandQueue.SetShaderResources({ g_staticShaderResources, dynamicResources });
		commandQueue.DrawIndexed(meshComp->mesh->indexBuffer, meshComp->mesh->indexBuffer->IndexCount());

		dynamicResources = GetDynamicShaderResources(frameIndex);
		objectConstantsCB = GetObjectConstantsCB(frameIndex);

		dynamicResources->BindConstantBuffer(objectConstantsCB, objectConstantsCBBinding);

		objectConstants.model_matrix = ModelMatrix(object.position, object.rotation, object.scale * 1.05f);
		objectConstants.inv_model_matrix = glm::inverse(objectConstants.model_matrix);

		objectConstantsCB->Update(&objectConstants, sizeof(ObjectConstants));

		commandQueue.SetPipeline(g_outlinePipeline);
		commandQueue.SetVertexBuffers({ meshComp->mesh->vertexBuffer });
		commandQueue.SetShaderResources({ g_staticShaderResources, dynamicResources });
		commandQueue.DrawIndexed(meshComp->mesh->indexBuffer, meshComp->mesh->indexBuffer->IndexCount());

		commandQueue.EndRenderPass();
	}
}

