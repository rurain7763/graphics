#pragma once

#include "Core.h"

#ifdef SUPPORT_VULKAN

#include "VkCore.h"
#include "Graphics/GraphicsContext.h"

#include <array>

namespace flaw {
	class VkSwapchain;
	class VkCommandQueue;

	class FAPI VkContext : public GraphicsContext {
	public:
		VkContext(PlatformContext& context, int32_t width, int32_t height);
		~VkContext();

		bool Prepare() override;

		Ref<VertexInputLayout> CreateVertexInputLayout(const VertexInputLayout::Descriptor& descriptor) override;

		Ref<VertexBuffer> CreateVertexBuffer(const VertexBuffer::Descriptor& descriptor) override;
		Ref<IndexBuffer> CreateIndexBuffer(const IndexBuffer::Descriptor& descriptor) override;

		Ref<ShaderResourcesLayout> CreateShaderResourcesLayout(const ShaderResourcesLayout::Descriptor& descriptor) override;
		Ref<ShaderResources> CreateShaderResources(const ShaderResources::Descriptor& descriptor) override;
		Ref<GraphicsShader> CreateGraphicsShader(const GraphicsShader::Descriptor& descriptor) override;
		Ref<GraphicsPipeline> CreateGraphicsPipeline() override;

		Ref<ConstantBuffer> CreateConstantBuffer(const ConstantBuffer::Descriptor& desc) override;
		Ref<StructuredBuffer> CreateStructuredBuffer(const StructuredBuffer::Descriptor& desc) override;

		Ref<Texture2D> CreateTexture2D(const Texture2D::Descriptor& descriptor) override;
		Ref<Texture2DArray> CreateTexture2DArray(const Texture2DArray::Descriptor& descriptor) override;
		Ref<TextureCube> CreateTextureCube(const TextureCube::Descriptor& descriptor) override;
		Ref<TextureCubeArray> CreateTextureCubeArray(const TextureCubeArray::Descriptor& descriptor) override;

		Ref<RenderPass> CreateRenderPass(const RenderPass::Descriptor& desc) override;
		Ref<Framebuffer> CreateFramebuffer(const Framebuffer::Descriptor& desc) override;
		
		uint32_t GetFrameCount() const override;
		uint32_t GetCurrentFrameIndex() const override;

		Ref<Texture2D> GetFrameColorAttachment(uint32_t frameIndex) const override;

		GraphicsCommandQueue& GetCommandQueue() override;

		void Resize(int32_t width, int32_t height) override;
		void GetSize(int32_t& width, int32_t& height) override;
		uint32_t GetMSAASampleCount() const override;

		PixelFormat GetSurfaceFormat() const override;
		PixelFormat GetDepthStencilFormat() const override;

		Ref<ComputeShader> CreateComputeShader(const ComputeShader::Descriptor& descriptor) override;
		Ref<ComputePipeline> CreateComputePipeline() override;

		void AddOnResizeHandler(uint32_t id, const std::function<void(int32_t, int32_t)>& handler);
		void RemoveOnResizeHandler(uint32_t id);

		void AddDelayedDeletionTasks(const std::function<void()>& task);

        inline vk::Instance GetVkInstance() const { return _instance; }
        inline vk::PhysicalDevice GetVkPhysicalDevice() const { return _physicalDevice; }
        inline vk::Device GetVkDevice() const { return _device; }
        inline vk::SurfaceKHR GetVkSurface() const { return _surface; }
		inline VkSwapchain& GetVkSwapchain() { return *_swapchain; }
		inline vk::Queue GetVkGraphicsQueue() const { return _graphicsQueue; }
		inline uint32_t GetVkGraphicsQueueFamilyIndex() const { return _queueFamilyIndices.graphicsFamily.value(); }
		inline vk::Queue GetVkPresentQueue() const { return _presentQueue; }
		inline uint32_t GetVkPresentQueueFamilyIndex() const { return _queueFamilyIndices.presentFamily.value(); }
		inline vk::CommandPool GetVkGraphicsCommandPool() const { return _graphicsCommandPool; }
		inline vk::DescriptorPool GetVkDescriptorPool() const { return _descriptorPool; }

	private:
        int32_t CreateInstance(PlatformContext& context, uint32_t version);

        int32_t CreateDebugMessenger();

        int32_t CreateSurface(PlatformContext& context);

        int32_t ChoosePhysicalDevice();

        int32_t CreateLogicalDevice();

		int32_t CreateCommandPools();

		int32_t CreateDescriptorPool();

	private:
		constexpr static uint32_t MaxDescriptorSetsCount = 1000;
		constexpr static uint32_t MaxConstantBufferBindingCount = 16;
		constexpr static uint32_t MaxStructuredBufferBindingCount = 16;
		constexpr static uint32_t MaxTextureBindingCount = 64;
		constexpr static uint32_t MaxInputAttachmentBindingCount = 8;
		
		constexpr static uint32_t MaxDeletionCounter = 1000;

        vk::Instance _instance;

        vk::DebugUtilsMessengerEXT _debugMessenger;

        vk::SurfaceKHR _surface;

		vk::PhysicalDevice _physicalDevice;
		VkQueueFamilyIndices _queueFamilyIndices;
		uint32_t _msaaSampleCount;

        vk::Device _device;
		
		vk::Queue _graphicsQueue;
		vk::Queue _presentQueue;

        vk::CommandPool _graphicsCommandPool;

		vk::DescriptorPool _descriptorPool;

		Ref<VkSwapchain> _swapchain;
		Ref<VkCommandQueue> _commandQueue;

		int32_t _renderWidth, _renderHeight;
		uint32_t _frameCount;

		std::unordered_map<uint32_t, std::list<std::function<void(int32_t, int32_t)>>::iterator> _onResizeHandlerIters;
		std::list<std::function<void(int32_t, int32_t)>> _onResizeHandlers;

		uint32_t _currentDeletionCounter;
		struct DelayedDeletionTask {
			uint32_t counterToExecute;
			std::function<void()> task;
		};
		std::queue<DelayedDeletionTask> _delayedDeletionTasks;
	};
}

#endif