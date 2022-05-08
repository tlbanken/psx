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

#include "mem/bus.hh"
#include "mem/ram.hh"
#include "bios/bios.hh"
#include "util/psxlog.hh"
#include "mem/memcontrol.hh"
#include "mem/scratchpad.hh"
#include "mem/dma.hh"
#include "view/imgui/dbgmod.hh"
#include "view/imgui/imgui_layer.hh"
#include "gpu/gpu.hh"
#include "io/timer.hh"

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
#ifdef PSX_DEBUG
    using namespace View::ImGuiLayer::DbgMod;
    Breakpoints::Saw<Breakpoints::BrkType::ReadWatch>(addr);
#endif

    // Main RAM
    constexpr u32 ram_size = 8 * 1024 * 1024; // 8 MB
    if (inRange(0x0000'0000, ram_size, addr) // KUSEG
     || inRange(0x8000'0000, ram_size, addr) // KSEG0
     || inRange(0xa000'0000, ram_size, addr))// KSEG1
    {
        return Ram::Read<T>(addr);
    }

    // BIOS
    constexpr u32 bios_size = 512 * 1024;
    if (inRange(0x1fc0'0000, bios_size, addr) // KUSEG
     || inRange(0x9fc0'0000, bios_size, addr) // KSEG0
     || inRange(0xbfc0'0000, bios_size, addr))// KSEG1
    {
        return Bios::Read<T>(addr);
    }

    // GPU
    if (addr == 0x1f80'1810 || addr == 0x1f80'1814) {
        return Gpu::Read<T>(addr);
    }

    // Scratchpad
    constexpr u32 sp_size = 1 * 1024;
    if (inRange(0x1f80'0000, sp_size, addr) // KUSEG
     || inRange(0x9f80'0000, sp_size, addr))// KSEG0
    {
        return Scratchpad::Read<T>(addr);
    }

    // DMA
    constexpr u32 dma_size = (0x1f80'10fc - 0x1f80'1080);
    if (inRange(0x1f80'1080, dma_size, addr)) {
        return Dma::Read<T>(addr);
    }

    // Timers
    constexpr u32 timer_size = (0x1f80'112c - 0x1f80'1100);
    if (inRange(0x1f80'1100, timer_size, addr)) {
        return Timer::Read<T>(addr);
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
#ifdef PSX_DEBUG
    using namespace View::ImGuiLayer::DbgMod;
    Breakpoints::Saw<Breakpoints::BrkType::WriteWatch>(addr);
#endif

    // Main RAM
    constexpr u32 ram_size = 8 * 1024 * 1024; // 8 MB
    if (inRange(0x0000'0000, ram_size, addr) // KUSEG
     || inRange(0x8000'0000, ram_size, addr) // KSEG0
     || inRange(0xa000'0000, ram_size, addr))// KSEG1
    {
        Ram::Write<T>(data, addr);
        return;
    }

    // BIOS
    constexpr u32 bios_size = 512 * 1024;
    if (inRange(0x1fc0'0000, bios_size, addr) // KUSEG
     || inRange(0x9fc0'0000, bios_size, addr) // KSEG0
     || inRange(0xbfc0'0000, bios_size, addr))// KSEG1
    {
        Bios::Write<T>(data, addr);
        return;
    }

    // GPU
    if (addr == 0x1f80'1810 || addr == 0x1f80'1814) {
        Gpu::Write<T>(data, addr);
        return;
    }

    // Scratchpad
    constexpr u32 sp_size = 1 * 1024;
    if (inRange(0x1f80'0000, sp_size, addr) // KUSEG
     || inRange(0x9f80'0000, sp_size, addr))// KSEG0
    {
        Scratchpad::Write<T>(data, addr);
        return;
    }

    // DMA
    constexpr u32 dma_size = (0x1f80'10fc - 0x1f80'1080);
    if (inRange(0x1f80'1080, dma_size, addr)) {
        Dma::Write<T>(data, addr);
        return;
    }

    // Timers
    constexpr u32 timer_size = (0x1f80'112c - 0x1f80'1100);
    if (inRange(0x1f80'1100, timer_size, addr)) {
        return Timer::Write<T>(data, addr);
    }

    // Memory Control Register
    if (inRangeMemControl(addr)) {
        MemControl::Write<T>(data, addr);
        return;
    }

#ifdef PSX_DEBUG
    // POST (not really important, but useful for debug)
    if (addr == 0x1f80'2041) {
        BUS_INFO("POST = 0x{:02x}", data);
        return;
    }
#endif

    if constexpr (std::is_same_v<T, u8>) {
        if (verb != RWVerbosity::Quiet)
            BUS_WARN("Write8 attempt [0x{:02x}] on invalid address [0x{:08x}]", data, addr);
    } else if constexpr (std::is_same_v<T, u16>) {
        if (verb != RWVerbosity::Quiet)
            BUS_WARN("Write16 attempt [0x{:04x}] on invalid address [0x{:08x}]", data, addr);
    } else if constexpr (std::is_same_v<T, u32>) {
        if (verb != RWVerbosity::Quiet)
            BUS_WARN("Write32 attempt [0x{:08x}] on invalid address [0x{:08x}]", data, addr);
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


