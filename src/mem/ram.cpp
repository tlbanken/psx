/*
 * ram.cpp
 * 
 * Travis Banken
 * 12/6/2020
 * 
 * Main Ram for the PSX.
 */

#include <vector>
#include <memory>

#include "mem/ram.h"

#define RAM_INFO(msg) PSXLOG_INFO("RAM", msg)
#define RAM_WARN(msg) PSXLOG_WARN("RAM", msg)
#define RAM_ERROR(msg) PSXLOG_ERROR("RAM", msg)

static std::pair<u32, bool> mirrorAddr(u32 addr)
{
    // TODO: Maybe add support for exceptions when access beyond 512MB region?
    // Hopefully no bugs in ps1 games :/

    // KUSEG
    // first 64KB is kernel, rest is user mem
    if (addr < 0x0020'0000) {
        return {addr, true};
    }

    // KSEG0
    if (addr >= 0x8000'0000 && addr < 0x8020'0000) {
        return {addr - 0x8000'0000, true};
    }

    // KSEG1
    if (addr >= 0xa000'0000 && addr < 0xa020'0000) {
        return {addr - 0xa000'0000, true};
    }

    return {0, false};
}

Ram::Ram()
{
    RAM_INFO("Initializing 2MB of System RAM");
    m_sysRam = std::unique_ptr<std::vector<u8>>(new std::vector<u8>(2 * 1024 * 1024, 0));
}

// reads
ASResult Ram::read8(u32 addr)
{
    u8 data = 0;
    auto [maddr, found] = mirrorAddr(addr);
    if (found) {
        data = (*m_sysRam)[maddr];
    }

    ASResult asres;
    asres.res.res8 = data;
    asres.found = found;
    return asres;
}

ASResult Ram::read16(u32 addr)
{
    u16 data = 0;
    auto [maddr, found] = mirrorAddr(addr);
    if (found) {
        // create 16-bit halfword in little-endian format
        data = (*m_sysRam)[maddr];
        data |= static_cast<u16>((*m_sysRam)[maddr+1]) << 8;
    }

    ASResult asres;
    asres.res.res16 = data;
    asres.found = found;
    return asres;
}

ASResult Ram::read32(u32 addr)
{
    u32 data = 0;
    auto [maddr, found] = mirrorAddr(addr);
    if (found) {
        // create 32-bit word in little-endian format
        data = (*m_sysRam)[maddr];
        data |= static_cast<u32>((*m_sysRam)[maddr+1]) << 8;
        data |= static_cast<u32>((*m_sysRam)[maddr+2]) << 16;
        data |= static_cast<u32>((*m_sysRam)[maddr+3]) << 24;
    }

    ASResult asres;
    asres.res.res32 = data;
    asres.found = found;
    return asres;
}


// writes
bool Ram::write8(u8 data, u32 addr)
{
    auto [maddr, found] = mirrorAddr(addr);
    if (found) {
        (*m_sysRam)[maddr] = data;
    }

    return found;
}

bool Ram::write16(u16 data, u32 addr)
{
    auto [maddr, found] = mirrorAddr(addr);
    if (found) {
        // write 16-bit halfword in little-endian format
        (*m_sysRam)[maddr] = static_cast<u8>(data & 0xff);
        (*m_sysRam)[maddr + 1] = static_cast<u8>((data >> 8) & 0xff);
    }

    return found;
}

bool Ram::write32(u32 data, u32 addr)
{
    auto [maddr, found] = mirrorAddr(addr);
    if (found) {
        // write 32-bit word in little-endian format
        (*m_sysRam)[maddr] = static_cast<u8>(data & 0xff);
        (*m_sysRam)[maddr + 1] = static_cast<u8>((data >> 8) & 0xff);
        (*m_sysRam)[maddr + 2] = static_cast<u8>((data >> 16) & 0xff);
        (*m_sysRam)[maddr + 3] = static_cast<u8>((data >> 24) & 0xff);
    }

    return found;
}
