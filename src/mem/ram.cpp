/*
 * ram.cpp
 * 
 * Travis Banken
 * 12/6/2020
 * 
 * Main Ram for the PSX.
 */
#include "mem/ram.h"

#include <vector>
#include <memory>

#include "imgui/imgui.h"

#include "layer/dbgmod.h"
#include "cpu/cop0.h"

#define RAM_INFO(...) PSXLOG_INFO("RAM", __VA_ARGS__)
#define RAM_WARN(...) PSXLOG_WARN("RAM", __VA_ARGS__)
#define RAM_ERROR(...) PSXLOG_ERROR("RAM", __VA_ARGS__)

// *** Private Functions and Data ***
namespace  {
struct State {
    std::vector<u8> sysram;
    Psx::ImGuiLayer::DbgMod::HexDump hexdump;
} s;
}// end namespace



namespace Psx {
namespace Ram {

void Init()
{
    RAM_INFO("Initializing 2MB of System RAM");
    s.sysram.resize(2048 * 1024, 0); // 2MB
}

void Reset()
{
    RAM_INFO("Resetting state");
    for (auto& byte : s.sysram) {
        byte = 0x00;
    }

    s.hexdump = ImGuiLayer::DbgMod::HexDump();
}

// *** Read ***
template<class T>
T Read(u32 addr)
{
#ifdef PSX_DEBUG
    // not sure if this is needed
    bool in_cache_region = (addr & 0x8000'0000) || ((addr & 0xf000'0000) == 0);
    if (in_cache_region && Cop0::CacheIsIsolated()) {
        RAM_WARN("Reading from RAM [0x{:08x}] while Cache is isolated. Did something break?", addr);
    }
#endif

    u32 maddr = addr & 0x1f'ffff; // addr % 2MB
    T data = 0;
    if constexpr (std::is_same_v<T, u8>) {
        PSX_ASSERT(maddr < s.sysram.size());
        // read8
        data = s.sysram[maddr];
    } else if constexpr (std::is_same_v<T, u16>) {
        PSX_ASSERT(maddr < s.sysram.size() - 2);
        // read16 as little endian
        data  = s.sysram[maddr];
        data |= static_cast<u16>(s.sysram[maddr + 1]) << 8;
    } else if constexpr (std::is_same_v<T, u32>) {
        PSX_ASSERT(maddr < s.sysram.size() - 4);
        // read32 as little endian
        data  = s.sysram[maddr];
        data |= static_cast<u32>(s.sysram[maddr + 1]) << 8;
        data |= static_cast<u32>(s.sysram[maddr + 2]) << 16;
        data |= static_cast<u32>(s.sysram[maddr + 3]) << 24;
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
    // don't do any writes if cache is isolated (kuseg or kseg0)
    bool in_cache_region = (addr & 0x8000'0000) || ((addr & 0xf000'0000) == 0);
    if (in_cache_region && Cop0::CacheIsIsolated()) {
        RAM_WARN("Cache is Isolated, skipping write [{}] to 0x{:08x}", data, addr);
        return;
    }

    u32 maddr = addr & 0x1f'ffff; // addr % 2MB
    if constexpr (std::is_same_v<T, u8>) {
        PSX_ASSERT(maddr < s.sysram.size());
        // write8
        s.sysram[maddr] = data;
    } else if constexpr (std::is_same_v<T, u16>) {
        PSX_ASSERT(maddr < s.sysram.size() - 2);
        // write16 as little endian
        s.sysram[maddr + 0] = static_cast<u8>(data);
        s.sysram[maddr + 1] = static_cast<u8>(data >> 8);
    } else if constexpr (std::is_same_v<T, u32>) {
        PSX_ASSERT(maddr < s.sysram.size() - 4);
        // write32 as little endian
        s.sysram[maddr + 0] = static_cast<u8>(data);
        s.sysram[maddr + 1] = static_cast<u8>(data >> 8);
        s.sysram[maddr + 2] = static_cast<u8>(data >> 16);
        s.sysram[maddr + 3] = static_cast<u8>(data >> 24);
    } else {
        static_assert(!std::is_same_v<T, T>);
    }
}
// template impl needs to be visable to other cpp files to avoid compile err
template void Write<u8>(u8 data, u32 addr);
template void Write<u16>(u16 data, u32 addr);
template void Write<u32>(u32 data, u32 addr);

/*
 * To be called on every ImGui update while the debug module is active.
 */
void OnActive(bool *active)
{
    if (!ImGui::Begin("Ram Debug", active)) {
        ImGui::End();
        return;
    }

    s.hexdump.Update(s.sysram);

    ImGui::End();
}


}// end namespace
}
