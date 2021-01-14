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

#define SYS_INFO(...) PSXLOG_INFO("SYS", __VA_ARGS__)
#define SYS_WARN(...) PSXLOG_WARN("SYS", __VA_ARGS__)
#define SYS_ERROR(...) PSXLOG_ERROR("SYS", __VA_ARGS__)

namespace Psx {

System::System(const std::string& bios_path, bool headless_mode)
    : m_headless_mode(headless_mode)
{
    SYS_INFO("Initializing global emu state");
    g_emu_state = {};

    SYS_INFO("Initializing all System Modules");
    SYS_INFO("Initializing Bus");
    Bus::Init();
    SYS_INFO("Initializing Ram");
    Ram::Init();
    // CPU
    SYS_INFO("Initializing CPU and COP");
    Cpu::Init();
    Cop0::Init();
    // BIOS
    SYS_INFO("Creating BIOS from {}", bios_path);
    Bios::Init(bios_path);

    if (!headless_mode) {
        ImGuiLayer::Init();
    }
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
    Ram::Reset();
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

    g_emu_state.paused = true;
    while (!ImGuiLayer::ShouldStop()) {
        // gui update
        ImGuiLayer::OnUpdate();

        // cpu update
        if (!g_emu_state.paused || g_emu_state.step_instr) {
            Cpu::Step();
        }

        // reset some state
        g_emu_state.step_instr = false;
    }
}

}// end namespace
