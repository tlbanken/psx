/*
 * device.cc
 *
 * Travis Banken
 * 7/17/2021
 * 
 * Device wrapper object for the vulkan backend.
 */

#include "device.hh"

#include <map>

#include "view/backend/vulkan/util.hh"

#define VDEVICE_INFO(...) PSXLOG_INFO("Vulkan Device", __VA_ARGS__)
#define VDEVICE_WARN(...) PSXLOG_WARN("Vulkan Device", __VA_ARGS__)
#define VDEVICE_ERROR(...) PSXLOG_ERROR("Vulkan Device", __VA_ARGS__)
#define VDEVICE_FATAL(...) VDEVICE_ERROR(__VA_ARGS__); throw std::runtime_error(PSX_FMT(__VA_ARGS__))

namespace Psx {
namespace Vulkan {

// **********************
// *** PUBLIC METHODS ***
// **********************
Device::Device(VkInstance instance, VkSurfaceKHR surface)
{
    pickPhysicalDevice(instance, surface);
    createLogicalDevice(surface);
}

VkPhysicalDevice Device::GetPhysicalDevice()
{
    return m_physical_device;
}

VkDevice Device::GetLogicalDevice()
{
    return m_logical_device;
}

// ***********************
// *** PRIVATE METHODS ***
// ***********************
/*
 * Pick a supported physical device. 
 */
void Device::pickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface)
{
    VkResult res;
    VDEVICE_INFO("Picking physical device");
    m_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    // get number of gpus
    u32 dev_count = 0;
    res = vkEnumeratePhysicalDevices(instance, &dev_count, nullptr);
    PSX_ASSERT(res == VK_SUCCESS);
    if (dev_count == 0) {
        VDEVICE_FATAL("No Vulkan supported physical devices found!");
    }

    // get list of gpus
    std::vector<VkPhysicalDevice> devs(dev_count);
    res = vkEnumeratePhysicalDevices(instance, &dev_count, devs.data());
    PSX_ASSERT(res == VK_SUCCESS);

    // pick highest rated suitable device
    std::multimap<int, VkPhysicalDevice> dev_candidates;
    for (const auto& dev : devs) {
        dev_candidates.insert(std::make_pair(ratePhysicalDevice(surface, dev, m_extensions), dev));
    }
    if (dev_candidates.rbegin()->first == 0) {
        VDEVICE_FATAL("Physical devices found, but none are usable");
    }
    VDEVICE_INFO("Physical device chosen! Score: {}", dev_candidates.rbegin()->first);
    m_physical_device = dev_candidates.rbegin()->second;
    PSX_ASSERT(m_physical_device != VK_NULL_HANDLE);

    // When running on portibility layer (like MoltenVK), we need to enable
    // VK_KHR_portability_subset extension
    u32 property_count;
    res = vkEnumerateDeviceExtensionProperties(m_physical_device, nullptr, &property_count, nullptr);
    PSX_ASSERT(res == VK_SUCCESS);
    std::vector<VkExtensionProperties> ext_props(property_count);
    res = vkEnumerateDeviceExtensionProperties(m_physical_device, nullptr, &property_count, ext_props.data());
    PSX_ASSERT(res == VK_SUCCESS);
    for (const auto& prop : ext_props) {
        if (strcmp(prop.extensionName, "VK_KHR_portability_subset") == 0) {
            VDEVICE_INFO("Enabling VK_KHR_portability_subset");
            m_extensions.push_back("VK_KHR_portability_subset");
            break;
        }
    }
}

/*
 * Rate the given device. A higher score is better. A score of 0 is unusable.
 */
int Device::ratePhysicalDevice(VkSurfaceKHR surface, VkPhysicalDevice dev, const std::vector<const char*>& device_extensions)
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
bool Device::isDeviceSuitable(VkSurfaceKHR surface, VkPhysicalDevice dev, const std::vector<const char*>& device_extensions)
{
    Util::QueueFamilyIndices indices = Util::FindQueueFamilies(dev, surface);

    bool extensions_supported = checkDeviceExtensionsSupported(dev, device_extensions);

    // check swap chain supports what we need
    bool swap_chain_good = false;
    if (extensions_supported) {
        Util::SwapChainSupportDetails details = Util::QuerySwapChainSupport(dev, surface);
        swap_chain_good = !details.formats.empty()
                       && !details.preset_modes.empty();
    }

    return indices.IsComplete() && extensions_supported && swap_chain_good;
}

/*
 * Check if device supports all extensions we need.
 */
bool Device::checkDeviceExtensionsSupported(VkPhysicalDevice dev, const std::vector<const char*>& device_extensions)
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
 * Creates a logical device.
 */
void Device::createLogicalDevice(VkSurfaceKHR surface)
{
    VDEVICE_INFO("Creating logical device");
    Util::QueueFamilyIndices indices = Util::FindQueueFamilies(m_physical_device, surface);
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
    create_info.enabledExtensionCount = static_cast<u32>(m_extensions.size());
    create_info.ppEnabledExtensionNames = m_extensions.data();
    VkResult res = vkCreateDevice(m_physical_device, &create_info, nullptr, &m_logical_device);
    if (res != VK_SUCCESS) {
        VDEVICE_FATAL("Failed to create logical device [rc: {}]", res);
    }

    // Do this later??
    // vkGetDeviceQueue(device, indices.graphics_family.value(), 0, &s.graphics_queue);
    // vkGetDeviceQueue(device, indices.present_family.value(), 0, &s.present_queue);
}

}// end ns
}