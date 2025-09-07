#pragma once

#include "Core.h"

#ifdef SUPPORT_DX11

#include "DXCore.h"
#include "Graphics/GraphicsShaders.h"

namespace flaw {
	class DXContext;

	class DXShaderResourcesLayout : public ShaderResourcesLayout {
	public:
		DXShaderResourcesLayout(DXContext& context, const Descriptor& descriptor);
		~DXShaderResourcesLayout() = default;

		inline const std::unordered_map<uint32_t, ResourceBinding>& GetTRegistryBindings() const { return _tRegistryBindings; }
		inline const std::unordered_map<uint32_t, ResourceBinding>& GetURegistryBindings() const { return _uRegistryBindings; }
		inline const std::unordered_map<uint32_t, ResourceBinding>& GetBRegistryBindings() const { return _bRegistryBindings; }

	private:
		DXContext& _context;

		std::unordered_map<uint32_t, ResourceBinding> _tRegistryBindings;
		std::unordered_map<uint32_t, ResourceBinding> _uRegistryBindings;
		std::unordered_map<uint32_t, ResourceBinding> _bRegistryBindings;
	};

	class DXShaderResources : public ShaderResources {
	public:
		DXShaderResources(DXContext& context, const Descriptor& descriptor);
		~DXShaderResources() = default;

		void BindTexture2D(const Ref<Texture2D>& texture, uint32_t binding) override;
		void BindTexture2DUA(const Ref<Texture2D>& texture, uint32_t binding) override;

		void BindTextureCube(const Ref<TextureCube>& texture, uint32_t binding) override;

		void BindConstantBuffer(const Ref<ConstantBuffer>& constantBuffer, uint32_t binding) override;

		void BindStructuredBuffer(const Ref<StructuredBuffer>& structuredBuffer, uint32_t binding) override;
		void BindStructuredBufferUA(const Ref<StructuredBuffer>& structuredBuffer, uint32_t binding) override;

		void BindInputAttachment(const Ref<Texture>& texture, uint32_t binding) override;

		inline Ref<DXShaderResourcesLayout> GetLayout() const { return _layout; }
		inline std::unordered_map<uint32_t, ComPtr<ID3D11ShaderResourceView>>& GetTRegistryResources() { return _tRegistryResources; }
		inline std::unordered_map<uint32_t, ComPtr<ID3D11UnorderedAccessView>>& GetURegistryResources() { return _uRegistryResources; }
		inline std::unordered_map<uint32_t, ComPtr<ID3D11Buffer>>& GetBRegistryResources() { return _bRegistryResources; }

	private:
		DXContext& _context;

		Ref<DXShaderResourcesLayout> _layout;

		std::unordered_map<uint32_t, ComPtr<ID3D11ShaderResourceView>> _tRegistryResources;
		std::unordered_map<uint32_t, ComPtr<ID3D11UnorderedAccessView>> _uRegistryResources;
		std::unordered_map<uint32_t, ComPtr<ID3D11Buffer>> _bRegistryResources;
	};

	class DXGraphicsShader : public GraphicsShader {
	public:
		DXGraphicsShader(DXContext& graphics, const Descriptor& descriptor);
		~DXGraphicsShader() override = default;

		inline ComPtr<ID3DBlob> GetDXVertexShaderBlob() const { return _vsBlob; }
		inline ComPtr<ID3D11VertexShader> GetNativeDXVertexShader() const { return _vertexShader; }
		inline ComPtr<ID3DBlob> GetHullShaderBlob() const { return _hsBlob; }
		inline ComPtr<ID3D11HullShader> GetHullShader() const { return _hullShader; }
		inline ComPtr<ID3DBlob> GetDomainShaderBlob() const { return _dsBlob; }
		inline ComPtr<ID3D11DomainShader> GetDomainShader() const { return _domainShader; }
		inline ComPtr<ID3DBlob> GetGeometryShaderBlob() const { return _gsBlob; }
		inline ComPtr<ID3D11GeometryShader> GetGeometryShader() const { return _geometryShader; }
		inline ComPtr<ID3DBlob> GetPixelShaderBlob() const { return _psBlob; }
		inline ComPtr<ID3D11PixelShader> GetPixelShader() const { return _pixelShader; }

	private:
		bool CreateVertexShader(const char* filePath, const char* entry);
		bool CreateHullShader(const char* filePath, const char* entry);
		bool CreateDomainShader(const char* filePath, const char* entry);
		bool CreateGeometryShader(const char* filePath, const char* entry);
		bool CreatePixelShader(const char* filePath, const char* entry);

		bool CompileShader(const char* filePath, const char* entryPoint, const char* target, ComPtr<ID3DBlob>& blob);

	private:
		DXContext& _context;

		ComPtr<ID3DBlob> _vsBlob;
		ComPtr<ID3D11VertexShader> _vertexShader;

		ComPtr<ID3DBlob> _hsBlob;
		ComPtr<ID3D11HullShader> _hullShader;

		ComPtr<ID3DBlob> _dsBlob;
		ComPtr<ID3D11DomainShader> _domainShader;

		ComPtr<ID3DBlob> _gsBlob;
		ComPtr<ID3D11GeometryShader> _geometryShader;

		ComPtr<ID3DBlob> _psBlob;
		ComPtr<ID3D11PixelShader> _pixelShader;
	};

	class DXComputeShader : public ComputeShader {
	public:
		DXComputeShader(DXContext& context, const Descriptor& descriptor);
		~DXComputeShader() override = default;

		inline ComPtr<ID3DBlob> GetDXCSBlob() const { return _csBlob; }
		inline ComPtr<ID3D11ComputeShader> GetNativeDXComputeShader() const { return _computeShader; }

	private:
		bool CreateComputeShader(const char* filepath, const char* entryPoint);

	private:
		DXContext& _context;

		ComPtr<ID3DBlob> _csBlob;
		ComPtr<ID3D11ComputeShader> _computeShader;
	};
}

#endif
