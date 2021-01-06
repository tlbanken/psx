/*
 * cpu.h
 *
 * Travis Banken
 * 12/13/2020
 *
 * Header for the CPU for the PSX. The Playstation uses a R3000A MIPS processor.
 * Additionally, the CPU also contains a System Control Coprocessor (cop0) which
 * handles interrupts/exceptions.
 */

#pragma once

#include <map>

#include "util/psxutil.h"
#include "mem/bus.h"
#include "cpu/asm/asm.h"
#include "layer/imgui_layer.h"

class Cpu final : public ImGuiLayer::DbgModule {
public:
    struct Exception {
        enum class Type {
            Interrupt     = 0x00,
            AddrErrLoad   = 0x04,
            AddrErrStore  = 0x05,
            IBusErr       = 0x06,
            DBusErr       = 0x07,
            Syscall       = 0x08,
            Break         = 0x09,
            ReservedInstr = 0x0a,
            CopUnusable   = 0x0b,
            Overflow      = 0x0c,
        } type;
        u32 badv = 0;
        bool on_branch = false; // set if exception occurs on branch instr
        u8 cop_num = 0;
    };

    Cpu(std::shared_ptr<Bus> bus);

    // functions
    void Step();
    void SetPC(u32 addr);
    u32 GetPC();
    u32 GetR(size_t r);
    void SetR(size_t r, u32 val);
    u32 GetHI();
    void SetHI(u32 val);
    u32 GetLO();
    void SetLO(u32 val);
    u8 ExecuteInstruction(u32 raw_instr);
    void Reset();

    // DbgModule Functions
    void OnActive(bool *active);
    std::string GetModuleLabel();
private:
    using opfunc = u8 (Cpu::*)(const Asm::Instruction&);
    opfunc m_prim_opmap[0x40] = {0};
    opfunc m_sec_opmap[0x40] = {0};
    std::map<u8, opfunc> m_bcondz_opmap;

    std::shared_ptr<Bus> m_bus;
    u32 m_next_instr = 0;
    u32 m_last_pc = 0;

    // registers
    struct Registers {
        // special
        u32 pc = 0xbfc0'0000; // beginning of BIOS
        u32 hi = 0;
        u32 lo = 0;
        // general purpose
        u32 r[32] = {0};
    }m_regs;
    // load delay slot
    struct LoadDelaySlot {
        u8  reg = 0;
        u32 val = 0;
        bool is_primed = false;  // current instruction is a load
        bool was_primed = false; // prev instruction was a load
    } m_lds;

    // helpers
    void buildOpMaps();

    // ops
#include "cpu/_cpu_ops.h"

    //-----------------------
    // COP0
    class Cop0 {
    public:
        Cop0(Cpu& cpu) : m_cpu(cpu){}
        void RaiseException(const Exception& ex);
        void Reset();
        std::string FmtLastException();
    private:
        // Registers
        struct CauseReg {
            Exception::Type ex_type;
            u8 int_pending = 0;
            u8 cop_num = 0;
            bool branch_delay = false;
        } m_cause; // r13
        u32 m_epc = 0;
        u32 m_bad_vaddr = 0;
        Cpu& m_cpu;
    } m_cop0;
    //-----------------------
};

