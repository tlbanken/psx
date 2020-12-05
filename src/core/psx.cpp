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

#include "core/psx.h"
#include "util/psxutil.h"
#include "util/psxlog.h"

#define PSX_INFO(msg) psxlog::ilog("PSX", msg)
#define PSX_WARN(msg) psxlog::wlog("PSX", msg)
#define PSX_ERROR(msg) psxlog::elog("PSX", msg)

Psx::Psx()
{
    // TODO: Init all hardware
    PSX_INFO("Building PSX Object");
}

void Psx::step()
{
    return;
}
