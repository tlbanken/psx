/*
 * vulkan_instance.cc
 *
 * Travis Banken
 * 6/23/21
 * 
 * Setup utils for the Vulkan backend.
 */

#include "vulkan_instance.hh"

#include "glfw/glfw3.h"
#include <map>
#include <set>
#include <optional>

#include "util/psxutil.hh"
#include "layer/imgui_layer.hh"
#include "render/utils.hh"

#define VULKAN_INFO(...) PSXLOG_INFO("Vulkan Backend", __VA_ARGS__)
#define VULKAN_WARN(...) PSXLOG_WARN("Vulkan Backend", __VA_ARGS__)
#define VULKAN_ERROR(...) PSXLOG_ERROR("Vulkan Backend", __VA_ARGS__)
#define VULKAN_FATAL(...) VULKAN_ERROR(__VA_ARGS__); throw std::runtime_error(PSX_FMT(__VA_ARGS__))

namespace {

using namespace Psx::Vulkan::Instance;

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

// struct State {
// } s;

// protos
bool checkValidationLayerSupport(const std::vector<const char*>& layers_wanted);
VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT msg_severity,
    VkDebugUtilsMessageTypeFlagsEXT msg_type,
    const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
    void *user_data);
VkDebugUtilsMessengerEXT setupDebugMessenger(VkInstance instance);
VkInstance createInstance(std::vector<const char*> extensions);
VkPhysicalDevice pickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface, std::vector<const char*>& device_extensions);
int ratePhysicalDevice(VkPhysicalDevice dev, VkSurfaceKHR surface, const std::vector<const char*>& device_extensions);
QueueFamilyIndices findQueueFamilies(VkPhysicalDevice dev, VkSurfaceKHR surface);
VkDevice createLogicalDevice(VkPhysicalDevice physical_device, VkSurfaceKHR surface, const std::vector<const char*>& device_extensions);
bool isDeviceSuitable(VkPhysicalDevice dev, VkSurfaceKHR surface, const std::vector<const char*>& device_extensions);
bool checkDeviceExtensionsSupported(VkPhysicalDevice dev, const std::vector<const char*>& device_extensions);
VkSurfaceKHR createSurface(VkInstance instance);
SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice dev, VkSurfaceKHR surface);
VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats);
VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes);
VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
Details::SwapChainDetails createSwapChain(VkPhysicalDevice physical_device, VkSurfaceKHR surface, VkDevice device);
void createImageViews(Details& ideets);
void createGraphicsPipeline(const std::string& vs_path, const std::string& fs_path, Details& ideets);
void createRenderPass(Details& ideets);

}// end private ns

namespace Psx {
namespace Vulkan {
namespace Instance {

Details Create(std::vector<const char*> extensions)
{
    Details d;
    d.device_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    d.instance = createInstance(extensions);
#ifdef PSX_DEBUG
    // setup debug callback function
    d.debug_messenger = setupDebugMessenger(d.instance);
#endif
    d.surface = createSurface(d.instance);
    d.physical_device = pickPhysicalDevice(d.instance, d.surface, d.device_extensions);
    d.device = createLogicalDevice(d.physical_device, d.surface, d.device_extensions);
    d.sc_deets = createSwapChain(d.physical_device, d.surface, d.device);
    createImageViews(d);
    createRenderPass(d);
    createGraphicsPipeline(
        std::string(PROJECT_ROOT_PATH) + "/src/render/shaders/shader.vert.spv", 
        std::string(PROJECT_ROOT_PATH) + "/src/render/shaders/shader.frag.spv", 
        d
    );
    return d;
}

void Destroy(Details d)
{
#ifdef PSX_DEBUG
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(d.instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(d.instance, d.debug_messenger, nullptr);
    }
#endif
    vkDestroyPipeline(d.device, d.graphics_pipeline, nullptr);
    vkDestroyPipelineLayout(d.device, d.pipeline_layout, nullptr);
    vkDestroyRenderPass(d.device, d.render_pass, nullptr);
    for (auto image_view : d.sc_deets.image_views) {
        vkDestroyImageView(d.device, image_view, nullptr);
    }
    vkDestroySwapchainKHR(d.device, d.sc_deets.swap_chain, nullptr);
    vkDestroyDevice(d.device, nullptr);
    vkDestroySurfaceKHR(d.instance, d.surface, nullptr);
    vkDestroyInstance(d.instance, nullptr);
}

}// end ns
}
}

// PRIVATE METHODS
namespace {

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
        VULKAN_WARN("**Callback**: {}", callback_data->pMessage);
    } else if (msg_severity > VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        VULKAN_FATAL("**Callback**: {}", callback_data->pMessage);
    }

    return VK_FALSE;
}

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
            VULKAN_ERROR("Validation layer [{}] not supported!", layer_wanted);
            return false;
        }
    }

    return true;
}

/*
 * Setup the debug messages from Vulkan.
 */
VkDebugUtilsMessengerEXT setupDebugMessenger(VkInstance instance)
{
    VULKAN_INFO("Setting up debug callback");
    VkDebugUtilsMessengerEXT debug_messenger;
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
        VkResult res = func(instance, &create_info, nullptr, &debug_messenger);
        if (res != VK_SUCCESS) {
            VULKAN_FATAL("Failed to create debug utils messenger!");
        }
    } else {
        VULKAN_FATAL("vkCreateDebugUtilsMessengerEXT extension not present!");
    }
    return debug_messenger;
}

VkInstance createInstance(std::vector<const char*> extensions)
{
    VkResult res;
    VkInstance instance;

    // create Vulkan Instance
    VULKAN_INFO("Creating Vulkan instance");
    VkInstanceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

#ifdef PSX_DEBUG
    // enable debug validation layers
    VULKAN_INFO("Setting up debug environment");
    std::vector<const char*> validation_layers = {
        "VK_LAYER_KHRONOS_validation"
    };
    if (!checkValidationLayerSupport(validation_layers)) {
        VULKAN_FATAL("Requested validation layers not supported!");
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
    res = vkCreateInstance(&create_info, nullptr, &instance);
    if (res != VK_SUCCESS) {
        VULKAN_FATAL("Failed to create instance");
    }
    return instance;
}

/*
 * Pick a supported physical device. 
 *
 * Note: modifies s.physical_device
 */
VkPhysicalDevice pickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface, std::vector<const char*>& device_extensions)
{
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    VkResult res;
    VULKAN_INFO("Picking physical device");

    // get number of gpus
    u32 dev_count = 0;
    res = vkEnumeratePhysicalDevices(instance, &dev_count, nullptr);
    PSX_ASSERT(res == VK_SUCCESS);
    if (dev_count == 0) {
        VULKAN_FATAL("No Vulkan supported physical devices found!");
    }

    // get list of gpus
    std::vector<VkPhysicalDevice> devs(dev_count);
    res = vkEnumeratePhysicalDevices(instance, &dev_count, devs.data());
    PSX_ASSERT(res == VK_SUCCESS);

    // pick highest rated suitable device
    std::multimap<int, VkPhysicalDevice> dev_candidates;
    for (const auto& dev : devs) {
        dev_candidates.insert(std::make_pair(ratePhysicalDevice(dev, surface, device_extensions), dev));
    }
    if (dev_candidates.rbegin()->first == 0) {
        VULKAN_FATAL("Physical devices found, but none are usable");
    }
    VULKAN_INFO("Physical device chosen! Score: {}", dev_candidates.rbegin()->first);
    physical_device = dev_candidates.rbegin()->second;
    PSX_ASSERT(physical_device != VK_NULL_HANDLE);

    // When running on portibility layer (like MoltenVK), we need to enable
    // VK_KHR_portability_subset extension
    u32 property_count;
    res = vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &property_count, nullptr);
    PSX_ASSERT(res == VK_SUCCESS);
    std::vector<VkExtensionProperties> ext_props(property_count);
    res = vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &property_count, ext_props.data());
    PSX_ASSERT(res == VK_SUCCESS);
    for (const auto& prop : ext_props) {
        if (strcmp(prop.extensionName, "VK_KHR_portability_subset") == 0) {
            VULKAN_INFO("Enabling VK_KHR_portability_subset");
            device_extensions.push_back("VK_KHR_portability_subset");
            break;
        }
    }

    return physical_device;
}

/*
 * Rate the given device. A higher score is better. A score of 0 is unusable.
 */
int ratePhysicalDevice(VkPhysicalDevice dev, VkSurfaceKHR surface, const std::vector<const char*>& device_extensions)
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
    if (!isDeviceSuitable(dev, surface, device_extensions)) {
        return 0;
    }

    return score;
}

/*
 * Verify if device supports everything we need.
 */
bool isDeviceSuitable(VkPhysicalDevice dev, VkSurfaceKHR surface, const std::vector<const char*>& device_extensions)
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

VkDevice createLogicalDevice(VkPhysicalDevice physical_device, VkSurfaceKHR surface, const std::vector<const char*>& device_extensions)
{
    VkDevice device;
    VULKAN_INFO("Creating logical device");
    QueueFamilyIndices indices = findQueueFamilies(physical_device, surface);
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
    create_info.enabledExtensionCount = static_cast<u32>(device_extensions.size());
    create_info.ppEnabledExtensionNames = device_extensions.data();
    VkResult res = vkCreateDevice(physical_device, &create_info, nullptr, &device);
    if (res != VK_SUCCESS) {
        VULKAN_FATAL("Failed to create logical device [rc: {}]", res);
    }

    // Do this later??
    // vkGetDeviceQueue(device, indices.graphics_family.value(), 0, &s.graphics_queue);
    // vkGetDeviceQueue(device, indices.present_family.value(), 0, &s.present_queue);
    return device;
}

/*
 * Create a surface for the given instance.
 */
VkSurfaceKHR createSurface(VkInstance instance)
{
    VkSurfaceKHR surface;
    GLFWwindow *window = Psx::ImGuiLayer::GetWindow();
    VkResult res = glfwCreateWindowSurface(instance, window, nullptr, &surface);
    if (res != VK_SUCCESS) {
        VULKAN_FATAL("Failed to create surface!");
    }
    return surface;
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
 * Return the best swap surface format from the list of available formats.
 */
VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats)
{
    // look for a format with 24-bit + transparency color with sRGB color space
    for (const auto& format : available_formats) {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB 
            && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return format;
        }
    }
    // if none are perfect, just return the first one.
    // Could also rank the formats and return the best one.
    return available_formats[0];
}

/*
 * Return the best swap present mode from the given list of available present modes.
 */
VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes)
{
    // we want Mailbox mode which will allows for the smallest latency
    // (similar to unlocked fps with little screen tearing)
    for (const auto& present_mode : available_present_modes) {
        if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            VULKAN_INFO("Using Mailbox Present Mode");
            return present_mode;
        }
    }

    // settle for FIFO mode (similar to V-Sync)
    VULKAN_INFO("Using FIFO Present Mode");
    return VK_PRESENT_MODE_FIFO_KHR;
}

/*
 * Return the swap extent limited by our capabilities and window.
 */
VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        GLFWwindow *window = Psx::ImGuiLayer::GetWindow();
        glfwGetFramebufferSize(window, &width, &height);

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
 * Create and return the swap chain for the device and surface given.
 */
Details::SwapChainDetails createSwapChain(VkPhysicalDevice physical_device, VkSurfaceKHR surface, VkDevice device)
{
    SwapChainSupportDetails details = querySwapChainSupport(physical_device, surface);

    VkSurfaceFormatKHR surface_format = chooseSwapSurfaceFormat(details.formats);
    VkPresentModeKHR present_mode = chooseSwapPresentMode(details.preset_modes);
    VkExtent2D extent = chooseSwapExtent(details.capabilities);

    // we want the minimum number of images + 1 in the swap chain
    // if we are allowed to
    u32 image_count = details.capabilities.minImageCount + 1;
    if (details.capabilities.maxImageCount > 0 && image_count > details.capabilities.maxImageCount) {
        // clamp
        image_count = details.capabilities.maxImageCount;
    }

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
    QueueFamilyIndices indices = findQueueFamilies(physical_device, surface);
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
    Details::SwapChainDetails sc_deets;
    VkResult res = vkCreateSwapchainKHR(device, &create_info, nullptr, &sc_deets.swap_chain);
    if (res != VK_SUCCESS) {
        VULKAN_FATAL("Failed to create swap chain [rc: {}]", res);
    }
    
    // fill in rest of details
    image_count = 0; // reset
    vkGetSwapchainImagesKHR(device, sc_deets.swap_chain, &image_count, nullptr);
    sc_deets.images.resize(image_count);
    vkGetSwapchainImagesKHR(device, sc_deets.swap_chain, &image_count, sc_deets.images.data());
    sc_deets.format = surface_format.format;
    sc_deets.extent = extent;

    return sc_deets;
}

void createImageViews(Details& ideets)
{
    ideets.sc_deets.image_views.resize(ideets.sc_deets.images.size());
    for (size_t i = 0; i < ideets.sc_deets.images.size(); i++) {
        VkImageViewCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = ideets.sc_deets.images[i];
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = ideets.sc_deets.format;
        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;
        VkResult res = vkCreateImageView(ideets.device, &create_info, nullptr, &ideets.sc_deets.image_views[i]);
        if (res != VK_SUCCESS) {
            VULKAN_FATAL("Failed to create image view");
        }
    }
}

/*
 * Create a graphics pipeline.
 */
void createGraphicsPipeline(const std::string& vs_path, const std::string& fs_path, Details& ideets)
{
    auto vert_shader_bytes = Psx::Vulkan::Utils::ReadSPVFile(vs_path);
    auto frag_shader_bytes = Psx::Vulkan::Utils::ReadSPVFile(fs_path);

    VkShaderModule vert_shader = 
        Psx::Vulkan::Utils::CreateShaderModule(vert_shader_bytes, ideets.device);
    VkShaderModule frag_shader = 
        Psx::Vulkan::Utils::CreateShaderModule(frag_shader_bytes, ideets.device);

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
    viewport.width = (float) ideets.sc_deets.extent.width;
    viewport.height = (float) ideets.sc_deets.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    // Scissor
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = ideets.sc_deets.extent;
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
    VkResult res = vkCreatePipelineLayout(ideets.device, &layout_info, nullptr, &ideets.pipeline_layout);
    if (res != VK_SUCCESS) {
        VULKAN_FATAL("Failed to create Pipeline Layout");
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
    gp_info.layout = ideets.pipeline_layout;
    gp_info.renderPass = ideets.render_pass;
    gp_info.subpass = 0;
    gp_info.basePipelineHandle = VK_NULL_HANDLE; // Optional
    gp_info.basePipelineIndex = -1; // Optional
    // create the object
    res = vkCreateGraphicsPipelines(ideets.device, VK_NULL_HANDLE, 1, &gp_info, nullptr, &ideets.graphics_pipeline);
    if (res != VK_SUCCESS) {
        VULKAN_FATAL("Failed to create Graphics Pipeline");
    }

    vkDestroyShaderModule(ideets.device, vert_shader, nullptr);
    vkDestroyShaderModule(ideets.device, frag_shader, nullptr);
}

void createRenderPass(Details& ideets)
{
    VkAttachmentDescription color_attachment{};
    color_attachment.format = ideets.sc_deets.format;
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
    VkResult res = vkCreateRenderPass(ideets.device, &rp_info, nullptr, &ideets.render_pass);
    if (res != VK_SUCCESS) {
        VULKAN_FATAL("Failed to create Render Pass");
    }
}

}// end private ns
