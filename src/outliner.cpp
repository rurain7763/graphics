#include "pch.h"
#include "outliner.h"
#include "world.h"

static Ref<RenderPass> g_renderPass;

void Outliner_Init() {
	auto renderPassLayout = g_graphicsContext->GetMainRenderPassLayout();

	RenderPass::Descriptor renderPassDesc;
	renderPassDesc.layout = renderPassLayout;

	RenderPass::ColorAttachmentOperation colorOp;
	colorOp.initialLayout = TextureLayout::Present;
	colorOp.finalLayout = TextureLayout::Present;
	colorOp.loadOp = AttachmentLoadOp::Load;
	colorOp.storeOp = AttachmentStoreOp::Store;
	renderPassDesc.colorAttachmentOps.push_back(colorOp);

	if (renderPassLayout->HasDepthStencilAttachment()) {
		RenderPass::DepthStencilAttachmentOperation depthOp;
		depthOp.initialLayout = TextureLayout::DepthStencil;
		depthOp.finalLayout = TextureLayout::DepthStencil;
		depthOp.loadOp = AttachmentLoadOp::Load;
		depthOp.storeOp = AttachmentStoreOp::Store;
		depthOp.stencilLoadOp = AttachmentLoadOp::Clear;
		depthOp.stencilStoreOp = AttachmentStoreOp::Store;
		renderPassDesc.depthStencilAttachmentOp = depthOp;
	}

	g_renderPass = g_graphicsContext->CreateRenderPass(renderPassDesc);
}

void Outliner_Cleanup() {
	g_renderPass.reset();
}

void Outliner_Render() {
	auto& commandQueue = g_graphicsContext->GetCommandQueue();
	auto frameBuffer = g_graphicsContext->GetMainFramebuffer(commandQueue.GetCurrentFrameIndex());
	
	for (const auto& object : g_objects) {
		commandQueue.BeginRenderPass(g_renderPass, g_renderPass, frameBuffer);

		commandQueue.ResetShaderResources();

		commandQueue.EndRenderPass();
	}
}

