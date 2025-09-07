#include "pch.h"
#include "world.h"

constexpr uint32_t ShadowMapSize = 1024;

struct ShadowConstants {
	mat4 light_space_view;
	mat4 light_space_proj;
};

struct PointLightShadowConstants {
	mat4 light_space_views[6];
	mat4 light_space_proj;
	vec3 light_position;
	float far_plane;
};

static Ref<RenderPass> g_shadowRenderPass;
static Ref<ShaderResourcesLayout> g_shadowShaderResourcesLayout;
static Ref<ShaderResourcesLayout> g_pointShadowShaderResourcesLayout;
static Ref<GraphicsPipeline> g_shadowPipeline;
static Ref<GraphicsPipeline> g_pointLightShadowPipeline;

static Ref<GraphicsResourcesPool<ShaderResources>> g_shadowShaderResourcesPool;
static Ref<GraphicsResourcesPool<ShaderResources>> g_pointShadowShaderResourcesPool;
static Ref<GraphicsResourcesPool<ConstantBuffer>> g_shadowConstantsCBPool;
static Ref<GraphicsResourcesPool<ConstantBuffer>> g_pointLightShadowConstantsCBPool;

void Shadow_Init() {
	// NOTE: Create shadow render pass
	RenderPass::Attachment shadowDepthAttachment;
	shadowDepthAttachment.format = PixelFormat::D32F;
	shadowDepthAttachment.loadOp = AttachmentLoadOp::Clear;
	shadowDepthAttachment.storeOp = AttachmentStoreOp::Store;
	shadowDepthAttachment.stencilLoadOp = AttachmentLoadOp::DontCare;
	shadowDepthAttachment.stencilStoreOp = AttachmentStoreOp::DontCare;
	shadowDepthAttachment.initialLayout = TextureLayout::Undefined;
	shadowDepthAttachment.finalLayout = TextureLayout::DepthStencilAttachment;

	RenderPass::SubPass shadowSubPass;
	shadowSubPass.depthStencilAttachmentRef = { 0, TextureLayout::DepthStencilAttachment };

	RenderPass::Descriptor renderPassDesc;
	renderPassDesc.attachments = { shadowDepthAttachment };
	renderPassDesc.subpasses = { shadowSubPass };

	g_shadowRenderPass = g_graphicsContext->CreateRenderPass(renderPassDesc);

	// NOTE: Create shader resources layout
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

	ShaderResourcesLayout::Descriptor pointLightShadowSRLDesc;
	pointLightShadowSRLDesc.bindings = {
		{ 0, ResourceType::ConstantBuffer, ShaderStage::Geometry | ShaderStage::Pixel, 1 },
	};

	g_pointShadowShaderResourcesLayout = g_graphicsContext->CreateShaderResourcesLayout(pointLightShadowSRLDesc);

	g_pointShadowShaderResourcesPool = CreateRef<GraphicsResourcesPool<ShaderResources>>(*g_graphicsContext, [](GraphicsContext& context) {
		ShaderResources::Descriptor desc;
		desc.layout = g_pointShadowShaderResourcesLayout;

		return context.CreateShaderResources(desc);
	});

	// NOTE: Create buffers
	g_shadowConstantsCBPool = CreateRef<GraphicsResourcesPool<ConstantBuffer>>(*g_graphicsContext, [](GraphicsContext& context) {
		ConstantBuffer::Descriptor desc;
		desc.memProperty = MemoryProperty::Dynamic;
		desc.bufferSize = sizeof(ShadowConstants);

		return context.CreateConstantBuffer(desc);
	});

	g_pointLightShadowConstantsCBPool = CreateRef<GraphicsResourcesPool<ConstantBuffer>>(*g_graphicsContext, [](GraphicsContext& context) {
		ConstantBuffer::Descriptor desc;
		desc.memProperty = MemoryProperty::Dynamic;
		desc.bufferSize = sizeof(PointLightShadowConstants);

		return context.CreateConstantBuffer(desc);
	});

	// NOTE: Create shadow pipeline
	GraphicsShader::Descriptor shadowPipelineShaderDesc;
#if USE_VULKAN
	shadowPipelineShaderDesc.vertexShaderFile = "assets/shaders/shadow.vert.spv";
	shadowPipelineShaderDesc.vertexShaderEntry = "main";
	shadowPipelineShaderDesc.pixelShaderFile = "assets/shaders/shadow.frag.spv";
	shadowPipelineShaderDesc.pixelShaderEntry = "main";
#elif USE_DX11
	shadowPipelineShaderDesc.vertexShaderFile = "assets/shaders/shadow.fx";
	shadowPipelineShaderDesc.vertexShaderEntry = "VSMain";
	shadowPipelineShaderDesc.pixelShaderFile = "assets/shaders/shadow.fx";
	shadowPipelineShaderDesc.pixelShaderEntry = "PSMain";
#endif

	auto shadowShader = g_graphicsContext->CreateGraphicsShader(shadowPipelineShaderDesc);

	g_shadowPipeline = g_graphicsContext->CreateGraphicsPipeline();
	g_shadowPipeline->SetShader(shadowShader);
	g_shadowPipeline->SetRenderPass(g_shadowRenderPass, 0);
	g_shadowPipeline->SetVertexInputLayouts({ g_texturedVertexInputLayout, g_instanceVertexInputLayout });
	g_shadowPipeline->SetShaderResourcesLayouts({ g_shadowShaderResourcesLayout });
	g_shadowPipeline->SetCullMode(CullMode::None);
	g_shadowPipeline->SetViewport(0, 0, ShadowMapSize, ShadowMapSize);
	g_shadowPipeline->SetScissor(0, 0, ShadowMapSize, ShadowMapSize);

	GraphicsShader::Descriptor pointLightShadowPipelineShaderDesc;
#if USE_VULKAN
	pointLightShadowPipelineShaderDesc.vertexShaderFile = "assets/shaders/shadow_point.vert.spv";
	pointLightShadowPipelineShaderDesc.vertexShaderEntry = "main";
	pointLightShadowPipelineShaderDesc.geometryShaderFile = "assets/shaders/shadow_point.geom.spv";
	pointLightShadowPipelineShaderDesc.geometryShaderEntry = "main";
	pointLightShadowPipelineShaderDesc.pixelShaderFile = "assets/shaders/shadow_point.frag.spv";
	pointLightShadowPipelineShaderDesc.pixelShaderEntry = "main";
#elif USE_DX11
	pointLightShadowPipelineShaderDesc.vertexShaderFile = "assets/shaders/shadow_point.fx";
	pointLightShadowPipelineShaderDesc.vertexShaderEntry = "VSMain";
	pointLightShadowPipelineShaderDesc.geometryShaderFile = "assets/shaders/shadow_point.fx";
	pointLightShadowPipelineShaderDesc.geometryShaderEntry = "GSMain";
	pointLightShadowPipelineShaderDesc.pixelShaderFile = "assets/shaders/shadow_point.fx";
	pointLightShadowPipelineShaderDesc.pixelShaderEntry = "PSMain";
#endif

	auto pointLightShadowShader = g_graphicsContext->CreateGraphicsShader(pointLightShadowPipelineShaderDesc);

	g_pointLightShadowPipeline = g_graphicsContext->CreateGraphicsPipeline();
	g_pointLightShadowPipeline->SetShader(pointLightShadowShader);
	g_pointLightShadowPipeline->SetRenderPass(g_shadowRenderPass, 0);
	g_pointLightShadowPipeline->SetVertexInputLayouts({ g_texturedVertexInputLayout, g_instanceVertexInputLayout });
	g_pointLightShadowPipeline->SetShaderResourcesLayouts({ g_pointShadowShaderResourcesLayout });
	g_pointLightShadowPipeline->SetCullMode(CullMode::None);
	g_pointLightShadowPipeline->SetViewport(0, 0, ShadowMapSize, ShadowMapSize);
	g_pointLightShadowPipeline->SetScissor(0, 0, ShadowMapSize, ShadowMapSize);

	// NOTE: set global shadow map info
	g_globalShadowMap.lightSpaceView = ViewMatrix(vec3(0.0f, 0.0f, -5.0f), vec3(0.0f));
	g_globalShadowMap.lightSpaceProj = Orthographic(-10.0f, 10.0f, -10.0f, 10.0f, 1.0f, 30.f);

	g_globalShadowMap.framebufferGroup = CreateRef<FramebufferGroup>(*g_graphicsContext, [](GraphicsContext& context, uint32_t frameIndex) {
		Texture2D::Descriptor depthDesc;
		depthDesc.width = ShadowMapSize;
		depthDesc.height = ShadowMapSize;
		depthDesc.format = PixelFormat::D32F;
		depthDesc.memProperty = MemoryProperty::Static;
		depthDesc.texUsages = TextureUsage::DepthStencilAttachment | TextureUsage::ShaderResource;
		depthDesc.initialLayout = TextureLayout::DepthStencilAttachment;

		Framebuffer::Descriptor desc;
		desc.renderPass = g_shadowRenderPass;
		desc.width = ShadowMapSize;
		desc.height = ShadowMapSize;
		desc.attachments = { context.CreateTexture2D(depthDesc) };

		return context.CreateFramebuffer(desc);
	});

	g_pointLightShadowMap.lightPosition = vec3(0.0f, 0.0f, 0.0f);
	g_pointLightShadowMap.farPlane = 25.0f;
	g_pointLightShadowMap.lightSpaceViews[0] = LookAt(g_pointLightShadowMap.lightPosition, g_pointLightShadowMap.lightPosition + Right, Up);
	g_pointLightShadowMap.lightSpaceViews[1] = LookAt(g_pointLightShadowMap.lightPosition, g_pointLightShadowMap.lightPosition + -Right, Up);
	g_pointLightShadowMap.lightSpaceViews[2] = LookAt(g_pointLightShadowMap.lightPosition, g_pointLightShadowMap.lightPosition + Up, -Forward);
	g_pointLightShadowMap.lightSpaceViews[3] = LookAt(g_pointLightShadowMap.lightPosition, g_pointLightShadowMap.lightPosition + -Up, Forward);
	g_pointLightShadowMap.lightSpaceViews[4] = LookAt(g_pointLightShadowMap.lightPosition, g_pointLightShadowMap.lightPosition + Forward, Up);
	g_pointLightShadowMap.lightSpaceViews[5] = LookAt(g_pointLightShadowMap.lightPosition, g_pointLightShadowMap.lightPosition + -Forward, Up);
	g_pointLightShadowMap.lightSpaceProj = Perspective(glm::radians(90.0f), 1.0f, 1.0f, g_pointLightShadowMap.farPlane);

	g_pointLightShadowMap.framebufferGroup = CreateRef<FramebufferGroup>(*g_graphicsContext, [](GraphicsContext& context, uint32_t frameIndex) {
		TextureCube::Descriptor depthDesc;
		depthDesc.width = ShadowMapSize;
		depthDesc.height = ShadowMapSize;
		depthDesc.format = PixelFormat::D32F;
		depthDesc.memProperty = MemoryProperty::Static;
		depthDesc.texUsages = TextureUsage::DepthStencilAttachment | TextureUsage::ShaderResource;
		depthDesc.initialLayout = TextureLayout::DepthStencilAttachment;

		Framebuffer::Descriptor desc;
		desc.renderPass = g_shadowRenderPass;
		desc.width = ShadowMapSize;
		desc.height = ShadowMapSize;
		desc.layers = 6;
		desc.attachments = { context.CreateTextureCube(depthDesc) };

		return context.CreateFramebuffer(desc);
	});
}

void Shadow_Cleanup() {
	g_shadowRenderPass.reset();
	g_shadowShaderResourcesLayout.reset();
	g_pointShadowShaderResourcesLayout.reset();
	g_shadowPipeline.reset();
	g_pointLightShadowPipeline.reset();
	g_shadowShaderResourcesPool.reset();
	g_pointShadowShaderResourcesPool.reset();
	g_shadowConstantsCBPool.reset();
	g_pointLightShadowConstantsCBPool.reset();
	g_globalShadowMap.framebufferGroup.reset();
	g_pointLightShadowMap.framebufferGroup.reset();
}

void Shadow_Update() {
	g_shadowShaderResourcesPool->Reset();
	g_pointShadowShaderResourcesPool->Reset();
	g_shadowConstantsCBPool->Reset();
	g_pointLightShadowConstantsCBPool->Reset();
}

void Shadow_Render() {
	auto& commandQueue = g_graphicsContext->GetCommandQueue();
	
	// NOTE: Render to shadow map
	auto frameBuffer = g_globalShadowMap.framebufferGroup->Get();
	auto instanceVB = g_instanceVBPool->Get();

	instanceVB->Update(g_meshOnlyRenderQueue.AllInstanceDatas().data(), sizeof(InstanceData) * g_meshOnlyRenderQueue.AllInstanceDatas().size());

	commandQueue.BeginRenderPass(g_shadowRenderPass, frameBuffer);

	commandQueue.SetPipeline(g_shadowPipeline);

	auto shadowSR = g_shadowShaderResourcesPool->Get();
	auto shadowCB = g_shadowConstantsCBPool->Get();

	ShadowConstants shadowConstants;
	shadowConstants.light_space_view = g_globalShadowMap.lightSpaceView;
	shadowConstants.light_space_proj = g_globalShadowMap.lightSpaceProj;

	shadowCB->Update(&shadowConstants, sizeof(ShadowConstants));

	shadowSR->BindConstantBuffer(shadowCB, 0);

	commandQueue.SetShaderResources({ shadowSR });

	uint32_t instanceOffset = 0;

	g_meshOnlyRenderQueue.Reset();
	while (!g_meshOnlyRenderQueue.Empty()) {
		const auto& entry = g_meshOnlyRenderQueue.Front();

		for (const auto& obj : entry.instancingObjects) {
			if (obj.HasSegment()) {
				continue;
			}

			commandQueue.SetVertexBuffers({ obj.mesh->vertexBuffer, instanceVB });
			commandQueue.DrawIndexedInstanced(obj.mesh->indexBuffer, obj.mesh->indexBuffer->IndexCount(), obj.instanceCount, 0, 0, instanceOffset);

			instanceOffset += obj.instanceCount;
		}

		g_meshOnlyRenderQueue.Next();
	}

	commandQueue.EndRenderPass();

	commandQueue.SetPipelineBarrier(
		frameBuffer->GetAttachment(0),
		TextureLayout::DepthStencilAttachment,
		TextureLayout::ShaderReadOnly,
		AccessType::DepthStencilAttachmentWrite,
		AccessType::ShaderRead,
		PipelineStage::EarlyPixelTests,
		PipelineStage::PixelShader
	);

	// NOTE: Render to point light shadow map
	frameBuffer = g_pointLightShadowMap.framebufferGroup->Get();

	commandQueue.BeginRenderPass(g_shadowRenderPass, frameBuffer);

	commandQueue.SetPipeline(g_pointLightShadowPipeline);

	auto pointShadowSR = g_pointShadowShaderResourcesPool->Get();

	auto pointLightShadowCB = g_pointLightShadowConstantsCBPool->Get();

	PointLightShadowConstants pointLightShadowConstants;
	for (uint32_t i = 0; i < 6; i++) {
		pointLightShadowConstants.light_space_views[i] = g_pointLightShadowMap.lightSpaceViews[i];
	}
	pointLightShadowConstants.light_space_proj = g_pointLightShadowMap.lightSpaceProj;
	pointLightShadowConstants.light_position = g_pointLightShadowMap.lightPosition;
	pointLightShadowConstants.far_plane = g_pointLightShadowMap.farPlane;

	pointLightShadowCB->Update(&pointLightShadowConstants, sizeof(PointLightShadowConstants));

	pointShadowSR->BindConstantBuffer(pointLightShadowCB, 0);

	commandQueue.SetShaderResources({ pointShadowSR });

	instanceOffset = 0;
	g_meshOnlyRenderQueue.Reset();
	while (!g_meshOnlyRenderQueue.Empty()) {
		const auto& entry = g_meshOnlyRenderQueue.Front();

		for (const auto& obj : entry.instancingObjects) {
			if (obj.HasSegment()) {
				continue;
			}

			commandQueue.SetVertexBuffers({ obj.mesh->vertexBuffer, instanceVB });
			commandQueue.DrawIndexedInstanced(obj.mesh->indexBuffer, obj.mesh->indexBuffer->IndexCount(), obj.instanceCount, 0, 0, instanceOffset);

			instanceOffset += obj.instanceCount;
		}

		g_meshOnlyRenderQueue.Next();
	}

	commandQueue.EndRenderPass();

	commandQueue.SetPipelineBarrier(
		frameBuffer->GetAttachment(0),
		TextureLayout::DepthStencilAttachment,
		TextureLayout::ShaderReadOnly,
		AccessType::DepthStencilAttachmentWrite,
		AccessType::ShaderRead,
		PipelineStage::EarlyPixelTests,
		PipelineStage::PixelShader
	);
}

