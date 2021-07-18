/*
 * util.cc
 *
 * Travis Banken
 * 7/17/2021
 * 
 * Common utilities for the vulkan backend.
 */

#include "view/backend/vulkan/util.hh"

namespace Psx {
namespace Vulkan {
namespace Util {

/*
 * Get the swap chain support details for a given device.
 */
SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice dev, VkSurfaceKHR surface)
{
    SwapChainSupportDetails details;

    // get capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dev, surface, &details.capabilities);

    // get formats
    u32 format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &format_count, nullptr);
    if (format_count != 0) {
        details.formats.resize(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &format_count, details.formats.data());
    }

    // get preset modes
    u32 present_mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &present_mode_count, nullptr);
    if (present_mode_count != 0) {
        details.preset_modes.resize(present_mode_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &present_mode_count, details.preset_modes.data());
    }

    return details;
}

/*
 * Get the Queue Families for the given device
 */
QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice dev, VkSurfaceKHR surface)
{
    QueueFamilyIndices indices;
    // get number of queue families
    u32 queue_count;
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &queue_count, nullptr);

    // get the queue families
    std::vector<VkQueueFamilyProperties> queue_families(queue_count);
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &queue_count, queue_families.data());

    for (u32 i = 0; i < queue_count; i++) {
        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphics_family = i;
        }

        // check for presentation support
        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, surface, &present_support);
        if (present_support) {
            indices.present_family = i;
        }

        // break early when entire struct if filled
        if (indices.IsComplete()) {
            break;
        }
    }


    return indices;
}

}// end ns
}
}