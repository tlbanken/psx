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
    m_context.reset(new Context(vec_extensions, m_window));
    delete[] extensions;

    Psx::View::ImGuiLayer::Init();
    m_context->InitializeImGui(m_window);
}

Window::~Window()
{
    VWINDOW_INFO("Destroying Vulkan Window");
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    Psx::View::ImGuiLayer::Shutdown();
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
    // glfwSwapBuffers(m_window);
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
