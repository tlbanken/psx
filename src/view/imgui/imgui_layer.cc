/*
 * imgui_layer.cc
 * 
 * Travis Banken
 * 12/10/2020
 * 
 * Layer over the Dear ImGui Framework
 */
#include "imgui_layer.hh"

#include <vector>

#include "imgui/imgui.h"
// #include "imgui/imgui_impl_vulkan.h"

#include "view/imgui/dbgmod.hh"
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
#include "io/timer.hh"
#include "cpu/interrupt.hh"

#define IMGUILAYER_INFO(...) PSXLOG_INFO("ImGui-Layer", __VA_ARGS__)
#define IMGUILAYER_WARN(...) PSXLOG_WARN("ImGui-Layer", __VA_ARGS__)
#define IMGUILAYER_ERR(...) PSXLOG_ERROR("ImGui-Layer", __VA_ARGS__)
#define IMGUILAYER_FATAL(...) IMGUILAYER_ERR(__VA_ARGS__); throw std::runtime_error(PSX_FMT(__VA_ARGS__))

namespace Psx {
namespace View {
namespace ImGuiLayer {

void Init(Style style)
{
    IMGUILAYER_INFO("Setting up ImGui");
    // create imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

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
    ImGui::DestroyContext();
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
    static bool timer_active = false;
    static bool int_active = false;

    // Call Modules if currently active
    if (ram_active) Ram::OnActive(&ram_active);
    if (scratch_active) Scratchpad::OnActive(&scratch_active);
    if (cpu_active) Cpu::OnActive(&cpu_active);
    if (cop0_active) Cop0::OnActive(&cop0_active);
    if (bios_active) Bios::OnActive(&bios_active);
    if (memctrl_active) MemControl::OnActive(&memctrl_active);
    if (dma_active) Dma::OnActive(&dma_active);
    if (gpu_active) Gpu::OnActive(&gpu_active);
    if (timer_active) Timer::OnActive(&timer_active);
    if (int_active) Interrupt::OnActive(&int_active);
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
            ImGui::MenuItem("Timers", NULL, &timer_active);
            ImGui::MenuItem("Interrupts", NULL, &int_active);

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
}

}// end namespace
}
}
