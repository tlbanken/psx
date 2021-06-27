/*
 * utils.hh
 *
 * Travis Banken
 * 6/25/21
 * 
 * Utility functions for the vulkan backend.
 */
#pragma once

#include <vulkan/vulkan.h>
#include <vector>

#include "util/psxutil.hh"

namespace Psx {
namespace Vulkan {
namespace Utils {

std::vector<char> ReadSPVFile(const std::string& filepath);
VkShaderModule CreateShaderModule(std::vector<char>& bytecode, VkDevice device);

}// end ns
}
}