/*
 * pipeline.hh
 *
 * Travis Banken
 * 7/18/2021
 * 
 * TODO
 */
#pragma once

#include "view/backend/vulkan/includes.hh"
#include "util/psxutil.hh"

#include <string>
#include <vector>

namespace Psx {
namespace Vulkan {

class Pipeline {
public:
    Pipeline() {} // does nothing
    Pipeline(VkDevice device, VkRenderPass render_pass, VkExtent2D extent,
        const std::string& vshader_path, const std::string& fshader_path);

    VkPipeline GetPipeline();
    VkPipelineLayout GetLayout();
private:
    VkPipeline m_pipeline;
    VkPipelineLayout m_layout;

    static std::vector<char> readSPVFile(const std::string& filepath);
    static VkShaderModule createShaderModule(std::vector<char>& bytecode, VkDevice device);

    void createGraphicsPipeline(VkDevice device, VkRenderPass render_pass, VkExtent2D extent,
        const std::string& vs_path, const std::string& fs_path);
};

}// end ns
}