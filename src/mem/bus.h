/*
 * bus.h
 * 
 * Travis Banken
 * 12/5/2020
 * 
 * Header file for the Bus class for the PSX project.
 */

#pragma once

#include <memory>
#include <vector>

#include "util/psxutil.h"

// AddressSpace result
struct ASResult {
    union {
        u8 res8;
        u16 res16;
        u32 res32;
    } res;
    bool found;
};

class AddressSpace {
public:
    // reads
    virtual ASResult read8(u32 addr) = 0;
    virtual ASResult read16(u32 addr) = 0;
    virtual ASResult read32(u32 addr) = 0;

    // writes
    virtual bool write8(u8 data, u32 addr) = 0;
    virtual bool write16(u16 data, u32 addr) = 0;
    virtual bool write32(u32 data, u32 addr) = 0;
};


enum class BusPriority {
    First = 0,
    High  = 1,
    Med   = 2,
    Low   = 3,
    Last  = 4,
};

struct ASEntry {
    std::shared_ptr<AddressSpace> as;
    BusPriority p;
};

class Bus {
private:
    std::vector<std::unique_ptr<ASEntry>> m_addressSpaces;

public:
    void addAddressSpace(std::shared_ptr<AddressSpace> as, BusPriority p);
    std::string toString();
    
    // reads
    u8 read8(u32 addr);
    u16 read16(u32 addr);
    u32 read32(u32 addr);

    // writes
    void write8(u32 addr);
    void write16(u32 addr);
    void write32(u32 addr);
};

