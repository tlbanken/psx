/*
 * bus.cpp
 * 
 * Travis Banken
 * 12/5/2020
 * 
 * The Bus class provides an abstraction layer over reading and writing
 * to the entire psx address space. This allows other classes to read and
 * write to the bus without caring about which HW device is the target.
 */

#include <algorithm>

#include "fmt/core.h"

#include "mem/bus.h"
#include "util/psxlog.h"

#define BUS_INFO(msg) psxlog::ilog("Bus", msg)
#define BUS_WARN(msg) psxlog::wlog("Bus", msg)
#define BUS_ERROR(msg) psxlog::elog("Bus", msg)


// custom compare for std::sort
static bool priorityLT(const std::unique_ptr<ASEntry>& e1, const std::unique_ptr<ASEntry>& e2)
{
    BusPriority p1 = e1.get()->p;
    BusPriority p2 = e2.get()->p;

    return p1 < p2;
}

void Bus::addAddressSpace(std::shared_ptr<AddressSpace> as, BusPriority p)
{
    BUS_INFO(fmt::format("Adding AddressSpace obj @{:p} to Bus with priority {}", static_cast<void*>(as.get()), p));
    // create new entry
    std::unique_ptr<ASEntry> entry(new ASEntry);
    entry->as = as;
    entry->p = p;

    // add to list
    m_addressSpaces.push_back(std::move(entry));

    // sort list to ensure priorities
    std::sort(m_addressSpaces.begin(), m_addressSpaces.end(), priorityLT);
}

std::string Bus::toString()
{
    std::string s("");
    for (auto& e : m_addressSpaces) {
        s.append(fmt::format("[@{:p}, {}] -> ", static_cast<void*>(e->as.get()), e->p));
    }
    return s;
}

// reads
u8 Bus::read8(u32 addr)
{
    for (auto& entry : m_addressSpaces) {
        auto [res, found] = entry->as->read8(addr);
        if (found) {
            return res.res8;
        }
    }

    BUS_WARN(fmt::format("Read8 attempt on invalid address 0x{:08x}", addr));
    return 0;
}

u16 Bus::read16(u32 addr)
{
    for (auto& entry : m_addressSpaces) {
        auto [res, found] = entry->as->read16(addr);
        if (found) {
            return res.res16;
        }
    }

    BUS_WARN(fmt::format("Read16 attempt on invalid address 0x{:08x}", addr));
    return 0;
}

u32 Bus::read32(u32 addr)
{
    for (auto& entry : m_addressSpaces) {
        auto [res, found] = entry->as->read32(addr);
        if (found) {
            return res.res32;
        }
    }

    BUS_WARN(fmt::format("Read32 attempt on invalid address 0x{:08x}", addr));
    return 0;
}


// writes
void Bus::write8(u32 addr)
{
    (void) addr;
    return;
}

void Bus::write16(u32 addr)
{
    (void) addr;
    return;
}

void Bus::write32(u32 addr)
{
    (void) addr;
    return;
}
