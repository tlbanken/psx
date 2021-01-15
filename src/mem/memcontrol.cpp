/*
 * memcontrol.cpp
 *
 * Travis Banken
 * 1/13/2021
 *
 * Memory Control Registers.
 */

#include "memcontrol.h"

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
    u32 ctrl1_i = (addr >> 2) % MCTRL_SIZE;
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
    u32 ctrl1_i = (addr >> 2) % MCTRL_SIZE;
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


}// end namespace
}
