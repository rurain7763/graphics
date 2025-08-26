#pragma once

#include "Core.h"
#include "GraphicsBuffers.h"
#include "GraphicsShaders.h"
#include "GraphicsRenderPass.h"
#include "GraphicsType.h"

namespace flaw {
	class GraphicsPipeline {
	public:
		struct StencilOperation {
			StencilOp failOp = StencilOp::Keep; // the action performed on samples that fail the stencil test.
			StencilOp passOp = StencilOp::Keep; // the action performed on samples that pass both the depth and stencil tests.
			StencilOp depthFailOp = StencilOp::Keep; // the action performed on samples that pass the stencil test and fail the depth test.
			CompareOp compareOp = CompareOp::Always; // the comparison operation for the stencil test.
			uint32_t reference = 0;
			uint32_t writeMask = 0xFF;
			uint32_t compareMask = 0xFF;
		};

		enum Behavior {
			AutoResizeViewport = 1 << 0,
			AutoResizeScissor = 1 << 1,
		};

		GraphicsPipeline() = default;
		virtual ~GraphicsPipeline() = default;

		virtual void SetVertexInputLayouts(const std::vector<Ref<VertexInputLayout>>& vertexInputLayouts) = 0;

		virtual void SetPrimitiveTopology(PrimitiveTopology primitiveTopology) = 0;
		virtual void SetViewport(float x, float y, float width, float height) = 0;
		virtual void SetScissor(int32_t x, int32_t y, int32_t width, int32_t height) = 0;

		virtual void EnableDepthTest(bool enable) = 0;
		virtual void SetDepthTest(CompareOp depthCompareOp, bool depthWrite = true) = 0;

		virtual void SetCullMode(CullMode cullMode) = 0;
		virtual void SetFillMode(FillMode fillMode) = 0;

		virtual void EnableStencilTest(bool enable) = 0;
		virtual void SetStencilTest(const StencilOperation& frontFace, const StencilOperation& backFace) = 0;

		virtual void SetShaderResourcesLayouts(const std::vector<Ref<ShaderResourcesLayout>>& shaderResourceLayouts) = 0;
		virtual void SetShader(const Ref<GraphicsShader>& shader) = 0;

		virtual void SetRenderPassLayout(const Ref<RenderPassLayout>& renderPassLayout) = 0;
		virtual void EnableBlendMode(uint32_t attachmentIndex, bool enable) = 0;
		virtual void SetBlendMode(uint32_t attachmentIndex, BlendMode blendMode) = 0;
		virtual void SetAlphaToCoverage(bool enable) = 0;

		virtual void SetBehaviorStates(uint32_t behaviors) = 0;
		virtual uint32_t GetBehaviorStates() const = 0;
	};

	class ComputePipeline {
	public:
		ComputePipeline() = default;
		virtual ~ComputePipeline() = default;

		virtual void SetShader(const Ref<ComputeShader>& shader) = 0;
		virtual void AddShaderResourcesLayout(const Ref<ShaderResourcesLayout>& shaderResourceLayout) = 0;
	};
}