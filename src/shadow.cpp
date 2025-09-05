#include "pch.h"
#include "world.h"

constexpr uint32_t ShadowMapSize = 1024;

struct ShadowConstants {
	mat4 light_space_view;
	mat4 light_space_proj;
};

struct ShadowMap {
	mat4 lightSpaceView;
	mat4 lightSpaceProj;
	Ref<FramebufferGroup> framebufferGroup;
};

static Ref<RenderPass> g_shadowRenderPass;
static Ref<ShaderResourcesLayout> g_shadowShaderResourcesLayout;
static Ref<GraphicsPipeline> g_shadowPipeline;

static Ref<GraphicsResourcesPool<ShaderResources>> g_shadowShaderResourcesPool;
static Ref<GraphicsResourcesPool<ConstantBuffer>> g_shadowConstantsCBPool;

static ShadowMap g_globalShadowMap;

#if false

void Shadow_Init() {
	// NOTE: Create shadow render pass
	RenderPassLayout::Descriptor shadowRenderPassLayoutDesc;
	shadowRenderPassLayoutDesc.depthStencilAttachment = { PixelFormat::D32F };

	g_shadowRenderPassLayout = g_graphicsContext->CreateRenderPassLayout(shadowRenderPassLayoutDesc);

	RenderPass::Descriptor shadowRenderPassDesc;
	shadowRenderPassDesc.layout = g_shadowRenderPassLayout;
	shadowRenderPassDesc.depthStencilAttachmentOp = {
		TextureLayout::Undefined, TextureLayout::DepthStencilAttachment, AttachmentLoadOp::Clear, AttachmentStoreOp::Store, AttachmentLoadOp::DontCare, AttachmentStoreOp::DontCare
	};

	g_shadowRenderPass = g_graphicsContext->CreateRenderPass(shadowRenderPassDesc);

	// NOTE: Create shadow shader resources layout
	ShaderResourcesLayout::Descriptor shadowSRLDesc;
	shadowSRLDesc.bindings = {
		{ 0, ResourceType::ConstantBuffer, ShaderStage::Vertex, 1 },
	};

	g_shadowShaderResourcesLayout = g_graphicsContext->CreateShaderResourcesLayout(shadowSRLDesc);

	g_shadowShaderResourcesPool = CreateRef<GraphicsResourcesPool<ShaderResources>>(*g_graphicsContext, [](GraphicsContext& context) {
		ShaderResources::Descriptor desc;
		desc.layout = g_shadowShaderResourcesLayout;

		return context.CreateShaderResources(desc);
		});

	// NOTE: Create shadow buffers
	g_shadowConstantsCBPool = CreateRef<GraphicsResourcesPool<ConstantBuffer>>(*g_graphicsContext, [](GraphicsContext& context) {
		ConstantBuffer::Descriptor desc;
		desc.memProperty = MemoryProperty::Dynamic;
		desc.bufferSize = sizeof(ShadowConstants);

		return context.CreateConstantBuffer(desc);
		});

	// NOTE: Create shadow pipeline
	GraphicsShader::Descriptor shadowPipelineShaderDesc;
#if USE_VULKAN
	shadowPipelineShaderDesc.vertexShaderFile = "assets/shaders/shadow.vert.spv";
	shadowPipelineShaderDesc.vertexShaderEntry = "main";
#elif USE_DX11
	shadowPipelineShaderDesc.vertexShaderFile = "assets/shaders/shadow.fx";
	shadowPipelineShaderDesc.vertexShaderEntry = "VSMain";
#endif

	auto shadowShader = g_graphicsContext->CreateGraphicsShader(shadowPipelineShaderDesc);

	g_shadowPipeline = g_graphicsContext->CreateGraphicsPipeline();
	g_shadowPipeline->SetShader(shadowShader);
	g_shadowPipeline->SetVertexInputLayouts({ g_texturedVertexInputLayout, g_instanceVertexInputLayout });
	g_shadowPipeline->SetRenderPass(g_shadowRenderPass);
	g_shadowPipeline->SetShaderResourcesLayouts({ g_shadowShaderResourcesLayout });
	g_shadowPipeline->SetViewport(0, 0, ShadowMapSize, ShadowMapSize);
	g_shadowPipeline->SetScissor(0, 0, ShadowMapSize, ShadowMapSize);

	// NOTE: temp shadow map
	g_globalShadowMap.lightSpaceView = ViewMatrix(vec3(-2.0f, 4.0f, -2.0f), vec3(0.0f));
	g_globalShadowMap.lightSpaceProj = Orthographic(-10.0f, 10.0f, -10.0f, 10.0f, 1.0f, 7.5f);

	Texture2D::Descriptor depthDesc;
	depthDesc.width = ShadowMapSize;
	depthDesc.height = ShadowMapSize;
	depthDesc.format = PixelFormat::D32F;
	depthDesc.memProperty = MemoryProperty::Static;
	depthDesc.texUsages = TextureUsage::DepthStencilAttachment | TextureUsage::ShaderResource;
	depthDesc.initialLayout = TextureLayout::DepthStencilAttachment;

	Ref<Texture2D> depthAttachment = g_graphicsContext->CreateTexture2D(depthDesc);

	Framebuffer::Descriptor desc;
	desc.renderPass = g_shadowRenderPass;
	desc.width = ShadowMapSize;
	desc.height = ShadowMapSize;
	desc.depthStencilAttachment = depthAttachment;

	g_globalShadowMap.framebufferGroup = CreateRef<FramebufferGroup>(*g_graphicsContext, desc);
}

void Shadow_Cleanup() {
	g_shadowRenderPassLayout.reset();
	g_shadowRenderPass.reset();
	g_shadowShaderResourcesLayout.reset();
	g_shadowPipeline.reset();
	g_shadowShaderResourcesPool.reset();
	g_shadowConstantsCBPool.reset();
	g_globalShadowMap.framebufferGroup.reset();
}

void Shadow_Update() {
	g_shadowShaderResourcesPool->Reset();
	g_shadowConstantsCBPool->Reset();
	g_instanceVBPool->Reset();	
}

void Shadow_Render() {
	auto& commandQueue = g_graphicsContext->GetCommandQueue();
	
	auto frameBuffer = g_globalShadowMap.framebufferGroup->Get();

	commandQueue.BeginRenderPass(g_shadowRenderPass, g_shadowRenderPass, frameBuffer);

	commandQueue.SetPipeline(g_shadowPipeline);
	
	auto instanceVB = g_instanceVBPool->Get();

	instanceVB->Update(g_renderQueue.AllInstanceDatas().data(), g_renderQueue.AllInstanceDatas().size());

	auto shadowSR = g_shadowShaderResourcesPool->Get();
	auto shadowCB = g_shadowConstantsCBPool->Get();

	ShadowConstants shadowConstants;
	shadowConstants.light_space_view = g_globalShadowMap.lightSpaceView;
	shadowConstants.light_space_proj = g_globalShadowMap.lightSpaceProj;

	shadowCB->Update(&shadowConstants, sizeof(ShadowConstants));

	shadowSR->BindConstantBuffer(shadowCB, 0);

	commandQueue.SetShaderResources({ shadowSR });

	uint32_t instanceOffset = 0;

	g_renderQueue.Reset();
	while (!g_renderQueue.Empty()) {
		const auto& entry = g_renderQueue.Front();

		for (const auto& obj : entry.instancingObjects) {
			const auto& segment = obj.mesh->segments[obj.segmentIndex];

			commandQueue.SetVertexBuffers({ obj.mesh->vertexBuffer, instanceVB });
			commandQueue.DrawIndexedInstanced(obj.mesh->indexBuffer, segment.indexCount, obj.instanceCount, segment.indexOffset, segment.vertexOffset, instanceOffset);

			instanceOffset += obj.instanceCount;
		}

		for (const auto& obj : entry.skeletalInstancingObjects) {
			const auto& segment = obj.mesh->segments[obj.segmentIndex];

			// TODO: render skeletal object

			instanceOffset += obj.instanceCount;
		}

		g_renderQueue.Next();
	}

	commandQueue.EndRenderPass();

	commandQueue.SetPipelineBarrier(
		frameBuffer->GetDepthStencilAttachment(),
		TextureLayout::DepthStencilAttachment,
		TextureLayout::ShaderReadOnly,
		AccessType::DepthStencilAttachmentWrite,
		AccessType::ShaderRead,
		PipelineStage::EarlyPixelTests,
		PipelineStage::PixelShader
	);
}

#endif