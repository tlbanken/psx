/*
 * builder.hh
 *
 * Travis Banken
 * 7/22/2021
 * 
 * Functions for building vulkan objects.
 */
#pragma once

#include "view/backend/vulkan/includes.hh"
#include "util/psxutil.hh"

#include <vector>

namespace Psx {
namespace Vulkan {
namespace Builder {

struct FrameData {
    VkCommandPool command_pool;
    VkCommandBuffer command_buffer;
    VkFence fence;
    VkImage backbuffer;
    VkImageView backbuffer_view;
    VkFramebuffer framebuffer;
};

struct FrameSemaphores {
    VkSemaphore image_acquire;
    VkSemaphore render_complete;
};

// similar struct used by imgui for vulkan window management
struct WindowData {
    VkExtent2D extent;
    VkSwapchainKHR swapchain;
    VkSurfaceKHR surface;
    VkSurfaceFormatKHR surface_format;
    VkPresentModeKHR present_mode;
    VkRenderPass render_pass;
    VkPipeline pipeline;
    VkPipelineLayout pipeline_layout;
    bool clear_enable;
    VkClearValue clear_value;
    u32 frame_index;
    u32 image_count;
    u32 min_image_count;
    u32 semaphore_index;
    std::vector<FrameData> frames;
    std::vector<FrameSemaphores> frame_semaphores;
    VkBuffer vertex_buffer;
    VkDeviceMemory vertex_buffer_memory;

    WindowData()
    {
        memset(this, 0, sizeof(*this));
        present_mode = VK_PRESENT_MODE_MAX_ENUM_KHR;
        clear_enable = true;
    }
};

struct DeviceData {
    struct PhysicalData {
        VkPhysicalDevice dev;
        VkPhysicalDeviceProperties props;
        VkQueue graphics_queue;
        VkQueue present_queue;
        u32 graphics_queue_family;
        u32 present_queue_family;
    } physdata;

    struct LogicalData {
        VkDevice dev;
        std::vector<const char*> extensions;
    } logidata;
};


// Functions
void Init(VkAllocationCallbacks *allocator);
void Destroy(WindowData *wd, DeviceData *dd, VkInstance instance);
void InitializeImGuiVulkan(WindowData *wd, DeviceData *dd, VkInstance instance, SDL_Window *window);
VkInstance CreateInstance(std::vector<const char*> extensions);
void SetupDebugMessenger(VkInstance instance);
void BuildSurfaceData(WindowData *wd, VkSurfaceKHR surface);
void BuildDeviceData(DeviceData *dd, VkInstance instance, VkSurfaceKHR surface);
void BuildPhysicalDeviceData(DeviceData *dd, VkInstance instance, VkSurfaceKHR surface);
void BuildLogicalDeviceData(DeviceData *dd, VkSurfaceKHR surface);
void BuildSwapchainData(WindowData *wd, DeviceData *dd, int width, int height);
void RebuildSwapchain(
    WindowData *wd, DeviceData *dd,
    int width, int height,
    const std::string& vert_shader_path, 
    const std::string& frag_shader_path);
void DestroySwapchain(WindowData *wd, DeviceData *dd);
void BuildImageViews(WindowData *wd, DeviceData *dd);
void BuildRenderPassData(WindowData *wd, DeviceData *dd);
void BuildPipelineData(WindowData *wd, DeviceData *dd, const std::string& vs_path, const std::string& fs_path);
void BuildFrameBuffersData(WindowData *wd, DeviceData *dd);
void BuildCommandBuffersData(WindowData *wd, DeviceData *dd);
void BuildVertexBuffer(WindowData *wd, DeviceData *dd);


}// end ns
}
}
