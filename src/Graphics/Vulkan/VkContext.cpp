#include "pch.h"
#include "VkContext.h"

#ifdef SUPPORT_VULKAN

#include "Platform/PlatformContext.h"
#include "Log/Log.h"
#include "VkSwapchain.h"
#include "VkShaders.h"
#include "VkCommandQueue.h"
#include "VkPipelines.h"
#include "VkBuffers.h"
#include "VkTextures.h"
#include "VkFramebuffer.h"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace flaw {
    static VKAPI_ATTR vk::Bool32 DebugCallback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        vk::DebugUtilsMessageTypeFlagsEXT messageType,
        const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) 
    {
        if (messageSeverity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eError) {
            Log::Error("Vulkan Error: %s", pCallbackData->pMessage);
        } else if (messageSeverity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning) {
            Log::Warn("Vulkan Warning: %s", pCallbackData->pMessage);
        } else {
            Log::Info("Vulkan Info: %s", pCallbackData->pMessage);
        }

        return VK_FALSE; // Returning false indicates that the message should not be aborted
    }

	VkContext::VkContext(PlatformContext& context, int32_t width, int32_t height) 
        : _renderWidth(width)
        , _renderHeight(height)
        , _currentDeletionCounter(0)
    {
        VULKAN_HPP_DEFAULT_DISPATCHER.init();

		uint32_t vkVersion = vk::enumerateInstanceVersion().value;
        Log::Info("Vulkan API Version: %d.%d.%d", VK_VERSION_MAJOR(vkVersion), VK_VERSION_MINOR(vkVersion), VK_VERSION_PATCH(vkVersion));

        vkVersion &= ~(0xFFFU); // Clear patch version

        if (CreateInstance(context, vkVersion)) {
            Log::Fatal("Failed to create Vulkan instance.");
            return;
        }

		VULKAN_HPP_DEFAULT_DISPATCHER.init(_instance);

        if (CreateDebugMessenger()) {
            Log::Fatal("Failed to create Vulkan debug messenger.");
            return;
        }

        if (CreateSurface(context)) {
            Log::Fatal("Failed to create Vulkan surface.");
            return;
        }

        if (ChoosePhysicalDevice()) {
            Log::Fatal("Failed to choose a suitable Vulkan physical device.");
            return;
        }

        auto surfaceDetails = GetVkSurfaceDetails(_physicalDevice, _surface);
        _frameCount = std::max(surfaceDetails.capabilities.minImageCount + 1, surfaceDetails.capabilities.maxImageCount);

        _queueFamilyIndices = GetVkQueueFamilyIndices(_physicalDevice, _surface);
        if (!_queueFamilyIndices.IsComplete()) {
            Log::Fatal("No suitable queue family found.");
            return;
        }

        _msaaSampleCount = GetMaxUsableSampleCount(_physicalDevice);

        if (CreateLogicalDevice()) {
            Log::Fatal("Failed to create Vulkan logical device.");
            return;
        }

        VULKAN_HPP_DEFAULT_DISPATCHER.init(_device);

        _graphicsQueue = _device.getQueue(_queueFamilyIndices.graphicsFamily.value(), 0);
        _presentQueue = _device.getQueue(_queueFamilyIndices.presentFamily.value(), 0);

        if (CreateCommandPools()) {
            return;
        }

        if (CreateDescriptorPool()) {
            return;
        }
        
        _commandQueue = CreateRef<VkCommandQueue>(*this);
        
        _swapchain = CreateRef<VkSwapchain>(*this);
        if (_swapchain->Create(_renderWidth, _renderHeight)) {
            Log::Fatal("Failed to create Vulkan swap chain.");
            return;
        }
    }

    int32_t VkContext::CreateInstance(PlatformContext& context, uint32_t version) {
        vk::ApplicationInfo appInfo;
        appInfo.pApplicationName = "Flaw Application";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "Flaw Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = version;

        std::vector<const char*> requiredExtensions;
        context.GetVkRequiredExtensions(requiredExtensions);
        requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        std::vector<const char*> validationLayers =  {
            "VK_LAYER_KHRONOS_validation"
        };
        
        if (!CheckSupportedInstanceExtensions(requiredExtensions)) {
            Log::Fatal("Required Vulkan instance extensions are not supported.");
            return -1;
        }

        if (!CheckSupportedInstanceLayers(validationLayers)) {
            Log::Fatal("Required Vulkan instance layers are not supported.");
            return -1;
        }

        vk::InstanceCreateInfo createInfo;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
        createInfo.ppEnabledExtensionNames = requiredExtensions.data();
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    #ifdef __APPLE__
        createInfo.flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
    #endif

        auto instanceWrapper = vk::createInstance(createInfo);
        if (instanceWrapper.result != vk::Result::eSuccess) {
            Log::Fatal("Failed to create Vulkan instance: %s", vk::to_string(instanceWrapper.result).c_str());
            return -1;
        }

        _instance = instanceWrapper.value;

        return 0;
    }

    int32_t VkContext::CreateDebugMessenger() {
        vk::DebugUtilsMessengerCreateInfoEXT createInfo;
        createInfo.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
                                     vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning;

        createInfo.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                                 vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                                 vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;

        createInfo.pfnUserCallback = DebugCallback;

        auto result = _instance.createDebugUtilsMessengerEXT(createInfo, nullptr);
        if (result.result != vk::Result::eSuccess) {
            Log::Fatal("Failed to create Vulkan debug messenger: %s", vk::to_string(result.result).c_str());
            return -1;
        }

        _debugMessenger = result.value;

        return 0;
    }

    int32_t VkContext::CreateSurface(PlatformContext& context) {
        context.GetVkSurface(_instance, _surface);
        if (!_surface) {
            Log::Error("Failed to create Vulkan surface.");
            return -1;
        }

        return 0;
    }

    int32_t VkContext::ChoosePhysicalDevice() {
        auto devices = _instance.enumeratePhysicalDevices().value;

        std::vector<const char*> requiredExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

    #ifdef __APPLE__
        requiredExtensions.push_back("VK_KHR_portability_subset");
    #endif

        for (const auto& device : devices) {
            auto properties = device.getProperties();

            Log::Info("Found Vulkan physical device: %s", properties.deviceName.data());
            Log::Info("  Type: %s", vk::to_string(properties.deviceType).c_str());

            if (CheckSupportedPhysicalDeviceExtensions(device, requiredExtensions)) {
                Log::Info("Selected Vulkan physical device: %s", properties.deviceName.data());
                _physicalDevice = device;
                return 0;
            }
            else {
                Log::Info("Physical device %s is not suitable.", properties.deviceName.data());
            }
        }

        return -1;
    }
    
    int32_t VkContext::CreateLogicalDevice() {
        std::unordered_set<uint32_t> uniqueQueueFamilies = {
            _queueFamilyIndices.graphicsFamily.value(),
            _queueFamilyIndices.presentFamily.value()
        };

        float queuePriority = 1.0f;
        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
        for (uint32_t queueFamilyIndex : uniqueQueueFamilies) {
            vk::DeviceQueueCreateInfo queueCreateInfo;
            queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;

            queueCreateInfos.push_back(queueCreateInfo);
        }

        vk::PhysicalDeviceFeatures deviceFeatures;
        deviceFeatures.samplerAnisotropy = VK_TRUE;
        deviceFeatures.sampleRateShading = VK_TRUE;
        deviceFeatures.tessellationShader = VK_TRUE;
		deviceFeatures.geometryShader = VK_TRUE;

        std::vector<const char*> requiredExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

    #ifdef __APPLE__
        requiredExtensions.push_back("VK_KHR_portability_subset");
    #endif

        std::vector<const char*> requiredLayers = {
            "VK_LAYER_KHRONOS_validation"
        };

        vk::DeviceCreateInfo createInfo;
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
        createInfo.ppEnabledExtensionNames = requiredExtensions.data();
        createInfo.enabledLayerCount = static_cast<uint32_t>(requiredLayers.size());
        createInfo.ppEnabledLayerNames = requiredLayers.data();
        createInfo.pEnabledFeatures = &deviceFeatures;

        auto deviceWrapper = _physicalDevice.createDevice(createInfo, nullptr);
        if (deviceWrapper.result != vk::Result::eSuccess) {
            Log::Fatal("Failed to create Vulkan logical device: %s", vk::to_string(deviceWrapper.result).c_str());
            return -1;
        }

        _device = deviceWrapper.value;
    
        Log::Info("Vulkan logical device created successfully.");

        return 0;
    }

    int32_t VkContext::CreateCommandPools() {
        vk::CommandPoolCreateInfo poolInfo;
        poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        poolInfo.queueFamilyIndex = _queueFamilyIndices.graphicsFamily.value();

        auto result = _device.createCommandPool(poolInfo, nullptr);
        if (result.result != vk::Result::eSuccess) {
            Log::Fatal("Failed to create Vulkan command pool: %s", vk::to_string(result.result).c_str());
            return -1;
        }

        _graphicsCommandPool = result.value;

        return 0;
    }

    int32_t VkContext::CreateDescriptorPool() {
        std::vector<vk::DescriptorPoolSize> poolSizes = {
            { vk::DescriptorType::eUniformBuffer, MaxConstantBufferBindingCount },
            { vk::DescriptorType::eStorageBuffer, MaxStructuredBufferBindingCount },
            { vk::DescriptorType::eCombinedImageSampler, MaxTextureBindingCount },
			{ vk::DescriptorType::eInputAttachment, MaxInputAttachmentBindingCount }
        };

        vk::DescriptorPoolCreateInfo poolInfo;
        poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
        poolInfo.maxSets = MaxDescriptorSetsCount;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();

        auto result = _device.createDescriptorPool(poolInfo, nullptr);
        if (result.result != vk::Result::eSuccess) {
            Log::Error("Failed to create Vulkan descriptor pool: %s", vk::to_string(result.result).c_str());
            return -1;
        }

        _descriptorPool = result.value;

        Log::Info("Vulkan descriptor pool created successfully.");

        return 0;
    }

	VkContext::~VkContext() {
        _device.waitIdle();

        _commandQueue.reset();
        _swapchain.reset();

        while( !_delayedDeletionTasks.empty()) {
            auto& task = _delayedDeletionTasks.front();
            task.task();
            _delayedDeletionTasks.pop();
        }

        _device.destroyDescriptorPool(_descriptorPool, nullptr);
        _device.destroyCommandPool(_graphicsCommandPool, nullptr);

        _device.destroy(nullptr);
        _instance.destroySurfaceKHR(_surface, nullptr);

        if (_debugMessenger) {
            _instance.destroyDebugUtilsMessengerEXT(_debugMessenger, nullptr);
        }

        _instance.destroy();
	}

	bool VkContext::Prepare() {
        if (!_commandQueue->Prepare()) {
            return false;
        }

        while(!_delayedDeletionTasks.empty()) {
            auto& task = _delayedDeletionTasks.front();
            if (task.counterToExecute == _currentDeletionCounter) {
                task.task();
                _delayedDeletionTasks.pop();
            } else {
                break;
            }
        }
        _currentDeletionCounter = (_currentDeletionCounter + 1) % MaxDeletionCounter;

        return true;
    }

    Ref<VertexInputLayout> VkContext::CreateVertexInputLayout(const VertexInputLayout::Descriptor& descriptor) {
        return CreateRef<VkVertexInputLayout>(*this, descriptor);
    }

	Ref<VertexBuffer> VkContext::CreateVertexBuffer(const VertexBuffer::Descriptor& descriptor) {
        return CreateRef<VkVertexBuffer>(*this, descriptor);
    }

	Ref<IndexBuffer> VkContext::CreateIndexBuffer(const IndexBuffer::Descriptor& descriptor) {
        return CreateRef<VkIndexBuffer>(*this, descriptor);
    }

    Ref<ShaderResourcesLayout> VkContext::CreateShaderResourcesLayout(const ShaderResourcesLayout::Descriptor& descriptor) {
        return CreateRef<VkShaderResourcesLayout>(*this, descriptor);
    }

    Ref<ShaderResources> VkContext::CreateShaderResources(const ShaderResources::Descriptor& descriptor) {
        return CreateRef<VkShaderResources>(*this, descriptor);
    }

	Ref<GraphicsShader> VkContext::CreateGraphicsShader(const GraphicsShader::Descriptor& descriptor) {
        return CreateRef<VkGraphicsShader>(*this, descriptor);
    }

	Ref<GraphicsPipeline> VkContext::CreateGraphicsPipeline() {
        return CreateRef<VkGraphicsPipeline>(*this);
	}

	Ref<ConstantBuffer> VkContext::CreateConstantBuffer(const ConstantBuffer::Descriptor& desc) {
		return CreateRef<VkConstantBuffer>(*this, desc);
	}

	Ref<StructuredBuffer> VkContext::CreateStructuredBuffer(const StructuredBuffer::Descriptor& desc) {
        return CreateRef<VkStructuredBuffer>(*this, desc);
    }

	Ref<Texture2D> VkContext::CreateTexture2D(const Texture2D::Descriptor& descriptor) {
        return CreateRef<VkTexture2D>(*this, descriptor);
    }

	Ref<Texture2DArray> VkContext::CreateTexture2DArray(const Texture2DArray::Descriptor& descriptor) {
		return nullptr;
	}

	Ref<TextureCube> VkContext::CreateTextureCube(const TextureCube::Descriptor& descriptor) {
		return CreateRef<VkTextureCube>(*this, descriptor);
	}

    uint32_t VkContext::GetFrameCount() const {
        return _frameCount;
    }

	uint32_t VkContext::GetCurrentFrameIndex() const {
		return _commandQueue->GetCurrentFrameIndex();
	}

	Ref<Texture2D> VkContext::GetFrameColorAttachment(uint32_t frameIndex) const {
		return _swapchain->GetColorAttachment(frameIndex);
	}

	GraphicsCommandQueue& VkContext::GetCommandQueue() {
		return *_commandQueue;
	}

	Ref<RenderPass> VkContext::CreateRenderPass(const RenderPass::Descriptor& desc) {
        return CreateRef<VkRenderPass>(*this, desc);
    }

    Ref<Framebuffer> VkContext::CreateFramebuffer(const Framebuffer::Descriptor& desc) {
        return CreateRef<VkFramebuffer>(*this, desc);
    }

	void VkContext::Resize(int32_t width, int32_t height) {
        if (width <= 0 || height <= 0) {
            Log::Error("Invalid dimensions for Vulkan context resize: %dx%d", width, height);
            return;
        }

        if (_renderWidth == width && _renderHeight == height) {
            return; // No change needed
        }

        _renderWidth = width;
        _renderHeight = height;

        _device.waitIdle();

        _swapchain->Resize(static_cast<uint32_t>(width), static_cast<uint32_t>(height));

		for (const auto& handler : _onResizeHandlers) {
			handler.second(width, height);
		}
	}

	void VkContext::GetSize(int32_t& width, int32_t& height) {
		width = _renderWidth;
		height = _renderHeight;
	}

	uint32_t VkContext::GetMSAASampleCount() const {
		return _msaaSampleCount;
	}

    PixelFormat VkContext::GetSurfaceFormat() const {
        return _swapchain->GetSurfaceFormat();
    }

    PixelFormat VkContext::GetDepthStencilFormat() const {
        return _swapchain->GetDepthStencilFormat();
    }

	Ref<ComputeShader> VkContext::CreateComputeShader(const ComputeShader::Descriptor& descriptor) {
        return CreateRef<VkComputeShader>(*this, descriptor);
	}

	Ref<ComputePipeline> VkContext::CreateComputePipeline() {
        return CreateRef<VkComputePipeline>(*this);
	}

	void VkContext::AddOnResizeHandler(uint32_t id, const std::function<void(int32_t, int32_t)>& handler) {
		_onResizeHandlers[id] = handler;
	}

	void VkContext::RemoveOnResizeHandler(uint32_t id) {
		_onResizeHandlers.erase(id);
	}

    void VkContext::AddDelayedDeletionTasks(const std::function<void()>& task) {
        _delayedDeletionTasks.push(DelayedDeletionTask{ (_currentDeletionCounter + 2) % MaxDeletionCounter, task });
    }
}

#endif
