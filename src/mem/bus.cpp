/*
 * bus.cpp
 * 
 * Travis Banken
 * 12/5/2020
 * 
 * The Bus class provides an abstraction layer over reading and writing
 * to the entire psx address space. This allows other classes to read and
 * write to the bus without caring about which HW device is the target.
 */

#include <type_traits>

#include "fmt/core.h"

#include "mem/bus.h"
#include "mem/ram.h"
#include "bios/bios.h"
#include "util/psxlog.h"
#include "mem/memcontrol.h"
#include "mem/scratchpad.h"
#include "layer/dbgmod.h"
#include "layer/imgui_layer.h"

#define BUS_INFO(...) PSXLOG_INFO("Bus", __VA_ARGS__)
#define BUS_WARN(...) PSXLOG_WARN("Bus", __VA_ARGS__)
#define BUS_ERROR(...) PSXLOG_ERROR("Bus", __VA_ARGS__)

// *** Private Helpers and Data ***
namespace {

bool inline inRangeMemControl(u32 addr);

}

namespace Psx {
namespace Bus {

// *** State Modifiers ***
void Init()
{
    BUS_INFO("Initializing state");
}

void Reset()
{

}

// *** Reads ***
template<class T>
T Read(u32 addr, Bus::RWVerbosity verb)
{
    // Helpers
    auto inRange = [] (u32 base, u32 size, u32 addr) {
        return addr >= base && addr < (base + size);
    };

    // check for breakpoint
    if (ImGuiLayer::DbgMod::Breakpoints::ShouldBreakMemR(addr)) {
        ImGuiLayer::DbgMod::Breakpoints::BreakMemR(addr);
        ImGuiLayer::OnUpdate();
    }

    // Main RAM
    constexpr u32 mb_2 = 2 * 1024 * 1024;
    if (inRange(0x0000'0000, mb_2, addr) // KUSEG
     || inRange(0x8000'0000, mb_2, addr) // KSEG0
     || inRange(0xa000'0000, mb_2, addr))// KSEG1
    {
        return Ram::Read<T>(addr);
    }

    // BIOS
    constexpr u32 kb_512 = 512 * 1024;
    if (inRange(0x1fc0'0000, kb_512, addr) // KUSEG
     || inRange(0x9fc0'0000, kb_512, addr) // KSEG0
     || inRange(0xbfc0'0000, kb_512, addr))// KSEG1
    {
        return Bios::Read<T>(addr);
    }

    // Scratchpad
    constexpr u32 kb_1 = 1 * 1024;
    if (inRange(0x1f80'0000, kb_1, addr) // KUSEG
     || inRange(0x9f80'0000, kb_1, addr))// KSEG0
    {
        return Scratchpad::Read<T>(addr);
    }

    // Memory Control Register
    if (inRangeMemControl(addr)) {
        return MemControl::Read<T>(addr);
    }

    if constexpr (std::is_same_v<T, u8>) {
        if (verb != RWVerbosity::Quiet)
            BUS_WARN("Read8 attempt on invalid address [0x{:08x}]", addr);
    } else if constexpr (std::is_same_v<T, u16>) {
        if (verb != RWVerbosity::Quiet)
            BUS_WARN("Read16 attempt on invalid address [0x{:08x}]", addr);
    } else if constexpr (std::is_same_v<T, u32>) {
        if (verb != RWVerbosity::Quiet)
            BUS_WARN("Read32 attempt on invalid address [0x{:08x}]", addr);
    } else {
        static_assert(!std::is_same_v<T, T>);
    }
    return 0;
}
// required to allow other files to "see" impl, otherwise compile error
template u8 Read<u8>(u32 addr, Bus::RWVerbosity verb);
template u16 Read<u16>(u32 addr, Bus::RWVerbosity verb);
template u32 Read<u32>(u32 addr, Bus::RWVerbosity verb);


// *** Writes ***
template<class T>
void Write(T data, u32 addr, Bus::RWVerbosity verb)
{
    // Helpers
    auto inRange = [] (u32 base, u32 size, u32 addr) {
        return addr >= base && addr < (base + size);
    };

    // check for breakpoint
    if (ImGuiLayer::DbgMod::Breakpoints::ShouldBreakMemW(addr)) {
        ImGuiLayer::DbgMod::Breakpoints::BreakMemW(addr);
        ImGuiLayer::OnUpdate();
    }

    // Main RAM
    constexpr u32 mb_2 = 2 * 1024 * 1024;
    if (inRange(0x0000'0000, mb_2, addr) // KUSEG
     || inRange(0x8000'0000, mb_2, addr) // KSEG0
     || inRange(0xa000'0000, mb_2, addr))// KSEG1
    {
        Ram::Write<T>(data, addr);
        return;
    }

    // BIOS
    constexpr u32 kb_512 = 512 * 1024;
    if (inRange(0x1fc0'0000, kb_512, addr) // KUSEG
     || inRange(0x9fc0'0000, kb_512, addr) // KSEG0
     || inRange(0xbfc0'0000, kb_512, addr))// KSEG1
    {
        Bios::Write<T>(data, addr);
        return;
    }

    // Scratchpad
    constexpr u32 kb_1 = 1 * 1024;
    if (inRange(0x1f80'0000, kb_1, addr) // KUSEG
     || inRange(0x9f80'0000, kb_1, addr))// KSEG0
    {
        Scratchpad::Write<T>(data, addr);
        return;
    }

    // Memory Control Register
    if (inRangeMemControl(addr)) {
        MemControl::Write<T>(data, addr);
        return;
    }

    if constexpr (std::is_same_v<T, u8>) {
        if (verb != RWVerbosity::Quiet)
            BUS_WARN("Write8 attempt [{}] on invalid address [0x{:08x}]", data, addr);
    } else if constexpr (std::is_same_v<T, u16>) {
        if (verb != RWVerbosity::Quiet)
            BUS_WARN("Write16 attempt [{}] on invalid address [0x{:08x}]", data, addr);
    } else if constexpr (std::is_same_v<T, u32>) {
        if (verb != RWVerbosity::Quiet)
            BUS_WARN("Write32 attempt [{}] on invalid address [0x{:08x}]", data, addr);
    } else {
        static_assert(!std::is_same_v<T, T>);
    }
}
// required to allow other files to "see" impl, otherwise compile error
template void Write<u8>(u8 data, u32 addr, Bus::RWVerbosity verb);
template void Write<u16>(u16 data, u32 addr, Bus::RWVerbosity verb);
template void Write<u32>(u32 data, u32 addr, Bus::RWVerbosity verb);

}// end namespace
}

namespace {

bool inline inRangeMemControl(u32 addr)
{
    return (addr >= 0x1f80'1000 && addr <= 0x1f80'1020) || addr == 0x1f80'1060 || addr == 0xfffe'0130;
}

}// end namespace


