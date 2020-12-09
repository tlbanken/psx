/*
 * psx.cpp
 * 
 * Travis Banken
 * 12/5/2020
 * 
 * Main psx class. Inits all hardware and provides simple functionality to control
 * all hardward at once.
 */

#include <cassert>
#include <iostream>

#include "core/psx.h"
#include "util/psxutil.h"
#include "util/psxlog.h"
#include "mem/bus.h"
#include "mem/ram.h"

#define PSX_INFO(msg) PSXLOG_INFO("PSX", msg)
#define PSX_WARN(msg) PSXLOG_WARN("PSX", msg)
#define PSX_ERROR(msg) PSXLOG_ERROR("PSX", msg)

Psx::Psx()
{
    PSX_INFO("Initializing the PSX");

    PSX_INFO("Creating Bus");
    m_bus = std::shared_ptr<Bus>(new Bus());

    PSX_INFO("Creating Ram");
    std::shared_ptr<Ram> ram(new Ram());

    m_bus->addAddressSpace(ram, BusPriority::First);
}

void Psx::step()
{
    return;
}
