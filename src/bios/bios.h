/*
 * bios.h
 *
 * Travis Banken
 * 1/9/21
 *
 * Header for the bios loader on the psx.
 */

#pragma once

#include <string>
#include <mem/bus.h>
#include <util/psxutil.h>

class Bios final : public AddressSpace {
public:
    Bios(const std::string& path);

    void LoadFromFile(const std::string& path);

    // AddressSpace
    ASResult Read8(u32 addr);
    ASResult Read16(u32 addr);
    ASResult Read32(u32 addr);
    bool Write8(u8 data, u32 addr);
    bool Write16(u16 data, u32 addr);
    bool Write32(u32 data, u32 addr);
    void Reset();

private:
    std::vector<u8> m_rom;
    std::string m_bios_path;
};

