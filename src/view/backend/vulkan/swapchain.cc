/*
 * swapchain.cc
 *
 * Travis Banken
 * 7/17/2021
 * 
 * Swapchain wrapper object for the vulkan backend.
 */

#include "swapchain.hh"

#include "view/backend/vulkan/util.hh"

#define VSWAPCHAIN_INFO(...) PSXLOG_INFO("Vulkan Swapchain", __VA_ARGS__)
#define VSWAPCHAIN_WARN(...) PSXLOG_WARN("Vulkan Swapchain", __VA_ARGS__)
#define VSWAPCHAIN_ERROR(...) PSXLOG_ERROR("Vulkan Swapchain", __VA_ARGS__)
#define VSWAPCHAIN_FATAL(...) VSWAPCHAIN_ERROR(__VA_ARGS__); throw std::runtime_error(PSX_FMT(__VA_ARGS__))

namespace Psx {
namespace Vulkan {

// **********************
// *** PUBLIC METHODS ***
// **********************
Swapchain::Swapchain(int width, int height, Device device, VkSurfaceKHR surface)
{
    createSwapChain(width, height, device, surface);
    createImageViews(device);
}

std::vector<VkImageView> Swapchain::GetImageViews()
{
    return m_image_views;
}

VkFormat Swapchain::GetFormat()
{
    return m_format;
}

VkExtent2D Swapchain::GetExtent()
{
    return m_extent;
}

/*
 * Returns the current VkSwapchainKHR object.
 */
VkSwapchainKHR Swapchain::GetVkSwapchainKHR()
{
    return m_swapchain;
}

u32 Swapchain::GetMinImageCount()
{
    return m_min_image_count;
}

u32 Swapchain::GetImageCount()
{
    return m_image_count;
}

void Swapchain::CreateFrameBuffers(VkDevice device, VkRenderPass render_pass)
{
    VSWAPCHAIN_INFO("Creating framebuffers");
    m_framebuffers.resize(m_image_views.size());
    for (int i = 0; i < m_image_views.size(); i++) {
        VkImageView attachments[] = {
            m_image_views[i]
        };

        VkFramebufferCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        info.renderPass = render_pass;
        info.attachmentCount = 1;
        info.pAttachments = attachments;
        info.width = m_extent.width;
        info.height = m_extent.height;
        info.layers = 1;

        VkResult res = vkCreateFramebuffer(device, &info, nullptr, &m_framebuffers[i]);
        if (res != VK_SUCCESS) {
            VSWAPCHAIN_FATAL("Failed to create framebuffer! [rc: {}]", res);
        }
    }
}

std::vector<VkFramebuffer> Swapchain::GetFrameBuffers()
{
    return m_framebuffers;
}

// ***********************
// *** PRIVATE METHODS ***
// ***********************
/*
 * Create and return the swap chain for the device and surface given.
 */
void Swapchain::createSwapChain(int width, int height, Device device, VkSurfaceKHR surface)
{
    VSWAPCHAIN_INFO("Creating swapchain");
    Util::SwapChainSupportDetails details = Util::QuerySwapChainSupport(device.GetPhysicalDevice(), surface);

    VkSurfaceFormatKHR surface_format = chooseSwapSurfaceFormat(details.formats);
    VkPresentModeKHR present_mode = chooseSwapPresentMode(details.preset_modes);
    VkExtent2D extent = chooseSwapExtent(details.capabilities, width, height);

    // we want the minimum number of images + 1 in the swap chain
    // if we are allowed to
    u32 image_count = details.capabilities.minImageCount + 1;
    if (details.capabilities.maxImageCount > 0 && image_count > details.capabilities.maxImageCount) {
        // clamp
        image_count = details.capabilities.maxImageCount;
    }
    m_min_image_count = image_count;

    // config the swap chain
    VkSwapchainCreateInfoKHR create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = surface;
    create_info.minImageCount = image_count;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageExtent = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    // specify how swap chain images will be used across mulitple queue families
    Util::QueueFamilyIndices indices = Util::FindQueueFamilies(device.GetPhysicalDevice(), surface);
    u32 indices_raw[] = {indices.graphics_family.value(), indices.present_family.value()};
    if (indices.graphics_family != indices.present_family) {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = indices_raw;
    } else {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0;
        create_info.pQueueFamilyIndices = nullptr;
    }
    create_info.preTransform = details.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;

    // create the swap chain
    VkResult res = vkCreateSwapchainKHR(device.GetLogicalDevice(), &create_info, nullptr, &m_swapchain);
    if (res != VK_SUCCESS) {
        VSWAPCHAIN_FATAL("Failed to create swap chain [rc: {}]", res);
    }
    
    // fill in rest of details
    image_count = 0; // reset
    vkGetSwapchainImagesKHR(device.GetLogicalDevice(), m_swapchain, &image_count, nullptr);
    m_images.resize(image_count);
    vkGetSwapchainImagesKHR(device.GetLogicalDevice(), m_swapchain, &image_count, m_images.data());
    m_format = surface_format.format;
    m_extent = extent;
    m_image_count = image_count;
}

/*
 * Return the best swap surface format from the list of available formats.
 */
VkSurfaceFormatKHR Swapchain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats)
{
    VSWAPCHAIN_INFO("Choosing Swap Surface Format");
    // look for a format with 24-bit + transparency color with sRGB color space
    for (const auto& format : available_formats) {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB 
            && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            VSWAPCHAIN_INFO("Ideal Swap Surface found");
            return format;
        }
    }
    // if none are perfect, just return the first one.
    // Could also rank the formats and return the best one.
    VSWAPCHAIN_WARN("Ideal Swap Surface not found, defaulting to first available surface");
    return available_formats[0];
}

/*
 * Return the best swap present mode from the given list of available present modes.
 */
VkPresentModeKHR Swapchain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes)
{
    VSWAPCHAIN_INFO("Choosing Swap Present Mode");
    // we want Mailbox mode which will allows for the smallest latency
    // (similar to unlocked fps with little screen tearing)
    for (const auto& present_mode : available_present_modes) {
        if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            VSWAPCHAIN_INFO("Using Mailbox Present Mode");
            return present_mode;
        }
    }

    // settle for FIFO mode (similar to V-Sync)
    VSWAPCHAIN_INFO("Using FIFO Present Mode");
    return VK_PRESENT_MODE_FIFO_KHR;
}

/*
 * Return the swap extent limited by our capabilities and window.
 */
VkExtent2D Swapchain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, int width, int height)
{
    VSWAPCHAIN_INFO("Choosing Swap Extent");
    if (capabilities.currentExtent.width != UINT32_MAX) {
        VSWAPCHAIN_WARN("Surface capabilities is UINT32_MAX, ignoring specified width/height");
        return capabilities.currentExtent;
    } else {
        VkExtent2D actual_extent = {
            static_cast<u32>(width),
            static_cast<u32>(height)
        };

        actual_extent.width = std::clamp(actual_extent.width, 
            capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actual_extent.height = std::clamp(actual_extent.width, 
            capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        
        return actual_extent;
    }
}

/*
 * Create image views for the swap chain.
 */
void Swapchain::createImageViews(Device device)
{
    VSWAPCHAIN_INFO("Creating Image Views");
    m_image_views.resize(m_images.size());
    for (size_t i = 0; i < m_images.size(); i++) {
        VkImageViewCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = m_images[i];
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = m_format;
        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;
        VkResult res = vkCreateImageView(device.GetLogicalDevice(), &create_info, nullptr, &m_image_views[i]);
        if (res != VK_SUCCESS) {
            VSWAPCHAIN_FATAL("Failed to create image view");
        }
    }
}

}// end ns
}