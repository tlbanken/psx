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
    // this is important to avoid -Wweak-vtables from clang
    virtual ~AddressSpace();

    // reads
    virtual ASResult Read8(u32 addr) = 0;
    virtual ASResult Read16(u32 addr) = 0;
    virtual ASResult Read32(u32 addr) = 0;

    // writes
    virtual bool Write8(u8 data, u32 addr) = 0;
    virtual bool Write16(u16 data, u32 addr) = 0;
    virtual bool Write32(u32 data, u32 addr) = 0;
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
    std::vector<std::unique_ptr<ASEntry>> m_address_spaces;

public:
    void AddAddressSpace(std::shared_ptr<AddressSpace> as, BusPriority p);
    std::string ToString();
    
    // reads
    u8 Read8(u32 addr);
    u16 Read16(u32 addr);
    u32 Read32(u32 addr);

    // writes
    void Write8(u8 data, u32 addr);
    void Write16(u16 data, u32 addr);
    void Write32(u32 data, u32 addr);
};

