/*
 * memctrl.h
 *
 * Travis Banken
 * 1/10/21
 *
 * Memory Control Header
 * Contains Memory Control Registers for things like Cache Control.
 */

#pragma once

#include "mem/bus.h"

class MemControl final : public AddressSpace {
public:
    struct CacheControl {

    };

    // AddressSpace
    // Reset
    void Reset();
    // reads
    ASResult Read8(u32 addr);
    ASResult Read16(u32 addr);
    ASResult Read32(u32 addr);
    // writes
    bool Write8(u8 data, u32 addr);
    bool Write16(u16 data, u32 addr);
    bool Write32(u32 data, u32 addr);

private:
};
