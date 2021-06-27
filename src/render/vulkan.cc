/*
 * vulkan.cc
 *
 * Travis Banken
 * 6/21/21
 *
 * Vulkan backend for rendering.
 */

#include "vulkan.hh"

#include <vulkan/vulkan.h>
#include "glfw/glfw3.h"

#include "util/psxutil.hh"
#include "render/vulkan_instance.hh"
#include "layer/imgui_layer.hh"
#include "render/utils.hh"

#define VULKAN_INFO(...) PSXLOG_INFO("Vulkan Backend", __VA_ARGS__)
#define VULKAN_WARN(...) PSXLOG_WARN("Vulkan Backend", __VA_ARGS__)
#define VULKAN_ERROR(...) PSXLOG_ERROR("Vulkan Backend", __VA_ARGS__)
#define VULKAN_FATAL(...) VULKAN_ERROR(__VA_ARGS__); throw std::runtime_error(PSX_FMT(__VA_ARGS__))

namespace {

struct State {
    Psx::Vulkan::Instance::Details ideets;
} s;

// protos

}// end private ns

namespace Psx {
namespace Vulkan {

/*
 * Initializes Vulkan as a backend to the PSX.
 */
void Init(std::vector<const char *> extensions)
{
    VULKAN_INFO("Initializing Vulkan backend");
    s.ideets = Instance::Create(extensions);
}

/*
 * Shutdown backend and release resources.
 */
void Shutdown()
{
    VULKAN_INFO("Shutting down Vulkan backend");
    Instance::Destroy(s.ideets);
}

/*
 * Initialize ImGui with the Vulkan Backend.
 */
void SetupImGui()
{
    VULKAN_INFO("Initializing ImGui for Vulkan");
    // Create Descriptor Pool
    VkDescriptorPoolSize pool_sizes[] = {
        {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}
    };
    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000 * std::size(pool_sizes);
    pool_info.poolSizeCount = (u32) std::size(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;
    VkDescriptorPool dpool;
    VkResult res = vkCreateDescriptorPool(s.ideets.device, &pool_info, nullptr, &dpool);
    if (res != VK_SUCCESS) {
        VULKAN_FATAL("Failed to create Descriptor Pool");
    }

    // TODO: Use "non-hacky" values
    ImGui_ImplVulkan_InitInfo info{};
    info.Instance = s.ideets.instance;
    info.PhysicalDevice = s.ideets.physical_device;
    info.Device = s.ideets.device;
    info.QueueFamily = 0;
    info.Queue = s.ideets.graphics_queue;
    info.DescriptorPool = dpool;
    info.Allocator = nullptr;
    info.MinImageCount = 2; // TODO
    info.ImageCount = 2; // TODO

    ImGui_ImplVulkan_Init(&info, s.ideets.render_pass);
}

/*
 * Returns the current render pass in use by the vulkan backend.
 */
VkRenderPass GetRenderPass()
{
    return s.ideets.render_pass;
}

}// end ns
}

// PRIVATE METHODS
namespace {

}// end private ns
