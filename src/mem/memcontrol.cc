/*
 * memcontrol.cpp
 *
 * Travis Banken
 * 1/13/2021
 *
 * Memory Control Registers.
 */

#include "memcontrol.hh"

#include "imgui/imgui.h"

#define MCTRL_INFO(...) PSXLOG_INFO("MemCtrl", __VA_ARGS__)
#define MCTRL_WARN(...) PSXLOG_WARN("MemCtrl", __VA_ARGS__)
#define MCTRL_ERROR(...) PSXLOG_ERROR("MemCtrl", __VA_ARGS__)

#define MCTRL_SIZE 9

namespace  {

struct State {
    struct MemControlRegs {
        u32 ctrl1[MCTRL_SIZE] = {0};
        u32 ram_size = 0;
        u32 cache_ctrl = 0;
    } regs;
}s;

// Protos
std::string formatCacheControl();

}

namespace Psx {
namespace MemControl {

void Init()
{
    MCTRL_INFO("Initializing state");
    s.regs = {};
}

void Reset()
{
    MCTRL_INFO("Resetting state");
    s.regs = {};
}

// *** Read ***
template<class T>
T Read(u32 addr)
{
    u32 ctrl1_i = ((addr - 0x1f80'1000) >> 2) % MCTRL_SIZE;
    bool is_ram_size = addr == 0x1f80'1060;
    bool is_cache_ctrl = addr == 0xfffe'0130;
    T data = 0;
    if constexpr (std::is_same_v<T, u8>) {
        // read8 (assuming little endian)
        u32 shift_amt = (addr & 0x3) << 3; // which byte do we need
        if (is_ram_size) {
            data = static_cast<u8>(s.regs.ram_size >> shift_amt);
        } else if (is_cache_ctrl) {
            data = static_cast<u8>(s.regs.cache_ctrl >> shift_amt);
        } else {
            PSX_ASSERT(ctrl1_i < MCTRL_SIZE);
            data = static_cast<u8>(s.regs.ctrl1[ctrl1_i] >> shift_amt);
        }
    } else if constexpr (std::is_same_v<T, u16>) {
        // read16 (assuming little endian)
        u32 shift_amt = (addr & 0x3) < 2 ? 0 : 16;
        if (is_ram_size) {
            data = static_cast<u16>(s.regs.ram_size >> shift_amt);
        } else if (is_cache_ctrl) {
            data = static_cast<u16>(s.regs.cache_ctrl >> shift_amt);
        } else {
            PSX_ASSERT(ctrl1_i < MCTRL_SIZE);
            data = static_cast<u16>(s.regs.ctrl1[ctrl1_i] >> shift_amt);
        }
    } else if constexpr (std::is_same_v<T, u32>) {
        // read32
        if (is_ram_size) {
            data = s.regs.ram_size;
        } else if (is_cache_ctrl) {
            data = s.regs.cache_ctrl;
        } else {
            PSX_ASSERT(ctrl1_i < MCTRL_SIZE);
            data = s.regs.ctrl1[ctrl1_i];
        }
    } else {
        static_assert(!std::is_same_v<T, T>);
    }
    PSX_ASSERT(data != 0 && PSX_FMT("Trying to read to 0x{:08x} before writing to it, does it have a default value?", addr).data());
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
    u32 ctrl1_i = ((addr - 0x1f80'1000) >> 2) % MCTRL_SIZE;
    bool is_ram_size = addr == 0x1f80'1060;
    bool is_cache_ctrl = addr == 0xfffe'0130;

    // assume aligned (otherwise need to implement the correct behavior)
    PSX_ASSERT((addr & 0x3) == 0 && "Unaligned address support not implemented");

    // don't care about data size
    if (is_ram_size) {
        s.regs.ram_size = static_cast<T>(data);
    } else if (is_cache_ctrl) {
        s.regs.cache_ctrl = static_cast<T>(data);
    } else {
        PSX_ASSERT(ctrl1_i < MCTRL_SIZE);
        s.regs.ctrl1[ctrl1_i] = static_cast<T>(data);
    }
}
// template impl needs to be visable to other cpp files to avoid compile err
template void Write<u8>(u8 data, u32 addr);
template void Write<u16>(u16 data, u32 addr);
template void Write<u32>(u32 data, u32 addr);

// ImGui Debug
void OnActive(bool *active)
{
    if (!ImGui::Begin("MemControl Regs Debug", active)) {
        ImGui::End();
        return;
    }

    //-----------------------
    // Registers Raw
    //-----------------------
    ImGui::BeginGroup();
    ImGui::TextUnformatted("Registers");
    ImGui::Separator();
    ImGui::TextUnformatted(PSX_FMT("0x1f80'1000 -- {:<25} = 0x{:08x}", "Expansion 1 Base Addr", s.regs.ctrl1[0]).c_str());
    ImGui::TextUnformatted(PSX_FMT("0x1f80'1004 -- {:<25} = 0x{:08x}", "Expansion 2 Base Addr", s.regs.ctrl1[1]).c_str());
    ImGui::TextUnformatted(PSX_FMT("0x1f80'1008 -- {:<25} = 0x{:08x}", "Expansion 1 Delay/Size", s.regs.ctrl1[2]).c_str());
    ImGui::TextUnformatted(PSX_FMT("0x1f80'100c -- {:<25} = 0x{:08x}", "Expansion 3 Delay/Size", s.regs.ctrl1[3]).c_str());
    ImGui::TextUnformatted(PSX_FMT("0x1f80'1010 -- {:<25} = 0x{:08x}", "BIOS ROM Delay/Size", s.regs.ctrl1[4]).c_str());
    ImGui::TextUnformatted(PSX_FMT("0x1f80'1014 -- {:<25} = 0x{:08x}", "SPU_DELAY Delay/Size", s.regs.ctrl1[5]).c_str());
    ImGui::TextUnformatted(PSX_FMT("0x1f80'1018 -- {:<25} = 0x{:08x}", "CDROM_DELAY Delay/Size", s.regs.ctrl1[6]).c_str());
    ImGui::TextUnformatted(PSX_FMT("0x1f80'101c -- {:<25} = 0x{:08x}", "Expansion 2 Delay/Size", s.regs.ctrl1[7]).c_str());
    ImGui::TextUnformatted(PSX_FMT("0x1f80'1020 -- {:<25} = 0x{:08x}", "COM_DELAY / COMMON_DELAY", s.regs.ctrl1[8]).c_str());
    ImGui::TextUnformatted(PSX_FMT("0x1f80'1060 -- {:<25} = 0x{:08x}", "RAM_SIZE", s.regs.ram_size).c_str());
    ImGui::TextUnformatted(PSX_FMT("0x1f80'1130 -- {:<25} = 0x{:08x}", "Cache Control", s.regs.cache_ctrl).c_str());
    ImGui::EndGroup();

    ImGui::SameLine();

    //-----------------------
    // Registers Formated
    //-----------------------
    ImGui::BeginGroup();
    ImGui::TextUnformatted("Formated Registers");
    ImGui::Separator();
    ImGui::BeginGroup(); // Cache Control
    ImGui::TextUnformatted("Cache Control");
    ImGui::Separator();
    ImGui::TextUnformatted(formatCacheControl().c_str());
    ImGui::EndGroup(); // Cache Control
    ImGui::Separator();
    // add more here
    ImGui::EndGroup();

    ImGui::End();
}


}// end namespace
}

namespace  {

std::string formatCacheControl()
{
    u8 scratch1_enabled = (s.regs.cache_ctrl >> 3) & 0x1;
    u8 scratch2_enabled = (s.regs.cache_ctrl >> 7) & 0x1;
    u8 crash = (s.regs.cache_ctrl >> 9) & 0x1;
    u8 code_cache_enable = (s.regs.cache_ctrl >> 11) & 0x1;
    return PSX_FMT("{:<25} = {}\n"
                   "{:<25} = {}\n"
                   "{:<25} = {}\n"
                   "{:<25} = {}\n"
                    , "Scratchpad 1 Enable", scratch1_enabled
                    , "Scratchpad 2 Enable", scratch2_enabled
                    , "Crash if Code-Cache On", crash
                    , "Code-Cache Enable", code_cache_enable
    );
}

}// end namespace
