/*
 * ram.cpp
 * 
 * Travis Banken
 * 12/6/2020
 * 
 * Main Ram for the PSX.
 */

#include "mem/ram.h"

// reads
ASResult Ram::read8(u32 addr)
{
    return {0, false};
}

ASResult Ram::read16(u32 addr)
{
    return {0, false};
}

ASResult Ram::read32(u32 addr)
{
    return {0, false};
}


// writes
bool Ram::write8(u8 data, u32 addr)
{
    return false;
}

bool Ram::write16(u16 data, u32 addr)
{
    return false;
}

bool Ram::write32(u32 data, u32 addr)
{
    return false;
}
