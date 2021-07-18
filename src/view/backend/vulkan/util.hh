/*
 * util.hh
 *
 * Travis Banken
 * 7/17/2021
 * 
 * Common utilities for the vulkan backend.
 */
#pragma once

#include "view/backend/vulkan/includes.hh"
#include "util/psxutil.hh"

#include <vector>
#include <optional>
#include <set>

namespace Psx {
namespace Vulkan {
namespace Util {

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

SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice dev, VkSurfaceKHR surface);
QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice dev, VkSurfaceKHR surface);

}// end ns
}
}