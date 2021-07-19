/*
 * pipeline.cc
 *
 * Travis Banken
 * 7/18/2021
 * 
 * TODO
 */

#include "pipeline.hh"

#define VPIPELINE_INFO(...) PSXLOG_INFO("Vulkan Pipeline", __VA_ARGS__)
#define VPIPELINE_WARN(...) PSXLOG_WARN("Vulkan Pipeline", __VA_ARGS__)
#define VPIPELINE_ERROR(...) PSXLOG_ERROR("Vulkan Pipeline", __VA_ARGS__)
#define VPIPELINE_FATAL(...) VPIPELINE_ERROR(__VA_ARGS__); throw std::runtime_error(PSX_FMT(__VA_ARGS__))

namespace Psx {
namespace Vulkan {

// **********************
// *** PUBLIC METHODS ***
// **********************
Pipeline::Pipeline(VkDevice device, VkRenderPass render_pass, VkExtent2D extent,
    const std::string& vshader_path, const std::string& fshader_path)
{
    createGraphicsPipeline(
        device,
        render_pass,
        extent,
        vshader_path,
        fshader_path
    );
}

VkPipeline Pipeline::GetPipeline()
{
    return m_pipeline;
}

VkPipelineLayout Pipeline::GetLayout()
{
    return m_layout;
}

// ***********************
// *** PRIVATE METHODS ***
// ***********************
/*
 * Create a graphics pipeline.
 */
void Pipeline::createGraphicsPipeline(VkDevice device, VkRenderPass render_pass, VkExtent2D extent,
    const std::string& vs_path, const std::string& fs_path)
{
    VPIPELINE_INFO("Creating Pipeline");
    auto vert_shader_bytes = readSPVFile(vs_path);
    auto frag_shader_bytes = readSPVFile(fs_path);
    VPIPELINE_INFO("Vertex Shader Size: {}", vert_shader_bytes.size());
    VPIPELINE_INFO("Fragment Shader Size: {}", frag_shader_bytes.size());

    VkShaderModule vert_shader = 
        createShaderModule(vert_shader_bytes, device);
    VkShaderModule frag_shader = 
        createShaderModule(frag_shader_bytes, device);

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
    viewport.width = (float) extent.width;
    viewport.height = (float) extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    // Scissor
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = extent;
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
    VkResult res = vkCreatePipelineLayout(device, &layout_info, nullptr, &m_layout);
    if (res != VK_SUCCESS) {
        VPIPELINE_FATAL("Failed to create Pipeline Layout");
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
    gp_info.layout = m_layout;
    gp_info.renderPass = render_pass;
    gp_info.subpass = 0;
    gp_info.basePipelineHandle = VK_NULL_HANDLE; // Optional
    gp_info.basePipelineIndex = -1; // Optional
    // create the object
    res = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &gp_info, nullptr, &m_pipeline);
    if (res != VK_SUCCESS) {
        VPIPELINE_FATAL("Failed to create Graphics Pipeline");
    }

    vkDestroyShaderModule(device, vert_shader, nullptr);
    vkDestroyShaderModule(device, frag_shader, nullptr);
}

/*
 * Open and read a given file and return the data in a byte vector.
 */
std::vector<char> Pipeline::readSPVFile(const std::string& filepath)
{
    // open file in binary mode, starting at the end (to get file size)
    std::ifstream file(filepath, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        VPIPELINE_FATAL("Failed to open file: {}", filepath);
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
VkShaderModule Pipeline::createShaderModule(std::vector<char>& bytecode, VkDevice device)
{
    VkShaderModuleCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = bytecode.size();
    info.pCode = reinterpret_cast<u32*>(bytecode.data());

    VkShaderModule smod;
    VkResult res = vkCreateShaderModule(device, &info, nullptr, &smod);
    if (res != VK_SUCCESS) {
        VPIPELINE_FATAL("Failed to create shader module!");
    }

    return smod;
}

}// end ns
}

