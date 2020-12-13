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
#include "layer/imgui_layer.h"

#define PSX_INFO(msg) PSXLOG_INFO("PSX", msg)
#define PSX_WARN(msg) PSXLOG_WARN("PSX", msg)
#define PSX_ERROR(msg) PSXLOG_ERROR("PSX", msg)

Psx::Psx()
    : m_imguiLayer(ImGuiLayer::Style::Dark)
{
    PSX_INFO("Initializing the PSX");

    PSX_INFO("Creating Bus");
    m_bus = std::shared_ptr<Bus>(new Bus());

    PSX_INFO("Creating Ram");
    std::shared_ptr<Ram> ram(new Ram());

    // add to bus and imgui
    m_bus->addAddressSpace(ram, BusPriority::First);
    m_imguiLayer.addDbgModule(ram);
}

/*
 * Main emulation loop. Only returns on Quit or Exception.
 */
void Psx::run()
{
    u32 ramSize = 2 * 1024 * 1024;
    while (!m_imguiLayer.shouldStop()) {
        // TEST some random writes
        u32 addr = (u32) rand() % ramSize;
        u32 addr1 = (u32) rand() % (100 << 4);
        u8 data = (u8) rand() % 256;
        m_bus->write8(data, addr);
        m_bus->write8(data, addr1);

        m_imguiLayer.onUpdate();
    }
}
