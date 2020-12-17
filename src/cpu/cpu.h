/*
 * cpu.h
 *
 * Travis Banken
 * 12/13/2020
 *
 * Header for the CPU for the PSX. The Playstation uses a R3000A MIPS processor.
 */

#pragma once

#include "util/psxutil.h"
#include "mem/bus.h"
#include "cpu/asm/asm.h"
#include "layer/imgui_layer.h"


class Cpu : public ImGuiLayer::DbgModule {
public:
    Cpu(std::shared_ptr<Bus> bus);

    void step();
    void setPC(u32 addr);

    // DbgModule Functions
    void onActive(bool *active);
    std::string getModuleLabel();
private:
    typedef void (*opfunc)(const Asm::Instruction& instr);

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

    // ops
#include "cpu/_cpu_ops.h"
};

