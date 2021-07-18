/*
 * bios.cpp
 *
 * Travis Banken
 * 1/9/2021
 *
 * Bios loader for the PSX.
 */

#include <fstream>
#include <vector>

#include "imgui/imgui.h"

#include "bios.hh"
#include "util/psxlog.hh"
#include "view/imgui/dbgmod.hh"

#define BIOS_INFO(...) PSXLOG_INFO("BIOS", __VA_ARGS__)
#define BIOS_WARN(...) PSXLOG_WARN("BIOS", __VA_ARGS__)
#define BIOS_ERROR(...) PSXLOG_ERROR("BIOS", __VA_ARGS__)

namespace {

struct State {
    std::vector<u8> rom;
    std::string bios_path;
    bool bios_loaded = false;
    Psx::View::ImGuiLayer::DbgMod::HexDump hexdump;
} s;

}// end namespace

namespace Psx {
namespace Bios {

void Init()
{
    BIOS_INFO("Initializing state");
    s.rom.resize(1024 * 512, 0);
}

void Init(const std::string& path)
{
    Init();
    LoadFromFile(path);
}

void Shutdown()
{
    BIOS_INFO("Shutting down");
    s.bios_loaded = false;
}

void Reset()
{
    BIOS_INFO("Resetting state");
    if (!s.bios_loaded) {
        // reload
        LoadFromFile(s.bios_path);
    }
}

/*
 * Returns true if a BIOS is currently loaded.
 */
bool IsLoaded()
{
    return s.bios_loaded;
}

/*
 * Load BIOS from the path specified.
 */
void LoadFromFile(const std::string& path)
{
    if (path == "") {
        BIOS_WARN("Path is empty, skipping BIOS load");
        return;
    }

    BIOS_INFO("Loading BIOS from {}", path);
    s.bios_path = path;
    // open file
    std::ifstream file;
    file.open(path, std::ios::binary);
    if (!file.is_open()) {
        BIOS_ERROR("Failed to open BIOS from {}", path);
        throw std::runtime_error(PSX_FMT("Failed to open BIOS from {}", path));
    }

    // dump bytes into local rom
    file.read((char*) s.rom.data(), s.rom.size());

    file.close();
    s.bios_loaded = true;
}


/*
 * To be called on every ImGui update while the debug window is active.
 */
void OnActive(bool *active)
{
    if (!ImGui::Begin("Bios Debug", active)) {
        ImGui::End();
        return;
    }

    s.hexdump.Update(s.rom);

    ImGui::End();
}

// *** Reads ***
template<class T>
T Read(u32 addr)
{
    u32 maddr = addr & 0x0007'ffff; // addr % 512K
    T data = 0;
    if constexpr (std::is_same_v<T, u8>) {
        PSX_ASSERT(maddr < s.rom.size());
        // read8
        data = s.rom[maddr];
    } else if constexpr (std::is_same_v<T, u16>) {
        PSX_ASSERT(maddr < s.rom.size() - 2);
        // read16 as little endian
        data  = s.rom[maddr];
        data |= static_cast<u16>(s.rom[maddr + 1]) << 8;
    } else if constexpr (std::is_same_v<T, u32>) {
        PSX_ASSERT(maddr < s.rom.size() - 4);
        // read32 as little endian
        data  = s.rom[maddr];
        data |= static_cast<u32>(s.rom[maddr + 1]) << 8;
        data |= static_cast<u32>(s.rom[maddr + 2]) << 16;
        data |= static_cast<u32>(s.rom[maddr + 3]) << 24;
    } else {
        static_assert(!std::is_same_v<T, T>);
    }
    return data;
}
// template impl needs to be visable to other cpp files to avoid compile err
template u8 Read<u8>(u32 addr);
template u16 Read<u16>(u32 addr);
template u32 Read<u32>(u32 addr);

// *** Write ***
template<class T>
void Write(T data, u32 addr)
{
    BIOS_ERROR("Trying to write [{:x}] to ROM @ 0x{:08x}", data, addr);
    PSX_ASSERT(0);
    // TODO Check for cache enable
    u32 maddr = addr & 0x0007'ffff; // addr % 512K
    if constexpr (std::is_same_v<T, u8>) {
        PSX_ASSERT(maddr < s.rom.size());
        // write8
        s.rom[maddr] = data;
    } else if constexpr (std::is_same_v<T, u16>) {
        PSX_ASSERT(maddr < s.rom.size() - 2);
        // write16 as little endian
        s.rom[maddr + 0] = static_cast<u8>(data);
        s.rom[maddr + 1] = static_cast<u8>(data >> 8);
    } else if constexpr (std::is_same_v<T, u32>) {
        PSX_ASSERT(maddr < s.rom.size() - 4);
        // write32 as little endian
        s.rom[maddr + 0] = static_cast<u8>(data);
        s.rom[maddr + 1] = static_cast<u8>(data >> 8);
        s.rom[maddr + 2] = static_cast<u8>(data >> 16);
        s.rom[maddr + 3] = static_cast<u8>(data >> 24);
    } else {
        static_assert(!std::is_same_v<T, T>);
    }
}
// template impl needs to be visable to other cpp files to avoid compile err
template void Write<u8>(u8 data, u32 addr);
template void Write<u16>(u16 data, u32 addr);
template void Write<u32>(u32 data, u32 addr);


}// end namespace
}
