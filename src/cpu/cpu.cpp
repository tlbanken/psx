/*
 * cpu.cpp
 *
 * Travis Banken
 * 12/13/2020
 *
 * CPU for the PSX. The Playstation uses a R3000A MIPS processor.
 */

#include "imgui/imgui.h"

#include "cpu/cpu.h"
#include "cpu/cop0.h"
#include "mem/bus.h"
#include "core/globals.h"
#include "layer/dbgmod.h"

#define CPU_INFO(...) PSXLOG_INFO("CPU", __VA_ARGS__)
#define CPU_WARN(...) PSXLOG_WARN("CPU", __VA_ARGS__)
#define CPU_ERROR(...) PSXLOG_ERROR("CPU", __VA_ARGS__)

using opfunc = u8 (*)(const Psx::Cpu::Asm::Instruction&);

// *** Private Data and Helpers ***
namespace  {
// Protos
void buildOpMaps();
using namespace Psx::Cpu;

// State
struct State {
    // op maps
    opfunc prim_opmap[0x40] = {0};
    opfunc sec_opmap[0x40] = {0};
    std::map<u8, opfunc> bcondz_opmap;

    // registers
    struct Registers {
        // special
        u32 pc = 0xbfc0'0000; // beginning of BIOS
        u32 hi = 0;
        u32 lo = 0;
        // general purpose
        u32 r[32] = {0};
    } regs;
    // load delay slot
    struct LoadDelaySlot {
        u8  reg = 0;
        u32 val = 0;
        bool is_primed = false;  // current instruction is a load
        bool was_primed = false; // prev instruction was a load
    } lds;
    // branch delay slot
    struct BranchDelaySlot {
        bool is_primed = false;
        bool was_primed = false;
        bool take_branch = false;
        u32 pc; // the pc to load after executing delay
    } bds;
} s;
}// namespace

namespace Psx {
namespace Cpu {

/*
 * Initialize state
 */
void Init()
{
    CPU_INFO("Initializing CPU");
    buildOpMaps();
    Reset();
}

/*
 * Resets the state of the CPU.
 */
void Reset()
{
    CPU_INFO("Resetting state");
    // reset branch/load slots
    s.lds = {};
    s.bds = {};
    // registers
    s.regs = {};
}

/*
 * Execute one instruction. On average, takes 1 psx clock cycle.
 */
void Step()
{
    // fetch next instruction
    // TODO: Right now, we ignore instruction cache. This shouldn't be
    // a problem for most games, but maybe something to come back to in
    // the future.
    u32 cur_instr = Bus::Read<u32>(s.regs.pc);

    // check if last instruction was a branch/jump
    s.bds.was_primed = s.bds.is_primed;
    s.bds.is_primed = false;
    bool take_branch = s.bds.take_branch;
    s.bds.take_branch = false;
    u32 baddr = s.bds.pc;


    // if previous instruction was a load, the load delay will be primed.
    // we need to check this here just in case the next instruction will
    // re-prime the load delay slot.
    s.lds.was_primed = s.lds.is_primed;
    s.lds.is_primed = false;

    // execute
    s.regs.pc += 4;
    u8 modified_reg = ExecuteInstruction(cur_instr);

    // update pc (dependent on branch)
    s.regs.pc = take_branch ? baddr : s.regs.pc;

    // race condition: if instruction writes to same register in load
    // delay slot, the instruction wins over the load.
    if (s.lds.was_primed && modified_reg != s.lds.reg) {
        s.regs.r[s.lds.reg] = s.lds.val;
    }

    // zero register should always be zero
    s.regs.r[0] = 0;
}



/*
 * Set the cpu's program counter to the specified address.
 */
void SetPC(u32 addr)
{
    CPU_WARN("Forcing PC to 0x{:08x}", addr);
    s.regs.pc = addr;
    s.bds = {};
}

/*
 * Return the current value of the program counter.
 */
u32 GetPC()
{
    return s.regs.pc;
}

/*
 * Get the current value stored in Register r.
 */
u32 GetR(size_t r)
{
    PSX_ASSERT(r < 32);
    return s.regs.r[r];
}

/*
 * Set the chosen register to the chosen value.
 */
void SetR(size_t r, u32 val)
{
    CPU_WARN("Forcing R{} to {}", r, val);
    PSX_ASSERT(r < 32);
    s.regs.r[r] = val;
}

/*
 * Return the current value of the HI register
 */
u32 GetHI()
{
    return s.regs.hi;
}

/*
 * Set the HI register to the given value.
 */
void SetHI(u32 val)
{
    CPU_WARN("Forcing HI to {}", val);
    s.regs.hi = val;
}

/*
 * Return the current value of the LO register
 */
u32 GetLO()
{
    return s.regs.lo;
}

/*
 * Set the LO register to the given value.
 */
void SetLO(u32 val)
{
    CPU_WARN("Forcing LO to {}", val);
    s.regs.lo = val;
}

/*
 * Executes a given instruction. This will change the state of the CPU.
 */
u8 ExecuteInstruction(u32 raw_instr)
{
    // decode
    Asm::Instruction instr = Asm::DecodeRawInstr(raw_instr);
    // execute
    return (s.prim_opmap[instr.op])(instr);
}

/*
 * Returns true if the current instruction is executing in the branch delay slot.
 */
bool InBranchDelaySlot()
{
    return s.bds.is_primed;
}

/*
 * Update function for ImGui.
 */
void OnActive(bool *active)
{
    constexpr u32 pc_region = (10 << 2);

    if (!ImGui::Begin("CPU Debug", active)) {
        ImGui::End();
        return;
    }

    //-------------------------------------
    // Step Buttons
    //-------------------------------------
    if (!g_emu_state.paused) {
        if (ImGui::Button("Pause")) {
            CPU_INFO("Pausing Emulation");
            // Pause Emulation
            g_emu_state.paused = true;
        }
    } else {
        if (ImGui::Button("Continue")) {
            CPU_INFO("Continuing Emulation");
            // Play Emulation
            g_emu_state.paused = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Step")) {
            // Step Mode
            g_emu_state.step_instr = true;
        }
        ImGui::SameLine();
        // Input new PC
        static char pc_buf[9] = "";
        auto input_flags = ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue;
        if (ImGui::InputTextWithHint("Set PC", "Enter new PC value (hex)", pc_buf, sizeof(pc_buf), input_flags)) {
            u32 new_pc = (u32)std::stol(std::string(pc_buf), nullptr, 16);
            SetPC(new_pc);
        }
    }
    //-------------------------------------

    u32 pc = s.regs.pc;
    u32 prePC = pc_region > pc ? 0 : pc - pc_region;
    u32 postPC = s.regs.pc + pc_region;


    //-------------------------------------
    // Instruction Disassembly
    //-------------------------------------
    ImGuiWindowFlags dasm_window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    ImGui::BeginGroup();
    ImGui::BeginChild("Instruction Disassembly", ImVec2(350,0), false, dasm_window_flags);
    ImGui::TextUnformatted("Instruction Disassembly");
    ImGui::Separator();
    for (u32 addr = prePC; addr <= postPC; addr += 4) {
        u32 instr = Bus::Read<u32>(addr, Bus::RWVerbosity::Quiet);
        if (addr == pc) {
            ImGui::TextUnformatted(PSX_FMT("{:08x} [{:08x}]  | {}", pc, instr, Asm::DasmInstruction(instr)).data());
        } else {
            ImGui::TextColored(ImVec4(0.6f,0.6f,0.6f,1), "%08x [%08x]  | %s", addr, instr,
                Asm::DasmInstruction(instr).data());
        }
    }
    ImGui::EndChild();
    //-------------------------------------

    ImGui::SameLine();

    //-------------------------------------
    // Registers
    //-------------------------------------
    ImGui::BeginChild("Registers", ImVec2(0,0), false, dasm_window_flags);
    ImGui::TextUnformatted("Registers");
    ImGui::Separator();
    ImGui::BeginGroup();
        // zero
        ImGui::TextUnformatted(PSX_FMT("R0(ZR)  = {:<26}", PSX_FMT("{0:#010x} ({0})", s.regs.r[0])).data());
        // reserved for assembler
        ImGui::TextUnformatted(PSX_FMT("R1(AT)  = {:<26}", PSX_FMT("{0:#010x} ({0})", s.regs.r[1])).data());
        // values for results and expr values
        ImGui::TextUnformatted(PSX_FMT("R2(V0)  = {:<26}", PSX_FMT("{0:#010x} ({0})", s.regs.r[2])).data());
        ImGui::TextUnformatted(PSX_FMT("R3(V1)  = {:<26}", PSX_FMT("{0:#010x} ({0})", s.regs.r[3])).data());
        // arguments
        ImGui::TextUnformatted(PSX_FMT("R4(A0)  = {:<26}", PSX_FMT("{0:#010x} ({0})", s.regs.r[4])).data());
        ImGui::TextUnformatted(PSX_FMT("R5(A1)  = {:<26}", PSX_FMT("{0:#010x} ({0})", s.regs.r[5])).data());
        ImGui::TextUnformatted(PSX_FMT("R6(A2)  = {:<26}", PSX_FMT("{0:#010x} ({0})", s.regs.r[6])).data());
        ImGui::TextUnformatted(PSX_FMT("R7(A3)  = {:<26}", PSX_FMT("{0:#010x} ({0})", s.regs.r[7])).data());
        // Temporaries
        ImGui::TextUnformatted(PSX_FMT("R8(T0)  = {:<26}", PSX_FMT("{0:#010x} ({0})", s.regs.r[8])).data());
        ImGui::TextUnformatted(PSX_FMT("R9(T1)  = {:<26}", PSX_FMT("{0:#010x} ({0})", s.regs.r[9])).data());
        ImGui::TextUnformatted(PSX_FMT("R10(T2) = {:<26}", PSX_FMT("{0:#010x} ({0})", s.regs.r[10])).data());
        ImGui::TextUnformatted(PSX_FMT("R11(T3) = {:<26}", PSX_FMT("{0:#010x} ({0})", s.regs.r[11])).data());
        ImGui::TextUnformatted(PSX_FMT("R12(T4) = {:<26}", PSX_FMT("{0:#010x} ({0})", s.regs.r[12])).data());
        ImGui::TextUnformatted(PSX_FMT("R13(T5) = {:<26}", PSX_FMT("{0:#010x} ({0})", s.regs.r[13])).data());
        ImGui::TextUnformatted(PSX_FMT("R14(T6) = {:<26}", PSX_FMT("{0:#010x} ({0})", s.regs.r[14])).data());
        ImGui::TextUnformatted(PSX_FMT("R15(T7) = {:<26}", PSX_FMT("{0:#010x} ({0})", s.regs.r[15])).data());
    ImGui::EndGroup();
    ImGui::SameLine();
    ImGui::BeginGroup();
        // Saved
        ImGui::TextUnformatted(PSX_FMT("R16(S0) = {:<26}", PSX_FMT("{0:#010x} ({0})", s.regs.r[16])).data());
        ImGui::TextUnformatted(PSX_FMT("R17(S1) = {:<26}", PSX_FMT("{0:#010x} ({0})", s.regs.r[17])).data());
        ImGui::TextUnformatted(PSX_FMT("R18(S2) = {:<26}", PSX_FMT("{0:#010x} ({0})", s.regs.r[18])).data());
        ImGui::TextUnformatted(PSX_FMT("R19(S3) = {:<26}", PSX_FMT("{0:#010x} ({0})", s.regs.r[19])).data());
        ImGui::TextUnformatted(PSX_FMT("R20(S4) = {:<26}", PSX_FMT("{0:#010x} ({0})", s.regs.r[20])).data());
        ImGui::TextUnformatted(PSX_FMT("R21(S5) = {:<26}", PSX_FMT("{0:#010x} ({0})", s.regs.r[21])).data());
        ImGui::TextUnformatted(PSX_FMT("R22(S6) = {:<26}", PSX_FMT("{0:#010x} ({0})", s.regs.r[22])).data());
        ImGui::TextUnformatted(PSX_FMT("R23(S7) = {:<26}", PSX_FMT("{0:#010x} ({0})", s.regs.r[23])).data());
        // Temporaries (continued)
        ImGui::TextUnformatted(PSX_FMT("R24(T8) = {:<26}", PSX_FMT("{0:#010x} ({0})", s.regs.r[24])).data());
        ImGui::TextUnformatted(PSX_FMT("R25(T9) = {:<26}", PSX_FMT("{0:#010x} ({0})", s.regs.r[25])).data());
        // Reserved for OS Kernel
        ImGui::TextUnformatted(PSX_FMT("R26(K0) = {:<26}", PSX_FMT("{0:#010x} ({0})", s.regs.r[26])).data());
        ImGui::TextUnformatted(PSX_FMT("R27(K1) = {:<26}", PSX_FMT("{0:#010x} ({0})", s.regs.r[27])).data());
        // Global Pointer
        ImGui::TextUnformatted(PSX_FMT("R28(GP) = {:<26}", PSX_FMT("{0:#010x} ({0})", s.regs.r[28])).data());
        // Stack Pointer
        ImGui::TextUnformatted(PSX_FMT("R29(SP) = {:<26}", PSX_FMT("{0:#010x} ({0})", s.regs.r[29])).data());
        // Frame Pointer
        ImGui::TextUnformatted(PSX_FMT("R30(FP) = {:<26}", PSX_FMT("{0:#010x} ({0})", s.regs.r[30])).data());
        // Return Address
        ImGui::TextUnformatted(PSX_FMT("R31(RA) = {:<26}", PSX_FMT("{0:#010x} ({0})", s.regs.r[31])).data());
    ImGui::EndGroup();

    // Special Registers
    ImGui::TextUnformatted("");
    ImGui::TextUnformatted("Special Registers");
    ImGui::BeginGroup();
        ImGui::TextUnformatted(PSX_FMT("HI = {:<26}", PSX_FMT("{0:#010x} ({0})", s.regs.hi)).data());
        ImGui::TextUnformatted(PSX_FMT("LO = {:<26}", PSX_FMT("{0:#010x} ({0})", s.regs.lo)).data());
        ImGui::TextUnformatted(PSX_FMT("PC = 0x{:08x}", s.regs.pc).data());
    ImGui::EndGroup();

    // end child
    ImGui::EndChild();
    //-------------------------------------
    ImGui::EndGroup(); // end instr/reg group

    ImGui::End();
}

//================================================
// Indirect Ops
//================================================
/*
 * Opcode = 0x00
 * Use Instruction.funct as the new op code. Most of these instructions are
 * ALU R-Type instructions.
 */
u8 Special(const Asm::Instruction& instr)
{
    return (s.sec_opmap[instr.funct])(instr);
}

/*
 * Opcode = 0x01
 * Use Instruction.bcondz_op as new op code. These instructions are branch compare
 * with zero register.
 */
u8 Bcondz(const Asm::Instruction& instr)
{
    // check if in map
    auto iter = s.bcondz_opmap.find(instr.bcondz_op);
    if (iter != s.bcondz_opmap.end()) {
        // found!
        return (iter->second)(instr);
    } else {
        return BadOp(instr);
    }
}

//================================================
// Special Ops
//================================================
/*
 * Unknown Instruction. Triggers a "Reserved Instruction Cop0::Exception" (excode = 0x0a)
 */
u8 BadOp(const Asm::Instruction& instr)
{
    CPU_WARN("Unknown CPU instruction: op[0x{:02}] funct[0x{:02}] bcondz[0x{:02x}]", instr.op, instr.funct, instr.bcondz_op);
    PSX_ASSERT(0);
    return 0;
}

/*
 */
u8 Syscall(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
    return 0;
}

u8 Break(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
    return 0;
}

//================================================
// Computational Instructions
//================================================

// *** Helpers ***
/*
 * Sign-extends a given u16 to a u32.
 */
static inline u32 signExtendTo32(u16 val16)
{
    bool neg = (val16 & 0x8000) != 0;
    u32 val32 = val16 | (neg ? 0xffff'0000 : 0x0000'0000);
    return val32;
}

/*
 * Return true if the resulting operation resulted in overflow.
 */
static inline bool overflowed(u32 res, u32 x, u32 y)
{
    return (~(x ^ y) & (x ^ res)) & 0x8000'0000;
}

/*
 * Transform into Two's Complement (for subtraction by adding).
 */
static inline u32 twosComplement(u32 val)
{
    return ~val + 1;
}

// *** Immediate ALU Ops ***
/*
 * Addition Immediate
 * opcode = 0x08
 * Format: ADDI rt, rs, (sign-extended)imm16
 * Add rs to imm16 and store in rt. TRAP on Two's-complement overflow.
 */
u8 Addi(const Asm::Instruction& instr)
{
    u32 signed_imm32 = signExtendTo32(instr.imm16);
    u32 rs = s.regs.r[instr.rs];
    u32 res = signed_imm32 + rs;
    if (overflowed(res, signed_imm32, rs)) {
        // TRAP!
        Cop0::Exception ex;
        ex.type = Cop0::Exception::Type::Overflow;
        Cop0::RaiseException(ex);
        return 0; // no modification
    } else {
        s.regs.r[instr.rt] = res;
        return instr.rt;
    }
}

/*
 * Addition Immediate (no trap)
 * opcode = 0x09
 * Format: ADDIU rt, rs, (sign-extended)imm16
 * Add rs to imm16 and store in rt. Do NOT TRAP on Two's-complement overflow.
 */
u8 Addiu(const Asm::Instruction& instr)
{
    u32 signed_imm32 = signExtendTo32(instr.imm16);
    s.regs.r[instr.rt] = signed_imm32 + s.regs.r[instr.rs];
    return instr.rt;
}

/*
 * Set on Less Than Immediate
 * opcode = 0x0a
 * Format: SLTI rt, rs, (sign-extended)imm16
 * Check if rs is less than imm16. TRAP on overflow.
 */
u8 Slti(const Asm::Instruction& instr)
{
    u32 signed_imm32 = signExtendTo32(instr.imm16);
    u32 rs = s.regs.r[instr.rs];
    u32 twos = twosComplement(signed_imm32);
    u32 res =  rs + twos;
    if (overflowed(res, rs, twos)) {
        // TRAP!
        Cop0::Exception ex;
        ex.type = Cop0::Exception::Type::Overflow;
        Cop0::RaiseException(ex);
        return 0; // no modification
    } else {
        s.regs.r[instr.rt] = res & 0x8000'0000 ? 1 : 0;
        return instr.rt;
    }
}

/*
 * Set on Less Than Immediate (no trap)
 * opcode = 0x0b
 * Format: SLTIU rt, rs, (sign-extended)imm16
 * Check if rs is less than imm16. Do NOT TRAP on overflow.
 */
u8 Sltiu(const Asm::Instruction& instr)
{
    u32 signed_imm32 = signExtendTo32(instr.imm16);
    u32 res = s.regs.r[instr.rs] + twosComplement(signed_imm32);
    s.regs.r[instr.rt] = res & 0x8000'0000 ? 1 : 0;
    return instr.rt;
}

/*
 * Bitwise AND Immediate
 * opcode = 0x0c
 * Format: ANDI rt, rs, (zero-extended)imm16
 * AND rs and imm16
 */
u8 Andi(const Asm::Instruction& instr)
{
    s.regs.r[instr.rt] = s.regs.r[instr.rs] & instr.imm16;
    return instr.rt;
}

/*
 * Bitwise OR Immediate
 * opcode = 0x0d
 * Format: ORI rt, rs, (zero-extended)imm16
 * OR rs and imm16
 */
u8 Ori(const Asm::Instruction& instr)
{
    s.regs.r[instr.rt] = s.regs.r[instr.rs] | instr.imm16;
    return instr.rt;
}

/*
 * Bitwise XOR Immediate
 * opcode = 0x0e
 * Format: XORI rt, rs, (zero-extended)imm16
 * XOR rs and imm16
 */
u8 Xori(const Asm::Instruction& instr)
{
    s.regs.r[instr.rt] = s.regs.r[instr.rs] ^ instr.imm16;
    return instr.rt;
}

/*
 * Load Upper Immediate
 * opcode = 0x0f
 * Format: LUI rt, (zero-extended)imm16
 * Load imm16 into the upper 16 bits of rt.
 */
u8 Lui(const Asm::Instruction& instr)
{
    s.regs.r[instr.rt] = (u32) (instr.imm16 << 16);
    return instr.rt;
}

// *** Three Operand Register-Type Ops ***
/*
 * Addition
 * funct = 0x20
 * Format: ADD rd, rs, rt
 * Add rs to rt and store in rd. TRAP on overflow.
 */
u8 Add(const Asm::Instruction& instr)
{
    u32 rt = s.regs.r[instr.rt];
    u32 rs = s.regs.r[instr.rs];
    u32 res = rs + rt;
    if (overflowed(res, rt, rs)) {
        // TRAP!
        Cop0::Exception ex;
        ex.type = Cop0::Exception::Type::Overflow;
        Cop0::RaiseException(ex);
        return 0;
    } else {
        s.regs.r[instr.rd] = res;
        return instr.rd;
    }
}

/*
 * Addition (no trap)
 * funct = 0x21
 * Format: ADDU rd, rs, rt
 * Add rs to rt and store in rd. Do NOT TRAP on overflow.
 */
u8 Addu(const Asm::Instruction& instr)
{
    s.regs.r[instr.rd] = s.regs.r[instr.rs] + s.regs.r[instr.rt];
    return instr.rd;
}

/*
 * Subtraction
 * funct = 0x22
 * Format: SUB rd, rs, rt
 * Sub rt from rs and store in rd. TRAP on overflow.
 */
u8 Sub(const Asm::Instruction& instr)
{
    u32 rt = twosComplement(s.regs.r[instr.rt]);
    u32 rs = s.regs.r[instr.rs];
    u32 res = rs + rt;
    if (overflowed(res, rt, rs)) {
        // TRAP!
        Cop0::Exception ex;
        ex.type = Cop0::Exception::Type::Overflow;
        Cop0::RaiseException(ex);
        return 0; // no modification
    } else {
        s.regs.r[instr.rd] = res;
        return instr.rd;
    }
}

/*
 * Subtraction (no trap)
 * funct = 0x23
 * Format: SUBU rd, rs, rt
 * Sub rt from rs and store in rd. Do NOT TRAP on overflow.
 */
u8 Subu(const Asm::Instruction& instr)
{
    s.regs.r[instr.rd] = s.regs.r[instr.rs] + twosComplement(s.regs.r[instr.rt]);
    return instr.rd;
}

/*
 * Set on Less Than
 * funct = 0x2a
 * Format: SLT rd, rs, rt
 * Set rd to 1 if rs less than rt. TRAP on overflow.
 */
u8 Slt(const Asm::Instruction& instr)
{
    u32 rt = s.regs.r[instr.rt];
    u32 rs = s.regs.r[instr.rs];
    u32 twos = twosComplement(rt);
    u32 res =  rs + twos;
    if (overflowed(res, rs, twos)) {
        // TRAP!
        Cop0::Exception ex;
        ex.type = Cop0::Exception::Type::Overflow;
        Cop0::RaiseException(ex);
        return 0; // no modification
    } else {
        s.regs.r[instr.rd] = res & 0x8000'0000 ? 1 : 0;
        return instr.rd;
    }
}

/*
 * Set on Less Than (no trap)
 * funct = 0x2b
 * Format: SLTU rd, rs, rt
 * Set rd to 1 if rs less than rt.
 */
u8 Sltu(const Asm::Instruction& instr)
{
    u32 res = s.regs.r[instr.rs] + twosComplement(s.regs.r[instr.rt]);
    s.regs.r[instr.rd] = res & 0x8000'0000 ? 1 : 0;
    return instr.rd;
}

/*
 * Bitwise AND
 * funct = 0x24
 * Format: AND rd, rs, rt
 * AND rs and rt, store in rd.
 */
u8 And(const Asm::Instruction& instr)
{
    s.regs.r[instr.rd] = s.regs.r[instr.rs] & s.regs.r[instr.rt];
    return instr.rd;
}

/*
 * Bitwise OR
 * funct = 0x25
 * Format: OR rd, rs, rt
 * OR rs and rt, store in rd.
 */
u8 Or(const Asm::Instruction& instr)
{
    s.regs.r[instr.rd] = s.regs.r[instr.rs] | s.regs.r[instr.rt];
    return instr.rd;
}

/*
 * Bitwise XOR
 * funct = 0x26
 * Format: XOR rd, rs, rt
 * XOR rs and rt, store in rd.
 */
u8 Xor(const Asm::Instruction& instr)
{
    s.regs.r[instr.rd] = s.regs.r[instr.rs] ^ s.regs.r[instr.rt];
    return instr.rd;
}

/*
 * Bitwise NOR
 * funct = 0x27
 * Format: NOR rd, rs, rt
 * NOR rs and rt, store in rd.
 */
u8 Nor(const Asm::Instruction& instr)
{
    s.regs.r[instr.rd] = ~(s.regs.r[instr.rs] | s.regs.r[instr.rt]);
    return instr.rd;
}

// *** Shift Operations ***
/*
 * Shift Left Logical
 * funct = 0x00
 * Format: SLL rd, rt, shamt
 * Shift rt left by shamt, inserting zeroes into low order bits. Store in rd.
 */
u8 Sll(const Asm::Instruction& instr)
{
    s.regs.r[instr.rd] = s.regs.r[instr.rt] << instr.shamt;
    return instr.rd;
}

/*
 * Shift Right Logical
 * funct = 0x02
 * Format: SRL rd, rt, shamt
 * Shift rt right by shamt, inserting zeroes into high order bits. Store in rd.
 */
u8 Srl(const Asm::Instruction& instr)
{
    s.regs.r[instr.rd] = s.regs.r[instr.rt] >> instr.shamt;
    return instr.rd;
}

/*
 * Shift Right Arithmetic
 * funct = 0x03
 * Format: SRA rd, rt, shamt
 * Shift rt right by shamt, keeping sign of highest order bit. Store in rd.
 */
u8 Sra(const Asm::Instruction& instr)
{
    bool msb = (s.regs.r[instr.rt] & 0x8000'0000) != 0;
    u32 upper_bits = (0xffff'ffff << (32 - instr.shamt));
    s.regs.r[instr.rd] = s.regs.r[instr.rt] >> instr.shamt;
    if (msb) {
        s.regs.r[instr.rd] |= upper_bits;
    }
    return instr.rd;
}

/*
 * Shift Left Logical Variable
 * funct = 0x04
 * Format: SLLV rd, rt, rs
 * Shift rt left by rs, inserting zeroes into low order bits. Store in rd.
 */
u8 Sllv(const Asm::Instruction& instr)
{
    s.regs.r[instr.rd] = s.regs.r[instr.rt] << s.regs.r[instr.rs];
    return instr.rd;
}

/*
 * Shift Right Logical Variable
 * funct = 0x06
 * Format: SRLV rd, rt, rs
 * Shift rt Right by rs, inserting zeroes into high order bits. Store in rd.
 */
u8 Srlv(const Asm::Instruction& instr)
{
    s.regs.r[instr.rd] = s.regs.r[instr.rt] >> s.regs.r[instr.rs];
    return instr.rd;
}

/*
 * Shift Right Arithmetic Variable
 * funct = 0x07
 * Format: SRAV rd, rt, rs
 * Shift rt Right by rs, keeping sign of highest order bit. Store in rd.
 */
u8 Srav(const Asm::Instruction& instr)
{
    bool msb = (s.regs.r[instr.rt] & 0x8000'0000) != 0;
    u32 rs = s.regs.r[instr.rs] & 0x1f; // only 5 lsb
    u32 upper_bits = (0xffff'ffff << (32 - rs));
    s.regs.r[instr.rd] = s.regs.r[instr.rt] >> rs;
    if (msb) {
        s.regs.r[instr.rd] |= upper_bits;
    }
    return instr.rd;
}

// *** Multiply and Divide Operations ***
// ** Helpers **
static inline u64 signExtendTo64(u32 val)
{
    u64 upper_bits = val & 0x8000'0000 ? 0xffff'ffff'0000'0000 : 0x0;
    return static_cast<u64>(val) | upper_bits;
}

/*
 * Multiply
 * funct = 0x18
 * Format: MULT rs, rt
 * Multiply rs and rt together as signed values and store 64 bit result
 * into HI/LO registers.
 */
u8 Mult(const Asm::Instruction& instr)
{
    i64 rs64 = static_cast<i64>(signExtendTo64(s.regs.r[instr.rs]));
    i64 rt64 = static_cast<i64>(signExtendTo64(s.regs.r[instr.rt]));
    i64 res = rs64 * rt64;
    s.regs.hi = (res >> 32) & 0xffff'ffff;
    s.regs.lo = res & 0xffff'ffff;
    return 0;
}

/*
 * Multiply Unsigned
 * funct = 0x19
 * Format: MULTU rs, rt
 * Multiply rs and rt together as unsigned values and store 64 bit result
 * into HI/LO registers.
 */
u8 Multu(const Asm::Instruction& instr)
{
    u64 rs64 = static_cast<u64>(s.regs.r[instr.rs]);
    u64 rt64 = static_cast<u64>(s.regs.r[instr.rt]);
    u64 res = rs64 * rt64;
    s.regs.hi = (res >> 32) & 0xffff'ffff;
    s.regs.lo = res & 0xffff'ffff;
    return 0;
}

/*
 * Division
 * funct = 0x1a
 * Format: DIV rs, rt
 * Divide rs by rt as signed values and store quotient in LO and remainder in HI.
 */
u8 Div(const Asm::Instruction& instr)
{
    i32 rs = static_cast<i32>(s.regs.r[instr.rs]);
    i32 rt = static_cast<i32>(s.regs.r[instr.rt]);
    if (rt != 0 && (rt != -1 && static_cast<u32>(rt) != 0x8000'0000)) {
        // normal case
        s.regs.hi = static_cast<u32>(rs % rt);
        s.regs.lo = static_cast<u32>(rs / rt);
    } else if (rt == 0) {
        // divide by zero special case
        s.regs.hi = static_cast<u32>(rs);
        if (rs >= 0) {
            s.regs.lo = static_cast<u32>(-1);
        } else {
            s.regs.lo = static_cast<u32>(1);
        }
    } else if (rt == -1 && static_cast<u32>(rs) == 0x8000'0000) {
        // special case
        s.regs.hi = 0;
        s.regs.lo = 0x8000'0000;
    } else {
        // should not get here
        assert(0);
    }
    return 0;
}

/*
 * Division Unsigned
 * funct = 0x1b
 * Format: DIVU rs, rt
 * Divide rs by rt as unsigned values and store quotient in LO and remainder in HI.
 */
u8 Divu(const Asm::Instruction& instr)
{
    u32 rs = s.regs.r[instr.rs];
    u32 rt = s.regs.r[instr.rt];
    if (rt != 0) {
        // normal case
        s.regs.hi = static_cast<u32>(rs % rt);
        s.regs.lo = static_cast<u32>(rs / rt);
    } else {
        s.regs.hi = static_cast<u32>(rs);
        s.regs.lo = 0xffff'ffff;
    }
    return 0;
}

/*
 * Move From HI
 * funct = 0x10
 * Format: MFHI rd
 * Move contents of Register HI to rd.
 */
u8 Mfhi(const Asm::Instruction& instr)
{
    s.regs.r[instr.rd] = s.regs.hi;
    return 0;
}

/*
 * Move From LO
 * funct = 0x12
 * Format: MFLO rd
 * Move contents of Register LO to rd.
 */
u8 Mflo(const Asm::Instruction& instr)
{
    s.regs.r[instr.rd] = s.regs.lo;
    return 0;
}

/*
 * Move To HI
 * funct = 0x11
 * Format: MTHI rd
 * Move contents of rd to Register HI
 */
u8 Mthi(const Asm::Instruction& instr)
{
    s.regs.hi = s.regs.r[instr.rd];
    return 0;
}

/*
 * Move To LO
 * funct = 0x13
 * Format: MTLO rd
 * Move contents of rd to Register LO
 */
u8 Mtlo(const Asm::Instruction& instr)
{
    s.regs.lo = s.regs.r[instr.rd];
    return 0;
}

//================================================
// Load and Store Instructions
//================================================
// *** Helpers ***
static inline u32 signExtendTo32(u8 val)
{
    u32 upper_bits = val & 0x80 ? 0xffff'ff00 : 0x0000'0000;
    return static_cast<u32>(val) | upper_bits;
}

// *** Load ***

/*
 * Load Byte
 * op = 0x20
 * Format: LB rt, imm(rs)
 * Load byte from address at imm + rs and store into rt (sign extended).
 */
u8 Lb(const Asm::Instruction& instr)
{
    u32 addr = signExtendTo32(instr.imm16) + s.regs.r[instr.rs];
    u8 byte = Bus::Read<u8>(addr);
    // Load Delay
    s.lds.is_primed = true;
    s.lds.reg = instr.rt;
    s.lds.val = signExtendTo32(byte);
    return 0;
}

/*
 * Load Byte Unsigned
 * op = 0x24
 * Format: LBU rt, imm(rs)
 * Load byte from address at imm + rs and store into rt (zero extended).
 */
u8 Lbu(const Asm::Instruction& instr)
{
    u32 addr = signExtendTo32(instr.imm16) + s.regs.r[instr.rs];
    u8 byte = Bus::Read<u8>(addr);
    // Load Delay!
    s.lds.is_primed = true;
    s.lds.reg = instr.rt;
    s.lds.val = static_cast<u32>(byte);
    return 0;
}

/*
 * Load Halfword
 * op = 0x21
 * Format: LH rt, imm(rs)
 * Load halfword from address at imm + rs and store into rt (sign extended).
 */
u8 Lh(const Asm::Instruction& instr)
{
    u32 addr = signExtendTo32(instr.imm16) + s.regs.r[instr.rs];
    // check alignment, needs to be 2-aligned
    if (addr & 0x1) {
        Cop0::Exception e;
        e.type = Cop0::Exception::Type::AddrErrLoad;
        Cop0::RaiseException(e);
    } else {
        u16 halfword = Bus::Read<u16>(addr);
        // Load Delay!
        s.lds.is_primed = true;
        s.lds.reg = instr.rt;
        s.lds.val = signExtendTo32(halfword);
    }
    return 0;
}

/*
 * Load Halfword Unsigned
 * op = 0x25
 * Format: LHU rt, imm(rs)
 * Load halfword from address at imm + rs and store into rt (zero extended).
 */
u8 Lhu(const Asm::Instruction& instr)
{
    u32 addr = signExtendTo32(instr.imm16) + s.regs.r[instr.rs];
    // check alignment, needs to be 2-aligned
    if (addr & 0x1) {
        Cop0::Exception e;
        e.type = Cop0::Exception::Type::AddrErrLoad;
        Cop0::RaiseException(e);
    } else {
        u16 halfword = Bus::Read<u16>(addr);
        // Load Delay!
        s.lds.is_primed = true;
        s.lds.reg = instr.rt;
        s.lds.val = static_cast<u32>(halfword);
    }
    return 0;
}

/*
 * Load Word
 * op = 0x23
 * Format: LW rt, imm(rs)
 * Load word from address at imm + rs and store into rt.
 */
u8 Lw(const Asm::Instruction& instr)
{
    u32 addr = signExtendTo32(instr.imm16) + s.regs.r[instr.rs];
    // check alignment, needs to be 4-aligned
    if (addr & 0x3) {
        Cop0::Exception e;
        e.type = Cop0::Exception::Type::AddrErrLoad;
        Cop0::RaiseException(e);
    } else {
        // Load Delay!
        s.lds.is_primed = true;
        s.lds.reg = instr.rt;
        s.lds.val = Bus::Read<u32>(addr);
    }
    return 0;
}

/*
 * Load Word Left
 * op = 0x22
 * Format: LWL rt, imm(rs)
 * Merge unaligned data (up to word boundary) into the MSB of the target register.
 */
u8 Lwl(const Asm::Instruction& instr)
{
    u32 addr = signExtendTo32(instr.imm16) + s.regs.r[instr.rs];
    u32 shift_to_align = (addr & 0x3) << 3; // mult by 8
    // read from 4-aligned addr
    u32 data = Bus::Read<u32>(addr & ~0x3u);
    // if the previous instruction was a load instruction, we need to grab the value
    // from the load delay slot.
    u32 rt = s.lds.was_primed && s.lds.reg == instr.rt ? s.lds.val : s.regs.r[instr.rt];
    s.lds.val = (rt & (0x00ff'ffff >> shift_to_align))
              | (data << (24 - shift_to_align));
    s.lds.reg = instr.rt;
    s.lds.is_primed = true;
    return 0;
}

/*
 * Load Word Right
 * op = 0x26
 * Format: LWR rt, imm(rs)
 * Merge unaligned data (down to word boundary) into LSB of the target register.
 */
u8 Lwr(const Asm::Instruction& instr)
{
    u32 addr = signExtendTo32(instr.imm16) + s.regs.r[instr.rs];
    u32 shift_to_align = (addr & 0x3) << 3; // mult by 8
    // read from 4-aligned addr
    u32 data = Bus::Read<u32>(addr & ~0x3u);
    // if the previous instruction was a load instruction, we need to grab the value
    // from the load delay slot.
    u32 rt = s.lds.was_primed && s.lds.reg == instr.rt ? s.lds.val : s.regs.r[instr.rt];
    u32 reg_mask = 0xffff'ff00 << (24 - shift_to_align);
    s.lds.val = (rt & reg_mask) | (data >> shift_to_align);
    s.lds.reg = instr.rt;
    s.lds.is_primed = true;
    return 0;
}

// *** Store ***
/*
 * Store Byte
 * op = 0x28
 * Format: SB rt, imm(rs)
 * Store the least significant byte in rt at address imm + base.
 */
u8 Sb(const Asm::Instruction& instr)
{
    u32 addr = signExtendTo32(instr.imm16) + s.regs.r[instr.rs];
    u8 rt = s.regs.r[instr.rt] & 0xff;
    Bus::Write<u8>(rt, addr);
    return 0;
}

/*
 * Store Halfword
 * op = 0x29
 * Format: SH rt, imm(rs)
 * Store the least significant halfword in rt at address imm + base.
 */
u8 Sh(const Asm::Instruction& instr)
{
    u32 addr = signExtendTo32(instr.imm16) + s.regs.r[instr.rs];
    // check if 2-aligned
    if (addr & 0x1) {
        // not aligned, throw Cop0::Exception
        Cop0::Exception e;
        e.type = Cop0::Exception::Type::AddrErrStore;
        Cop0::RaiseException(e);
    } else {
        u16 rt = s.regs.r[instr.rt] & 0xffff;
        Bus::Write<u16>(rt, addr);
    }
    return 0;
}

/*
 * Store Word
 * op = 0x2b
 * Format: SW rt, imm(rs)
 * Store the word in rt at address imm + base.
 */
u8 Sw(const Asm::Instruction& instr)
{
    u32 addr = signExtendTo32(instr.imm16) + s.regs.r[instr.rs];
    if (addr & 0x3) {
        // not aligned, throw Cop0::Exception
        Cop0::Exception e;
        e.type = Cop0::Exception::Type::AddrErrStore;
        Cop0::RaiseException(e);
    } else {
        u32 rt = s.regs.r[instr.rt];
        Bus::Write<u32>(rt, addr);
    }
    return 0;
}

/*
 * Store Word Left
 * op = 0x2a
 * Format: SWL rt, imm(rs)
 * Merge the word stored in rt into the unaligned address down to the word
 * boundary.
 */
u8 Swl(const Asm::Instruction& instr)
{
    u32 addr = signExtendTo32(instr.imm16) + s.regs.r[instr.rs];
    u32 shift_to_align = (addr & 0x3) << 3; // mult by 8
    // read from 4-aligned addr
    u32 old_data = Bus::Read<u32>(addr & ~0x3u);
    u32 rt = s.regs.r[instr.rt];
    u32 mask = (0xffff'ff00 << shift_to_align);
    u32 new_data = (old_data & mask) | (rt >> (24 - shift_to_align));
    Bus::Write<u32>(new_data, addr & ~0x3u);
    return 0;
}

/*
 * Store Word Right
 * op = 0x2e
 * Format: SWR rt, imm(rs)
 * Merge the word stored in rt into the unaligned address up to the word
 * boundary.
 */
u8 Swr(const Asm::Instruction& instr)
{
    u32 addr = signExtendTo32(instr.imm16) + s.regs.r[instr.rs];
    u32 shift_to_align = (addr & 0x3) << 3; // mult by 8
    // read from 4-aligned addr
    u32 old_data = Bus::Read<u32>(addr & ~0x3u);
    u32 rt = s.regs.r[instr.rt];
    u32 mask = (0x00ff'ffff >> (24 - shift_to_align));
    u32 new_data = (old_data & mask) | (rt << shift_to_align);
    Bus::Write<u32>(new_data, addr & ~0x3u);
    return 0;
}

//================================================
// Jump and Branch Instructions
//================================================
// *** Jump instructions ***
/*
 * Jump
 * op = 0x02
 * Format: J target
 * Shift target left by 2 and combine with high 4 bits of PC to form new PC.
 */
u8 J(const Asm::Instruction& instr)
{
    u32 target = instr.target << 2;
    target |= (s.regs.pc & 0xf000'0000);
    s.bds.pc = target;
    s.bds.is_primed = true;
    s.bds.take_branch = true;
    return 0;
}

/*
 * Jump and Link
 * op = 0x03
 * Format: JAL target
 * Jump to Target and store instruction following delay into R31.
 */
u8 Jal(const Asm::Instruction& instr)
{
    u32 target = instr.target << 2;
    target |= (s.regs.pc & 0xf000'0000);
    s.regs.r[31] = s.regs.pc + 4; // return addr
    s.bds.pc = target;
    s.bds.is_primed = true;
    s.bds.take_branch = true;
    return 31;
}

/*
 * Jump Register
 * funct = 0x08
 * Format: JR rs
 * Jump to the address stored in rs.
 */
u8 Jr(const Asm::Instruction& instr)
{
    u32 target = s.regs.r[instr.rs];
    // check 4-alignment
    if (target & 0x3) {
        Cop0::Exception e;
        e.type = Cop0::Exception::Type::AddrErrLoad;
        e.badv = target;
        Cop0::RaiseException(e);
    } else {
        s.bds.pc = target;
        s.bds.take_branch = true;
    }
    s.bds.is_primed = true;
    return 0;
}

/*
 * Jump Register and Link
 * funct = 0x09
 * Format: JALR rs, rd
 * Jump to the address stored in rs, store the return address in rd.
 */
u8 Jalr(const Asm::Instruction& instr)
{
    u32 target = s.regs.r[instr.rs];
    // check 4-alignment
    if (target & 0x3) {
        Cop0::Exception e;
        e.type = Cop0::Exception::Type::AddrErrLoad;
        e.badv = target;
        Cop0::RaiseException(e);
    } else {
        s.bds.pc = target;
        s.regs.r[instr.rd] = s.regs.pc + 4;
        s.bds.take_branch = true;
    }
    s.bds.is_primed = true;
    return 0;
}

// *** Branch instructions ***
/*
 * Branch if Equal
 * op = 0x04
 * Format: BEQ rs, rt, offset
 * Branch to pc + offset if rs equals rt.
 */
u8 Beq(const Asm::Instruction& instr)
{
    if (s.regs.r[instr.rs] == s.regs.r[instr.rt]) {
        s.bds.take_branch = true;
        u32 offset = signExtendTo32(instr.imm16) << 2;
        s.bds.pc = s.regs.pc + offset;
    }
    s.bds.is_primed = true;
    return 0;
}

/*
 * Branch if Not Equal
 * op = 0x05
 * Format: BNE rs, rt, offset
 * Branch to pc + offset if rs does not equals rt.
 */
u8 Bne(const Asm::Instruction& instr)
{
    if (s.regs.r[instr.rs] != s.regs.r[instr.rt]) {
        u32 offset = signExtendTo32(instr.imm16) << 2;
        s.bds.pc = s.regs.pc + offset;
        s.bds.take_branch = true;
    }
    s.bds.is_primed = true;
    return 0;
}

/*
 * Branch if less than or equal to zero.
 * op = 0x06
 * Format: BLEZ rs, offset
 * Branch to pc + offset if rs greater than zero.
 */
u8 Blez(const Asm::Instruction& instr)
{
    i32 rs = static_cast<i32>(s.regs.r[instr.rs]);
    if (rs <= 0) {
        s.bds.take_branch = true;
        u32 offset = signExtendTo32(instr.imm16) << 2;
        s.bds.pc = s.regs.pc + offset;
    }
    s.bds.is_primed = true;
    return 0;
}

/*
 * Branch if greater than 0
 * op = 0x07
 * Format: BGTZ rs, offset
 * Branch to pc + offset if rs greater than zero.
 */
u8 Bgtz(const Asm::Instruction& instr)
{
    i32 rs = static_cast<i32>(s.regs.r[instr.rs]);
    if (rs > 0) {
        s.bds.take_branch = true;
        u32 offset = signExtendTo32(instr.imm16) << 2;
        s.bds.pc = s.regs.pc + offset;
    }
    s.bds.is_primed = true;
    return 0;
}

// *** bcondz ***
/*
 * Branch if less than 0
 * bcondz = 0x00
 * Format: BLTZ rs, offset
 * Branch to pc + offset if rs less than zero.
 */
u8 Bltz(const Asm::Instruction& instr)
{
    i32 rs = static_cast<i32>(s.regs.r[instr.rs]);
    if (rs < 0) {
        s.bds.take_branch = true;
        u32 offset = signExtendTo32(instr.imm16) << 2;
        s.bds.pc = s.regs.pc + offset;
    }
    s.bds.is_primed = true;
    return 0;
}

/*
 * Branch if greater than or equal to 0
 * bcondz = 0x01
 * Format: BGEZ rs, offset
 * Branch to pc + offset if rs greater than or equal to zero.
 */
u8 Bgez(const Asm::Instruction& instr)
{
    i32 rs = static_cast<i32>(s.regs.r[instr.rs]);
    if (rs >= 0) {
        s.bds.take_branch = true;
        u32 offset = signExtendTo32(instr.imm16) << 2;
        s.bds.pc = s.regs.pc + offset;
    }
    s.bds.is_primed = true;
    return 0;
}

/*
 * Branch if less then zero and link
 * bcondz = 0x10
 * Format: BLTZAL rs, offset
 * Branch to pc + offset if rs less than zero and link.
 */
u8 Bltzal(const Asm::Instruction& instr)
{
    i32 rs = static_cast<i32>(s.regs.r[instr.rs]);
    if (rs < 0) {
        s.bds.take_branch = true;
        u32 offset = signExtendTo32(instr.imm16) << 2;
        s.bds.pc = s.regs.pc + offset;
        s.regs.r[31] = s.regs.pc + 8;
    }
    s.bds.is_primed = true;
    return 0;
}

/*
 * Branch if greater or equal to zero and link
 * bcondz = 0x10
 * Format: BGEZAL rs, offset
 * Branch to pc + offset if rs greater or equal to zero and link.
 */
u8 Bgezal(const Asm::Instruction& instr)
{
    i32 rs = static_cast<i32>(s.regs.r[instr.rs]);
    if (rs >= 0) {
        s.bds.take_branch = true;
        u32 offset = signExtendTo32(instr.imm16) << 2;
        s.bds.pc = s.regs.pc + offset;
        s.regs.r[31] = s.regs.pc + 8;
    }
    s.bds.is_primed = true;
    return 0;
}

//================================================
// Co-Processor Instructions
//================================================
// *** Cop General ***
/*
 * Cop0 Command
 * op = 0x10
 * Format: Cop0 ...
 */
u8 Cop0(const Asm::Instruction& instr)
{
    // check operation
    u8 modified_reg = 0;
    if ((instr.cop_op & 0x10) == 0) {
        if ((instr.cop_op & 0x08) != 0) {
            // Branch
            PSX_ASSERT(0);
        } else {
            // Moves
            // MF rt, cop0_rd
            // MT rt, cop0_rd
            switch (instr.cop_op) {
            case 0x00: // MFCn
                s.regs.r[instr.rt] = Cop0::Mf(instr.rd);
                modified_reg = instr.rt;
                break;
            case 0x02: // CFCn
            {
                // reserved instruction exception
                Cop0::Exception e;
                e.type = Cop0::Exception::Type::ReservedInstr;
                Cop0::RaiseException(e);
                break;
            }
            case 0x04: // MTCn
                Cop0::Mt(s.regs.r[instr.rt], instr.rd);
                break;
            case 0x06: // CTCn
            {
                // reserved instruction exception
                Cop0::Exception e;
                e.type = Cop0::Exception::Type::ReservedInstr;
                Cop0::RaiseException(e);
                break;
            }
            default:
                CPU_ERROR("Unkown COP0 move operation: {:x}", instr.cop_op);
                PSX_ASSERT(0);
                break;
            }
        }
    } else {
        // Cop Command
        Cop0::ExeCmd(instr.imm25);
    }
    return modified_reg;
}

u8 Cop1(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
    return 0;
}

u8 Cop2(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
    return 0;
}

u8 Cop3(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
    return 0;
}

// *** Cop Loads ***
u8 LwC0(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
    return 0;
}

u8 LwC1(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
    return 0;
}

u8 LwC2(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
    return 0;
}

u8 LwC3(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
    return 0;
}

// *** Cop Store ***
u8 SwC0(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
    return 0;
}

u8 SwC1(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
    return 0;
}

u8 SwC2(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
    return 0;
}

u8 SwC3(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
    return 0;
}


}// end Namespace
}

// *** Private Helpers ***
namespace  {

/*
 * Build the cpu op maps to be used during the execution cycle.
 */
void buildOpMaps()
{
    using namespace Psx::Cpu;
    // Primary Op Encoding
    s.prim_opmap[0x00] = Special; s.prim_opmap[0x01] = Bcondz;
    s.prim_opmap[0x02] = J;       s.prim_opmap[0x03] = Jal;
    s.prim_opmap[0x04] = Beq;     s.prim_opmap[0x05] = Bne;
    s.prim_opmap[0x06] = Blez;    s.prim_opmap[0x07] = Bgtz;
    s.prim_opmap[0x08] = Addi;    s.prim_opmap[0x09] = Addiu;
    s.prim_opmap[0x0a] = Slti;    s.prim_opmap[0x0b] = Sltiu;
    s.prim_opmap[0x0c] = Andi;    s.prim_opmap[0x0d] = Ori;
    s.prim_opmap[0x0e] = Xori;    s.prim_opmap[0x0f] = Lui;

    s.prim_opmap[0x10] = Cop0;    s.prim_opmap[0x11] = Cop1;
    s.prim_opmap[0x12] = Cop2;    s.prim_opmap[0x13] = Cop3;
    s.prim_opmap[0x14] = BadOp;   s.prim_opmap[0x15] = BadOp;
    s.prim_opmap[0x16] = BadOp;   s.prim_opmap[0x17] = BadOp;
    s.prim_opmap[0x18] = BadOp;   s.prim_opmap[0x19] = BadOp;
    s.prim_opmap[0x1a] = BadOp;   s.prim_opmap[0x1b] = BadOp;
    s.prim_opmap[0x1c] = BadOp;   s.prim_opmap[0x1d] = BadOp;
    s.prim_opmap[0x1e] = BadOp;   s.prim_opmap[0x1f] = BadOp;

    s.prim_opmap[0x20] = Lb;      s.prim_opmap[0x21] = Lh;
    s.prim_opmap[0x22] = Lwl;     s.prim_opmap[0x23] = Lw;
    s.prim_opmap[0x24] = Lbu;     s.prim_opmap[0x25] = Lhu;
    s.prim_opmap[0x26] = Lwr;     s.prim_opmap[0x27] = BadOp;
    s.prim_opmap[0x28] = Sb;      s.prim_opmap[0x29] = Sh;
    s.prim_opmap[0x2a] = Swl;     s.prim_opmap[0x2b] = Sw;
    s.prim_opmap[0x2c] = BadOp;   s.prim_opmap[0x2d] = BadOp;
    s.prim_opmap[0x2e] = Swr;     s.prim_opmap[0x2f] = BadOp;

    s.prim_opmap[0x30] = LwC0;    s.prim_opmap[0x31] = LwC1;
    s.prim_opmap[0x32] = LwC2;    s.prim_opmap[0x33] = LwC3;
    s.prim_opmap[0x34] = BadOp;   s.prim_opmap[0x35] = BadOp;
    s.prim_opmap[0x36] = BadOp;   s.prim_opmap[0x37] = BadOp;
    s.prim_opmap[0x38] = SwC0;    s.prim_opmap[0x39] = SwC1;
    s.prim_opmap[0x3a] = SwC2;    s.prim_opmap[0x3b] = SwC3;
    s.prim_opmap[0x3c] = BadOp;   s.prim_opmap[0x3d] = BadOp;
    s.prim_opmap[0x3e] = BadOp;   s.prim_opmap[0x3f] = BadOp;

    // Secondary Op Encoding
    s.sec_opmap[0x00] = Sll;     s.sec_opmap[0x01] = BadOp;
    s.sec_opmap[0x02] = Srl;     s.sec_opmap[0x03] = Sra;
    s.sec_opmap[0x04] = Sllv;    s.sec_opmap[0x05] = BadOp;
    s.sec_opmap[0x06] = Srlv;    s.sec_opmap[0x07] = Srav;
    s.sec_opmap[0x08] = Jr;      s.sec_opmap[0x09] = Jalr;
    s.sec_opmap[0x0a] = BadOp;   s.sec_opmap[0x0b] = BadOp;
    s.sec_opmap[0x0c] = Syscall; s.sec_opmap[0x0d] = Break;
    s.sec_opmap[0x0e] = BadOp;   s.sec_opmap[0x0f] = BadOp;

    s.sec_opmap[0x10] = Mfhi;    s.sec_opmap[0x11] = Mthi;
    s.sec_opmap[0x12] = Mflo;    s.sec_opmap[0x13] = Mtlo;
    s.sec_opmap[0x14] = BadOp;   s.sec_opmap[0x15] = BadOp;
    s.sec_opmap[0x16] = BadOp;   s.sec_opmap[0x17] = BadOp;
    s.sec_opmap[0x18] = Mult;    s.sec_opmap[0x19] = Multu;
    s.sec_opmap[0x1a] = Div;     s.sec_opmap[0x1b] = Divu;
    s.sec_opmap[0x1c] = BadOp;   s.sec_opmap[0x1d] = BadOp;
    s.sec_opmap[0x1e] = BadOp;   s.sec_opmap[0x1f] = BadOp;

    s.sec_opmap[0x20] = Add;     s.sec_opmap[0x21] = Addu;
    s.sec_opmap[0x22] = Sub;     s.sec_opmap[0x23] = Subu;
    s.sec_opmap[0x24] = And;     s.sec_opmap[0x25] = Or;
    s.sec_opmap[0x26] = Xor;     s.sec_opmap[0x27] = Nor;
    s.sec_opmap[0x28] = BadOp;   s.sec_opmap[0x29] = BadOp;
    s.sec_opmap[0x2a] = Slt;     s.sec_opmap[0x2b] = Sltu;
    s.sec_opmap[0x2c] = BadOp;   s.sec_opmap[0x2d] = BadOp;
    s.sec_opmap[0x2e] = BadOp;   s.sec_opmap[0x2f] = BadOp;

    s.sec_opmap[0x30] = BadOp;   s.sec_opmap[0x31] = BadOp;
    s.sec_opmap[0x32] = BadOp;   s.sec_opmap[0x33] = BadOp;
    s.sec_opmap[0x34] = BadOp;   s.sec_opmap[0x35] = BadOp;
    s.sec_opmap[0x36] = BadOp;   s.sec_opmap[0x37] = BadOp;
    s.sec_opmap[0x38] = BadOp;   s.sec_opmap[0x39] = BadOp;
    s.sec_opmap[0x3a] = BadOp;   s.sec_opmap[0x3b] = BadOp;
    s.sec_opmap[0x3c] = BadOp;   s.sec_opmap[0x3d] = BadOp;
    s.sec_opmap[0x3e] = BadOp;   s.sec_opmap[0x3f] = BadOp;

    // Bcondz Op encoding
    s.bcondz_opmap[0x00] = Bltz;
    s.bcondz_opmap[0x01] = Bgez;
    s.bcondz_opmap[0x10] = Bltzal;
    s.bcondz_opmap[0x11] = Bgezal;
}

}
