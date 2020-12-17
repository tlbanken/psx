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
    : m_imguiLayer(ImGuiLayer::Style::Dark)
{
    PSX_INFO("Initializing the PSX");

    PSX_INFO("Initializing Bus");
    m_bus = std::shared_ptr<Bus>(new Bus());

    PSX_INFO("Initializing Ram");
    std::shared_ptr<Ram> ram(new Ram());

    // add to bus and imgui
    m_bus->addAddressSpace(ram, BusPriority::First);
    m_imguiLayer.addDbgModule(ram);

    PSX_INFO("Initializing CPU");
    m_cpu = std::shared_ptr<Cpu>(new Cpu(m_bus));

    // add to imgui
    m_imguiLayer.addDbgModule(m_cpu);
}

/*
 * Main emulation loop. Only returns on Quit or Exception.
 */
void Psx::run()
{
    g_emuState.paused = true;
    u32 ramSize = 2 * 1024 * 1024;
    m_cpu->setPC(0x0000'0000);
    while (!m_imguiLayer.shouldStop()) {
        // gui update
        m_imguiLayer.onUpdate();

        // TEST some random writes
        u32 addr = (u32) rand() % ramSize;
        u32 addr1 = (u32) rand() % (100 << 4);
        u8 data = (u8) rand() % 256;
        m_bus->write8(data, addr);
        m_bus->write8(data, addr1);

        // cpu update
        if (!g_emuState.paused || g_emuState.stepInstr) {
            m_cpu->step();
        }


        // reset some state
        g_emuState.stepInstr = false;
    }
}
