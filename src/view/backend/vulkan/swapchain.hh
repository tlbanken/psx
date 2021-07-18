/*
 * swapchain.hh
 *
 * Travis Banken
 * 7/17/2021
 * 
 * Swapchain wrapper object for the vulkan backend.
 */
#pragma once

#include "view/backend/vulkan/includes.hh"
#include "view/backend/vulkan/device.hh"

#include <vector>

namespace Psx {
namespace Vulkan {

class Swapchain {
public:
    Swapchain() {}
    Swapchain(GLFWwindow *window, Device device, VkSurfaceKHR surface);

    VkSwapchainKHR GetVkSwapchainKHR();
    std::vector<VkImageView> GetImageViews();
    VkFormat GetFormat();
    VkExtent2D GetExtent();
    // TODO: recreate swap chain function
private:
    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> preset_modes;
    };

    // member vars
    VkSwapchainKHR m_swapchain;
    std::vector<VkImage> m_images;
    std::vector<VkImageView> m_image_views;
    VkFormat m_format;
    VkExtent2D m_extent;

    // private member functions
    void createSwapChain(GLFWwindow *window, Device device, VkSurfaceKHR surface);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow *window);
    void createImageViews(Device device);
};

}// end ns
}