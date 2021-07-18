/*
 * device.hh
 *
 * Travis Banken
 * 7/17/2021
 * 
 * Device wrapper object for the vulkan backend.
 */
#pragma once

#include "view/backend/vulkan/includes.hh"
#include "util/psxutil.hh"

#include <vector>
#include <set>

namespace Psx {
namespace Vulkan {

class Device {
public:
    Device() {} // do nothing constructor
    Device(VkInstance instance, VkSurfaceKHR surface);

    VkPhysicalDevice GetPhysicalDevice();
    VkDevice GetLogicalDevice();
private:

    VkPhysicalDevice m_physical_device = VK_NULL_HANDLE;
    VkDevice m_logical_device = VK_NULL_HANDLE;
    std::vector<const char*> m_extensions;

    // logical device
    void createLogicalDevice(VkSurfaceKHR surface);

    // physical device
    void pickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);
    int ratePhysicalDevice(VkSurfaceKHR surface, VkPhysicalDevice dev, const std::vector<const char*>& device_extensions);
    bool isDeviceSuitable(VkSurfaceKHR surface, VkPhysicalDevice dev, const std::vector<const char*>& device_extensions);
    bool checkDeviceExtensionsSupported(VkPhysicalDevice dev, const std::vector<const char*>& device_extensions);
};

}// end ns
}