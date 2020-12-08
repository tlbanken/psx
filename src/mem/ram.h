/*
 * ram.h
 * 
 * Travis Banken
 * 12/6/2020
 * 
 * Header for the Ram class in th PSX project.
 */

#pragma once

#include <vector>
#include <memory>

#include "mem/bus.h"

class Ram final : public AddressSpace {
private:
    std::unique_ptr<std::vector<u8>> m_sysRam;

public:
    Ram();
    // ~Ram() {};

    // reads
    ASResult read8(u32 addr);
    ASResult read16(u32 addr);
    ASResult read32(u32 addr);

    // writes
    bool write8(u8 data, u32 addr);
    bool write16(u16 data, u32 addr);
    bool write32(u32 data, u32 addr);
};
