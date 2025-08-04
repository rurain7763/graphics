#pragma once

#include "Core.h"

#ifdef SUPPORT_VULKAN

#include "VkCore.h"

namespace flaw {
    class VkContext;

    class VkPhysicalDevices {
    public:
        VkPhysicalDevices(VkContext& context);

        void SearchDevices();
        
        int32_t SelectDevice(vk::QueueFlags queueFlags, bool supportPresent);
        vk::PhysicalDevice SelectedDevice() const;
        uint32_t SelectedQueueFamilyIndex() const;

        void PrintAllDevices() const;

    private:
        VkContext& _context;

        struct DeviceEntry {
            vk::PhysicalDevice device;
            vk::PhysicalDeviceProperties properties;
            std::vector<vk::QueueFamilyProperties> queueFamilyProperties;
            std::vector<vk::Bool32> supportsPresent;
            std::vector<vk::SurfaceFormatKHR> surfaceFormats;
            vk::SurfaceCapabilitiesKHR surfaceCapabilities;
            std::vector<vk::PresentModeKHR> surfacePresentModes;
            vk::PhysicalDeviceMemoryProperties memoryProperties;
            vk::PhysicalDeviceFeatures features;
        };

        std::vector<DeviceEntry> _devices;
        uint32_t _selectedDeviceIndex;
        uint32_t _selectedQueueFamilyIndex;
    };
}

#endif