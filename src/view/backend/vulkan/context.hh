/*
 * context.hh
 *
 * Travis Banken
 * 7/12/2021
 * 
 * Vulkan context for the graphics backend.
 */
#pragma once

#include <vector>

#include <vulkan/vulkan.h>
#include "glfw/glfw3.h"

#include "util/psxutil.hh"
#include "view/backend/vulkan/device.hh"
#include "view/backend/vulkan/swapchain.hh"

namespace Psx {
namespace Vulkan {

// struct SwapChain {
//     VkSwapchainKHR ptr;
//     std::vector<VkImage> images;
//     std::vector<VkImageView> image_views;
//     VkFormat format;
//     VkExtent2D extent;
// };

struct Pipeline {
    VkPipelineLayout layout;
    VkPipeline ptr;
};

class Context {
public:
    Context(std::vector<const char*> extensions, GLFWwindow *window);
    ~Context();

private:
    // structs and vars
    // struct Device {
    //     VkPhysicalDevice physical = VK_NULL_HANDLE;
    //     VkDevice logical = VK_NULL_HANDLE;
    //     std::vector<const char*> extensions;
    // } m_device;
    VkInstance m_instance;
    VkDebugUtilsMessengerEXT m_debug_messenger;
    VkSurfaceKHR m_surface;
    Device m_device;
    Swapchain m_swapchain;
    // VkFormat m_format;
    // VkExtent2D m_extent;
    // SwapChain m_swapchain;
    VkRenderPass m_render_pass;
    Pipeline m_graphics_pipeline;


    // private methods
    bool checkValidationLayerSupport(const std::vector<const char*>& layers_wanted);
    void setupDebugMessenger();
    void createInstance(std::vector<const char*> extensions);
    void createSurface(GLFWwindow *window);
    // void pickPhysicalDevice();
    // int ratePhysicalDevice(VkPhysicalDevice dev, const std::vector<const char*>& device_extensions);
    // bool isDeviceSuitable(VkPhysicalDevice dev, const std::vector<const char*>& device_extensions);
    // bool checkDeviceExtensionsSupported(VkPhysicalDevice dev, const std::vector<const char*>& device_extensions);
    // SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice dev, VkSurfaceKHR surface);
    // QueueFamilyIndices findQueueFamilies(VkPhysicalDevice dev, VkSurfaceKHR surface);
    // void createLogicalDevice();
    void createSwapChain(GLFWwindow *window);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow *window);
    void createImageViews();
    void createRenderPass();
    void createGraphicsPipeline(const std::string& vs_path, const std::string& fs_path);
    std::vector<char> readSPVFile(const std::string& filepath);
    VkShaderModule createShaderModule(std::vector<char>& bytecode, VkDevice device);
};

}// end ns
}
