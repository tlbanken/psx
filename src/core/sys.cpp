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

    SYS_INFO("Initializing all System Modules");
    Bus::Init();
    Ram::Init();
    Dma::Init();
    Scratchpad::Init();
    MemControl::Init();
    Cpu::Init();
    Cop0::Init();
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

    bool new_frame = true;
    while (!ImGuiLayer::ShouldStop()) {
        // gui update
        if (g_emu_state.paused || new_frame) {
            ImGuiLayer::OnUpdate();
            new_frame = false;
        }

        // system step
        if (!g_emu_state.paused || g_emu_state.step_instr) {
            Cpu::Step();
            // TODO update new_frame when gpu finishes a new frame
            static int count = 0;
            new_frame = count++ >= 10'000; // plz fix me
            if (new_frame) count = 0;

            // check breakpoints
            if (ImGuiLayer::DbgMod::Breakpoints::ShouldBreakPC(Cpu::GetPC())) {
                ImGuiLayer::OnUpdate();
            }
        }

        // reset some state
        g_emu_state.step_instr = false;
    }
}

}// end namespace
