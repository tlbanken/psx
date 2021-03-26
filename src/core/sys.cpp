/*
 * sys.cpp
 * 
 * Travis Banken
 * 12/5/2020
 * 
 * Main psx class. Inits all hardware and provides simple functionality to control
 * all hardward at once.
 */

#include <iostream>
#include <random>

#include "core/sys.h"
#include "util/psxutil.h"
#include "util/psxlog.h"
#include "mem/bus.h"
#include "mem/ram.h"
#include "cpu/cpu.h"
#include "cpu/cop0.h"
#include "layer/imgui_layer.h"
#include "core/globals.h"
#include "bios/bios.h"
#include "mem/memcontrol.h"
#include "mem/scratchpad.h"
#include "mem/dma.h"
#include "layer/dbgmod.h"
#include "cpu/asm/asm.h"
#include "gpu/gpu.h"

#define SYS_INFO(...) PSXLOG_INFO("System", __VA_ARGS__)
#define SYS_WARN(...) PSXLOG_WARN("System", __VA_ARGS__)
#define SYS_ERROR(...) PSXLOG_ERROR("System", __VA_ARGS__)

namespace Psx {

System::System(const std::string& bios_path, bool headless_mode)
    : m_headless_mode(headless_mode)
{
    SYS_INFO("Headless Mode: {}", headless_mode ? "True" : "False");
    SYS_INFO("Initializing global emu state");
    g_emu_state = {};

    if (!headless_mode) {
        ImGuiLayer::Init();
    }
    SYS_INFO("Initializing all System Modules");
    Bus::Init();
    Ram::Init();
    Dma::Init();
    Scratchpad::Init();
    MemControl::Init();
    Cpu::Init();
    Gpu::Init();
    Cop0::Init();
    Bios::Init(bios_path);
}

System::~System()
{
    SYS_INFO("Shutting Down all system modules");
    Bios::Shutdown();
    if (!m_headless_mode) {
        ImGuiLayer::Shutdown();
    }
}

void System::Reset()
{
    SYS_INFO("Reseting all system modules");
    Bios::Reset();
    Cop0::Reset();
    Cpu::Reset();
    Gpu::Reset();
    Ram::Reset();
    Dma::Reset();
    Scratchpad::Reset();
    MemControl::Reset();
    Bus::Reset();
}

/*
 * Main emulation loop. Only returns on Quit or Exception.
 */
void System::Run()
{
    if (m_headless_mode) {
        SYS_ERROR("Cannot Run() while in headless mode, use Step() instead");
        throw std::runtime_error("Cannot Run() while in headless mode, use Step() instead");
    }

    // DEBUG
    g_emu_state.paused = true;
    using namespace Psx::ImGuiLayer::DbgMod;
    // <declare breakpoints here>

    while (!ImGuiLayer::ShouldStop()) {
        // display current cpu emulation speed
        if (Util::OneSecPassed()) {
            ImGuiLayer::SetTitleExtra(PSX_FMT(" -- CPU: {:.4f} MHz ({:.1f}%)", (double)m_clocks / 1'000'000, (double)m_clocks / 33'868'8/*00*/));
            m_clocks = 0;
        }

        // gui update
        ImGuiLayer::OnUpdate();

        // system step
        if (g_emu_state.step_instr) {
            Step();
        } else {
            // TODO: Replace this with a better timing system
            // uint i = 0;
            // while (i++ < 2'000'000 && !g_emu_state.paused && !ImGuiLayer::ShouldStop()) {
            while (!g_emu_state.paused && !ImGuiLayer::ShouldStop()) {
                // Step();
                m_clocks++;
                if (Step()) break;
            }
        }

        // render current Gpu State
        Gpu::RenderFrame();

        // reset some state
        g_emu_state.step_instr = false;
    }
}

bool System::Step()
{
    using namespace Psx::ImGuiLayer::DbgMod;
    // Some timing notes:
    // Scanline:
    //  CPU Cycles = 2172
    //  GPU Cylces = 3413
    // Frame:
    //  CPU Cycles = 2172 * 263
    //  GPU Cycles = 3413 * 263

    // CPU
    Cpu::Step();
#ifdef PSX_DEBUG
    // check breakpoints
    Breakpoints::Saw<Breakpoints::BrkType::PCWatch>(Cpu::GetPC());
    if (Breakpoints::ReadyToBreak()) {
        ImGuiLayer::OnUpdate();
    }
#endif

    // GPU
    bool time_to_render = Gpu::Step();
    
    // DMA
    Dma::Step();
    return time_to_render;
    // return false;
}

}// end namespace
