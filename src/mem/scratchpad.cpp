/*
 * scratchpad.cpp
 *
 * Travis Banken
 * 1/22/21
 *
 * The PSX does not use a Data Cache. Instead is uses scatchpad which acts
 * like a "fast ram".
 */

#include "mem/scratchpad.h"

#include <vector>

#include "imgui/imgui.h"

#include "layer/dbgmod.h"

#define SP_INFO(...) PSXLOG_INFO("Scratchpad", __VA_ARGS__)
#define SP_WARN(...) PSXLOG_WARN("Scratchpad", __VA_ARGS__)
#define SP_ERROR(...) PSXLOG_ERROR("Scratchpad", __VA_ARGS__)

namespace  {

struct State {
    std::vector<u8> mem;
    Psx::ImGuiLayer::DbgMod::HexDump hexdump;
} s;

}//end ns

namespace Psx {
namespace Scratchpad {

void Init()
{
    SP_INFO("Intializing 1KB of Scratchpad");
    s.mem.resize(1024);
}

void Reset()
{
    SP_INFO("Resetting state");
    for (auto& byte : s.mem) {
        byte = 0x00;
    }
}

// *** Read ***
template<class T>
T Read(u32 addr)
{
    u32 maddr = addr & 0x3ff; // addr % 1KB
    T data = 0;
    if constexpr (std::is_same_v<T, u8>) {
        PSX_ASSERT(maddr < s.mem.size());
        // read8
        data = s.mem[maddr];
    } else if constexpr (std::is_same_v<T, u16>) {
        PSX_ASSERT(maddr < s.mem.size() - 2);
        // read16 as little endian
        data  = s.mem[maddr];
        data |= static_cast<u16>(s.mem[maddr + 1]) << 8;
    } else if constexpr (std::is_same_v<T, u32>) {
        PSX_ASSERT(maddr < s.mem.size() - 4);
        // read32 as little endian
        data  = s.mem[maddr];
        data |= static_cast<u32>(s.mem[maddr + 1]) << 8;
        data |= static_cast<u32>(s.mem[maddr + 2]) << 16;
        data |= static_cast<u32>(s.mem[maddr + 3]) << 24;
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
    u32 maddr = addr & 0x3ff; // addr % 1KB
    if constexpr (std::is_same_v<T, u8>) {
        PSX_ASSERT(maddr < s.mem.size());
        // write8
        s.mem[maddr] = data;
    } else if constexpr (std::is_same_v<T, u16>) {
        PSX_ASSERT(maddr < s.mem.size() - 2);
        // write16 as little endian
        s.mem[maddr + 0] = static_cast<u8>(data);
        s.mem[maddr + 1] = static_cast<u8>(data >> 8);
    } else if constexpr (std::is_same_v<T, u32>) {
        PSX_ASSERT(maddr < s.mem.size() - 4);
        // write32 as little endian
        s.mem[maddr + 0] = static_cast<u8>(data);
        s.mem[maddr + 1] = static_cast<u8>(data >> 8);
        s.mem[maddr + 2] = static_cast<u8>(data >> 16);
        s.mem[maddr + 3] = static_cast<u8>(data >> 24);
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
    if (!ImGui::Begin("Scratchpad Debug", active)) {
        ImGui::End();
        return;
    }

    s.hexdump.Update(s.mem);

    ImGui::End();
}

}// end namespace
}
