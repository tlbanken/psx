/*
 * renderer.cc
 *
 * Travis Banken
 * 7/12/2021
 * 
 * Vulkan renderer backend.
 */

#include "renderer.hh"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_vulkan.h"
#include "imgui/imgui_impl_sdl.h"

#include <map>
#include <set>

#define VRENDERER_INFO(...) PSXLOG_INFO("Vulkan Renderer", __VA_ARGS__)
#define VRENDERER_WARN(...) PSXLOG_WARN("Vulkan Renderer", __VA_ARGS__)
#define VRENDERER_ERROR(...) PSXLOG_ERROR("Vulkan Renderer", __VA_ARGS__)
#define VRENDERER_FATAL(...) VRENDERER_ERROR(__VA_ARGS__); throw std::runtime_error(PSX_FMT(__VA_ARGS__))

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
        VRENDERER_WARN("**Callback**: {}", callback_data->pMessage);
    } else if (msg_severity > VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        VRENDERER_FATAL("**Callback**: {}", callback_data->pMessage);
    }

    return VK_FALSE;
}

}// end private ns


namespace Psx {
namespace Vulkan {

// **********************
// *** PUBLIC METHODS ***
// **********************
Renderer::Renderer(std::vector<const char*> extensions, SDL_Window* window)
{
    createInstance(extensions);
#ifdef PSX_DEBUG
    // settup debug callback function
    setupDebugMessenger();
#endif
    createSurface(window);
    m_device = Device(m_instance, m_surface);
    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    m_swapchain = Swapchain(w, h, m_device, m_surface);
    createRenderPass();
    const std::string shader_folder("/src/view/backend/vulkan/shaders/");
    m_pipeline = Pipeline(
        m_device.GetLogicalDevice(),
        m_render_pass,
        m_swapchain.GetExtent(),
        std::string(PROJECT_ROOT_PATH) + shader_folder + "shader.vert.spv", 
        std::string(PROJECT_ROOT_PATH) + shader_folder + "shader.frag.spv" 
    );
    m_swapchain.CreateFrameBuffers(m_device.GetLogicalDevice(), m_render_pass);
    createCommandPool();
    createCommandBuffers();
    createSemaphores();
}

Renderer::~Renderer()
{
    VRENDERER_INFO("Destoying Vulkan Renderer");
#ifdef PSX_DEBUG
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(m_instance, m_debug_messenger, nullptr);
    }
#endif
    vkDestroySemaphore(m_device.GetLogicalDevice(), m_image_available_sem, nullptr);
    vkDestroySemaphore(m_device.GetLogicalDevice(), m_render_finished_sem, nullptr);
    vkDestroyCommandPool(m_device.GetLogicalDevice(), m_command_pool, nullptr);
    for (auto framebuffer : m_swapchain.GetFrameBuffers()) {
        vkDestroyFramebuffer(m_device.GetLogicalDevice(), framebuffer, nullptr);
    }
    vkDestroyDescriptorPool(m_device.GetLogicalDevice(), m_imgui_descriptor_pool, nullptr);
    vkDestroyPipeline(m_device.GetLogicalDevice(), m_pipeline.GetPipeline(), nullptr);
    vkDestroyPipelineLayout(m_device.GetLogicalDevice(), m_pipeline.GetLayout(), nullptr);
    vkDestroyRenderPass(m_device.GetLogicalDevice(), m_render_pass, nullptr);
    for (auto image_view : m_swapchain.GetImageViews()) {
        vkDestroyImageView(m_device.GetLogicalDevice(), image_view, nullptr);
    }
    vkDestroySwapchainKHR(m_device.GetLogicalDevice(), m_swapchain.GetVkSwapchainKHR(), nullptr);
    vkDestroyDevice(m_device.GetLogicalDevice(), nullptr);
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    vkDestroyInstance(m_instance, nullptr);
}

/*
 * Initialize ImGui for vulkan.
 * https://github.com/blurrypiano/littleVulkanEngine/blob/c25099a7da6770072fdfc9ec3bd4d38aa9380906/littleVulkanEngine/imguiDemo/lve_imgui.cpp
 */
void Renderer::InitializeImGui(SDL_Window *window)
{
    VRENDERER_INFO("Intializing ImGui for vulkan");
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
    if (vkCreateDescriptorPool(m_device.GetLogicalDevice(), &pool_info, nullptr, &m_imgui_descriptor_pool) != VK_SUCCESS) {
        VRENDERER_FATAL("Failed to set up descriptor pool for imgui");
    }

    // Setup Platform/Renderer backends
    // Initialize imgui for vulkan
    ImGui_ImplSDL2_InitForVulkan(window);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = m_instance;
    init_info.PhysicalDevice = m_device.GetPhysicalDevice();
    init_info.Device = m_device.GetLogicalDevice();
    init_info.QueueFamily = m_device.GetGraphicsQueueFamily();
    init_info.Queue = m_device.GetGraphicsQueue();

    // pipeline cache is a potential future optimization, ignoring for now
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = m_imgui_descriptor_pool;
    init_info.Allocator = VK_NULL_HANDLE; // TODO: Implement allocator (like VMA)
    init_info.MinImageCount = m_swapchain.GetMinImageCount();
    init_info.ImageCount = m_swapchain.GetImageCount();
    init_info.CheckVkResultFn = nullptr;// TODO: do I need this?
    ImGui_ImplVulkan_Init(&init_info, m_render_pass);
}

void Renderer::DrawFrame()
{
    // Steps to drawing a frame:
    // 1. Acquire image from swap chain
    // 2. Execute the command buffer with that image as attachment in the framebuffer
    // 3. Return the image to the swap chain for presentation

    // acquire image from swap chain
    u32 image_index;
    vkAcquireNextImageKHR(
        m_device.GetLogicalDevice(),
        m_swapchain.GetVkSwapchainKHR(),
        UINT64_MAX,
        m_image_available_sem,
        VK_NULL_HANDLE,
        &image_index
    );
}

// ***********************
// *** PRIVATE METHODS ***
// ***********************
void Renderer::createInstance(std::vector<const char*> extensions)
{
    VkResult res;

    // create Vulkan Instance
    VRENDERER_INFO("Creating Vulkan instance");
    VkInstanceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

#ifdef PSX_DEBUG
    // enable debug validation layers
    VRENDERER_INFO("Setting up debug environment");
    std::vector<const char*> validation_layers = {
        "VK_LAYER_KHRONOS_validation"
    };
    if (!checkValidationLayerSupport(validation_layers)) {
        VRENDERER_FATAL("Requested validation layers not supported!");
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
        VRENDERER_FATAL("Failed to create instance");
    }
}

/*
 * Check that all of the validation layers given are available.
 */
bool Renderer::checkValidationLayerSupport(const std::vector<const char*>& layers_wanted)
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
            VRENDERER_ERROR("Validation layer [{}] not supported!", layer_wanted);
            return false;
        }
    }

    return true;
}

/*
 * Setup the debug messages from Vulkan.
 */
void Renderer::setupDebugMessenger()
{
    VRENDERER_INFO("Setting up debug callback");
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
            VRENDERER_FATAL("Failed to create debug utils messenger!");
        }
    } else {
        VRENDERER_FATAL("vkCreateDebugUtilsMessengerEXT extension not present!");
    }
}

/*
 * Create a surface for the given instance.
 */
void Renderer::createSurface(SDL_Window *window)
{
    if (!SDL_Vulkan_CreateSurface(window, m_instance, &m_surface)) {
        VRENDERER_FATAL("Failed to create surface!");
    }
}

void Renderer::createRenderPass()
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
        VRENDERER_FATAL("Failed to create Render Pass");
    }
}

void Renderer::createCommandPool()
{
    VRENDERER_INFO("Creating command pool");
    auto graphics_family_index = m_device.GetGraphicsQueueFamily();

    VkCommandPoolCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.queueFamilyIndex = graphics_family_index;
    info.flags = 0; // TODO eventually want to set this flag

    VkResult res = vkCreateCommandPool(m_device.GetLogicalDevice(), &info, nullptr, &m_command_pool);
    if (res != VK_SUCCESS) {
        VRENDERER_FATAL("Failed to create command pool. [rc: {}]", res);
    }
}

void Renderer::createCommandBuffers()
{
    VRENDERER_INFO("Creating command buffers");
    m_command_buffers.resize(m_swapchain.GetFrameBuffers().size());

    VkCommandBufferAllocateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.commandPool = m_command_pool;
    info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    info.commandBufferCount = (u32)m_command_buffers.size();

    VkResult res = vkAllocateCommandBuffers(m_device.GetLogicalDevice(), &info, m_command_buffers.data());
    if (res != VK_SUCCESS) {
        VRENDERER_FATAL("Failed to allocate command buffers. [rc: {}]", res);
    }

    // Command Buffer Recording
    for (size_t i = 0; i < m_command_buffers.size(); i++) {
        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = 0;
        begin_info.pInheritanceInfo = nullptr;

        // begin command buffer record
        res = vkBeginCommandBuffer(m_command_buffers[i], &begin_info);
        if (res != VK_SUCCESS) {
            VRENDERER_FATAL("Failed to begin command buffer. [rc: {}]", res);
        }

        // imgui fonts
        // ImGui_ImplVulkan_CreateFontsTexture(m_command_buffers[i]);

        // Render Pass info
        VkRenderPassBeginInfo rpb_info{};
        rpb_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpb_info.renderPass = m_render_pass;
        rpb_info.framebuffer = m_swapchain.GetFrameBuffers()[i];
        rpb_info.renderArea.offset = {0, 0};
        rpb_info.renderArea.extent = m_swapchain.GetExtent();
        rpb_info.clearValueCount = 1;
        VkClearValue clear_color = {0.0, 0.0, 0.0, 1.0};
        rpb_info.pClearValues = &clear_color;

        // <Render pass start>
        vkCmdBeginRenderPass(m_command_buffers[i], &rpb_info, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(m_command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.GetPipeline());
        vkCmdDraw(m_command_buffers[i], 3, 1, 0, 0);
        vkCmdEndRenderPass(m_command_buffers[i]);
        // <Render pass end>

        // end command buffer record
        res = vkEndCommandBuffer(m_command_buffers[i]);
        if (res != VK_SUCCESS) {
            VRENDERER_FATAL("Failed to end command buffer. [rc: {}]", res);
        }

        // vkDeviceWaitIdle(g_Device);
        // ImGui_ImplVulkan_DestroyFontUploadObjects();
    }

}

void Renderer::createSemaphores()
{
    VkSemaphoreCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkResult res = vkCreateSemaphore(m_device.GetLogicalDevice(), &info, nullptr, &m_image_available_sem);
    if (res != VK_SUCCESS) {
        VRENDERER_FATAL("Failed to create image available semaphore. [rc: {}]", res);
    }
    res = vkCreateSemaphore(m_device.GetLogicalDevice(), &info, nullptr, &m_render_finished_sem);
    if (res != VK_SUCCESS) {
        VRENDERER_FATAL("Failed to create render finished semaphore. [rc: {}]", res);
    }
}

}// end ns
}
