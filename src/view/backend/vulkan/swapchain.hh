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
    Swapchain(int width, int height, Device device, VkSurfaceKHR surface);

    VkSwapchainKHR GetVkSwapchainKHR();
    std::vector<VkImageView> GetImageViews();
    VkFormat GetFormat();
    VkExtent2D GetExtent();
    u32 GetImageCount();
    u32 GetMinImageCount();
    void CreateFrameBuffers(VkDevice device, VkRenderPass render_pass);
    std::vector<VkFramebuffer> GetFrameBuffers();
    // TODO: Aquire next image index
    // TODO: Swapchain Present
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
    u32 m_image_count;
    u32 m_min_image_count;
    std::vector<VkFramebuffer> m_framebuffers;

    // private member functions
    void createSwapChain(int width, int height, Device device, VkSurfaceKHR surface);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, int width, int height);
    void createImageViews(Device device);
};

}// end ns
}