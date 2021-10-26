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

#include "core/sys.hh"
#include "view/view.hh"
#include "util/psxutil.hh"
#include "util/psxlog.hh"
#include "mem/bus.hh"
#include "mem/ram.hh"
#include "cpu/cpu.hh"
#include "cpu/cop0.hh"
#include "core/globals.hh"
#include "bios/bios.hh"
#include "mem/memcontrol.hh"
#include "mem/scratchpad.hh"
#include "mem/dma.hh"
#include "view/imgui/dbgmod.hh"
#include "cpu/asm/asm.hh"
#include "gpu/gpu.hh"

#define SYS_INFO(...) PSXLOG_INFO("System", __VA_ARGS__)
#define SYS_WARN(...) PSXLOG_WARN("System", __VA_ARGS__)
#define SYS_ERROR(...) PSXLOG_ERROR("System", __VA_ARGS__)

#define CPU_MAX_CLOCK_RATE (33'868'800)

namespace Psx {

System::System(const std::string& bios_path, bool headless_mode)
    : m_headless_mode(headless_mode)
{
    SYS_INFO("Headless Mode: {}", headless_mode ? "True" : "False");
    SYS_INFO("Initializing global emu state");
    g_emu_state = {};

    if (!headless_mode) {
        View::Init();
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
        View::Shutdown();
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
    // using namespace Psx::ImGuiLayer::DbgMod;
    // <declare breakpoints here>

    bool should_close = false;
    long long delta_ns = 0;
    const long long frame_time_ns = (1.0 / 60.0) * 1'000'000'000; // nano seconds in 1 frame for 60 fps
    u64 clocks = 0;
    SYS_INFO("Target Frame Time: {} ns", frame_time_ns);
    while (!should_close) {
        // display current cpu emulation speed
        if (Util::OneSecPassed()) {
            View::SetTitleExtra(PSX_FMT(" -- CPU: {:.4f} MHz ({:.1f}%)", (double)clocks / 1'000'000, (double)clocks / 33'868'8/*00*/));
            clocks = 0;
        }

        // gui update
        View::OnUpdate();

        // system step
        if (g_emu_state.step_instr) {
            Step();
        } else {
            while (!g_emu_state.paused && delta_ns < frame_time_ns && clocks <= CPU_MAX_CLOCK_RATE) {
                Step();
                clocks++;
                delta_ns += Util::GetDeltaTime();
            }
        }

        // reset some state
        delta_ns = 0;
        g_emu_state.step_instr = false;
        should_close = View::ShouldClose();
    }
}

bool System::Step()
{
    using namespace Psx::View::ImGuiLayer::DbgMod;
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
        View::OnUpdate();
    }
#endif

    // GPU
    bool time_to_render = Gpu::Step();
    
    // DMA
    Dma::Step();
    return true;
}

}// end namespace
