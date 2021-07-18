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
#include <imgui/imgui_impl_glfw.h>

#include <vector>

#include "view/imgui/imgui_layer.hh"

#define VWINDOW_INFO(...) PSXLOG_INFO("Vulkan Window", __VA_ARGS__)
#define VWINDOW_WARN(...) PSXLOG_WARN("Vulkan Window", __VA_ARGS__)
#define VWINDOW_ERROR(...) PSXLOG_ERROR("Vulkan Window", __VA_ARGS__)
#define VWINDOW_FATAL(...) VWINDOW_ERROR(__VA_ARGS__); throw std::runtime_error(PSX_FMT(__VA_ARGS__))


// *** PRIVATE STUFF ***
namespace {

/*
 * Will be called by GLFW on errors.
 */
void errorCallback(int error, const char* description)
{
    PSXLOG_ERROR("GLFW Error", "({:d}), {}", error, description);
}
}// end ns

namespace Psx {
namespace Vulkan {

Window::Window(int width, int height, const std::string& title)
    : m_title_base(title)
{
    VWINDOW_INFO("Initializing window with Vulkan backend");

    // setup glfw
    glfwSetErrorCallback(errorCallback);
    if (!glfwInit()) {
        VWINDOW_FATAL("Failed to initialize GLFW!");
    }

    // create window
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (m_window == nullptr) {
        VWINDOW_FATAL("Failed to create GLFW window!");
    }

    // get extensions
    u32 extensions_count = 0;
    const char **extensions = glfwGetRequiredInstanceExtensions(&extensions_count);
    std::vector<const char*> vec_extensions;
    for (u32 i = 0; i < extensions_count; i++) {
        vec_extensions.push_back(extensions[i]);
    }
    m_context.reset(new Context(vec_extensions, m_window));
}

Window::~Window()
{
    VWINDOW_INFO("Destroying Vulkan Window");
    ImGui_ImplVulkan_Shutdown();
    Psx::View::ImGuiLayer::Shutdown();
}

/*
 * Returns true if the window was told to close.
 */
bool Window::ShouldClose()
{
    return glfwWindowShouldClose(m_window);
}

/*
 * Set the extra info in the title. Can be used to add things like frame rate.
 */
void Window::SetTitleExtra(const std::string& extra)
{
    glfwSetWindowTitle(m_window, (m_title_base + extra).c_str());
}

/*
 * Start of the new frame.
 */
void Window::NewFrame()
{
    glfwPollEvents();
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

/*
 * Render the current data to the screen.
 */
void Window::Render()
{
    ImGui::Render();
    glfwSwapBuffers(m_window);
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
