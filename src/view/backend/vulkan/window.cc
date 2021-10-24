/*
 * window.hh
 *
 * Travis Banken
 * 7/9/2021
 * 
 * Vulkan-backed window.
 */

#include "window.hh"

#include <imgui/imgui.h>
#include <imgui/imgui_impl_vulkan.h>
#include <imgui/imgui_impl_sdl.h>

#include <vector>

#include "view/imgui/imgui_layer.hh"

#define VWINDOW_INFO(...) PSXLOG_INFO("Vulkan Window", __VA_ARGS__)
#define VWINDOW_WARN(...) PSXLOG_WARN("Vulkan Window", __VA_ARGS__)
#define VWINDOW_ERROR(...) PSXLOG_ERROR("Vulkan Window", __VA_ARGS__)
#define VWINDOW_FATAL(...) VWINDOW_ERROR(__VA_ARGS__); throw std::runtime_error(PSX_FMT(__VA_ARGS__))

#define CLEAR_COLOR {0.0, 0.0, 0.0, 1.0}

// *** PRIVATE NAMESPACE ***
namespace {
using namespace Psx::Vulkan;

// protos
void frameRender(Builder::WindowData *wd, Builder::DeviceData *dd, ImDrawData *draw_data);
void framePresent(Builder::WindowData *wd, Builder::DeviceData *dd);
void uploadImGuiFonts(Builder::WindowData *wd, Builder::DeviceData *dd);

}// end private ns


namespace Psx {
namespace Vulkan {

Window::Window(int width, int height, const std::string& title)
    : m_title_base(title)
{
    VWINDOW_INFO("Initializing window with Vulkan backend");


    // setup sdl2
    if (SDL_Init(SDL_INIT_VIDEO)) {
        VWINDOW_FATAL("Failed to initialize SDL2: {}", SDL_GetError());
    }

    // create window
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    m_window = SDL_CreateWindow(
        m_title_base.c_str(), 
        SDL_WINDOWPOS_CENTERED, 
        SDL_WINDOWPOS_CENTERED, 
        width, 
        height, 
        window_flags
    );
    if (m_window == nullptr) {
        VWINDOW_FATAL("Failed to create SDL Window: {}", SDL_GetError());
    }


    // get extensions
    u32 extensions_count = 0;
    SDL_Vulkan_GetInstanceExtensions(m_window, &extensions_count, NULL);
    const char **extensions = new const char*[extensions_count];
    SDL_Vulkan_GetInstanceExtensions(m_window, &extensions_count, extensions);
    std::vector<const char*> vec_extensions;
    for (u32 i = 0; i < extensions_count; i++) {
        vec_extensions.push_back(extensions[i]);
    }
    delete[] extensions;

    // build vulkan pieces
    m_wd = new Builder::WindowData();
    m_dd = new Builder::DeviceData();
    Builder::Init(m_allocator_callbacks);
    m_instance = Builder::CreateInstance(vec_extensions);
    if (!SDL_Vulkan_CreateSurface(m_window, m_instance, &m_wd->surface)) {
        VWINDOW_FATAL("Failed to create surface!");
    }
    // set clear color
    m_wd->clear_value = {{CLEAR_COLOR}};
    Builder::SetupDebugMessenger(m_instance);
    Builder::BuildDeviceData(m_dd, m_instance, m_wd->surface);
    Builder::BuildSwapchainData(m_wd, m_dd, width, height);
    Builder::BuildImageViews(m_wd, m_dd);
    Builder::BuildRenderPassData(m_wd, m_dd);
    const std::string shader_folder("/src/view/backend/vulkan/shaders/");
    const std::string vs_path = std::string(PROJECT_ROOT_PATH) + shader_folder + "shader.vert.spv"; 
    const std::string fs_path = std::string(PROJECT_ROOT_PATH) + shader_folder + "shader.frag.spv";
    Builder::BuildPipelineData(m_wd, m_dd, vs_path, fs_path);
    Builder::BuildFrameBuffersData(m_wd, m_dd);
    Builder::BuildCommandBuffersData(m_wd, m_dd);
    Builder::BuildVertexBuffer(m_wd, m_dd);

    Psx::View::ImGuiLayer::Init();
    Builder::InitializeImGuiVulkan(m_wd, m_dd, m_instance, m_window);
    uploadImGuiFonts(m_wd, m_dd);
}

Window::~Window()
{
    VWINDOW_INFO("Destroying Vulkan Window");
    vkDeviceWaitIdle(m_dd->logidata.dev);
    vkQueueWaitIdle(m_dd->physdata.graphics_queue);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    Psx::View::ImGuiLayer::Shutdown();
    Builder::Destroy(m_wd, m_dd, m_instance);
    delete m_wd;
    delete m_dd;
    SDL_DestroyWindow(m_window);
    SDL_Quit();
}

/*
 * Returns true if the window was told to close.
 */
bool Window::ShouldClose()
{
    SDL_Event event;
    bool done = false;
    while (SDL_PollEvent(&event))
    {
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT)
            done = true;
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(m_window))
            done = true;
    }
    return done;
}

/*
 * Set the extra info in the title. Can be used to add things like frame rate.
 */
void Window::SetTitleExtra(const std::string& extra)
{
    SDL_SetWindowTitle(m_window, (m_title_base + extra).c_str());
}

/*
 * Start of the new frame.
 */
void Window::NewFrame()
{
    // glfwPollEvents(); // TODO handle events
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL2_NewFrame(m_window);
    ImGui::NewFrame();
}

/*
 * Render the current data to the screen.
 */
void Window::Render()
{
    ImGui::Render();
    frameRender(m_wd, m_dd, ImGui::GetDrawData());
    framePresent(m_wd, m_dd);
}

/*
 * Call this to update the window. Requests a new frame and renders data.
 */
void Window::OnUpdate()
{
    NewFrame();
    Psx::View::ImGuiLayer::OnUpdate();
    Render();
}

}// end ns
}

// *** PRIVATE NAMESPACE ***
namespace {
using namespace Psx::Vulkan;

void frameRender(Builder::WindowData *wd, Builder::DeviceData *dd, ImDrawData *draw_data)
{
    VkResult res;
    PSX_ASSERT(draw_data != nullptr);
    PSX_ASSERT(wd != nullptr);
    PSX_ASSERT(dd != nullptr);

    VkSemaphore image_acquired_semaphore = wd->frame_semaphores[wd->semaphore_index].image_acquire;
    VkSemaphore render_complete_semaphore = wd->frame_semaphores[wd->semaphore_index].render_complete;
    res = vkAcquireNextImageKHR(dd->logidata.dev, wd->swapchain, UINT64_MAX, image_acquired_semaphore, VK_NULL_HANDLE, &wd->frame_index);
    if (res == VK_ERROR_OUT_OF_DATE_KHR) {
        // TODO need to rebuild swap chain
        VWINDOW_FATAL("No support for rebuilding of swapchain!!\n");
    }
    else if (res == VK_SUBOPTIMAL_KHR) {
        // TODO may want to rebuild swap chain here as well
        VWINDOW_WARN("Not rebuilding swapchain when suggested!!\n");
    }
    else if (res != VK_SUCCESS) {
        VWINDOW_FATAL("Failed to acquire next image. [rc: {}]", res);
    }

    Builder::FrameData *fd = &wd->frames[wd->frame_index];

    // wait for fences
    res = vkWaitForFences(dd->logidata.dev, 1, &fd->fence, VK_TRUE, UINT64_MAX);
    if (res != VK_SUCCESS) {
        VWINDOW_FATAL("Failed to wait for fences. [rc: {}]", res);
    }
    res = vkResetFences(dd->logidata.dev, 1, &fd->fence);
    if (res != VK_SUCCESS) {
        VWINDOW_FATAL("Failed to reset fences. [rc: {}]", res);
    }

    // reset command pool
    res = vkResetCommandPool(dd->logidata.dev, fd->command_pool, 0);
    if (res != VK_SUCCESS) {
        VWINDOW_FATAL("Failed to reset command pool. [rc: {}]", res);
    }
    VkCommandBufferBeginInfo cb_info{};
    cb_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cb_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    res = vkBeginCommandBuffer(fd->command_buffer, &cb_info);
    if (res != VK_SUCCESS) {
        VWINDOW_FATAL("Failed to begin command buffer. [rc: {}]", res);
    }

    // render pass
    VkRenderPassBeginInfo rp_info{};
    rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rp_info.renderPass = wd->render_pass;
    rp_info.framebuffer = fd->framebuffer;
    rp_info.renderArea.extent = wd->extent;
    rp_info.clearValueCount = 1;
    rp_info.pClearValues = &wd->clear_value;
    vkCmdBeginRenderPass(fd->command_buffer, &rp_info, VK_SUBPASS_CONTENTS_INLINE);

    // ------------------------
    // Render PSX Graphics here
    // ------------------------

    // just draw a basic triangle for now
    vkCmdBindPipeline(fd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, wd->pipeline);

    // TESTING
    static float r = 0.9f;
    static float g = 0.5f;
    static float b = 0.0f;
    static float rstep = 0.05;
    static float gstep = 0.05;
    static float bstep = 0.05;
    wd->vertex_buffer->At(0).col = {r, 1.0 - g, b};
    wd->vertex_buffer->At(1).col = {1.0 - r, g, b};
    wd->vertex_buffer->At(2).col = {r, g, 1.0 - b};
    r += rstep;
    if (r > 1.0 || r < 0.0) rstep *= -1.0;
    g += gstep;
    if (g > 1.0 || g < 0.0) gstep *= -1.0;
    b += bstep;
    if (b > 1.0 || b < 0.0) bstep *= -1.0;

    wd->vertex_buffer->Draw(fd->command_buffer);

    // ------------------------

    // imgui primitives
    ImGui_ImplVulkan_RenderDrawData(draw_data, fd->command_buffer);

    // submit command buffer
    vkCmdEndRenderPass(fd->command_buffer);
    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo subinfo{};
    subinfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    subinfo.waitSemaphoreCount = 1;
    subinfo.pWaitSemaphores = &image_acquired_semaphore;
    subinfo.pWaitDstStageMask = &wait_stage;
    subinfo.commandBufferCount = 1;
    subinfo.pCommandBuffers = &fd->command_buffer;
    subinfo.signalSemaphoreCount = 1;
    subinfo.pSignalSemaphores = &render_complete_semaphore;
    res = vkEndCommandBuffer(fd->command_buffer);
    if (res != VK_SUCCESS) {
        VWINDOW_FATAL("Failed to end command buffer. [rc: {}]", res);
    }
    res = vkQueueSubmit(dd->physdata.graphics_queue, 1, &subinfo, fd->fence);
    if (res != VK_SUCCESS) {
        VWINDOW_FATAL("Failed to submit queue. [rc: {}]", res);
    }
}

void framePresent(Builder::WindowData *wd, Builder::DeviceData *dd)
{
    // TODO if need to rebuild swap chain, return

    VkSemaphore render_complete_semaphore = wd->frame_semaphores[wd->semaphore_index].render_complete;
    VkPresentInfoKHR info{};
    info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &render_complete_semaphore;
    info.swapchainCount = 1;
    info.pSwapchains = &wd->swapchain;
    info.pImageIndices = &wd->frame_index;
    VkResult res = vkQueuePresentKHR(dd->physdata.present_queue, &info);
    if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
        // TODO need to rebuild swap chain
        VWINDOW_FATAL("No support for rebuilding of swapchain!!\n");
        // Builder::RebuildSwapchain(wd, dd, ...);
    }
    if (res != VK_SUCCESS) {
        VWINDOW_FATAL("Failed to present queue. [rc: {}]", res);
    }
    wd->semaphore_index = (wd->semaphore_index + 1) % wd->image_count;
}

void uploadImGuiFonts(Builder::WindowData *wd, Builder::DeviceData *dd)
{
    // use any command pool
    VkCommandPool command_pool = wd->frames[wd->frame_index].command_pool;
    VkCommandBuffer command_buffer = wd->frames[wd->frame_index].command_buffer;

    VkResult res = vkResetCommandPool(dd->logidata.dev, command_pool, 0);
    if (res != VK_SUCCESS) {
        VWINDOW_FATAL("Failed to reset command pool. [rc: {}]", res);
    }
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    res = vkBeginCommandBuffer(command_buffer, &begin_info);
    if (res != VK_SUCCESS) {
        VWINDOW_FATAL("Failed to begin command buffer. [rc: {}]", res);
    }

    // upload font
    ImGui_ImplVulkan_CreateFontsTexture(command_buffer);

    VkSubmitInfo end_info{};
    end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    end_info.commandBufferCount = 1;
    end_info.pCommandBuffers = &command_buffer;
    res = vkEndCommandBuffer(command_buffer);
    if (res != VK_SUCCESS) {
        VWINDOW_FATAL("Failed to end command buffer. [rc: {}]", res);
    }
    res = vkQueueSubmit(dd->physdata.graphics_queue, 1, &end_info, VK_NULL_HANDLE);
    if (res != VK_SUCCESS) {
        VWINDOW_FATAL("Failed to submit queue. [rc: {}]", res);
    }

    res = vkDeviceWaitIdle(dd->logidata.dev);
    if (res != VK_SUCCESS) {
        VWINDOW_FATAL("Failed vkDeviceWaitIdle. [rc: {}]", res);
    }
    ImGui_ImplVulkan_DestroyFontUploadObjects();
}

}// end ns
