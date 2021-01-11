/*
 * bios.cpp
 *
 * Travis Banken
 * 1/9/21
 *
 * Bios loader for the PSX.
 */

#include <fstream>

#include "bios.h"
#include "util/psxlog.h"

#define BIOS_INFO(msg) PSXLOG_INFO("BIOS", msg)
#define BIOS_WARN(msg) PSXLOG_WARN("BIOS", msg)
#define BIOS_ERROR(msg) PSXLOG_ERROR("BIOS", msg)

Bios::Bios(const std::string& path)
{
    BIOS_INFO("Initializing BIOS");
    m_rom.resize(1024 * 512, 0);
    LoadFromFile(path);
}

/*
 * Load BIOS from the path specified.
 */
void Bios::LoadFromFile(const std::string& path)
{
    BIOS_INFO(PSX_FMT("Loading BIOS from {}", path));
    m_bios_path = path;
    // open file
    std::ifstream file;
    file.open(path, std::ios::binary);
    if (!file.is_open()) {
        BIOS_ERROR(PSX_FMT("Failed to open BIOS from {}", path));
        throw std::runtime_error(PSX_FMT("Failed to open BIOS from {}", path));
    }

    // dump bytes into local rom
    for (auto& byte : m_rom) {
        file >> byte;
    }

    // DEBUG
//    std::ofstream ofile;
//    ofile.open("bios_debug", std::ios::binary);
//    for (const auto& byte : m_rom) {
//        ofile << byte;
//    }
    file.close();
}

// *** AddressSpace ***
void Bios::Reset()
{
    BIOS_INFO("Resetting BIOS");
}

static std::pair<bool, u32> mapAddr(u32 addr)
{
    auto plus512K = [] (u32 a) {return a + (1024 * 512);};
    auto inRange = [&] (u32 base, u32 a) {return a >= base && a < plus512K(base);};

    // KSEG1
    if (inRange(0xbfc0'0000, addr)) {
        return {true, addr & 0x3ff};
    }
    // KSEG0
    if (inRange(0x9fc0'0000, addr)) {
        return {true, addr & 0x3ff};
    }
    // KUSEG
    if (inRange(0x1fc0'0000, addr)) {
        return {true, addr & 0x3ff};
    }

    return {false, 0};
}

ASResult Bios::Read8(u32 addr)
{
    auto [found, maddr] = mapAddr(addr);
    u8 data = 0;
    if (found) {
        PSX_ASSERT(maddr < m_rom.size());
        data = m_rom[maddr];
    }
    ASResult asr;
    asr.found = found;
    asr.res.res8 = data;
    return asr;
}

ASResult Bios::Read16(u32 addr)
{
    auto [found, maddr] = mapAddr(addr);
    u16 data = 0;
    if (found) {
        PSX_ASSERT(maddr < m_rom.size() - 2);
        data = m_rom[maddr];
        data |= static_cast<u16>(m_rom[maddr+1]) << 8;
    }
    ASResult asr;
    asr.found = found;
    asr.res.res16 = data;
    return asr;
}

ASResult Bios::Read32(u32 addr)
{
    auto [found, maddr] = mapAddr(addr);
    u32 data = 0;
    if (found) {
        PSX_ASSERT(maddr < m_rom.size() - 4);
        data = m_rom[maddr];
        data |= static_cast<u32>(m_rom[maddr+1]) << 8;
        data |= static_cast<u32>(m_rom[maddr+2]) << 16;
        data |= static_cast<u32>(m_rom[maddr+3]) << 24;
    }
    ASResult asr;
    asr.found = found;
    asr.res.res32 = data;
    return asr;
}

bool Bios::Write8(u8 data, u32 addr)
{
    // ROM is read-only
    BIOS_WARN(PSX_FMT("BIOS is Read Only! Attempting to Write [{}] to BIOS @ 0x{:08x}", data, addr));
    (void) data;
    (void) addr;
    return false;
}

bool Bios::Write16(u16 data, u32 addr)
{
    // ROM is read-only
    BIOS_WARN(PSX_FMT("BIOS is Read Only! Attempting to Write [{}] to BIOS @ 0x{:08x}", data, addr));
    (void) data;
    (void) addr;
    return false;
}

bool Bios::Write32(u32 data, u32 addr)
{
    // ROM is read-only
    BIOS_WARN(PSX_FMT("BIOS is Read Only! Attempting to Write [{}] to BIOS @ 0x{:08x}", data, addr));
    (void) data;
    (void) addr;
    return false;
}
