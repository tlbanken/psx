/*
 * memctrl.cpp
 *
 * Travis Banken
 * 1/10/21
 *
 * Memory Control
 * Contains Memory Control Registers for things like Cache Control.
 */

#include "memctrl.h"

// *** AddressSpace ***
// Reset
void Reset()
{

}

// reads
ASResult Read8(u32 addr)
{
    return {};
}

ASResult Read16(u32 addr)
{
    return {};
}

ASResult Read32(u32 addr)
{
    return {};
}

// writes
bool Write8(u8 data, u32 addr)
{
    return false;
}

bool Write16(u16 data, u32 addr)
{
    return false;
}

bool Write32(u32 data, u32 addr)
{
    return false;
}
