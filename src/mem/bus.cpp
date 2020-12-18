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

#define BUS_INFO(msg) PSXLOG_INFO("Bus", msg)
#define BUS_WARN(msg) PSXLOG_WARN("Bus", msg)
#define BUS_ERROR(msg) PSXLOG_ERROR("Bus", msg)


// this is important to avoid -Wweak-vtables from clang
AddressSpace::~AddressSpace() = default;

// custom compare for std::sort
static bool priorityLT(const std::unique_ptr<ASEntry>& e1, const std::unique_ptr<ASEntry>& e2)
{
    BusPriority p1 = e1.get()->p;
    BusPriority p2 = e2.get()->p;

    return p1 < p2;
}

void Bus::AddAddressSpace(std::shared_ptr<AddressSpace> as, BusPriority p)
{
    BUS_INFO(PSX_FMT("Adding AddressSpace obj @{:p} to Bus with priority {}", static_cast<void*>(as.get()), p));
    // create new entry
    std::unique_ptr<ASEntry> entry(new ASEntry);
    entry->as = as;
    entry->p = p;

    // add to list
    m_address_spaces.push_back(std::move(entry));

    // sort list to ensure priorities
    std::sort(m_address_spaces.begin(), m_address_spaces.end(), priorityLT);
}

std::string Bus::ToString()
{
    std::string s("");
    for (const auto& e : m_address_spaces) {
        s.append(PSX_FMT("[@{:p}, {}] -> ", static_cast<void*>(e->as.get()), e->p));
    }
    return s;
}

// reads
u8 Bus::Read8(u32 addr)
{
    for (const auto& entry : m_address_spaces) {
        auto [res, found] = entry->as->Read8(addr);
        if (found) {
            return res.res8;
        }
    }

    BUS_WARN(PSX_FMT("Read8 attempt on invalid address 0x{:08x}", addr));
    return 0;
}

u16 Bus::Read16(u32 addr)
{
    for (const auto& entry : m_address_spaces) {
        auto [res, found] = entry->as->Read16(addr);
        if (found) {
            return res.res16;
        }
    }

    BUS_WARN(PSX_FMT("Read16 attempt on invalid address 0x{:08x}", addr));
    return 0;
}

u32 Bus::Read32(u32 addr)
{
    for (const auto& entry : m_address_spaces) {
        auto [res, found] = entry->as->Read32(addr);
        if (found) {
            return res.res32;
        }
    }

    BUS_WARN(PSX_FMT("Read32 attempt on invalid address 0x{:08x}", addr));
    return 0;
}


// writes
void Bus::Write8(u8 data, u32 addr)
{
    for (const auto& entry : m_address_spaces) {
        if (entry->as->Write8(data, addr)) {
            return;
        }
    }
}

void Bus::Write16(u16 data, u32 addr)
{
    for (const auto& entry : m_address_spaces) {
        if (entry->as->Write16(data, addr)) {
            return;
        }
    }
}

void Bus::Write32(u32 data, u32 addr)
{
    for (const auto& entry : m_address_spaces) {
        if (entry->as->Write32(data, addr)) {
            return;
        }
    }
}
