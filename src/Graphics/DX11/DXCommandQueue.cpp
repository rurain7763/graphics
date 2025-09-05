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
	{
	}

	void DXCommandQueue::SetPipelineBarrier(Ref<Texture> texture, TextureLayout oldLayout, TextureLayout newLayout, AccessTypes srcAccess, AccessTypes dstAccess, PipelineStages srcStage, PipelineStages dstStage) {
		// DirectX 11 does not have explicit pipeline barriers like Vulkan or DirectX 12.
		// Resource state transitions are handled automatically by the driver.
		// However, we can use resource barriers for certain scenarios if needed.
		// For now, this function will be a no-op.
	}

	void DXCommandQueue::SetPipeline(const Ref<GraphicsPipeline>& pipeline) {
		auto dxPipeline = std::static_pointer_cast<DXGraphicsPipeline>(pipeline);
		FASSERT(dxPipeline, "Pipeline is not a DXGraphicsPipeline");

		auto shader = dxPipeline->GetDXShader();
		auto inputLayout = dxPipeline->GetDXInputLayout();
		auto primitiveTopology = dxPipeline->GetDXPrimitiveTopology();
		auto depthStencilState = dxPipeline->GetDXDepthStencilState();
		uint32_t stencilRef = dxPipeline->GetDXStencilRef();
		auto rasterizerState = dxPipeline->GetDXRasterizerState();
		auto blendState = dxPipeline->GetDXBlendState();

		_context.DeviceContext()->VSSetShader(shader->GetNativeDXVertexShader().Get(), nullptr, 0);
		_context.DeviceContext()->PSSetShader(shader->GetPixelShader().Get(), nullptr, 0);
		_context.DeviceContext()->GSSetShader(shader->GetGeometryShader().Get(), nullptr, 0);
		_context.DeviceContext()->HSSetShader(shader->GetHullShader().Get(), nullptr, 0);
		_context.DeviceContext()->DSSetShader(shader->GetDomainShader().Get(), nullptr, 0);

		_context.DeviceContext()->IASetInputLayout(inputLayout.Get());
		_context.DeviceContext()->IASetPrimitiveTopology(primitiveTopology);
		_context.DeviceContext()->OMSetDepthStencilState(depthStencilState.Get(), stencilRef);
		_context.DeviceContext()->RSSetState(rasterizerState.Get());
		_context.DeviceContext()->OMSetBlendState(blendState.Get(), nullptr, 0xFFFFFFFF);

		bool needBindViewport = false;
		bool needBindScissor = false;

		const uint32_t behaviorStates = dxPipeline->GetBehaviorStates();
		if (behaviorStates & GraphicsPipeline::Behavior::AutoResizeViewport) {
			if (!_currentBeginFramebuffer) {
				LOG_ERROR("No framebuffer bound for the command queue.");
				return;
			}
			
			D3D11_VIEWPORT newViewport;
			newViewport.TopLeftX = 0.0f;
			newViewport.TopLeftY = 0.0f;
			newViewport.Width = static_cast<float>(_currentBeginFramebuffer->GetWidth());
			newViewport.Height = static_cast<float>(_currentBeginFramebuffer->GetHeight());
			newViewport.MinDepth = 0.0f;
			newViewport.MaxDepth = 1.0f;

			needBindViewport = _currentViewport != newViewport;
			_currentViewport = newViewport;
		}
		else {
			const auto& newViewport = dxPipeline->GetDXViewport();

			needBindViewport = _currentViewport != newViewport;
			_currentViewport = newViewport;
		}

		if (behaviorStates & GraphicsPipeline::Behavior::AutoResizeScissor) {
			if (!_currentBeginFramebuffer) {
				LOG_ERROR("No framebuffer bound for the command queue.");
				return;
			}

			D3D11_RECT newScissor;
			newScissor.left = 0;
			newScissor.top = 0;
			newScissor.right = _currentBeginFramebuffer->GetWidth();
			newScissor.bottom = _currentBeginFramebuffer->GetHeight();

			needBindScissor = newScissor != _currentScissor;
			_currentScissor = newScissor;
		}
		else {
			const auto& newScissor = dxPipeline->GetDXScissorRect();

			needBindScissor = newScissor != _currentScissor;
			_currentScissor = newScissor;
		}

		if (needBindViewport) {
			_context.DeviceContext()->RSSetViewports(1, &_currentViewport);
		}

		if (needBindScissor) {
			_context.DeviceContext()->RSSetScissorRects(1, &_currentScissor);
		}

		_currentGraphicsPipeline = dxPipeline;
	}

	void DXCommandQueue::SetVertexBuffers(const std::vector<Ref<VertexBuffer>>& vertexBuffers) {
		_currentVertexBuffers.clear();
		_currentDXVertexBuffers.clear();
		_currentDXVertexBufferStrides.clear();
		_currentDXVertexBufferOffsets.clear();
		for (uint32_t i = 0; i < vertexBuffers.size(); i++) {
			auto dxVertexBuffer = std::static_pointer_cast<DXVertexBuffer>(vertexBuffers[i]);
			FASSERT(dxVertexBuffer, "VertexBuffer is not a DXVertexBuffer");

			const auto& dxNativeBuff = static_cast<const DXNativeBuffer&>(dxVertexBuffer->GetNativeBuffer());

			_currentVertexBuffers.push_back(dxVertexBuffer);
			_currentDXVertexBuffers.push_back(dxNativeBuff.buffer.Get());
			_currentDXVertexBufferStrides.push_back(dxVertexBuffer->ElementSize());
			_currentDXVertexBufferOffsets.push_back(0); // Offset is always 0 for now
		}

		_context.DeviceContext()->IASetVertexBuffers(0, _currentDXVertexBuffers.size(), _currentDXVertexBuffers.data(), _currentDXVertexBufferStrides.data(), _currentDXVertexBufferOffsets.data());
	}

	void DXCommandQueue::ResetVertexBuffers() {
		_currentVertexBuffers.clear();
		_currentDXVertexBuffers.clear();
		_currentDXVertexBufferStrides.clear();
		_currentDXVertexBufferOffsets.clear();
	}

	void DXCommandQueue::SetShaderResources(const std::vector<Ref<ShaderResources>>& shaderResources) {
		ResetShaderResources();

		_currentShaderResourcesVec.reserve(shaderResources.size());
		for (uint32_t i = 0; i < shaderResources.size(); ++i) {
			const auto& resource = shaderResources[i];

			auto dxShaderResources = std::static_pointer_cast<DXShaderResources>(resource);
			FASSERT(dxShaderResources, "ShaderResources is not a DXShaderResources");

			auto dxShaderResourcesLayout = dxShaderResources->GetLayout();

			const auto& tRegistryBindings = dxShaderResourcesLayout->GetTRegistryBindings();
			const auto& tRegistryResources = dxShaderResources->GetTRegistryResources();

			for (const auto& [slot, resource] : tRegistryResources) {
				const auto& binding = tRegistryBindings.at(slot);

				uint32_t absoluteSlot = i * ShaderResourceSetInterval + slot;

				if (binding.shaderStages & ShaderStage::Vertex) {
					_context.DeviceContext()->VSSetShaderResources(absoluteSlot, 1, resource.GetAddressOf());
				}
				if (binding.shaderStages & ShaderStage::Pixel) {
					_context.DeviceContext()->PSSetShaderResources(absoluteSlot, 1, resource.GetAddressOf());
				}
				if (binding.shaderStages & ShaderStage::Geometry) {
					_context.DeviceContext()->GSSetShaderResources(absoluteSlot, 1, resource.GetAddressOf());
				}
				if (binding.shaderStages & ShaderStage::Hull) {
					_context.DeviceContext()->HSSetShaderResources(absoluteSlot, 1, resource.GetAddressOf());
				}
				if (binding.shaderStages & ShaderStage::Domain) {
					_context.DeviceContext()->DSSetShaderResources(absoluteSlot, 1, resource.GetAddressOf());
				}
			}

			const auto& cRegistryBindings = dxShaderResourcesLayout->GetCRegistryBindings();
			const auto& cRegistryResources = dxShaderResources->GetCRegistryResources();

			for (const auto& [slot, resource] : cRegistryResources) {
				const auto& binding = cRegistryBindings.at(slot);

				uint32_t absoluteSlot = i * ShaderResourceSetInterval + slot;

				if (binding.shaderStages & ShaderStage::Vertex) {
					_context.DeviceContext()->VSSetConstantBuffers(absoluteSlot, 1, resource.GetAddressOf());
				}
				if (binding.shaderStages & ShaderStage::Pixel) {
					_context.DeviceContext()->PSSetConstantBuffers(absoluteSlot, 1, resource.GetAddressOf());
				}
				if (binding.shaderStages & ShaderStage::Geometry) {
					_context.DeviceContext()->GSSetConstantBuffers(absoluteSlot, 1, resource.GetAddressOf());
				}
				if (binding.shaderStages & ShaderStage::Hull) {
					_context.DeviceContext()->HSSetConstantBuffers(absoluteSlot, 1, resource.GetAddressOf());
				}
				if (binding.shaderStages & ShaderStage::Domain) {
					_context.DeviceContext()->DSSetConstantBuffers(absoluteSlot, 1, resource.GetAddressOf());
				}
			}

			_currentShaderResourcesVec.push_back(dxShaderResources);
		}
	}

	void DXCommandQueue::ResetShaderResources() {
		ID3D11ShaderResourceView* nullSRV[] = { nullptr };
		ID3D11Buffer* nullCB[] = { nullptr };

		for (const auto& dxResources : _currentShaderResourcesVec) {
			auto dxResourcesLayout = dxResources->GetLayout();

			const auto& tRegistryBindings = dxResourcesLayout->GetTRegistryBindings();

			for (const auto& [slot, binding] : tRegistryBindings) {
				if (binding.shaderStages & ShaderStage::Vertex) {
					_context.DeviceContext()->VSSetShaderResources(slot, 1, nullSRV);
				}
				if (binding.shaderStages & ShaderStage::Pixel) {
					_context.DeviceContext()->PSSetShaderResources(slot, 1, nullSRV);
				}
				if (binding.shaderStages & ShaderStage::Geometry) {
					_context.DeviceContext()->GSSetShaderResources(slot, 1, nullSRV);
				}
				if (binding.shaderStages & ShaderStage::Hull) {
					_context.DeviceContext()->HSSetShaderResources(slot, 1, nullSRV);
				}
				if (binding.shaderStages & ShaderStage::Domain) {
					_context.DeviceContext()->DSSetShaderResources(slot, 1, nullSRV);
				}
			}

			const auto& cRegistryBindings = dxResourcesLayout->GetCRegistryBindings();

			for (const auto& [slot, binding] : tRegistryBindings) {
				if (binding.shaderStages & ShaderStage::Vertex) {
					_context.DeviceContext()->VSSetConstantBuffers(slot, 1, nullCB);
				}
				if (binding.shaderStages & ShaderStage::Pixel) {
					_context.DeviceContext()->PSSetConstantBuffers(slot, 1, nullCB);
				}
				if (binding.shaderStages & ShaderStage::Geometry) {
					_context.DeviceContext()->GSSetConstantBuffers(slot, 1, nullCB);
				}
				if (binding.shaderStages & ShaderStage::Hull) {
					_context.DeviceContext()->HSSetConstantBuffers(slot, 1, nullCB);
				}
				if (binding.shaderStages & ShaderStage::Domain) {
					_context.DeviceContext()->DSSetConstantBuffers(slot, 1, nullCB);
				}
			}
		}

		_currentShaderResourcesVec.clear();
	}

	void DXCommandQueue::Draw(uint32_t vertexCount, uint32_t vertexOffset) {
		_context.DeviceContext()->Draw(vertexCount, vertexOffset);
	}

	void DXCommandQueue::DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t vertexOffset, uint32_t instanceOffset) {
		_context.DeviceContext()->DrawInstanced(vertexCount, instanceCount, vertexOffset, instanceOffset);
	}

	void DXCommandQueue::DrawIndexed(const Ref<IndexBuffer>& indexBuffer, uint32_t indexCount, uint32_t indexOffset, uint32_t vertexOffset) {
		auto dxIndexBuffer = std::static_pointer_cast<DXIndexBuffer>(indexBuffer);
		FASSERT(dxIndexBuffer, "IndexBuffer is not a DXIndexBuffer");

		const auto& dxNativeBuff = static_cast<const DXNativeBuffer&>(dxIndexBuffer->GetNativeBuffer());
				
		_context.DeviceContext()->IASetIndexBuffer(dxNativeBuff.buffer.Get(), DXGI_FORMAT_R32_UINT, 0);
		_context.DeviceContext()->DrawIndexed(indexCount, indexOffset, vertexOffset);
	}

	void DXCommandQueue::DrawIndexedInstanced(const Ref<IndexBuffer>& indexBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t indexOffset, uint32_t vertexOffset, uint32_t instanceOffset) {
		auto dxIndexBuffer = std::static_pointer_cast<DXIndexBuffer>(indexBuffer);
		FASSERT(dxIndexBuffer, "IndexBuffer is not a DXIndexBuffer");

		const auto& dxNativeBuff = static_cast<const DXNativeBuffer&>(dxIndexBuffer->GetNativeBuffer());
				
		_context.DeviceContext()->IASetIndexBuffer(dxNativeBuff.buffer.Get(), DXGI_FORMAT_R32_UINT, 0);
		_context.DeviceContext()->DrawIndexedInstanced(indexCount, instanceCount, indexOffset, vertexOffset, instanceOffset);
	}

	void DXCommandQueue::BeginRenderPass(const Ref<RenderPass>& renderpass, const Ref<Framebuffer>& framebuffer) {
		auto dxRenderPass = std::static_pointer_cast<DXRenderPass>(renderpass);
		FASSERT(dxRenderPass, "Invalid render pass type for DXCommandQueue");

		auto dxFramebuffer = std::static_pointer_cast<DXFramebuffer>(framebuffer);
		FASSERT(dxFramebuffer, "Invalid framebuffer type for DXCommandQueue");

		if (_currentBeginRenderPass) {
			LOG_ERROR("A render pass is already active. Nested render passes are not supported in this implementation.");
			return;
		}

		_currentBeginRenderPass = dxRenderPass;
		_currentBeginSubpass = 0;
		_currentBeginFramebuffer = dxFramebuffer;

		std::vector<ID3D11RenderTargetView*> rtvs(_currentBeginRenderPass->GetColorAttachmentRefsCount(_currentBeginSubpass));
		for (uint32_t i = 0; i < _currentBeginRenderPass->GetColorAttachmentRefsCount(_currentBeginSubpass); i++) {
			auto& attachmentRef = _currentBeginRenderPass->GetColorAttachmentRef(_currentBeginSubpass, i);
			auto& attachment = _currentBeginRenderPass->GetAttachment(attachmentRef.attachmentIndex);
			auto& renderTargetTex = std::static_pointer_cast<DXTexture2D>(_currentBeginFramebuffer->GetAttachment(attachmentRef.attachmentIndex));
			const auto& dxNativeTexView = static_cast<const DXNativeTextureView&>(renderTargetTex->GetNativeTextureView());

			rtvs[i] = dxNativeTexView.rtv.Get();
			if (attachment.loadOp == AttachmentLoadOp::Clear) {
				std::array<float, 4> clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
				_context.DeviceContext()->ClearRenderTargetView(rtvs[i], clearColor.data());
			}
		}

		ID3D11DepthStencilView* dsv = nullptr;
		if (_currentBeginRenderPass->HasDepthStencilAttachmentRef(_currentBeginSubpass)) {
			auto& attachmentRef = _currentBeginRenderPass->GetDepthStencilAttachmentRef(_currentBeginSubpass);
			auto& attachment = _currentBeginRenderPass->GetAttachment(attachmentRef.attachmentIndex);
			auto& depthStencilTex = std::static_pointer_cast<DXTexture2D>(_currentBeginFramebuffer->GetAttachment(attachmentRef.attachmentIndex));
			const auto& dxNativeTexView = static_cast<const DXNativeTextureView&>(depthStencilTex->GetNativeTextureView());

			dsv = dxNativeTexView.dsv.Get();

			uint32_t clearFlags = 0;
			if (attachment.loadOp == AttachmentLoadOp::Clear) {
				clearFlags |= D3D11_CLEAR_DEPTH;
			}
			if (attachment.stencilLoadOp == AttachmentLoadOp::Clear) {
				clearFlags |= D3D11_CLEAR_STENCIL;
			}

			_context.DeviceContext()->ClearDepthStencilView(dsv, clearFlags, 1.0f, 0);
		}

		_context.DeviceContext()->OMSetRenderTargets(rtvs.size(), rtvs.data(), dsv);
	}

	void DXCommandQueue::NextSubpass() {
		if (!_currentBeginRenderPass) {
			LOG_WARN("No active render pass to advance subpass.");
			return;
		}

		for (uint32_t i = 0; i < _currentBeginRenderPass->GetResolveAttachmentRefCount(_currentBeginSubpass); i++) {
			auto& resolveAttachmentRef = _currentBeginRenderPass->GetResolveAttachmentRef(_currentBeginSubpass, i);
			auto& colorAttachmentRef = _currentBeginRenderPass->GetColorAttachmentRef(_currentBeginSubpass, i);

			auto& renderTargetTex = std::static_pointer_cast<DXTexture2D>(_currentBeginFramebuffer->GetAttachment(colorAttachmentRef.attachmentIndex));
			const auto& dxNativeRTTex = static_cast<const DXNativeTexture&>(renderTargetTex->GetNativeTexture());
			auto& resolveTex = std::static_pointer_cast<DXTexture2D>(_currentBeginFramebuffer->GetAttachment(resolveAttachmentRef.attachmentIndex));
			const auto& dxNativeResolveTex = static_cast<const DXNativeTexture&>(resolveTex->GetNativeTexture());

			_context.DeviceContext()->ResolveSubresource(dxNativeResolveTex.texture.Get(), 0, dxNativeRTTex.texture.Get(), 0, ConvertToDXFormat(resolveTex->GetPixelFormat()));
		}

		_currentBeginSubpass++;

		std::vector<ID3D11RenderTargetView*> rtvs(_currentBeginRenderPass->GetColorAttachmentRefsCount(_currentBeginSubpass));
		for (uint32_t i = 0; i < _currentBeginRenderPass->GetColorAttachmentRefsCount(_currentBeginSubpass); i++) {
			auto& attachmentRef = _currentBeginRenderPass->GetColorAttachmentRef(_currentBeginSubpass, i);
			auto& attachment = _currentBeginRenderPass->GetAttachment(attachmentRef.attachmentIndex);
			auto& renderTargetTex = std::static_pointer_cast<DXTexture2D>(_currentBeginFramebuffer->GetAttachment(attachmentRef.attachmentIndex));
			const auto& dxNativeTexView = static_cast<const DXNativeTextureView&>(renderTargetTex->GetNativeTextureView());

			rtvs[i] = dxNativeTexView.rtv.Get();
			if (attachment.loadOp == AttachmentLoadOp::Clear) {
				std::array<float, 4> clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
				_context.DeviceContext()->ClearRenderTargetView(rtvs[i], clearColor.data());
			}
		}

		ID3D11DepthStencilView* dsv = nullptr;
		if (_currentBeginRenderPass->HasDepthStencilAttachmentRef(_currentBeginSubpass)) {
			auto& attachmentRef = _currentBeginRenderPass->GetDepthStencilAttachmentRef(_currentBeginSubpass);
			auto& attachment = _currentBeginRenderPass->GetAttachment(attachmentRef.attachmentIndex);
			auto& depthStencilTex = std::static_pointer_cast<DXTexture2D>(_currentBeginFramebuffer->GetAttachment(attachmentRef.attachmentIndex));
			const auto& dxNativeTexView = static_cast<const DXNativeTextureView&>(depthStencilTex->GetNativeTextureView());

			dsv = dxNativeTexView.dsv.Get();

			uint32_t clearFlags = 0;
			if (attachment.loadOp == AttachmentLoadOp::Clear) {
				clearFlags |= D3D11_CLEAR_DEPTH;
			}
			if (attachment.stencilLoadOp == AttachmentLoadOp::Clear) {
				clearFlags |= D3D11_CLEAR_STENCIL;
			}

			_context.DeviceContext()->ClearDepthStencilView(dsv, clearFlags, 1.0f, 0);
		}

		_context.DeviceContext()->OMSetRenderTargets(rtvs.size(), rtvs.data(), dsv);
	}

	void DXCommandQueue::EndRenderPass() {
		if (!_currentBeginRenderPass) {
			LOG_WARN("No active render pass to end.");
			return;
		}

		for (uint32_t i = 0; i < _currentBeginRenderPass->GetResolveAttachmentRefCount(_currentBeginSubpass); i++) {
			auto& resolveAttachmentRef = _currentBeginRenderPass->GetResolveAttachmentRef(_currentBeginSubpass, i);
			auto& colorAttachmentRef = _currentBeginRenderPass->GetColorAttachmentRef(_currentBeginSubpass, i);

			auto& renderTargetTex = std::static_pointer_cast<DXTexture2D>(_currentBeginFramebuffer->GetAttachment(colorAttachmentRef.attachmentIndex));
			const auto& dxNativeRTTex = static_cast<const DXNativeTexture&>(renderTargetTex->GetNativeTexture());
			auto& resolveTex = std::static_pointer_cast<DXTexture2D>(_currentBeginFramebuffer->GetAttachment(resolveAttachmentRef.attachmentIndex));
			const auto& dxNativeResolveTex = static_cast<const DXNativeTexture&>(resolveTex->GetNativeTexture());

			_context.DeviceContext()->ResolveSubresource(dxNativeResolveTex.texture.Get(), 0, dxNativeRTTex.texture.Get(), 0, ConvertToDXFormat(resolveTex->GetPixelFormat()));
		}

		_currentBeginRenderPass = nullptr;
		_currentBeginSubpass = 0;
		_currentBeginFramebuffer = nullptr;

		_context.DeviceContext()->OMSetRenderTargets(0, nullptr, nullptr);
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
#if false
		auto dxTexture = std::static_pointer_cast<DXTexture2D>(texture);

		if (texUsages & TextureUsage::ShaderResource) {
			auto srv = dxTexture->GetShaderResourceView();
			if (!srv) {
				Log::Error("ShaderResourceView is nullptr for texture at slot %d", slot);
				return;
			}

			BindToComputeTRegistry(slot, static_cast<ID3D11ShaderResourceView*>(srv));
		}
		else if (texUsages & TextureUsage::UnorderedAccess) {
			auto uav = dxTexture->GetUnorderedAccessView();
			if (!uav) {
				Log::Error("UnorderedAccessView is nullptr for texture at slot %d", slot);
				return;
			}

			BindToComputeURegistry(slot, static_cast<ID3D11UnorderedAccessView*>(uav));
		}
#endif
	}

	void DXCommandQueue::SetComputeStructuredBuffer(const Ref<StructuredBuffer>& buffer, BufferUsages buffUsages, uint32_t slot) {
#if false
		auto dxSBuffer = std::static_pointer_cast<DXStructuredBuffer>(buffer);
		if (buffUsages & BufferUsage::ShaderResource) {
			auto srv = dxSBuffer->GetNativeSRV();
			if (!srv) {
				Log::Error("ShaderResourceView is nullptr for structured buffer at slot %d", slot);
				return;
			}

			BindToComputeTRegistry(slot, srv.Get());
		}
		else if (buffUsages & BufferUsage::UnorderedAccess) {
			auto uav = dxSBuffer->GetNativeUAV();
			if (!uav) {
				Log::Error("UnorderedAccessView is nullptr for structured buffer at slot %d", slot);
				return;
			}

			BindToComputeURegistry(slot, uav.Get());
		}
#endif
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