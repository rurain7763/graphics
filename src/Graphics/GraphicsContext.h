#pragma once

#include "Core.h"
#include "Platform/PlatformContext.h"
#include "GraphicsBuffers.h"
#include "GraphicsShaders.h"
#include "GraphicsPipelines.h"
#include "GraphicsCommandQueue.h"
#include "GraphicsRenderPass.h"
#include "GraphicsFramebuffer.h"
#include "GraphicsTextures.h"

namespace flaw {
	class FAPI GraphicsContext {
	public:
		GraphicsContext() = default;
		virtual ~GraphicsContext() = default;

		virtual bool Prepare() = 0;

		virtual Ref<VertexInputLayout> CreateVertexInputLayout(const VertexInputLayout::Descriptor& descriptor) = 0;

		virtual Ref<VertexBuffer> CreateVertexBuffer(const VertexBuffer::Descriptor& descriptor) = 0;
		virtual Ref<IndexBuffer> CreateIndexBuffer(const IndexBuffer::Descriptor& descriptor) = 0;

		virtual Ref<ShaderResourcesLayout> CreateShaderResourcesLayout(const ShaderResourcesLayout::Descriptor& descriptor) = 0;
		virtual Ref<ShaderResources> CreateShaderResources(const ShaderResources::Descriptor& descriptor) = 0;
		virtual Ref<GraphicsShader> CreateGraphicsShader(const GraphicsShader::Descriptor& descriptor) = 0;
		virtual Ref<GraphicsPipeline> CreateGraphicsPipeline() = 0;

		virtual Ref<ConstantBuffer> CreateConstantBuffer(const ConstantBuffer::Descriptor& desc) = 0;
		virtual Ref<StructuredBuffer> CreateStructuredBuffer(const StructuredBuffer::Descriptor& desc) = 0;

		virtual Ref<Texture2D> CreateTexture2D(const Texture2D::Descriptor& descriptor) = 0;
		virtual Ref<Texture2DArray> CreateTexture2DArray(const Texture2DArray::Descriptor& descriptor) = 0;
		virtual Ref<TextureCube> CreateTextureCube(const TextureCube::Descriptor& descriptor) = 0;

		virtual Ref<GraphicsRenderPassLayout> CreateRenderPassLayout(const GraphicsRenderPassLayout::Descriptor& desc) = 0;
		virtual Ref<GraphicsRenderPass> CreateRenderPass(const GraphicsRenderPass::Descriptor& desc) = 0;
		virtual Ref<GraphicsFramebuffer> CreateFramebuffer(const GraphicsFramebuffer::Descriptor& desc) = 0;

		virtual Ref<GraphicsRenderPassLayout> GetMainRenderPassLayout() = 0;

		virtual uint32_t GetMainFramebuffersCount() const = 0;
		virtual Ref<GraphicsFramebuffer> GetMainFramebuffer(uint32_t index) = 0;

		virtual GraphicsCommandQueue& GetCommandQueue() = 0;
		
		virtual void Resize(int32_t width, int32_t height) = 0;
		virtual void GetSize(int32_t& width, int32_t& height) = 0;
		virtual void SetMSAAState(bool enable) = 0;
		virtual bool GetMSAAState() const = 0;

		virtual Ref<ComputeShader> CreateComputeShader(const ComputeShader::Descriptor& descriptor) = 0;
		virtual Ref<ComputePipeline> CreateComputePipeline() = 0;
	};
}