/*
 * cpu.h
 *
 * Travis Banken
 * 12/13/2020
 *
 * Header for the CPU for the PSX. The Playstation uses a R3000A MIPS processor.
 */

#pragma once

#include <map>

#include "util/psxutil.h"
#include "mem/bus.h"
#include "cpu/asm/asm.h"
#include "layer/imgui_layer.h"


class Cpu final : public ImGuiLayer::DbgModule {
public:
    Cpu(std::shared_ptr<Bus> bus);

    // functions
    void Step();
    void SetPC(u32 addr);

    // DbgModule Functions
    void OnActive(bool *active);
    std::string GetModuleLabel();
private:
    typedef void (Cpu::*opfunc)(const Asm::Instruction&);
    opfunc m_prim_opmap[0x40] = {0};
    opfunc m_sec_opmap[0x40] = {0};
    std::map<u8, opfunc> m_bcondz_opmap;

    std::shared_ptr<Bus> m_bus;

    // registers
    struct Registers {
        // special
        u32 pc = 0xbfc0'0000; // beginning of BIOS
        u64 hi = 0;
        u64 lo = 0;
        // general purpose
        u32 r[32] = {0};
    }m_regs;

    // helpers
    void buildOpMaps();

    // ops
#include "cpu/_cpu_ops.h"
};

