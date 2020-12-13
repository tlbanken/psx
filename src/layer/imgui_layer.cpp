/*
 * imgui_layer.cpp
 * 
 * Travis Banken
 * 12/10/2020
 * 
 * Layer over the Dear ImGui Framework
 */

#include <stdexcept>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include "fmt/core.h"

#include "layer/imgui_layer.h"
#include "util/psxlog.h"

#define WINDOW_H 1280
#define WINDOW_W 720

#define IMGUILAYER_INFO(msg) PSXLOG_INFO("ImGui-Layer", msg)
#define IMGUILAYER_WARN(msg) PSXLOG_WARN("ImGui-Layer", msg)
#define IMGUILAYER_ERR(msg) PSXLOG_ERROR("ImGui-Layer", msg)

/*
 * Will be called by GLFW on errors.
 */
static void errorCallback(int error, const char* description)
{
    PSXLOG_ERROR("GLFW Error", fmt::format("({:d}), {}", error, description));
}

// ===============================================
// PUBLIC FUNCTIONS
// ===============================================
ImGuiLayer::ImGuiLayer()
{
    init();
}

ImGuiLayer::ImGuiLayer(ImGuiLayer::Style style)
{
    init();
    if (style == ImGuiLayer::Style::Dark) {
        ImGui::StyleColorsDark();
    } else {
        ImGui::StyleColorsLight();
    }
}

ImGuiLayer::~ImGuiLayer()
{
    IMGUILAYER_INFO("Shutting down");

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(m_window);
    glfwTerminate();
}


/*
 * Handles updating everything ImGui side of things. Should be called in the main
 * emulation loop.
 */
void ImGuiLayer::onUpdate()
{
    newFrame();

    // Call Modules if currently active
    for (auto& entry : m_modEntries) {
        if (entry.active) {
            entry.module->onActive(&entry.active);
        }
    }

    // Main Menu Bar
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Debug")) {
            for (auto& e : m_modEntries) {
                ImGui::MenuItem(e.module->getModuleLabel().data(), NULL, &e.active);
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    // Demo
    ImGui::ShowDemoWindow();

    render();
}

/*
 * Checks if the ImGui needs to terminate. This most common case being if the
 * window is closed.
 */
bool ImGuiLayer::shouldStop()
{
    return glfwWindowShouldClose(m_window);
}

/*
 * Add a ImGui Debug Module which will be show up in the "Debug" main menu tab.
 */
void ImGuiLayer::addDbgModule(std::shared_ptr<ImGuiLayer::DbgModule> module)
{
    IMGUILAYER_INFO(fmt::format("Adding debug module [{}]", module->getModuleLabel()));
    DbgModEntry entry{module, false};
    m_modEntries.push_back(entry);
}



// ===============================================
// PRIVATE HELPER FUNCTIONS
// ===============================================
void ImGuiLayer::init()
{
    IMGUILAYER_INFO("Setting up ImGui");
    // setup glfw
    glfwSetErrorCallback(errorCallback);
    if (!glfwInit()) {
        IMGUILAYER_ERR("Failed to Initialize GLFW!");
        throw std::runtime_error("Failed to Initialize GLFW");
    }

    // create window
    m_window = glfwCreateWindow(WINDOW_H, WINDOW_W, "PSX Emulator", nullptr, nullptr);
    if (m_window == nullptr) {
        IMGUILAYER_ERR("Failed to create GLFW window!");
        throw std::runtime_error("Failed to create GLFW window!");
    }
    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1); // enable vsync TODO: make this an option?

    // init opengl loader
    if (!gladLoadGL()) {
        IMGUILAYER_ERR("Failed to Initialize OpenGL Loader!");
        throw std::runtime_error("Failed to Initialize OpenGL Loader!");
    }

    // create imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // setup OpenGL and GLFW
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init();
}

/*
 * Creates a new frame in ImGui. Should be called BEFORE ImGuiLayer::render()
 */
void ImGuiLayer::newFrame()
{
    glfwPollEvents();
    // start new frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

/*
 * Renders the current draw data in ImGui.
 */
void ImGuiLayer::render()
{
    ImGui::Render();
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(m_window);
}
