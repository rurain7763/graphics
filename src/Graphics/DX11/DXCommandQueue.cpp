#include "pch.h"
#include "DXCommandQueue.h"
#include "DXTextures.h"
#include "DXStructuredBuffer.h"
#include "DXContext.h"
#include "Log/Log.h"
#include "DXType.h"

namespace flaw {
	DXCommandQueue::DXCommandQueue(DXContext& context)
		: _context(context)
	{
	}

	void DXCommandQueue::SetPrimitiveTopology(PrimitiveTopology primitiveTopology) {
		if (_primitiveTopology != PrimitiveTopology::Count && _primitiveTopology == primitiveTopology) {
			return;
		}

		_context.DeviceContext()->IASetPrimitiveTopology(ConvertToD3D11Topology(primitiveTopology));
		_primitiveTopology = primitiveTopology;
	}

	void DXCommandQueue::ClearAllRegistries() {
		_graphicsTRegistries.clear();
		_computeTRegistries.clear();
		_computeURegistries.clear();

		std::vector<ID3D11ShaderResourceView*> nullSRVs(D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, nullptr);
		_context.DeviceContext()->VSSetShaderResources(0, nullSRVs.size(), nullSRVs.data());
		_context.DeviceContext()->PSSetShaderResources(0, nullSRVs.size(), nullSRVs.data());
		_context.DeviceContext()->GSSetShaderResources(0, nullSRVs.size(), nullSRVs.data());
		_context.DeviceContext()->HSSetShaderResources(0, nullSRVs.size(), nullSRVs.data());
		_context.DeviceContext()->DSSetShaderResources(0, nullSRVs.size(), nullSRVs.data());
		_context.DeviceContext()->CSSetShaderResources(0, nullSRVs.size(), nullSRVs.data());

		std::vector<ID3D11UnorderedAccessView*> nullUAVs(D3D11_PS_CS_UAV_REGISTER_COUNT, nullptr);
		_context.DeviceContext()->CSSetUnorderedAccessViews(0, nullUAVs.size(), nullUAVs.data(), nullptr);
	}

	void DXCommandQueue::SetPipeline(const Ref<GraphicsPipeline>& pipeline) {
		ClearAllRegistries(); // Clear all registries before binding a new pipeline
		pipeline->Bind(); 
	}

	void DXCommandQueue::SetVertexBuffer(const Ref<VertexBuffer>& vertexBuffer) {
		vertexBuffer->Bind();
	}

	void DXCommandQueue::SetConstantBuffer(const Ref<ConstantBuffer>& constantBuffer, uint32_t slot) {
		constantBuffer->Unbind();
		constantBuffer->BindToGraphicsShader(slot); 
	}

	void DXCommandQueue::BindToGraphicsTRegistry(uint32_t slot, ID3D11ShaderResourceView* srv) {
		auto it = _graphicsTRegistries.find(slot);
		if (it != _graphicsTRegistries.end() && it->second == srv) {
			return; // Already bound
		}

		auto& registry = _graphicsTRegistries[slot];
		registry = srv;

		_context.DeviceContext()->VSSetShaderResources(slot, 1, &registry);
		_context.DeviceContext()->PSSetShaderResources(slot, 1, &registry);
		_context.DeviceContext()->GSSetShaderResources(slot, 1, &registry);
		_context.DeviceContext()->HSSetShaderResources(slot, 1, &registry);
		_context.DeviceContext()->DSSetShaderResources(slot, 1, &registry);
	}

	void DXCommandQueue::SetStructuredBuffer(const Ref<StructuredBuffer>& buffer, uint32_t slot) {
		auto dxSBuffer = std::static_pointer_cast<DXStructuredBuffer>(buffer);
		auto srv = dxSBuffer->GetShaderResourceView();
		if (!srv) {
			Log::Error("ShaderResourceView is nullptr for structured buffer at slot %d", slot);
			return;
		}

		BindToGraphicsTRegistry(slot, srv.Get());
	}

	void DXCommandQueue::SetTexture(const Ref<Texture>& texture, uint32_t slot) {
		auto dxTexture = std::static_pointer_cast<DXTexture2D>(texture);
		auto srv = dxTexture->GetShaderResourceView();
		if (!srv) {
			Log::Error("ShaderResourceView is nullptr for texture at slot %d", slot);
			return;
		}

		BindToGraphicsTRegistry(slot, static_cast<ID3D11ShaderResourceView*>(srv));
	}

	void DXCommandQueue::Draw(uint32_t vertexCount, uint32_t vertexOffset) {
		_context.DeviceContext()->Draw(vertexCount, vertexOffset);
	}

	void DXCommandQueue::DrawIndexed(const Ref<IndexBuffer>& indexBuffer, uint32_t indexCount, uint32_t indexOffset, uint32_t vertexOffset) {
		indexBuffer->Bind();
		_context.DeviceContext()->DrawIndexed(indexCount, indexOffset, vertexOffset);
	}

	void DXCommandQueue::DrawIndexedInstanced(const Ref<IndexBuffer>& indexBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t indexOffset, uint32_t vertexOffset) {
		indexBuffer->Bind();
		_context.DeviceContext()->DrawIndexedInstanced(indexCount, instanceCount, indexOffset, vertexOffset, 0);
	}

	void DXCommandQueue::SetComputePipeline(const Ref<ComputePipeline>& pipeline) {
		ClearAllRegistries(); // Clear all registries before binding a new compute pipeline
		pipeline->Bind(); 
	}

	void DXCommandQueue::SetComputeConstantBuffer(const Ref<ConstantBuffer>& constantBuffer, uint32_t slot) {
		constantBuffer->Unbind();
		constantBuffer->BindToComputeShader(slot); 
	}

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

	void DXCommandQueue::SetComputeTexture(const Ref<Texture>& texture, BindFlag bindFlag, uint32_t slot) {
		auto dxTexture = std::static_pointer_cast<DXTexture2D>(texture);

		if (bindFlag & BindFlag::ShaderResource) {
			auto srv = dxTexture->GetShaderResourceView();
			if (!srv) {
				Log::Error("ShaderResourceView is nullptr for texture at slot %d", slot);
				return;
			}

			BindToComputeTRegistry(slot, static_cast<ID3D11ShaderResourceView*>(srv));
		}
		else if (bindFlag & BindFlag::UnorderedAccess) {
			auto uav = dxTexture->GetUnorderedAccessView();
			if (!uav) {
				Log::Error("UnorderedAccessView is nullptr for texture at slot %d", slot);
				return;
			}

			BindToComputeURegistry(slot, static_cast<ID3D11UnorderedAccessView*>(uav));
		}
	}

	void DXCommandQueue::SetComputeStructuredBuffer(const Ref<StructuredBuffer>& buffer, BindFlag bindFlag, uint32_t slot) {
		auto dxSBuffer = std::static_pointer_cast<DXStructuredBuffer>(buffer);
		if (bindFlag & BindFlag::ShaderResource) {
			auto srv = dxSBuffer->GetShaderResourceView();
			if (!srv) {
				Log::Error("ShaderResourceView is nullptr for structured buffer at slot %d", slot);
				return;
			}

			BindToComputeTRegistry(slot, srv.Get());
		}
		else if (bindFlag & BindFlag::UnorderedAccess) {
			auto uav = dxSBuffer->GetUnorderedAccessView();
			if (!uav) {
				Log::Error("UnorderedAccessView is nullptr for structured buffer at slot %d", slot);
				return;
			}

			BindToComputeURegistry(slot, uav.Get());
		}
	}

	void DXCommandQueue::Dispatch(uint32_t x, uint32_t y, uint32_t z) {
		_context.DeviceContext()->Dispatch(x, y, z);
	}

	void DXCommandQueue::Execute() {
#if false
		while (!_commands.empty()) {
			_commands.front()();
			_commands.pop();
		}
#else
		// TODO: nothing to do now, but keep this for future use
#endif
	}
}