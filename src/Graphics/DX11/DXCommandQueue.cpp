#include "pch.h"
#include "DXCommandQueue.h"

#ifdef SUPPORT_DX11

#include "DXContext.h"
#include "DXTextures.h"
#include "DXShaders.h"
#include "DXPipelines.h"
#include "DXBuffers.h"
#include "DXFramebuffer.h"
#include "DXRenderPass.h"
#include "Log/Log.h"

namespace flaw {
	DXCommandQueue::DXCommandQueue(DXContext& context)
		: _context(context)
		, _currentGraphicsPipeline(nullptr)
		, _currentShaderResources(nullptr)
		, _needBindGraphicsPipeline(false)
		, _needBindVertexBuffers(false)
		, _needBindShaderResources(false)
	{
		int32_t width, height;
		_context.GetSize(width, height);

		_currentViewport.TopLeftX = 0.0f;
		_currentViewport.TopLeftY = 0.0f;
		_currentViewport.Width = static_cast<float>(width);
		_currentViewport.Height = static_cast<float>(height);
		_currentViewport.MinDepth = 0.0f;
		_currentViewport.MaxDepth = 1.0f;

		_currentScissor.left = 0;
		_currentScissor.top = 0;
		_currentScissor.right = width;
		_currentScissor.bottom = height;
	}

	void DXCommandQueue::SetPipeline(const Ref<GraphicsPipeline>& pipeline) {
		if (pipeline == _currentGraphicsPipeline) {
			return; // Already set
		}

		auto dxPipeline = std::static_pointer_cast<DXGraphicsPipeline>(pipeline);
		FASSERT(dxPipeline, "Pipeline is not a DXGraphicsPipeline");

		_currentGraphicsPipeline = dxPipeline;
		_needBindGraphicsPipeline = true;
	}

	void DXCommandQueue::SetVertexBuffers(const std::vector<Ref<VertexBuffer>>& vertexBuffers) {
		_needBindVertexBuffers = true;

		_currentVertexBuffers.clear();
		_currentDXVertexBuffers.clear();
		_currentDXVertexBufferStrides.clear();
		_currentDXVertexBufferOffsets.clear();
		for (uint32_t i = 0; i < vertexBuffers.size(); i++) {
			auto dxVertexBuffer = std::static_pointer_cast<DXVertexBuffer>(vertexBuffers[i]);
			FASSERT(dxVertexBuffer, "VertexBuffer is not a DXVertexBuffer");

			_currentVertexBuffers.push_back(dxVertexBuffer);
			_currentDXVertexBuffers.push_back(dxVertexBuffer->GetNativeDXBuffer().Get());
			_currentDXVertexBufferStrides.push_back(dxVertexBuffer->ElementSize());
			_currentDXVertexBufferOffsets.push_back(0); // Offset is always 0 for now
		}
	}

	void DXCommandQueue::SetShaderResources(const std::vector<Ref<ShaderResources>>& shaderResources) {
		if (shaderResources.size() > 1) {
			LOG_ERROR("DX11 only support 1 set shader resource");
			return;
		}

		if (shaderResources[0] == _currentShaderResources) {
			return; // Already set
		}
		
		auto dxShaderResources = std::static_pointer_cast<DXShaderResources>(shaderResources[0]);
		FASSERT(dxShaderResources, "ShaderResources is not a DXShaderResources");

		_currentShaderResources = dxShaderResources;
		_needBindShaderResources = true;
	}

	void DXCommandQueue::BindRenderResources() {
		if (_needBindGraphicsPipeline) {
			auto shader = _currentGraphicsPipeline->GetDXShader();
			auto inputLayout = _currentGraphicsPipeline->GetDXInputLayout();
			auto primitiveTopology = _currentGraphicsPipeline->GetDXPrimitiveTopology();
			auto depthStencilState = _currentGraphicsPipeline->GetDXDepthStencilState();
			auto rasterizerState = _currentGraphicsPipeline->GetDXRasterizerState();
			auto blendState = _currentGraphicsPipeline->GetDXBlendState();

			_context.DeviceContext()->VSSetShader(shader->GetNativeDXVertexShader().Get(), nullptr, 0);
			_context.DeviceContext()->PSSetShader(shader->GetPixelShader().Get(), nullptr, 0);
			_context.DeviceContext()->GSSetShader(shader->GetGeometryShader().Get(), nullptr, 0);
			_context.DeviceContext()->HSSetShader(shader->GetHullShader().Get(), nullptr, 0);
			_context.DeviceContext()->DSSetShader(shader->GetDomainShader().Get(), nullptr, 0);

			_context.DeviceContext()->IASetInputLayout(inputLayout.Get());
			_context.DeviceContext()->IASetPrimitiveTopology(primitiveTopology);
			_context.DeviceContext()->OMSetDepthStencilState(depthStencilState.Get(), 0);
			_context.DeviceContext()->RSSetState(rasterizerState.Get());
			_context.DeviceContext()->OMSetBlendState(blendState.Get(), nullptr, 0xFFFFFFFF);

			const uint32_t behaviorStates = _currentGraphicsPipeline->GetBehaviorStates();
			if (behaviorStates & GraphicsPipeline::Behavior::AutoResizeScissor) {
				_context.DeviceContext()->RSSetScissorRects(1, &_currentScissor);
			}
			else {
				_context.DeviceContext()->RSSetScissorRects(1, &_currentGraphicsPipeline->GetDXScissorRect());
			}

			if (behaviorStates & GraphicsPipeline::Behavior::AutoResizeViewport) {
				_context.DeviceContext()->RSSetViewports(1, &_currentViewport);
			}
			else {
				_context.DeviceContext()->RSSetViewports(1, &_currentGraphicsPipeline->GetDXViewport());
			}

			_needBindGraphicsPipeline = false;
		}

		if (_needBindVertexBuffers) {
			_context.DeviceContext()->IASetVertexBuffers(0, _currentDXVertexBuffers.size(), _currentDXVertexBuffers.data(), _currentDXVertexBufferStrides.data(), _currentDXVertexBufferOffsets.data());

			_needBindVertexBuffers = false;
		}

		if (_needBindShaderResources) {
			auto dxShaderResourcesLayout = _currentShaderResources->GetLayout();

			const auto& tRegistryBindings = dxShaderResourcesLayout->GetTRegistryBindings();
			const auto& tRegistryResources = _currentShaderResources->GetTRegistryResources();

			for (const auto& [slot, resource] : tRegistryResources) {
				const auto& binding = tRegistryBindings.at(slot);

				if (binding.shaderStages & ShaderStage::Vertex) {
					_context.DeviceContext()->VSSetShaderResources(slot, 1, resource.GetAddressOf());
				}
				if (binding.shaderStages & ShaderStage::Pixel) {
					_context.DeviceContext()->PSSetShaderResources(slot, 1, resource.GetAddressOf());
				}
				if (binding.shaderStages & ShaderStage::Geometry) {
					_context.DeviceContext()->GSSetShaderResources(slot, 1, resource.GetAddressOf());
				}
				if (binding.shaderStages & ShaderStage::Hull) {
					_context.DeviceContext()->HSSetShaderResources(slot, 1, resource.GetAddressOf());
				}
				if (binding.shaderStages & ShaderStage::Domain) {
					_context.DeviceContext()->DSSetShaderResources(slot, 1, resource.GetAddressOf());
				}
			}

			const auto& cRegistryBindings = dxShaderResourcesLayout->GetCRegistryBindings();
			const auto& cRegistryResources = _currentShaderResources->GetCRegistryResources();

			for (const auto& [slot, resource] : cRegistryResources) {
				const auto& binding = cRegistryBindings.at(slot);

				if (binding.shaderStages & ShaderStage::Vertex) {
					_context.DeviceContext()->VSSetConstantBuffers(slot, 1, resource.GetAddressOf());
				}
				if (binding.shaderStages & ShaderStage::Pixel) {
					_context.DeviceContext()->PSSetConstantBuffers(slot, 1, resource.GetAddressOf());
				}
				if (binding.shaderStages & ShaderStage::Geometry) {
					_context.DeviceContext()->GSSetConstantBuffers(slot, 1, resource.GetAddressOf());
				}
				if (binding.shaderStages & ShaderStage::Hull) {
					_context.DeviceContext()->HSSetConstantBuffers(slot, 1, resource.GetAddressOf());
				}
				if (binding.shaderStages & ShaderStage::Domain) {
					_context.DeviceContext()->DSSetConstantBuffers(slot, 1, resource.GetAddressOf());
				}
			}

			_needBindShaderResources = false;
		}
	}

	void DXCommandQueue::Draw(uint32_t vertexCount, uint32_t vertexOffset) {
		BindRenderResources();

		_context.DeviceContext()->Draw(vertexCount, vertexOffset);
	}

	void DXCommandQueue::DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t vertexOffset) {
		BindRenderResources();

		_context.DeviceContext()->DrawInstanced(vertexCount, instanceCount, vertexOffset, 0);
	}

	void DXCommandQueue::DrawIndexed(const Ref<IndexBuffer>& indexBuffer, uint32_t indexCount, uint32_t indexOffset, uint32_t vertexOffset) {
		auto dxIndexBuffer = std::static_pointer_cast<DXIndexBuffer>(indexBuffer);
		FASSERT(dxIndexBuffer, "IndexBuffer is not a DXIndexBuffer");
		
		BindRenderResources();
		
		_context.DeviceContext()->IASetIndexBuffer(dxIndexBuffer->GetNativeDXBuffer().Get(), DXGI_FORMAT_R32_UINT, 0);
		_context.DeviceContext()->DrawIndexed(indexCount, indexOffset, vertexOffset);
	}

	void DXCommandQueue::DrawIndexedInstanced(const Ref<IndexBuffer>& indexBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t indexOffset, uint32_t vertexOffset) {
		auto dxIndexBuffer = std::static_pointer_cast<DXIndexBuffer>(indexBuffer);
		FASSERT(dxIndexBuffer, "IndexBuffer is not a DXIndexBuffer");
		
		BindRenderResources();
		
		_context.DeviceContext()->IASetIndexBuffer(dxIndexBuffer->GetNativeDXBuffer().Get(), DXGI_FORMAT_R32_UINT, 0);
		_context.DeviceContext()->DrawIndexedInstanced(indexCount, instanceCount, indexOffset, vertexOffset, 0);
	}

	void DXCommandQueue::BeginRenderPassImpl(const Ref<DXRenderPass>& beginRenderPass, const Ref<DXFramebuffer>& framebuffer) {
		std::vector<ID3D11RenderTargetView*> rtvs(beginRenderPass->GetColorAttachmentOpCount());
		for (uint32_t i = 0; i < beginRenderPass->GetColorAttachmentOpCount(); i++) {
			auto& op = beginRenderPass->GetColorAttachmentOp(i);
			auto& renderTargetTex = std::static_pointer_cast<DXTexture2D>(framebuffer->GetAttachment(i));

			rtvs[i] = renderTargetTex->GetNativeRTV().Get();
			if (op.loadOp == AttachmentLoadOp::Clear) {
				std::array<float, 4> clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
				_context.DeviceContext()->ClearRenderTargetView(rtvs[i], clearColor.data());
			}
		}

		ID3D11DepthStencilView* dsv = nullptr;
		if (beginRenderPass->HasDepthStencilAttachmentOp()) {
			auto& depthStencilOp = beginRenderPass->GetDepthStencilAttachmentOp();
			auto& depthStencilTex = std::static_pointer_cast<DXTexture2D>(framebuffer->GetDepthStencilAttachment());

			dsv = depthStencilTex->GetNativeDSV().Get();
			if (depthStencilOp.loadOp == AttachmentLoadOp::Clear) {
				_context.DeviceContext()->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
			}
		}

		_context.DeviceContext()->OMSetRenderTargets(rtvs.size(), rtvs.data(), dsv);
	}

	void DXCommandQueue::BeginRenderPass() {
		auto framebuffer = std::static_pointer_cast<GraphicsFramebuffer>(_context.MainDXFramebuffer());
		auto beginRenderPass = std::static_pointer_cast<GraphicsRenderPass>(_context.MainClearDXRenderPass());
		auto resumeRenderPass = std::static_pointer_cast<GraphicsRenderPass>(_context.MainLoadDXRenderPass());

		BeginRenderPass(beginRenderPass, resumeRenderPass, framebuffer);
	}

	void DXCommandQueue::BeginRenderPass(const Ref<GraphicsRenderPass>& beginRenderPass, const Ref<GraphicsRenderPass>& resumeRenderPass, const Ref<GraphicsFramebuffer>& framebuffer) {
		auto dxBeginRenderPass = std::static_pointer_cast<DXRenderPass>(beginRenderPass);
		FASSERT(dxBeginRenderPass, "Invalid render pass type for DXCommandQueue");

		auto dxResumeRenderPass = std::static_pointer_cast<DXRenderPass>(resumeRenderPass);
		FASSERT(dxResumeRenderPass, "Invalid render pass type for DXCommandQueue");

		auto dxFramebuffer = std::static_pointer_cast<DXFramebuffer>(framebuffer);
		FASSERT(dxFramebuffer, "Invalid framebuffer type for DXCommandQueue");

		_currentViewport.TopLeftX = 0.0f;
		_currentViewport.TopLeftY = 0.0f;
		_currentViewport.Width = static_cast<float>(dxFramebuffer->GetWidth());
		_currentViewport.Height = static_cast<float>(dxFramebuffer->GetHeight());
		_currentViewport.MinDepth = 0.0f;
		_currentViewport.MaxDepth = 1.0f;

		_currentScissor.left = 0;
		_currentScissor.top = 0;
		_currentScissor.right = static_cast<LONG>(dxFramebuffer->GetWidth());
		_currentScissor.bottom = static_cast<LONG>(dxFramebuffer->GetHeight());

		_currentBeginInfoStack.push_back({ dxFramebuffer, dxBeginRenderPass, dxResumeRenderPass });

		BeginRenderPassImpl(dxBeginRenderPass, dxFramebuffer);
	}

	void DXCommandQueue::EndRenderPass() {
		_currentBeginInfoStack.pop_back();
		if (_currentBeginInfoStack.empty()) {
			_context.DeviceContext()->OMSetRenderTargets(0, nullptr, nullptr);
			return;
		}

		auto& currentBeginInfo = _currentBeginInfoStack.back();
		auto dxFramebuffer = currentBeginInfo.framebuffer;
		auto dxResumeRenderPass = currentBeginInfo.resumeRenderPass;

		_currentViewport.TopLeftX = 0.0f;
		_currentViewport.TopLeftY = 0.0f;
		_currentViewport.Width = static_cast<float>(dxFramebuffer->GetWidth());
		_currentViewport.Height = static_cast<float>(dxFramebuffer->GetHeight());
		_currentViewport.MinDepth = 0.0f;
		_currentViewport.MaxDepth = 1.0f;

		_currentScissor.left = 0;
		_currentScissor.top = 0;
		_currentScissor.right = static_cast<LONG>(dxFramebuffer->GetWidth());
		_currentScissor.bottom = static_cast<LONG>(dxFramebuffer->GetHeight());

		BeginRenderPassImpl(dxResumeRenderPass, dxFramebuffer);
	}

	void DXCommandQueue::Submit() {
		// NOTE: nothing to do here for DX11, as the command queue is submitted automatically
	}

	void DXCommandQueue::Present() {
		_context.DXSwapChain()->Present(0, 0);
	}

	void DXCommandQueue::SetComputePipeline(const Ref<ComputePipeline>& pipeline) {
		if (pipeline == _currentComputePipeline) {
			return; // Already set
		}

		auto dxPipeline = std::static_pointer_cast<DXComputePipeline>(pipeline);
		FASSERT(dxPipeline, "Pipeline is not a DXComputePipeline");

		_currentComputePipeline = dxPipeline;
		_needBindComputePipeline = true;
	}

	void DXCommandQueue::SetComputeConstantBuffer(const Ref<ConstantBuffer>& constantBuffer, uint32_t slot) {
#if false
		constantBuffer->Unbind();
		constantBuffer->BindToComputeShader(slot); 
#endif
	}

#if false
	void DXCommandQueue::BindToComputeTRegistry(uint32_t slot, ID3D11ShaderResourceView* srv) {
		auto it = _computeTRegistries.find(slot);
		if (it != _computeTRegistries.end() && it->second == srv) {
			return;
		}

		auto& registry = _computeTRegistries[slot];
		registry = srv;

		_context.DeviceContext()->CSSetShaderResources(slot, 1, &registry);
	}

	void DXCommandQueue::BindToComputeURegistry(uint32_t slot, ID3D11UnorderedAccessView* uav) {
		auto it = _computeURegistries.find(slot);
		if (it != _computeURegistries.end() && it->second == uav) {
			return;
		}

		auto& registry = _computeURegistries[slot];
		registry = uav;

		_context.DeviceContext()->CSSetUnorderedAccessViews(slot, 1, &registry, nullptr);
	}
#endif

	void DXCommandQueue::SetComputeTexture(const Ref<Texture>& texture, TextureUsages texUsages, uint32_t slot) {
		auto dxTexture = std::static_pointer_cast<DXTexture2D>(texture);

		if (texUsages & TextureUsage::ShaderResource) {
			auto srv = dxTexture->GetShaderResourceView();
			if (!srv) {
				Log::Error("ShaderResourceView is nullptr for texture at slot %d", slot);
				return;
			}

			//BindToComputeTRegistry(slot, static_cast<ID3D11ShaderResourceView*>(srv));
		}
		else if (texUsages & TextureUsage::UnorderedAccess) {
			auto uav = dxTexture->GetUnorderedAccessView();
			if (!uav) {
				Log::Error("UnorderedAccessView is nullptr for texture at slot %d", slot);
				return;
			}

			//BindToComputeURegistry(slot, static_cast<ID3D11UnorderedAccessView*>(uav));
		}
	}

	void DXCommandQueue::SetComputeStructuredBuffer(const Ref<StructuredBuffer>& buffer, BufferUsages buffUsages, uint32_t slot) {
		auto dxSBuffer = std::static_pointer_cast<DXStructuredBuffer>(buffer);
		if (buffUsages & BufferUsage::ShaderResource) {
			auto srv = dxSBuffer->GetNativeSRV();
			if (!srv) {
				Log::Error("ShaderResourceView is nullptr for structured buffer at slot %d", slot);
				return;
			}

			//BindToComputeTRegistry(slot, srv.Get());
		}
		else if (buffUsages & BufferUsage::UnorderedAccess) {
			auto uav = dxSBuffer->GetNativeUAV();
			if (!uav) {
				Log::Error("UnorderedAccessView is nullptr for structured buffer at slot %d", slot);
				return;
			}

			//BindToComputeURegistry(slot, uav.Get());
		}
	}

	void DXCommandQueue::BindComputeResources() {
		if (_needBindComputePipeline) {
			auto shader = _currentComputePipeline->GetDXShader();
			_context.DeviceContext()->CSSetShader(shader->GetNativeDXComputeShader().Get(), nullptr, 0);
			_needBindComputePipeline = false;
		}
	}

	void DXCommandQueue::Dispatch(uint32_t x, uint32_t y, uint32_t z) {
		_context.DeviceContext()->Dispatch(x, y, z);
	}
}

#endif