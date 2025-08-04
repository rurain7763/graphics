#include "pch.h"
#include "VkPhysicalDevices.h"

#ifdef SUPPORT_VULKAN

#include "VkContext.h"
#include "Log/Log.h"

namespace flaw {
    VkPhysicalDevices::VkPhysicalDevices(VkContext& context)
        : _context(context)
        , _selectedDeviceIndex(UINT_MAX)
    {
    }

    void VkPhysicalDevices::SearchDevices() {
        auto physicalDevices = _context.GetVkInstance().enumeratePhysicalDevices().value;

        _devices.resize(physicalDevices.size());
        for (int32_t i = 0; i < physicalDevices.size(); ++i) {
            vk::PhysicalDevice& physicalDevice = physicalDevices[i];

            DeviceEntry& entry = _devices[i];
            entry.device = physicalDevice;
            entry.properties = physicalDevice.getProperties();
            entry.queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
            entry.supportsPresent.resize(entry.queueFamilyProperties.size(), false);
            for (uint32_t j = 0; j < entry.queueFamilyProperties.size(); ++j) {
                entry.supportsPresent[j] = physicalDevice.getSurfaceSupportKHR(j, _context.GetVkSurface()).value;
            }
            entry.surfaceFormats = physicalDevice.getSurfaceFormatsKHR(_context.GetVkSurface()).value;
            entry.surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(_context.GetVkSurface()).value;
            entry.surfacePresentModes = physicalDevice.getSurfacePresentModesKHR(_context.GetVkSurface()).value;
            entry.memoryProperties = physicalDevice.getMemoryProperties();
            entry.features = physicalDevice.getFeatures();
        }
    }

    int32_t VkPhysicalDevices::SelectDevice(vk::QueueFlags queueFlags, bool supportPresent) {
        for (uint32_t i = 0; i < _devices.size(); ++i) {
            const auto& entry = _devices[i];

            for (uint32_t j = 0; j < entry.queueFamilyProperties.size(); ++j) {
                const auto& qfp = entry.queueFamilyProperties[j];

                bool matchesFlags = (qfp.queueFlags & queueFlags) == queueFlags;
                bool canPresent = entry.supportsPresent[j];

                if (matchesFlags && canPresent == supportPresent) {
                    _selectedDeviceIndex = i;
                    _selectedQueueFamilyIndex = j;
                    return 0;
                }
            }
        }

        return -1;
    }

    vk::PhysicalDevice VkPhysicalDevices::SelectedDevice() const {
        return _devices[_selectedDeviceIndex].device;
    }

    uint32_t VkPhysicalDevices::SelectedQueueFamilyIndex() const {
        return _selectedQueueFamilyIndex;
    }

    void VkPhysicalDevices::PrintAllDevices() const {
        for (const auto& entry : _devices) {
            Log::Info("Device: %s", entry.properties.deviceName.data());
            Log::Info("  Type: %s", vk::to_string(entry.properties.deviceType).c_str());
            Log::Info("  API Version: %d.%d.%d",
                      VK_VERSION_MAJOR(entry.properties.apiVersion),
                      VK_VERSION_MINOR(entry.properties.apiVersion),
                      VK_VERSION_PATCH(entry.properties.apiVersion));

            Log::Info("  Queue Families Count: %d", entry.queueFamilyProperties.size());
            for (size_t i = 0; i < entry.queueFamilyProperties.size(); ++i) {
                const auto& qfp = entry.queueFamilyProperties[i];
                Log::Info("    Queue Family %zu:", i);
                Log::Info("      Count: %d", qfp.queueCount);
                Log::Info("      Flags: %s", vk::to_string(qfp.queueFlags).c_str());
                Log::Info("      Supports Present: %s", entry.supportsPresent[i] ? "Yes" : "No");
            }
        }
    }
}

#endif