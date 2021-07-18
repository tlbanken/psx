/*
 * context.cc
 *
 * Travis Banken
 * 7/12/2021
 * 
 * Vulkan context for the graphics backend.
 */

#include "context.hh"

#include <map>
#include <set>


#define VCTX_INFO(...) PSXLOG_INFO("Vulkan Context", __VA_ARGS__)
#define VCTX_WARN(...) PSXLOG_WARN("Vulkan Context", __VA_ARGS__)
#define VCTX_ERROR(...) PSXLOG_ERROR("Vulkan Context", __VA_ARGS__)
#define VCTX_FATAL(...) VCTX_ERROR(__VA_ARGS__); throw std::runtime_error(PSX_FMT(__VA_ARGS__))

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
        VCTX_WARN("**Callback**: {}", callback_data->pMessage);
    } else if (msg_severity > VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        VCTX_FATAL("**Callback**: {}", callback_data->pMessage);
    }

    return VK_FALSE;
}

}// end private ns


namespace Psx {
namespace Vulkan {

// **********************
// *** PUBLIC METHODS ***
// **********************
Context::Context(std::vector<const char*> extensions, GLFWwindow* window)
{
    createInstance(extensions);
#ifdef PSX_DEBUG
    // settup debug callback function
    setupDebugMessenger();
#endif
    createSurface(window);
    m_device = Device(m_instance, m_surface);
    m_swapchain = Swapchain(window, m_device, m_surface);
    createRenderPass();
    const std::string shader_folder("/src/view/backend/vulkan/shaders/");
    createGraphicsPipeline(
        std::string(PROJECT_ROOT_PATH) + shader_folder + "shader.vert.spv", 
        std::string(PROJECT_ROOT_PATH) + shader_folder + "shader.frag.spv" 
    );
}

Context::~Context()
{
#ifdef PSX_DEBUG
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(m_instance, m_debug_messenger, nullptr);
    }
#endif
    vkDestroyPipeline(m_device.GetLogicalDevice(), m_graphics_pipeline.ptr, nullptr);
    vkDestroyPipelineLayout(m_device.GetLogicalDevice(), m_graphics_pipeline.layout, nullptr);
    vkDestroyRenderPass(m_device.GetLogicalDevice(), m_render_pass, nullptr);
    for (auto image_view : m_swapchain.GetImageViews()) {
        vkDestroyImageView(m_device.GetLogicalDevice(), image_view, nullptr);
    }
    vkDestroySwapchainKHR(m_device.GetLogicalDevice(), m_swapchain.GetVkSwapchainKHR(), nullptr);
    vkDestroyDevice(m_device.GetLogicalDevice(), nullptr);
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    vkDestroyInstance(m_instance, nullptr);
}

// ***********************
// *** PRIVATE METHODS ***
// ***********************
void Context::createInstance(std::vector<const char*> extensions)
{
    VkResult res;

    // create Vulkan Instance
    VCTX_INFO("Creating Vulkan instance");
    VkInstanceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

#ifdef PSX_DEBUG
    // enable debug validation layers
    VCTX_INFO("Setting up debug environment");
    std::vector<const char*> validation_layers = {
        "VK_LAYER_KHRONOS_validation"
    };
    if (!checkValidationLayerSupport(validation_layers)) {
        VCTX_FATAL("Requested validation layers not supported!");
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
    res = vkCreateInstance(&create_info, nullptr, &m_instance);
    if (res != VK_SUCCESS) {
        VCTX_FATAL("Failed to create instance");
    }
}

/*
 * Check that all of the validation layers given are available.
 */
bool Context::checkValidationLayerSupport(const std::vector<const char*>& layers_wanted)
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
            VCTX_ERROR("Validation layer [{}] not supported!", layer_wanted);
            return false;
        }
    }

    return true;
}

/*
 * Setup the debug messages from Vulkan.
 */
void Context::setupDebugMessenger()
{
    VCTX_INFO("Setting up debug callback");
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
        vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        VkResult res = func(m_instance, &create_info, nullptr, &m_debug_messenger);
        if (res != VK_SUCCESS) {
            VCTX_FATAL("Failed to create debug utils messenger!");
        }
    } else {
        VCTX_FATAL("vkCreateDebugUtilsMessengerEXT extension not present!");
    }
}

/*
 * Create a surface for the given instance.
 */
void Context::createSurface(GLFWwindow *window)
{
    VkResult res = glfwCreateWindowSurface(m_instance, window, nullptr, &m_surface);
    if (res != VK_SUCCESS) {
        VCTX_FATAL("Failed to create surface!");
    }
}

void Context::createRenderPass()
{
    VkAttachmentDescription color_attachment{};
    color_attachment.format = m_swapchain.GetFormat();
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
    VkResult res = vkCreateRenderPass(m_device.GetLogicalDevice(), &rp_info, nullptr, &m_render_pass);
    if (res != VK_SUCCESS) {
        VCTX_FATAL("Failed to create Render Pass");
    }
}

/*
 * Create a graphics pipeline.
 */
void Context::createGraphicsPipeline(const std::string& vs_path, const std::string& fs_path)
{
    auto vert_shader_bytes = readSPVFile(vs_path);
    auto frag_shader_bytes = readSPVFile(fs_path);

    VkShaderModule vert_shader = 
        createShaderModule(vert_shader_bytes, m_device.GetLogicalDevice());
    VkShaderModule frag_shader = 
        createShaderModule(frag_shader_bytes, m_device.GetLogicalDevice());

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
    viewport.width = (float) m_swapchain.GetExtent().width;
    viewport.height = (float) m_swapchain.GetExtent().height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    // Scissor
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = m_swapchain.GetExtent();
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
    VkResult res = vkCreatePipelineLayout(m_device.GetLogicalDevice(), &layout_info, nullptr, &m_graphics_pipeline.layout);
    if (res != VK_SUCCESS) {
        VCTX_FATAL("Failed to create Pipeline Layout");
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
    gp_info.layout = m_graphics_pipeline.layout;
    gp_info.renderPass = m_render_pass;
    gp_info.subpass = 0;
    gp_info.basePipelineHandle = VK_NULL_HANDLE; // Optional
    gp_info.basePipelineIndex = -1; // Optional
    // create the object
    res = vkCreateGraphicsPipelines(m_device.GetLogicalDevice(), VK_NULL_HANDLE, 1, &gp_info, nullptr, &m_graphics_pipeline.ptr);
    if (res != VK_SUCCESS) {
        VCTX_FATAL("Failed to create Graphics Pipeline");
    }

    vkDestroyShaderModule(m_device.GetLogicalDevice(), vert_shader, nullptr);
    vkDestroyShaderModule(m_device.GetLogicalDevice(), frag_shader, nullptr);
}

/*
 * Open and read a given file and return the data in a byte vector.
 */
std::vector<char> Context::readSPVFile(const std::string& filepath)
{
    // open file in binary mode, starting at the end (to get file size)
    std::ifstream file(filepath, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        VCTX_FATAL("Failed to open file: {}", filepath);
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
VkShaderModule Context::createShaderModule(std::vector<char>& bytecode, VkDevice device)
{
    VkShaderModuleCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = bytecode.size();
    info.pCode = reinterpret_cast<u32*>(bytecode.data());

    VkShaderModule smod;
    VkResult res = vkCreateShaderModule(device, &info, nullptr, &smod);
    if (res != VK_SUCCESS) {
        VCTX_FATAL("Failed to create shader module!");
    }

    return smod;
}

}// end ns
}
