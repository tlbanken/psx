/*
 * builder.cc
 *
 * Travis Banken
 * 7/22/2021
 * 
 * Functions for building vulkan objects.
 */

#include "builder.hh"

#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_vulkan.h"

#include <map>
#include <set>

#define VBUILDER_INFO(...) PSXLOG_INFO("Vulkan Builder", __VA_ARGS__)
#define VBUILDER_WARN(...) PSXLOG_WARN("Vulkan Builder", __VA_ARGS__)
#define VBUILDER_ERROR(...) PSXLOG_ERROR("Vulkan Builder", __VA_ARGS__)
#define VBUILDER_FATAL(...) VBUILDER_ERROR(__VA_ARGS__); throw std::runtime_error(PSX_FMT(__VA_ARGS__))

namespace {

struct QueueFamilyIndices {
    std::optional<u32> graphics_family;
    std::optional<u32> present_family;

    bool IsComplete()
    {
        return graphics_family.has_value() &&
            present_family.has_value();
    }

    std::set<u32> GetUniqueQueueFamilies()
    {
        return std::set<u32>{graphics_family.value(), present_family.value()};
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> preset_modes;
};

struct State {
    VkAllocationCallbacks *allocator;
    VkDebugUtilsMessengerEXT debug_messenger;
    
    // TODO Figure out what to do with this
    VkDescriptorPool imgui_descriptor_pool;
}s;

// protos
bool checkValidationLayerSupport(const std::vector<const char*>& layers_wanted);
int ratePhysicalDevice(VkSurfaceKHR surface, VkPhysicalDevice dev, const std::vector<const char*>& device_extensions);
bool isDeviceSuitable(VkSurfaceKHR surface, VkPhysicalDevice dev, const std::vector<const char*>& device_extensions);
bool checkDeviceExtensionsSupported(VkPhysicalDevice dev, const std::vector<const char*>& device_extensions);
SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice dev, VkSurfaceKHR surface);
QueueFamilyIndices findQueueFamilies(VkPhysicalDevice dev, VkSurfaceKHR surface);
VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats);
VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes);
VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, int width, int height);
std::vector<char> readSPVFile(const std::string& filepath);
VkShaderModule createShaderModule(std::vector<char>& bytecode, VkDevice device);

/*
 * Callback function for debug messages from Vulkan.
 */
VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT msg_severity,
    VkDebugUtilsMessageTypeFlagsEXT msg_type,
    const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
    void *user_data)
{
    (void) msg_type;
    (void) user_data;
    // only print warnings and errors
    if (msg_severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        VBUILDER_WARN("**Callback**: {}", callback_data->pMessage);
    } else if (msg_severity > VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        VBUILDER_FATAL("**Callback**: {}", callback_data->pMessage);
    }

    return VK_FALSE;
}

}// end private ns

namespace Psx {
namespace Vulkan {
namespace Builder {

void Init(VkAllocationCallbacks *allocator)
{
    if (allocator == nullptr) {
        VBUILDER_INFO("Not using allocation callbacks");
    }
    s.allocator = allocator;
}

/*
 * Destroy all objects inside the window and device data structs as well as the
 * given vulkan instance.
 */
void Destroy(WindowData *wd, DeviceData *dd, VkInstance instance)
{
    VBUILDER_INFO("Destroying WindowData obj@{}", static_cast<void*>(wd));
    VBUILDER_INFO("Destroying DeviceData obj@{}", static_cast<void*>(dd));

#ifdef PSX_DEBUG
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, s.debug_messenger, s.allocator);
    }
#endif


    // frame data and semaphores
    for (u32 i = 0; i < wd->image_count; i++) {
        FrameData *fd = &wd->frames[i];
        FrameSemaphores *fs = &wd->frame_semaphores[i];

        // wait for command buffers to be done
        vkWaitForFences(dd->logidata.dev, 1, &fd->fence, VK_TRUE, UINT64_MAX);
        vkResetFences(dd->logidata.dev, 1, &fd->fence);
        vkResetCommandPool(dd->logidata.dev, fd->command_pool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);

        // semaphores
        vkDestroySemaphore(dd->logidata.dev, fs->image_acquire, s.allocator);
        vkDestroySemaphore(dd->logidata.dev, fs->render_complete, s.allocator);

        // fence
        vkDestroyFence(dd->logidata.dev, fd->fence, s.allocator);

        // frame buffers
        vkDestroyFramebuffer(dd->logidata.dev, fd->framebuffer, s.allocator);

        // command pool
        vkDestroyCommandPool(dd->logidata.dev, fd->command_pool, s.allocator);

        // image views
        vkDestroyImageView(dd->logidata.dev, fd->backbuffer_view, s.allocator);
    }

    // TODO Descriptor pool
    vkDestroyDescriptorPool(dd->logidata.dev, s.imgui_descriptor_pool, s.allocator);

    // pipeline
    vkDestroyPipeline(dd->logidata.dev, wd->pipeline, s.allocator);
    vkDestroyPipelineLayout(dd->logidata.dev, wd->pipeline_layout, s.allocator);

    // render pass
    vkDestroyRenderPass(dd->logidata.dev, wd->render_pass, s.allocator);

    // swapchain
    vkDestroySwapchainKHR(dd->logidata.dev, wd->swapchain, s.allocator);

    // device
    vkDestroyDevice(dd->logidata.dev, s.allocator);

    // surface
    vkDestroySurfaceKHR(instance, wd->surface, s.allocator);

    // instance
    vkDestroyInstance(instance, s.allocator);
}


VkInstance CreateInstance(std::vector<const char*> extensions)
{
    VBUILDER_INFO("Creating instance");
    VkInstance instance = VK_NULL_HANDLE;

    VkResult res;

    // create Vulkan Instance
    VkInstanceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

#ifdef PSX_DEBUG
    // enable debug validation layers
    VBUILDER_INFO("Setting up debug environment");
    std::vector<const char*> validation_layers = {
        "VK_LAYER_KHRONOS_validation"
    };
    if (!checkValidationLayerSupport(validation_layers)) {
        VBUILDER_FATAL("Requested validation layers not supported!");
    }
    create_info.enabledLayerCount = static_cast<u32>(validation_layers.size());
    create_info.ppEnabledLayerNames = validation_layers.data();

    // add debug instance extension
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#else
    create_info.enabledLayerCount = 0;
#endif
    extensions.push_back("VK_KHR_get_physical_device_properties2"); // needed for VK_KHR_portability_subset
    create_info.enabledExtensionCount = static_cast<u32>(extensions.size());
    create_info.ppEnabledExtensionNames = extensions.data();
    res = vkCreateInstance(&create_info, s.allocator, &instance);
    if (res != VK_SUCCESS) {
        VBUILDER_FATAL("Failed to create instance");
    }

    return instance;
}

/*
 * Initialize ImGui for vulkan.
 * https://github.com/blurrypiano/littleVulkanEngine/blob/c25099a7da6770072fdfc9ec3bd4d38aa9380906/littleVulkanEngine/imguiDemo/lve_imgui.cpp
 */
void InitializeImGuiVulkan(Builder::WindowData *wd, Builder::DeviceData *dd, VkInstance instance, SDL_Window *window)
{
    VBUILDER_INFO("Intializing ImGui for vulkan");
    // set up a descriptor pool stored on this instance
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
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
    pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;
    if (vkCreateDescriptorPool(dd->logidata.dev, &pool_info, s.allocator, &s.imgui_descriptor_pool) != VK_SUCCESS) {
        VBUILDER_FATAL("Failed to set up descriptor pool for imgui");
    }

    // Setup Platform/Renderer backends
    // Initialize imgui for vulkan
    ImGui_ImplSDL2_InitForVulkan(window);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = instance;
    init_info.PhysicalDevice = dd->physdata.dev;
    init_info.Device = dd->logidata.dev;
    init_info.QueueFamily = dd->physdata.graphics_queue_family;
    init_info.Queue = dd->physdata.graphics_queue;

    // pipeline cache is a potential future optimization, ignoring for now
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = s.imgui_descriptor_pool;
    init_info.Allocator = s.allocator;
    init_info.MinImageCount = wd->min_image_count;
    init_info.ImageCount = wd->image_count;
    init_info.CheckVkResultFn = nullptr;// TODO: do I need this?
    ImGui_ImplVulkan_Init(&init_info, wd->render_pass);
}

/*
 * Setup the debug messages from Vulkan.
 */
void SetupDebugMessenger(VkInstance instance)
{
    VBUILDER_INFO("Setting up debug callback");
    VkDebugUtilsMessengerCreateInfoEXT create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    create_info.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    create_info.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT    |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    create_info.pfnUserCallback = debugCallback;
    create_info.pUserData = nullptr;

    // load the debug creation function
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)
        vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        VkResult res = func(instance, &create_info, nullptr, &s.debug_messenger);
        if (res != VK_SUCCESS) {
            VBUILDER_FATAL("Failed to create debug utils messenger!");
        }
    } else {
        VBUILDER_FATAL("vkCreateDebugUtilsMessengerEXT extension not present!");
    }
}

/*
 * Modifies WindowData:
 *  1. Add VkSurfaceKHR
 */
void BuildSurfaceData(WindowData *wd, VkSurfaceKHR surface)
{
    VBUILDER_INFO("Building surface for WindowData obj@{}", static_cast<void*>(wd));
    PSX_ASSERT(surface != VK_NULL_HANDLE);
    PSX_ASSERT(wd != nullptr);

    wd->surface = surface;
}

/*
 * Modifies DeviceData
 *  1. Build Physical Device data
 *  2. Build Logical Device data
 */
void BuildDeviceData(DeviceData *dd, VkInstance instance, VkSurfaceKHR surface)
{
    VBUILDER_INFO("Building device data for DeviceData obj@{}", static_cast<void*>(dd));
    PSX_ASSERT(dd != nullptr);

    BuildPhysicalDeviceData(dd, instance, surface);
    BuildLogicalDeviceData(dd, surface);
}

/*
 * Modifies DeviceData:
 *  1. Set VkPhysicalDevice
 *  2. Set logical device extensions
 */
void BuildPhysicalDeviceData(DeviceData *dd, VkInstance instance, VkSurfaceKHR surface)
{
    VBUILDER_INFO("Building physical device data for DeviceData obj@{}", static_cast<void*>(dd));
    PSX_ASSERT(dd != nullptr);

    VkResult res;
    VBUILDER_INFO("Picking physical device");
    dd->logidata.extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    // get number of gpus
    u32 dev_count = 0;
    res = vkEnumeratePhysicalDevices(instance, &dev_count, nullptr);
    PSX_ASSERT(res == VK_SUCCESS);
    if (dev_count == 0) {
        VBUILDER_FATAL("No Vulkan supported physical devices found!");
    }

    // get list of gpus
    std::vector<VkPhysicalDevice> devs(dev_count);
    res = vkEnumeratePhysicalDevices(instance, &dev_count, devs.data());
    PSX_ASSERT(res == VK_SUCCESS);

    // pick highest rated suitable device
    std::multimap<int, VkPhysicalDevice> dev_candidates;
    for (const auto& dev : devs) {
        dev_candidates.insert(std::make_pair(ratePhysicalDevice(surface, dev, dd->logidata.extensions), dev));
    }
    if (dev_candidates.rbegin()->first == 0) {
        VBUILDER_FATAL("Physical devices found, but none are usable");
    }
    dd->physdata.dev = dev_candidates.rbegin()->second;
    PSX_ASSERT(dd->physdata.dev != VK_NULL_HANDLE);
    vkGetPhysicalDeviceProperties(dd->physdata.dev, &dd->physdata.props);
    VBUILDER_INFO("Physical device: {}, Score: {}", dd->physdata.props.deviceName, dev_candidates.rbegin()->first);

    // When running on portibility layer (like MoltenVK), we need to enable
    // VK_KHR_portability_subset extension
    u32 property_count;
    res = vkEnumerateDeviceExtensionProperties(dd->physdata.dev, nullptr, &property_count, nullptr);
    PSX_ASSERT(res == VK_SUCCESS);
    std::vector<VkExtensionProperties> ext_props(property_count);
    res = vkEnumerateDeviceExtensionProperties(dd->physdata.dev, nullptr, &property_count, ext_props.data());
    PSX_ASSERT(res == VK_SUCCESS);
    for (const auto& prop : ext_props) {
        if (strcmp(prop.extensionName, "VK_KHR_portability_subset") == 0) {
            VBUILDER_INFO("Enabling VK_KHR_portability_subset");
            dd->logidata.extensions.push_back("VK_KHR_portability_subset");
            break;
        }
    }
}

/*
 * Modifies DeviceData:
 *  1. Add VkDevice
 *  2. Set Queues
 */
void BuildLogicalDeviceData(DeviceData *dd, VkSurfaceKHR surface)
{
    VBUILDER_INFO("Building logical device data for DeviceData obj@{}", static_cast<void*>(dd));
    PSX_ASSERT(dd != nullptr);

    QueueFamilyIndices indices = findQueueFamilies(dd->physdata.dev, surface);
    // create queue info
    std::set<u32> unique_queue_families = indices.GetUniqueQueueFamilies();
    std::vector<VkDeviceQueueCreateInfo> queue_infos;
    float queue_priority = 1.0f;
    for (const auto& queue_family : unique_queue_families) {
        VkDeviceQueueCreateInfo queue_info{};
        queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info.queueFamilyIndex = queue_family;
        queue_info.queueCount = 1;
        queue_info.pQueuePriorities = &queue_priority;
        queue_infos.push_back(queue_info);
    }

    // specify device features (none for now)
    VkPhysicalDeviceFeatures dev_features{};

    // create device info
    VkDeviceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.queueCreateInfoCount = static_cast<u32>(queue_infos.size());
    create_info.pQueueCreateInfos = queue_infos.data();
    create_info.pEnabledFeatures = &dev_features;
    create_info.enabledExtensionCount = static_cast<u32>(dd->logidata.extensions.size());
    create_info.ppEnabledExtensionNames = dd->logidata.extensions.data();
    VkResult res = vkCreateDevice(dd->physdata.dev, &create_info, s.allocator, &dd->logidata.dev);
    if (res != VK_SUCCESS) {
        VBUILDER_FATAL("Failed to create logical device [rc: {}]", res);
    }

    dd->physdata.graphics_queue_family = indices.graphics_family.value();
    dd->physdata.present_queue_family = indices.graphics_family.value();
    // get our queues
    vkGetDeviceQueue(dd->logidata.dev, indices.graphics_family.value(), 0, &dd->physdata.graphics_queue);
    vkGetDeviceQueue(dd->logidata.dev, indices.present_family.value(), 0, &dd->physdata.present_queue);
}

/*
 * Modifies WindowData:
 *  1. Set VkSwapchain
 *  2. Set Min Image Count
 *  3. Set Surface Format
 *  4. Set Images
 *  5. Set Extent
 *  6. Set Image Count
 *  7. Set Min Image Count
 */
void BuildSwapchainData(WindowData *wd, DeviceData *dd, int width, int height)
{
    VBUILDER_INFO("Building swapchain for WindowData obj@{}", static_cast<void*>(wd));
    PSX_ASSERT(wd != nullptr);
    PSX_ASSERT(dd != nullptr);

    SwapChainSupportDetails details = querySwapChainSupport(dd->physdata.dev, wd->surface);

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
    wd->min_image_count = image_count;

    // config the swap chain
    VkSwapchainCreateInfoKHR create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = wd->surface;
    create_info.minImageCount = image_count;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageExtent = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    // specify how swap chain images will be used across mulitple queue families
    QueueFamilyIndices indices = findQueueFamilies(dd->physdata.dev, wd->surface);
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
    VkResult res = vkCreateSwapchainKHR(dd->logidata.dev, &create_info, s.allocator, &wd->swapchain);
    if (res != VK_SUCCESS) {
        VBUILDER_FATAL("Failed to create swap chain [rc: {}]", res);
    }
    
    // fill in rest of details
    image_count = 0; // reset
    vkGetSwapchainImagesKHR(dd->logidata.dev, wd->swapchain, &image_count, nullptr);
    VkImage images[image_count];
    wd->frames.resize(image_count);
    vkGetSwapchainImagesKHR(dd->logidata.dev, wd->swapchain, &image_count, images);
    for (u32 i = 0; i < image_count; i++) {
        wd->frames.at(i).backbuffer = images[i];
    }
    wd->surface_format = surface_format;
    wd->extent = extent;
    wd->image_count = image_count;
}

/*
 * Modifies WindowData:
 *  1. Set Image Views
 */
void BuildImageViews(WindowData *wd, DeviceData *dd)
{
    VBUILDER_INFO("Building image views for WindowData obj@{}", static_cast<void*>(wd));
    PSX_ASSERT(wd != nullptr);
    PSX_ASSERT(dd != nullptr);

    for (size_t i = 0; i < wd->image_count; i++) {
        VkImageViewCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = wd->frames.at(i).backbuffer;
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = wd->surface_format.format;
        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;
        VkResult res = vkCreateImageView(dd->logidata.dev, &create_info, s.allocator, &wd->frames.at(i).backbuffer_view);
        if (res != VK_SUCCESS) {
            VBUILDER_FATAL("Failed to create image view. [rc: {}]", res);
        }
    }
}

/*
 * Modifies WindowData:
 *  1. Set render pass
 */
void BuildRenderPassData(WindowData *wd, DeviceData *dd)
{
    VBUILDER_INFO("Building render pass for WindowData obj@{}", static_cast<void*>(wd));
    PSX_ASSERT(wd != nullptr);

    VkAttachmentDescription color_attachment{};
    color_attachment.format = wd->surface_format.format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_ref{};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;

    // create render pass
    VkRenderPassCreateInfo rp_info{};
    rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rp_info.attachmentCount = 1;
    rp_info.pAttachments = &color_attachment;
    rp_info.subpassCount = 1;
    rp_info.pSubpasses = &subpass;
    VkResult res = vkCreateRenderPass(dd->logidata.dev, &rp_info, s.allocator, &wd->render_pass);
    if (res != VK_SUCCESS) {
        VBUILDER_FATAL("Failed to create Render Pass");
    }
}

void BuildPipelineData(
    WindowData *wd, 
    DeviceData *dd,
    const std::string& vs_path,
    const std::string& fs_path)
{
    VBUILDER_INFO("Building pipeline for WindowData obj@{}", static_cast<void*>(wd));
    PSX_ASSERT(wd != nullptr);
    PSX_ASSERT(dd != nullptr);

    auto vert_shader_bytes = readSPVFile(vs_path);
    auto frag_shader_bytes = readSPVFile(fs_path);
    VBUILDER_INFO("Vertex Shader Size: {}", vert_shader_bytes.size());
    VBUILDER_INFO("Fragment Shader Size: {}", frag_shader_bytes.size());

    VkShaderModule vert_shader = 
        createShaderModule(vert_shader_bytes, dd->logidata.dev);
    VkShaderModule frag_shader = 
        createShaderModule(frag_shader_bytes, dd->logidata.dev);

    // create shader stage infos
    // vertex shader module
    VkPipelineShaderStageCreateInfo vs_stage_info{};
    vs_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vs_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vs_stage_info.module = vert_shader;
    vs_stage_info.pName = "main";
    // fragment shader module
    VkPipelineShaderStageCreateInfo fs_stage_info{};
    fs_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fs_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fs_stage_info.module = frag_shader;
    fs_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stage_infos[] = {vs_stage_info, fs_stage_info};

    // Vertex Input
    VkPipelineVertexInputStateCreateInfo vi_state_info{};
    vi_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vi_state_info.vertexBindingDescriptionCount = 0;
    vi_state_info.pVertexBindingDescriptions = nullptr;
    vi_state_info.vertexAttributeDescriptionCount = 0;
    vi_state_info.pVertexAttributeDescriptions = nullptr;

    // Input Assembly
    VkPipelineInputAssemblyStateCreateInfo ia_state_info{};
    ia_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia_state_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    ia_state_info.primitiveRestartEnable = VK_FALSE;

    // Viewport
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) wd->extent.width;
    viewport.height = (float) wd->extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    // Scissor
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = wd->extent;
    // Viewport State
    VkPipelineViewportStateCreateInfo vp_state_info{};
    vp_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp_state_info.viewportCount = 1;
    vp_state_info.pViewports = &viewport;
    vp_state_info.scissorCount = 1;
    vp_state_info.pScissors = &scissor;

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f; // Optional
    rasterizer.depthBiasClamp = 0.0f; // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE; // disable for now
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f; // Optional
    multisampling.pSampleMask = nullptr; // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE; // Optional

    // Depth and Stencil testing
    // None for now

    // Color Blending
    // attachment
    VkPipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_FALSE; // turned off
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional
    // info
    VkPipelineColorBlendStateCreateInfo color_blend_info{};
    color_blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_info.logicOpEnable = VK_FALSE;
    color_blend_info.logicOp = VK_LOGIC_OP_COPY; // Optional
    color_blend_info.attachmentCount = 1;
    color_blend_info.pAttachments = &color_blend_attachment;
    color_blend_info.blendConstants[0] = 0.0f; // Optional
    color_blend_info.blendConstants[1] = 0.0f; // Optional
    color_blend_info.blendConstants[2] = 0.0f; // Optional
    color_blend_info.blendConstants[3] = 0.0f; // Optional
    
    // Not using Dynamic State right now
    // VkPipelineDynamicStateCreateInfo dynamicState{};
    // ...

    // Pipeline Layout
    // Turing it off for now
    VkPipelineLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.setLayoutCount = 0; // Optional
    layout_info.pSetLayouts = nullptr; // Optional
    layout_info.pushConstantRangeCount = 0; // Optional
    layout_info.pPushConstantRanges = nullptr; // Optional
    VkResult res = vkCreatePipelineLayout(dd->logidata.dev, &layout_info, s.allocator, &wd->pipeline_layout);
    if (res != VK_SUCCESS) {
        VBUILDER_FATAL("Failed to create Pipeline Layout");
    }

    // Finally, Create the Graphics Pipeline
    // first, setup info
    VkGraphicsPipelineCreateInfo gp_info{};
    gp_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    gp_info.stageCount = 2;
    gp_info.pStages = shader_stage_infos;
    gp_info.pVertexInputState = &vi_state_info;
    gp_info.pInputAssemblyState = &ia_state_info;
    gp_info.pViewportState = &vp_state_info;
    gp_info.pRasterizationState = &rasterizer;
    gp_info.pMultisampleState = &multisampling;
    gp_info.pDepthStencilState = nullptr; // Optional
    gp_info.pColorBlendState = &color_blend_info;
    gp_info.pDynamicState = nullptr; // Optional
    gp_info.layout = wd->pipeline_layout;
    gp_info.renderPass = wd->render_pass;
    gp_info.subpass = 0;
    gp_info.basePipelineHandle = VK_NULL_HANDLE; // Optional
    gp_info.basePipelineIndex = -1; // Optional
    // create the object
    res = vkCreateGraphicsPipelines(dd->logidata.dev, VK_NULL_HANDLE, 1, &gp_info, s.allocator, &wd->pipeline);
    if (res != VK_SUCCESS) {
        VBUILDER_FATAL("Failed to create Graphics Pipeline");
    }

    vkDestroyShaderModule(dd->logidata.dev, vert_shader, s.allocator);
    vkDestroyShaderModule(dd->logidata.dev, frag_shader, s.allocator);
}

void BuildFrameBuffersData(WindowData *wd, DeviceData *dd)
{
    VBUILDER_INFO("Building frame buffers for WindowData obj@{}", static_cast<void*>(wd));
    PSX_ASSERT(wd != nullptr);
    PSX_ASSERT(dd != nullptr);
    PSX_ASSERT(wd->frames.size() != 0);
    PSX_ASSERT(wd->frames.size() == wd->image_count);

    for (u32 i = 0; i < wd->image_count; i++) {
        VkImageView attachments[] = {
            wd->frames[i].backbuffer_view
        };

        VkFramebufferCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        info.renderPass = wd->render_pass;
        info.attachmentCount = 1;
        info.pAttachments = attachments;
        info.width = wd->extent.width;
        info.height = wd->extent.height;
        info.layers = 1;

        VkResult res = vkCreateFramebuffer(dd->logidata.dev, &info, s.allocator, &wd->frames[i].framebuffer);
        if (res != VK_SUCCESS) {
            VBUILDER_FATAL("Failed to create framebuffer! [rc: {}]", res);
        }
    }
}

void BuildCommandBuffersData(WindowData *wd, DeviceData *dd)
{
    VBUILDER_INFO("Building command buffers for WindowData obj@{}", static_cast<void*>(wd));
    PSX_ASSERT(wd != nullptr);
    PSX_ASSERT(dd != nullptr);
    PSX_ASSERT(wd->frames.size() != 0);
    PSX_ASSERT(wd->frames.size() == wd->image_count);

    // make room for semaphores
    wd->frame_semaphores.resize(wd->image_count);

    for (u32 i = 0; i < wd->image_count; i++) {
        FrameData *fd = &wd->frames[i];
        FrameSemaphores *fs = &wd->frame_semaphores[i];

        // command pool
        VkCommandPoolCreateInfo cp_info{};
        cp_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cp_info.queueFamilyIndex = dd->physdata.graphics_queue_family;
        cp_info.flags = 0; // TODO eventually want to set this flag
        VkResult res = vkCreateCommandPool(dd->logidata.dev, &cp_info, s.allocator, &fd->command_pool);
        if (res != VK_SUCCESS) {
            VBUILDER_FATAL("Failed to create command pool. [rc: {}]", res);
        }

        // Command Buffer
        VkCommandBufferAllocateInfo cb_info{};
        cb_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cb_info.commandPool = fd->command_pool;
        cb_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cb_info.commandBufferCount = 1;
        res = vkAllocateCommandBuffers(dd->logidata.dev, &cb_info, &fd->command_buffer);
        if (res != VK_SUCCESS) {
            VBUILDER_FATAL("Failed to create command buffer. [rc: {}]", res);
        }
        
        // Fence
        VkFenceCreateInfo fence_info{};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        res = vkCreateFence(dd->logidata.dev, &fence_info, s.allocator, &fd->fence);
        if (res != VK_SUCCESS) {
            VBUILDER_FATAL("Failed to create fence. [rc: {}]", res);
        }

        // Semaphores
        VkSemaphoreCreateInfo sem_info{};
        sem_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        res = vkCreateSemaphore(dd->logidata.dev, &sem_info, s.allocator, &fs->image_acquire);
        if (res != VK_SUCCESS) {
            VBUILDER_FATAL("Failed to create image acquire semaphore. [rc: {}]", res);
        }
        res = vkCreateSemaphore(dd->logidata.dev, &sem_info, s.allocator, &fs->render_complete);
        if (res != VK_SUCCESS) {
            VBUILDER_FATAL("Failed to create image acquire semaphore. [rc: {}]", res);
        }
    }
}

}// end ns
}
}

namespace {
/*
 * Check that all of the validation layers given are available.
 */
bool checkValidationLayerSupport(const std::vector<const char*>& layers_wanted)
{
    VkResult res;

    // get number of layer properties
    u32 layer_count;
    res = vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
    PSX_ASSERT(res == VK_SUCCESS);

    std::vector<VkLayerProperties> available_layers(layer_count);
    res = vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());
    PSX_ASSERT(res == VK_SUCCESS);

    // check that the layers we want are available
    for (const auto& layer_wanted : layers_wanted) {
        bool layer_found = false;
        for (const auto& layer_prop : available_layers) {
            if (strcmp(layer_prop.layerName, layer_wanted) == 0) {
                layer_found = true;
                break;
            }
        }

        if (!layer_found) {
            VBUILDER_ERROR("Validation layer [{}] not supported!", layer_wanted);
            return false;
        }
    }

    return true;
}

/*
 * Rate the given device. A higher score is better. A score of 0 is unusable.
 */
int ratePhysicalDevice(VkSurfaceKHR surface, VkPhysicalDevice dev, const std::vector<const char*>& device_extensions)
{
    int score = 1; // min usable score
    VkPhysicalDeviceProperties dev_properties;
    VkPhysicalDeviceFeatures dev_features;
    vkGetPhysicalDeviceProperties(dev, &dev_properties);
    vkGetPhysicalDeviceFeatures(dev, &dev_features);

    // prefer discrete graphics over integrated
    if (dev_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score += 1000;
    }

    // don't care about anything else right now

    // make sure device supports the "must-haves"
    if (!isDeviceSuitable(surface, dev, device_extensions)) {
        return 0;
    }

    return score;
}

/*
 * Verify if device supports everything we need.
 */
bool isDeviceSuitable(VkSurfaceKHR surface, VkPhysicalDevice dev, const std::vector<const char*>& device_extensions)
{
    QueueFamilyIndices indices = findQueueFamilies(dev, surface);

    bool extensions_supported = checkDeviceExtensionsSupported(dev, device_extensions);

    // check swap chain supports what we need
    bool swap_chain_good = false;
    if (extensions_supported) {
        SwapChainSupportDetails details = querySwapChainSupport(dev, surface);
        swap_chain_good = !details.formats.empty()
                       && !details.preset_modes.empty();
    }

    return indices.IsComplete() && extensions_supported && swap_chain_good;
}

/*
 * Check if device supports all extensions we need.
 */
bool checkDeviceExtensionsSupported(VkPhysicalDevice dev, const std::vector<const char*>& device_extensions)
{
    // get number of extensions
    u32 ext_count;
    vkEnumerateDeviceExtensionProperties(dev, nullptr, &ext_count, nullptr);

    // get extension properties
    std::vector<VkExtensionProperties> ext_props(ext_count);
    vkEnumerateDeviceExtensionProperties(dev, nullptr, &ext_count, ext_props.data());

    // create set of all extensions which are required
    std::set<std::string> required_extensions(device_extensions.begin(), device_extensions.end());
    for (const auto& ext_prop : ext_props) {
        required_extensions.erase(ext_prop.extensionName);
    }

    // if empty, all extensions were found
    return required_extensions.empty();
}

/*
 * Get the swap chain support details for a given device.
 */
SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice dev, VkSurfaceKHR surface)
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
QueueFamilyIndices findQueueFamilies(VkPhysicalDevice dev, VkSurfaceKHR surface)
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

/*
 * Return the best swap surface format from the list of available formats.
 */
VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats)
{
    VBUILDER_INFO("Choosing Swap Surface Format");
    // look for a format with 24-bit + transparency color with sRGB color space
    for (const auto& format : available_formats) {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB 
            && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            VBUILDER_INFO("Ideal Swap Surface found");
            return format;
        }
    }
    // if none are perfect, just return the first one.
    // Could also rank the formats and return the best one.
    VBUILDER_WARN("Ideal Swap Surface not found, defaulting to first available surface");
    return available_formats[0];
}

/*
 * Return the best swap present mode from the given list of available present modes.
 */
VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes)
{
    VBUILDER_INFO("Choosing Swap Present Mode");
    // we want Mailbox mode which will allows for the smallest latency
    // (similar to unlocked fps with little screen tearing)
    for (const auto& present_mode : available_present_modes) {
        if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            VBUILDER_INFO("Using Mailbox Present Mode");
            return present_mode;
        }
    }

    // settle for FIFO mode (similar to V-Sync)
    VBUILDER_INFO("Using FIFO Present Mode");
    return VK_PRESENT_MODE_FIFO_KHR;
}

/*
 * Return the swap extent limited by our capabilities and window.
 */
VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, int width, int height)
{
    VBUILDER_INFO("Choosing Swap Extent");
    if (capabilities.currentExtent.width != UINT32_MAX) {
        VBUILDER_WARN("Surface capabilities is UINT32_MAX, ignoring specified width/height");
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
 * Open and read a given file and return the data in a byte vector.
 */
std::vector<char> readSPVFile(const std::string& filepath)
{
    // open file in binary mode, starting at the end (to get file size)
    std::ifstream file(filepath, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        VBUILDER_FATAL("Failed to open file: {}", filepath);
    }

    // get file size, then seek to beginning
    size_t filesize = (size_t) file.tellg();
    std::vector<char> bytes(filesize);
    file.seekg(0);

    // read the data
    file.read(bytes.data(), (std::streamsize)filesize);
    
    file.close();
    PSX_ASSERT(!bytes.empty()); // probably expecting the shader to be non-empty
    PSX_ASSERT(bytes[0] != 0x00); // first 4 bytes are a magic number, make sure we didn't read all 0's
    return bytes;
}

/*
 * Create and return a shader module from the given byte code and device.
 */
VkShaderModule createShaderModule(std::vector<char>& bytecode, VkDevice device)
{
    VkShaderModuleCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = bytecode.size();
    info.pCode = reinterpret_cast<u32*>(bytecode.data());

    VkShaderModule smod;
    VkResult res = vkCreateShaderModule(device, &info, nullptr, &smod);
    if (res != VK_SUCCESS) {
        VBUILDER_FATAL("Failed to create shader module!");
    }

    return smod;
}

}// end private ns