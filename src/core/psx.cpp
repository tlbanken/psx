/*
 * psx.cpp
 * 
 * Travis Banken
 * 12/5/2020
 * 
 * Main psx class. Inits all hardware and provides simple functionality to control
 * all hardward at once.
 */

#include <iostream>
#include <random>

#include "core/psx.h"
#include "util/psxutil.h"
#include "util/psxlog.h"
#include "mem/bus.h"
#include "mem/ram.h"
#include "cpu/cpu.h"
#include "layer/imgui_layer.h"
#include "core/globals.h"

#define PSX_INFO(msg) PSXLOG_INFO("PSX", msg)
#define PSX_WARN(msg) PSXLOG_WARN("PSX", msg)
#define PSX_ERROR(msg) PSXLOG_ERROR("PSX", msg)

Psx::Psx()
    : m_imgui_layer(ImGuiLayer::Style::Dark)
{
    PSX_INFO("Initializing the PSX");

    PSX_INFO("Initializing Bus");
    m_bus = std::shared_ptr<Bus>(new Bus());

    PSX_INFO("Initializing Ram");
    std::shared_ptr<Ram> ram(new Ram());

    // add to bus and imgui
    m_bus->AddAddressSpace(ram, BusPriority::First);
    m_imgui_layer.AddDbgModule(ram);

    PSX_INFO("Initializing CPU");
    m_cpu = std::shared_ptr<Cpu>(new Cpu(m_bus));

    // add to imgui
    m_imgui_layer.AddDbgModule(m_cpu);
}

/*
 * Main emulation loop. Only returns on Quit or Exception.
 */
void Psx::Run()
{
    g_emu_state.paused = true;
    u32 pc = 0x0000'0100;
    m_cpu->SetPC(pc);
    // write a little test program @ 0x0000'0100
    m_bus->Write32(0x04234543, pc);
    pc += 4;
    m_bus->Write32(Asm::AsmInstruction("BLTZ    R2 5"), pc);
    pc += 4;
    m_bus->Write32(Asm::AsmInstruction("SYSCALL"), pc);
    pc += 4;
    m_bus->Write32(Asm::AsmInstruction("ADD     R2 R0 R1"), pc);
    pc += 4;
    m_bus->Write32(Asm::AsmInstruction("ADDI    R1 R0 23"), pc);
    pc += 4;
    while (!m_imgui_layer.ShouldStop()) {
        // gui update
        m_imgui_layer.OnUpdate();

        // cpu update
        if (!g_emu_state.paused || g_emu_state.step_instr) {
            m_cpu->Step();
        }

        // reset some state
        g_emu_state.step_instr = false;
    }
}
