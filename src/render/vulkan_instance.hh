/*
 * vulkan_instance.hh
 *
 * Travis Banken
 * 6/23/21
 * 
 * Setup utils for the Vulkan backend.
 */
#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace Psx {
namespace Vulkan {
namespace Instance {

struct Details {
    VkInstance instance;
    
    VkDebugUtilsMessengerEXT debug_messenger;

    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    VkDevice device;
    VkQueue graphics_queue;
    VkQueue present_queue;

    struct SwapChainDetails {
        VkSwapchainKHR swap_chain;
        std::vector<VkImage> images;
        VkFormat format;
        VkExtent2D extent;
    }sc_deets;

    VkSurfaceKHR surface;

    // device extensions required
    std::vector<const char*> device_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
};

Details Create(std::vector<const char*> extensions);
void Destroy(Details d);


} // end ns
}
}