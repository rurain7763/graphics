#include "pch.h"
#include "world.h"

struct SSAOConstants {
	vec4 samples[64];
};

static Ref<RenderPass> g_ssaoRenderPass;
static Ref<FramebufferGroup> g_ssaoFramebufferGroup;
static Ref<RenderPass> g_ssaoBlurRenderPass;
static Ref<FramebufferGroup> g_ssaoBlurFramebufferGroup;

static Ref<Texture2D> g_ssaoNoiseTexture;
static Ref<ConstantBuffer> g_ssaoCB;

static Ref<ShaderResourcesLayout> g_ssaoSRL;
static Ref<GraphicsResourcesPool<ShaderResources>> g_ssaoSRPool;
static Ref<ShaderResourcesLayout> g_ssaoBlurSRL;
static Ref<GraphicsResourcesPool<ShaderResources>> g_ssaoBlurSRPool;

static Ref<GraphicsPipeline> g_ssaoPipeline;
static Ref<GraphicsPipeline> g_ssaoBlurPipeline;

void SSAO_Init() {
	// NOTE: Create render passes
	RenderPass::Descriptor ssaoPassDesc;
	ssaoPassDesc.attachments.resize(1);
	ssaoPassDesc.subpasses.resize(1);

	RenderPass::Attachment& ssaoColorAtt = ssaoPassDesc.attachments[0];
	ssaoColorAtt.format = PixelFormat::R32F;
	ssaoColorAtt.loadOp = AttachmentLoadOp::Clear;
	ssaoColorAtt.storeOp = AttachmentStoreOp::Store;
	ssaoColorAtt.initialLayout = TextureLayout::Undefined;
	ssaoColorAtt.finalLayout = TextureLayout::ColorAttachment;

	RenderPass::SubPass& ssaoSubPass = ssaoPassDesc.subpasses[0];
	ssaoSubPass.colorAttachmentRefs = { {0, TextureLayout::ColorAttachment} };

	g_ssaoRenderPass = g_graphicsContext->CreateRenderPass(ssaoPassDesc);

	RenderPass::Descriptor ssaoBlurPassDesc;
	ssaoBlurPassDesc.attachments.resize(1);
	ssaoBlurPassDesc.subpasses.resize(1);

	RenderPass::Attachment& ssaoBlurColorAtt = ssaoBlurPassDesc.attachments[0];
	ssaoBlurColorAtt.format = PixelFormat::R32F;
	ssaoBlurColorAtt.loadOp = AttachmentLoadOp::Clear;
	ssaoBlurColorAtt.storeOp = AttachmentStoreOp::Store;
	ssaoBlurColorAtt.initialLayout = TextureLayout::Undefined;
	ssaoBlurColorAtt.finalLayout = TextureLayout::ColorAttachment;

	RenderPass::SubPass& ssaoBlurSubPass = ssaoBlurPassDesc.subpasses[0];
	ssaoBlurSubPass.colorAttachmentRefs = { {0, TextureLayout::ColorAttachment} };

	g_ssaoBlurRenderPass = g_graphicsContext->CreateRenderPass(ssaoBlurPassDesc);

	// NOTE: Create framebuffers
	g_ssaoFramebufferGroup = CreateRef<FramebufferGroup>(*g_graphicsContext, [](GraphicsContext& context, uint32_t frameIndex) {
		int32_t width, height;
		context.GetSize(width, height);

		Texture2D::Descriptor ssaoDesc;
		ssaoDesc.width = width;
		ssaoDesc.height = height;
		ssaoDesc.format = PixelFormat::R32F;
		ssaoDesc.memProperty = MemoryProperty::Static;
		ssaoDesc.texUsages = TextureUsage::ColorAttachment | TextureUsage::ShaderResource;
		ssaoDesc.initialLayout = TextureLayout::ColorAttachment;

		Framebuffer::Descriptor desc;
		desc.renderPass = g_ssaoRenderPass;
		desc.width = width;
		desc.height = height;
		desc.attachments = { context.CreateTexture2D(ssaoDesc) };
		desc.resizeHandler = [&context, ssaoDesc](uint32_t width, uint32_t height, std::vector<Ref<Texture>>& attachments) mutable {
			ssaoDesc.width = width;
			ssaoDesc.height = height;

			attachments[0] = context.CreateTexture2D(ssaoDesc);
		};

		return context.CreateFramebuffer(desc);
	});

	g_ssaoBlurFramebufferGroup = CreateRef<FramebufferGroup>(*g_graphicsContext, [](GraphicsContext& context, uint32_t frameIndex) {
		int32_t width, height;
		context.GetSize(width, height);

		Texture2D::Descriptor ssaoBlurDesc;
		ssaoBlurDesc.width = width;
		ssaoBlurDesc.height = height;
		ssaoBlurDesc.format = PixelFormat::R32F;
		ssaoBlurDesc.memProperty = MemoryProperty::Static;
		ssaoBlurDesc.texUsages = TextureUsage::ColorAttachment | TextureUsage::ShaderResource;
		ssaoBlurDesc.initialLayout = TextureLayout::ColorAttachment;

		Framebuffer::Descriptor desc;
		desc.renderPass = g_ssaoBlurRenderPass;
		desc.width = width;
		desc.height = height;
		desc.attachments = { context.CreateTexture2D(ssaoBlurDesc) };
		desc.resizeHandler = [&context, ssaoBlurDesc](uint32_t width, uint32_t height, std::vector<Ref<Texture>>& attachments) mutable {
			ssaoBlurDesc.width = width;
			ssaoBlurDesc.height = height;

			attachments[0] = context.CreateTexture2D(ssaoBlurDesc);
		};

		return context.CreateFramebuffer(desc);
	});

	// NOTE: Create resources
	std::vector<vec4> noise = CalcSSAONoise(4 * 4);
	
	Texture2D::Descriptor noiseDesc;
	noiseDesc.data = (uint8_t*)noise.data();
	noiseDesc.width = 4;
	noiseDesc.height = 4;
	noiseDesc.format = PixelFormat::RGBA32F;
	noiseDesc.memProperty = MemoryProperty::Static;
	noiseDesc.texUsages = TextureUsage::ShaderResource;
	noiseDesc.initialLayout = TextureLayout::ShaderReadOnly;
	noiseDesc.minFilter = FilterMode::Nearest;
	noiseDesc.magFilter = FilterMode::Nearest;
	noiseDesc.wrapModeU = WrapMode::Repeat;
	noiseDesc.wrapModeV = WrapMode::Repeat;

	g_ssaoNoiseTexture = g_graphicsContext->CreateTexture2D(noiseDesc);

	std::vector<vec4> ssaoKernel = CalcSSAOKernel(64);

	SSAOConstants ssaoConstants;
	for (uint32_t i = 0; i < 64; i++) {
		ssaoConstants.samples[i] = ssaoKernel[i];
	}

	ConstantBuffer::Descriptor ssaoCBDesc;
	ssaoCBDesc.memProperty = MemoryProperty::Static;
	ssaoCBDesc.bufferSize = sizeof(ssaoConstants);
	ssaoCBDesc.initialData = &ssaoConstants;
	
	g_ssaoCB = g_graphicsContext->CreateConstantBuffer(ssaoCBDesc);

	// NOTE: Create shader resources layout
	ShaderResourcesLayout::Descriptor ssaoSRLDesc;
	ssaoSRLDesc.bindings = {
		{ 0, ResourceType::ConstantBuffer, ShaderStage::Pixel, 1 }, // camera parameters
		{ 1, ResourceType::ConstantBuffer, ShaderStage::Pixel, 1 }, // ssao parameters
		{ 2, ResourceType::Texture2D, ShaderStage::Pixel, 1 }, // gPosition
		{ 3, ResourceType::Texture2D, ShaderStage::Pixel, 1 }, // gNormal
		{ 4, ResourceType::Texture2D, ShaderStage::Pixel, 1 }, // noise texture
	};

	g_ssaoSRL = g_graphicsContext->CreateShaderResourcesLayout(ssaoSRLDesc);

	g_ssaoSRPool = CreateRef<GraphicsResourcesPool<ShaderResources>>(*g_graphicsContext, [](GraphicsContext& context) {
		ShaderResources::Descriptor desc;
		desc.layout = g_ssaoSRL;
		return context.CreateShaderResources(desc);
	});

	ShaderResourcesLayout::Descriptor ssaoBlurSRLDesc;
	ssaoBlurSRLDesc.bindings = {
		{ 0, ResourceType::Texture2D, ShaderStage::Pixel, 1 }, // ssao texture
	};

	g_ssaoBlurSRL = g_graphicsContext->CreateShaderResourcesLayout(ssaoBlurSRLDesc);

	g_ssaoBlurSRPool = CreateRef<GraphicsResourcesPool<ShaderResources>>(*g_graphicsContext, [](GraphicsContext& context) {
		ShaderResources::Descriptor desc;
		desc.layout = g_ssaoBlurSRL;
		return context.CreateShaderResources(desc);
	});

	// NOTE: Create pipelines
	GraphicsShader::Descriptor ssaoShaderDesc;
	ssaoShaderDesc.vertexShaderFile = "assets/shaders/fullscreen.vert.spv";
	ssaoShaderDesc.vertexShaderEntry = "main";
	ssaoShaderDesc.pixelShaderFile = "assets/shaders/ssao.frag.spv";
	ssaoShaderDesc.pixelShaderEntry = "main";

	auto ssaoShader = g_graphicsContext->CreateGraphicsShader(ssaoShaderDesc);

	g_ssaoPipeline = g_graphicsContext->CreateGraphicsPipeline();
	g_ssaoPipeline->SetShader(ssaoShader);
	g_ssaoPipeline->SetShaderResourcesLayouts({ g_ssaoSRL });
	g_ssaoPipeline->SetVertexInputLayouts({ g_texturedVertexInputLayout });
	g_ssaoPipeline->SetRenderPass(g_ssaoRenderPass, 0);
	g_ssaoPipeline->EnableBlendMode(0, true);
	g_ssaoPipeline->SetBlendMode(0, BlendMode::Default);
	g_ssaoPipeline->EnableDepthTest(false);
	g_ssaoPipeline->SetBehaviorStates(GraphicsPipeline::Behavior::AutoResizeViewport | GraphicsPipeline::Behavior::AutoResizeScissor);

	GraphicsShader::Descriptor ssaoBlurShaderDesc;
	ssaoBlurShaderDesc.vertexShaderFile = "assets/shaders/fullscreen.vert.spv";
	ssaoBlurShaderDesc.vertexShaderEntry = "main";
	ssaoBlurShaderDesc.pixelShaderFile = "assets/shaders/ssao_blur.frag.spv";
	ssaoBlurShaderDesc.pixelShaderEntry = "main";

	auto ssaoBlurShader = g_graphicsContext->CreateGraphicsShader(ssaoBlurShaderDesc);

	g_ssaoBlurPipeline = g_graphicsContext->CreateGraphicsPipeline();
	g_ssaoBlurPipeline->SetShader(ssaoBlurShader);
	g_ssaoBlurPipeline->SetShaderResourcesLayouts({ g_ssaoBlurSRL });
	g_ssaoBlurPipeline->SetVertexInputLayouts({ g_texturedVertexInputLayout });
	g_ssaoBlurPipeline->SetRenderPass(g_ssaoBlurRenderPass, 0);
	g_ssaoBlurPipeline->EnableBlendMode(0, true);
	g_ssaoBlurPipeline->SetBlendMode(0, BlendMode::Default);
	g_ssaoBlurPipeline->EnableDepthTest(false);
	g_ssaoBlurPipeline->SetBehaviorStates(GraphicsPipeline::Behavior::AutoResizeViewport | GraphicsPipeline::Behavior::AutoResizeScissor);
}

void SSAO_Cleanup() {
	g_ssaoRenderPass.reset();
	g_ssaoFramebufferGroup.reset();
	g_ssaoBlurRenderPass.reset();
	g_ssaoBlurFramebufferGroup.reset();
	g_ssaoNoiseTexture.reset();
	g_ssaoCB.reset();
	g_ssaoSRL.reset();
	g_ssaoSRPool.reset();
	g_ssaoBlurSRL.reset();
	g_ssaoBlurSRPool.reset();
	g_ssaoPipeline.reset();
	g_ssaoBlurPipeline.reset();
}

void SSAO_Render() {
	g_ssaoSRPool->Reset();
	g_ssaoBlurSRPool->Reset();

	auto& commandQueue = g_graphicsContext->GetCommandQueue();

	auto ssaoFramebuffer = g_ssaoFramebufferGroup->Get();
	auto ssaoBlurFramebuffer = g_ssaoBlurFramebufferGroup->Get();

	auto gBuffer = GetGBuffer();
	auto quadMesh = GetMesh("quad");

	commandQueue.SetVertexBuffers({ quadMesh->vertexBuffer });

	auto ssaoSR = g_ssaoSRPool->Get();
	ssaoSR->BindConstantBuffer(g_cameraCB, 0);
	ssaoSR->BindConstantBuffer(g_ssaoCB, 1);
	ssaoSR->BindTexture2D(gBuffer.position, 2);
	ssaoSR->BindTexture2D(gBuffer.normal, 3);
	ssaoSR->BindTexture2D(g_ssaoNoiseTexture, 4);

	commandQueue.BeginRenderPass(g_ssaoRenderPass, ssaoFramebuffer);

	commandQueue.SetPipeline(g_ssaoPipeline);
	commandQueue.SetShaderResources({ ssaoSR });
	commandQueue.DrawIndexed(quadMesh->indexBuffer, quadMesh->indexBuffer->IndexCount());

	commandQueue.EndRenderPass();

	commandQueue.SetPipelineBarrier(
		{ ssaoFramebuffer->GetAttachment(0) },
		TextureLayout::ColorAttachment,
		TextureLayout::ShaderReadOnly,
		AccessType::ColorAttachmentWrite,
		AccessType::ShaderRead,
		PipelineStage::ColorAttachmentOutput,
		PipelineStage::PixelShader
	);

	auto ssaoBlurSR = g_ssaoBlurSRPool->Get();
	ssaoBlurSR->BindTexture2D(As<Texture2D>(ssaoFramebuffer->GetAttachment(0)), 0);

	commandQueue.BeginRenderPass(g_ssaoBlurRenderPass, ssaoBlurFramebuffer);

	commandQueue.SetPipeline(g_ssaoBlurPipeline);
	commandQueue.SetShaderResources({ ssaoBlurSR });
	commandQueue.DrawIndexed(quadMesh->indexBuffer, quadMesh->indexBuffer->IndexCount());

	commandQueue.EndRenderPass();

	commandQueue.SetPipelineBarrier(
		{ ssaoBlurFramebuffer->GetAttachment(0) },
		TextureLayout::ColorAttachment,
		TextureLayout::ShaderReadOnly,
		AccessType::ColorAttachmentWrite,
		AccessType::ShaderRead,
		PipelineStage::ColorAttachmentOutput,
		PipelineStage::PixelShader
	);
}

Ref<Texture2D> GetSSAOTexture() {
	return As<Texture2D>(g_ssaoBlurFramebufferGroup->Get()->GetAttachment(0));
}

std::vector<vec4> CalcSSAOKernel(uint32_t kernelCount) {
	std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f); // random floats between 0.0 - 1.0
	std::default_random_engine generator;

	std::vector<vec4> ssaoKernel(kernelCount);
	for (uint32_t i = 0; i < kernelCount; ++i) {
		vec3 sample(
			randomFloats(generator) * 2.0f - 1.0f,
			randomFloats(generator) * 2.0f - 1.0f,
			randomFloats(generator)
		);
		sample = glm::normalize(sample);
		sample *= randomFloats(generator);

		// Scale samples so they're more aligned to the center of the kernel
		float scale = static_cast<float>(i) / static_cast<float>(kernelCount);
		scale = glm::mix(0.1f, 1.0f, scale * scale);
		sample *= scale;
		ssaoKernel[i] = vec4(sample, 0.0f);
	}

	return ssaoKernel;
}

std::vector<vec4> CalcSSAONoise(uint32_t noiseCount) {
	std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f); // random floats between 0.0 - 1.0
	std::default_random_engine generator;

	std::vector<vec4> ssaoNoise(noiseCount);
	for (uint32_t i = 0; i < noiseCount; ++i) {
		vec3 noise(
			randomFloats(generator) * 2.0f - 1.0f,
			randomFloats(generator) * 2.0f - 1.0f,
			0.0f); // rotate around z-axis (in tangent space)
		ssaoNoise[i] = vec4(noise, 0.0f);
	}

	return ssaoNoise;
}