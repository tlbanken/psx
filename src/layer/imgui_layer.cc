/*
 * imgui_layer.cpp
 * 
 * Travis Banken
 * 12/10/2020
 * 
 * Layer over the Dear ImGui Framework
 */


#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_vulkan.h"

#include "layer/imgui_layer.hh"
#include "layer/dbgmod.hh"
#include "cpu/cpu.hh"
#include "cpu/cop0.hh"
#include "bios/bios.hh"
#include "util/psxlog.hh"
#include "util/psxutil.hh"
#include "mem/ram.hh"
#include "mem/memcontrol.hh"
#include "mem/scratchpad.hh"
#include "mem/dma.hh"
#include "core/globals.hh"
#include "core/sys.hh"
#include "gpu/gpu.hh"

#define WINDOW_H 1280
#define WINDOW_W 720

#define IMGUILAYER_INFO(...) PSXLOG_INFO("ImGui-Layer", __VA_ARGS__)
#define IMGUILAYER_WARN(...) PSXLOG_WARN("ImGui-Layer", __VA_ARGS__)
#define IMGUILAYER_ERR(...) PSXLOG_ERROR("ImGui-Layer", __VA_ARGS__)
#define IMGUILAYER_FATAL(...) IMGUILAYER_ERR(__VA_ARGS__); throw std::runtime_error(PSX_FMT(__VA_ARGS__))

namespace  {
struct State {
    GLFWwindow *window;
    const std::string title_base = "PSX Emulator";
} s;

// Protos
void newFrame();
void render();

}// end namespace

/*
 * Will be called by GLFW on errors.
 */
static void errorCallback(int error, const char* description)
{
    PSXLOG_ERROR("GLFW Error", "({:d}), {}", error, description);
}


namespace Psx {
namespace ImGuiLayer {

void Init(ImGuiLayer::Style style)
{
    IMGUILAYER_INFO("Setting up ImGui");

    // setup glfw
    glfwSetErrorCallback(errorCallback);
    if (!glfwInit()) {
        IMGUILAYER_FATAL("Failed to Initialize GLFW!");
    }

    // create window
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    s.window = glfwCreateWindow(WINDOW_H, WINDOW_W, s.title_base.c_str(), nullptr, nullptr);
    if (s.window == nullptr) {
        IMGUILAYER_FATAL("Failed to create GLFW window!");
    }
    // ???
    // glfwMakeContextCurrent(s.window);
    // glfwSwapInterval(1); // enable vsync TODO: make this an option?
    // ???

    // get extensions
    u32 extensions_count = 0;
    const char **extensions = glfwGetRequiredInstanceExtensions(&extensions_count);
    // TODO Setup Vulkan

    // create imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // setup OpenGL and GLFW
    ImGui_ImplGlfw_InitForVulkan(s.window, true);
    // TODO setup init info for ImGui_ImplVulkan_InitInfo
    // TODO call ImGui_ImplVulkan_Init(<init info>, <render pass>);
    // ImGui_ImplOpenGL3_Init();

    // set style
    if (style == ImGuiLayer::Style::Dark) {
        ImGui::StyleColorsDark();
    } else {
        ImGui::StyleColorsLight();
    }
}

void Shutdown()
{
    IMGUILAYER_INFO("Shutting down");

    // ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(s.window);
    glfwTerminate();
}

/*
 * Handles updating everything ImGui side of things. Should be called in the main
 * emulation loop.
 */
void OnUpdate()
{
    // active states
    static bool ram_active = false;
    static bool cpu_active = true; // start with cpu enabled for now
    static bool bios_active = false;
    static bool breakpoints_active = false;
    static bool cop0_active = false;
    static bool memctrl_active = false;
    static bool scratch_active = false;
    static bool dma_active = false;
    static bool gpu_active = false;

    newFrame();

    // Call Modules if currently active
    if (ram_active) Ram::OnActive(&ram_active);
    if (scratch_active) Scratchpad::OnActive(&scratch_active);
    if (cpu_active) Cpu::OnActive(&cpu_active);
    if (cop0_active) Cop0::OnActive(&cop0_active);
    if (bios_active) Bios::OnActive(&bios_active);
    if (memctrl_active) MemControl::OnActive(&memctrl_active);
    if (dma_active) Dma::OnActive(&dma_active);
    if (gpu_active) Gpu::OnActive(&gpu_active);
#ifdef PSX_DEBUG
    if (breakpoints_active) DbgMod::Breakpoints::OnActive(&breakpoints_active);
#endif

    // Main Menu Bar
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Debug")) {

#ifdef PSX_DEBUG
            ImGui::MenuItem("Breakpoints", NULL, &breakpoints_active);
#endif
            ImGui::MenuItem("Ram", NULL, &ram_active);
            ImGui::MenuItem("Scratchpad", NULL, &scratch_active);
            ImGui::MenuItem("Cpu", NULL, &cpu_active);
            ImGui::MenuItem("Cop0", NULL, &cop0_active);
            ImGui::MenuItem("Bios", NULL, &bios_active);
            ImGui::MenuItem("MemControl", NULL, &memctrl_active);
            ImGui::MenuItem("DMA", NULL, &dma_active);
            ImGui::MenuItem("GPU", NULL, &gpu_active);

            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("State")) {
            if (g_emu_state.paused) {
                if (ImGui::MenuItem("Resume")) {
                    IMGUILAYER_INFO("Resuming emulation");
                    g_emu_state.paused = false;
                }
            } else {
                if (ImGui::MenuItem("Pause")) {
                    IMGUILAYER_INFO("Pausing emulation");
                    g_emu_state.paused = true;
                }
            }

            if (ImGui::MenuItem("Reset")) {
                IMGUILAYER_INFO("Resetting Emulator");
                System::Reset();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
#ifdef PSX_DEBUG
    DbgMod::Breakpoints::OnUpdate();
#endif

    // Demo
    // ImGui::ShowDemoWindow();

    render();
}

/*
 * Checks if the ImGui needs to terminate. This most common case being if the
 * window is closed.
 */
bool ShouldStop()
{
    return glfwWindowShouldClose(s.window);
}

void SetTitleExtra(const std::string& extra)
{
    glfwSetWindowTitle(s.window, (s.title_base + extra).c_str());
}


}// end namespace
}

namespace {

// ===============================================
// PRIVATE HELPER FUNCTIONS
// ==============================================
/*
 * Creates a new frame in ImGui. Should be called BEFORE ImGuiLayer::render()
 */
void newFrame()
{
    glfwPollEvents();
    // start new frame
    // ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

/*
 * Renders the current draw data in ImGui.
 */
void render()
{
    ImGui::Render();
    // glClear(GL_COLOR_BUFFER_BIT);
    // ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    // TODO make render function
    glfwSwapBuffers(s.window);
}

}
