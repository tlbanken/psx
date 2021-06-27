/*
 * utils.cc
 *
 * Travis Banken
 * 6/25/21
 * 
 * Utility functions for the vulkan backend.
 */

#include "utils.hh"

#define VULKAN_INFO(...) PSXLOG_INFO("Vulkan Backend", __VA_ARGS__)
#define VULKAN_WARN(...) PSXLOG_WARN("Vulkan Backend", __VA_ARGS__)
#define VULKAN_ERROR(...) PSXLOG_ERROR("Vulkan Backend", __VA_ARGS__)
#define VULKAN_FATAL(...) VULKAN_ERROR(__VA_ARGS__); throw std::runtime_error(PSX_FMT(__VA_ARGS__))

namespace Psx {
namespace Vulkan {
namespace Utils {

/*
 * Open and read a given file and return the data in a byte vector.
 */
std::vector<char> ReadSPVFile(const std::string& filepath)
{
    // open file in binary mode, starting at the end (to get file size)
    std::ifstream file(filepath, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        VULKAN_FATAL("Failed to open file: {}", filepath);
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
VkShaderModule CreateShaderModule(std::vector<char>& bytecode, VkDevice device)
{
    VkShaderModuleCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = bytecode.size();
    info.pCode = reinterpret_cast<u32*>(bytecode.data());

    VkShaderModule smod;
    VkResult res = vkCreateShaderModule(device, &info, nullptr, &smod);
    if (res != VK_SUCCESS) {
        VULKAN_FATAL("Failed to create shader module!");
    }

    return smod;
}


}// end ns
}
}