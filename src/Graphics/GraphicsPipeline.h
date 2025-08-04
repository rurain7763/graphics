#pragma once

#include "Core.h"
#include "GraphicsBuffers.h"
#include "GraphicsShaders.h"
#include "GraphicsRenderPass.h"
#include "GraphicsType.h"

namespace flaw {
	class GraphicsPipeline {
	public:
		enum BehaviorFlag {
			AutoResizeViewport = 1 << 0,
			AutoResizeScissor = 1 << 1,
		};

		GraphicsPipeline() = default;
		virtual ~GraphicsPipeline() = default;

		virtual void SetVertexInputLayout(const Ref<GraphicsVertexInputLayout>& vertexInputLayout) = 0;

		virtual void SetPrimitiveTopology(PrimitiveTopology primitiveTopology) = 0;
		virtual void SetViewport(float x, float y, float width, float height) = 0;
		virtual void SetScissor(int32_t x, int32_t y, int32_t width, int32_t height) = 0;

		virtual void SetDepthTest(DepthTest depthTest, bool depthWrite = true) = 0;
		virtual void SetCullMode(CullMode cullMode) = 0;
		virtual void SetFillMode(FillMode fillMode) = 0;

		virtual void Bind() = 0;

		virtual void AddShaderResourcesLayout(const Ref<ShaderResourcesLayout>& shaderResourceLayout) = 0;
		virtual void SetShader(const Ref<GraphicsShader>& shader) = 0;

		virtual void SetRenderPassLayout(const Ref<GraphicsRenderPassLayout>& renderPassLayout) = 0;

		virtual void SetBehaviorStates(uint32_t flags) = 0;
		virtual uint32_t GetBehaviorStates() const = 0;
	};
}