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
#include "core/globals.h"

#define CPU_INFO(msg) PSXLOG_INFO("CPU", msg)
#define CPU_WARN(msg) PSXLOG_WARN("CPU", msg)
#define CPU_ERROR(msg) PSXLOG_ERROR("CPU", msg)

// simple constructor
Cpu::Cpu(std::shared_ptr<Bus> bus)
    : m_bus(bus), m_cop0(*this)
{
    CPU_INFO("Initializing CPU");
    buildOpMaps();
    PSX_ASSERT(m_bus.get() != nullptr);
    Reset();
}

/*
 * Execute one instruction. On average, takes 1 psx clock cycle.
 */
void Cpu::Step()
{
    // fetch next instruction
    u32 cur_instr = m_bus->Read32(m_regs.pc);

    // check if last instruction was a branch/jump
    m_bds.was_primed = m_bds.is_primed;
    m_bds.is_primed = false;
    bool take_branch = m_bds.take_branch;
    m_bds.take_branch = false;
    u32 baddr = m_bds.pc;


    // if previous instruction was a load, the load delay will be primed.
    // we need to check this here just in case the next instruction will
    // re-prime the load delay slot.
    m_lds.was_primed = m_lds.is_primed;
    m_lds.is_primed = false;

    // execute
    u8 modified_reg = ExecuteInstruction(cur_instr);

    // update pc (dependent on branch)
    m_regs.pc = take_branch ? baddr : m_regs.pc + 4;

    // race condition: if instruction writes to same register in load
    // delay slot, the instruction wins over the load.
    if (m_lds.was_primed && modified_reg != m_lds.reg) {
        m_regs.r[m_lds.reg] = m_lds.val;
    }

    // zero register should always be zero
    m_regs.r[0] = 0;
}

/*
 * Resets the state of the CPU.
 */
void Cpu::Reset()
{
    CPU_INFO("Resetting state");
    // reset cop0
    m_cop0.Reset();
    // reset branch/load slots
    m_lds = {};
    m_bds = {};
    // reset registers
    Registers new_regs;
    m_regs = new_regs;
}

/*
 * Set the cpu's program counter to the specified address.
 */
void Cpu::SetPC(u32 addr)
{
    CPU_WARN(PSX_FMT("Forcing PC to 0x{:08x}", addr));
    m_regs.pc = addr;
//    m_regs.next_pc = addr + 4;
    m_bds = {};
}

/*
 * Return the current value of the program counter.
 */
u32 Cpu::GetPC()
{
    return m_regs.pc;
}

/*
 * Get the current value stored in Register r.
 */
u32 Cpu::GetR(size_t r)
{
    PSX_ASSERT(r < 32);
    return m_regs.r[r];
}

/*
 * Set the chosen register to the chosen value.
 */
void Cpu::SetR(size_t r, u32 val)
{
    CPU_WARN(PSX_FMT("Forcing R{} to {}", r, val));
    PSX_ASSERT(r < 32);
    m_regs.r[r] = val;
}

/*
 * Return the current value of the HI register
 */
u32 Cpu::GetHI()
{
    return m_regs.hi;
}

/*
 * Set the HI register to the given value.
 */
void Cpu::SetHI(u32 val)
{
    CPU_WARN(PSX_FMT("Forcing HI to {}", val));
    m_regs.hi = val;
}

/*
 * Return the current value of the LO register
 */
u32 Cpu::GetLO()
{
    return m_regs.lo;
}

/*
 * Set the LO register to the given value.
 */
void Cpu::SetLO(u32 val)
{
    CPU_WARN(PSX_FMT("Forcing LO to {}", val));
    m_regs.lo = val;
}

/*
 * Executes a given instruction. This will change the state of the CPU.
 */
u8 Cpu::ExecuteInstruction(u32 raw_instr)
{
    // decode
    Asm::Instruction instr = Asm::DecodeRawInstr(raw_instr);
    // execute
    return (this->*m_prim_opmap[instr.op])(instr);
}

/*
 * Returns true if the current instruction is executing in the branch delay slot.
 */
bool Cpu::InBranchDelaySlot()
{
    return m_bds.is_primed;
}

/*
 * Update function for ImGui.
 */
void Cpu::OnActive(bool *active)
{
    const u32 pc_region = (10 << 2);

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

    u32 pc = m_regs.pc;
    u32 prePC = pc_region > pc ? 0 : pc - pc_region;
    u32 postPC = m_regs.pc + pc_region;


    //-------------------------------------
    // Instruction Disassembly
    //-------------------------------------
    ImGuiWindowFlags dasm_window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    ImGui::BeginGroup();
    ImGui::BeginChild("Instruction Disassembly", ImVec2(300,0), false, dasm_window_flags);
    ImGui::TextUnformatted("Instruction Disassembly");
    for (u32 addr = prePC; addr <= postPC; addr += 4) {
        u32 instr = m_bus->Read32(addr, Bus::RWVerbosity::Quiet);
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
    ImGui::BeginGroup();
        // zero
        ImGui::TextUnformatted(PSX_FMT("R0(ZR)  = {:<26}", PSX_FMT("{0:#010x} ({0})", m_regs.r[0])).data());
        // reserved for assembler
        ImGui::TextUnformatted(PSX_FMT("R1(AT)  = {:<26}", PSX_FMT("{0:#010x} ({0})", m_regs.r[1])).data());
        // values for results and expr values
        ImGui::TextUnformatted(PSX_FMT("R2(V0)  = {:<26}", PSX_FMT("{0:#010x} ({0})", m_regs.r[2])).data());
        ImGui::TextUnformatted(PSX_FMT("R3(V1)  = {:<26}", PSX_FMT("{0:#010x} ({0})", m_regs.r[3])).data());
        // arguments
        ImGui::TextUnformatted(PSX_FMT("R4(A0)  = {:<26}", PSX_FMT("{0:#010x} ({0})", m_regs.r[4])).data());
        ImGui::TextUnformatted(PSX_FMT("R5(A1)  = {:<26}", PSX_FMT("{0:#010x} ({0})", m_regs.r[5])).data());
        ImGui::TextUnformatted(PSX_FMT("R6(A2)  = {:<26}", PSX_FMT("{0:#010x} ({0})", m_regs.r[6])).data());
        ImGui::TextUnformatted(PSX_FMT("R7(A3)  = {:<26}", PSX_FMT("{0:#010x} ({0})", m_regs.r[7])).data());
        // Temporaries
        ImGui::TextUnformatted(PSX_FMT("R8(T0)  = {:<26}", PSX_FMT("{0:#010x} ({0})", m_regs.r[8])).data());
        ImGui::TextUnformatted(PSX_FMT("R9(T1)  = {:<26}", PSX_FMT("{0:#010x} ({0})", m_regs.r[9])).data());
        ImGui::TextUnformatted(PSX_FMT("R10(T2) = {:<26}", PSX_FMT("{0:#010x} ({0})", m_regs.r[10])).data());
        ImGui::TextUnformatted(PSX_FMT("R11(T3) = {:<26}", PSX_FMT("{0:#010x} ({0})", m_regs.r[11])).data());
        ImGui::TextUnformatted(PSX_FMT("R12(T4) = {:<26}", PSX_FMT("{0:#010x} ({0})", m_regs.r[12])).data());
        ImGui::TextUnformatted(PSX_FMT("R13(T5) = {:<26}", PSX_FMT("{0:#010x} ({0})", m_regs.r[13])).data());
        ImGui::TextUnformatted(PSX_FMT("R14(T6) = {:<26}", PSX_FMT("{0:#010x} ({0})", m_regs.r[14])).data());
        ImGui::TextUnformatted(PSX_FMT("R15(T7) = {:<26}", PSX_FMT("{0:#010x} ({0})", m_regs.r[15])).data());
    ImGui::EndGroup();
    ImGui::SameLine();
    ImGui::BeginGroup();
        // Saved
        ImGui::TextUnformatted(PSX_FMT("R16(S0) = {:<26}", PSX_FMT("{0:#010x} ({0})", m_regs.r[16])).data());
        ImGui::TextUnformatted(PSX_FMT("R17(S1) = {:<26}", PSX_FMT("{0:#010x} ({0})", m_regs.r[17])).data());
        ImGui::TextUnformatted(PSX_FMT("R18(S2) = {:<26}", PSX_FMT("{0:#010x} ({0})", m_regs.r[18])).data());
        ImGui::TextUnformatted(PSX_FMT("R19(S3) = {:<26}", PSX_FMT("{0:#010x} ({0})", m_regs.r[19])).data());
        ImGui::TextUnformatted(PSX_FMT("R20(S4) = {:<26}", PSX_FMT("{0:#010x} ({0})", m_regs.r[20])).data());
        ImGui::TextUnformatted(PSX_FMT("R21(S5) = {:<26}", PSX_FMT("{0:#010x} ({0})", m_regs.r[21])).data());
        ImGui::TextUnformatted(PSX_FMT("R22(S6) = {:<26}", PSX_FMT("{0:#010x} ({0})", m_regs.r[22])).data());
        ImGui::TextUnformatted(PSX_FMT("R23(S7) = {:<26}", PSX_FMT("{0:#010x} ({0})", m_regs.r[23])).data());
        // Temporaries (continued)
        ImGui::TextUnformatted(PSX_FMT("R24(T8) = {:<26}", PSX_FMT("{0:#010x} ({0})", m_regs.r[24])).data());
        ImGui::TextUnformatted(PSX_FMT("R25(T9) = {:<26}", PSX_FMT("{0:#010x} ({0})", m_regs.r[25])).data());
        // Reserved for OS Kernel
        ImGui::TextUnformatted(PSX_FMT("R26(K0) = {:<26}", PSX_FMT("{0:#010x} ({0})", m_regs.r[26])).data());
        ImGui::TextUnformatted(PSX_FMT("R27(K1) = {:<26}", PSX_FMT("{0:#010x} ({0})", m_regs.r[27])).data());
        // Global Pointer
        ImGui::TextUnformatted(PSX_FMT("R28(GP) = {:<26}", PSX_FMT("{0:#010x} ({0})", m_regs.r[28])).data());
        // Stack Pointer
        ImGui::TextUnformatted(PSX_FMT("R29(SP) = {:<26}", PSX_FMT("{0:#010x} ({0})", m_regs.r[29])).data());
        // Frame Pointer
        ImGui::TextUnformatted(PSX_FMT("R30(FP) = {:<26}", PSX_FMT("{0:#010x} ({0})", m_regs.r[30])).data());
        // Return Address
        ImGui::TextUnformatted(PSX_FMT("R31(RA) = {:<26}", PSX_FMT("{0:#010x} ({0})", m_regs.r[31])).data());
    ImGui::EndGroup();

    // Special Registers
    ImGui::TextUnformatted("");
    ImGui::TextUnformatted("Special Registers");
    ImGui::BeginGroup();
        ImGui::TextUnformatted(PSX_FMT("HI = {:<26}", PSX_FMT("{0:#010x} ({0})", m_regs.hi)).data());
        ImGui::TextUnformatted(PSX_FMT("LO = {:<26}", PSX_FMT("{0:#010x} ({0})", m_regs.lo)).data());
        ImGui::TextUnformatted(PSX_FMT("PC = 0x{:08x}", m_regs.pc).data());
    ImGui::EndGroup();

    // end child
    ImGui::EndChild();
    //-------------------------------------
    ImGui::EndGroup(); // end instr/reg group

    ImGui::End();
}

/*
 * Return the label for this Debug module.
 */
std::string Cpu::GetModuleLabel()
{
    return "CPU";
}

/*
 * Build the cpu op maps to be used during the execution cycle.
 */
void Cpu::buildOpMaps()
{
    // Primary Op Encoding
    m_prim_opmap[0x00] = &Cpu::Special; m_prim_opmap[0x01] = &Cpu::Bcondz;
    m_prim_opmap[0x02] = &Cpu::J;       m_prim_opmap[0x03] = &Cpu::Jal;
    m_prim_opmap[0x04] = &Cpu::Beq;     m_prim_opmap[0x05] = &Cpu::Bne;
    m_prim_opmap[0x06] = &Cpu::Blez;    m_prim_opmap[0x07] = &Cpu::Bgtz;
    m_prim_opmap[0x08] = &Cpu::Addi;    m_prim_opmap[0x09] = &Cpu::Addiu;
    m_prim_opmap[0x0a] = &Cpu::Slti;    m_prim_opmap[0x0b] = &Cpu::Sltiu;
    m_prim_opmap[0x0c] = &Cpu::Andi;    m_prim_opmap[0x0d] = &Cpu::Ori;
    m_prim_opmap[0x0e] = &Cpu::Xori;    m_prim_opmap[0x0f] = &Cpu::Lui;

    m_prim_opmap[0x10] = &Cpu::Cop0;    m_prim_opmap[0x11] = &Cpu::Cop1;
    m_prim_opmap[0x12] = &Cpu::Cop2;    m_prim_opmap[0x13] = &Cpu::Cop3;
    m_prim_opmap[0x14] = &Cpu::BadOp;   m_prim_opmap[0x15] = &Cpu::BadOp;
    m_prim_opmap[0x16] = &Cpu::BadOp;   m_prim_opmap[0x17] = &Cpu::BadOp;
    m_prim_opmap[0x18] = &Cpu::BadOp;   m_prim_opmap[0x19] = &Cpu::BadOp;
    m_prim_opmap[0x1a] = &Cpu::BadOp;   m_prim_opmap[0x1b] = &Cpu::BadOp;
    m_prim_opmap[0x1c] = &Cpu::BadOp;   m_prim_opmap[0x1d] = &Cpu::BadOp;
    m_prim_opmap[0x1e] = &Cpu::BadOp;   m_prim_opmap[0x1f] = &Cpu::BadOp;

    m_prim_opmap[0x20] = &Cpu::Lb;      m_prim_opmap[0x21] = &Cpu::Lh;
    m_prim_opmap[0x22] = &Cpu::Lwl;     m_prim_opmap[0x23] = &Cpu::Lw;
    m_prim_opmap[0x24] = &Cpu::Lbu;     m_prim_opmap[0x25] = &Cpu::Lhu;
    m_prim_opmap[0x26] = &Cpu::Lwr;     m_prim_opmap[0x27] = &Cpu::BadOp;
    m_prim_opmap[0x28] = &Cpu::Sb;      m_prim_opmap[0x29] = &Cpu::Sh;
    m_prim_opmap[0x2a] = &Cpu::Swl;     m_prim_opmap[0x2b] = &Cpu::Sw;
    m_prim_opmap[0x2c] = &Cpu::BadOp;   m_prim_opmap[0x2d] = &Cpu::BadOp;
    m_prim_opmap[0x2e] = &Cpu::Swr;     m_prim_opmap[0x2f] = &Cpu::BadOp;

    m_prim_opmap[0x30] = &Cpu::LwC0;    m_prim_opmap[0x31] = &Cpu::LwC1;
    m_prim_opmap[0x32] = &Cpu::LwC2;    m_prim_opmap[0x33] = &Cpu::LwC3;
    m_prim_opmap[0x34] = &Cpu::BadOp;   m_prim_opmap[0x35] = &Cpu::BadOp;
    m_prim_opmap[0x36] = &Cpu::BadOp;   m_prim_opmap[0x37] = &Cpu::BadOp;
    m_prim_opmap[0x38] = &Cpu::SwC0;    m_prim_opmap[0x39] = &Cpu::SwC1;
    m_prim_opmap[0x3a] = &Cpu::SwC2;    m_prim_opmap[0x3b] = &Cpu::SwC3;
    m_prim_opmap[0x3c] = &Cpu::BadOp;   m_prim_opmap[0x3d] = &Cpu::BadOp;
    m_prim_opmap[0x3e] = &Cpu::BadOp;   m_prim_opmap[0x3f] = &Cpu::BadOp;

    // Secondary Op Encoding
    m_sec_opmap[0x00] = &Cpu::Sll;     m_sec_opmap[0x01] = &Cpu::BadOp;
    m_sec_opmap[0x02] = &Cpu::Srl;     m_sec_opmap[0x03] = &Cpu::Sra;
    m_sec_opmap[0x04] = &Cpu::Sllv;    m_sec_opmap[0x05] = &Cpu::BadOp;
    m_sec_opmap[0x06] = &Cpu::Srlv;    m_sec_opmap[0x07] = &Cpu::Srav;
    m_sec_opmap[0x08] = &Cpu::Jr;      m_sec_opmap[0x09] = &Cpu::Jalr;
    m_sec_opmap[0x0a] = &Cpu::BadOp;   m_sec_opmap[0x0b] = &Cpu::BadOp;
    m_sec_opmap[0x0c] = &Cpu::Syscall; m_sec_opmap[0x0d] = &Cpu::Break;
    m_sec_opmap[0x0e] = &Cpu::BadOp;   m_sec_opmap[0x0f] = &Cpu::BadOp;

    m_sec_opmap[0x10] = &Cpu::Mfhi;    m_sec_opmap[0x11] = &Cpu::Mthi;
    m_sec_opmap[0x12] = &Cpu::Mflo;    m_sec_opmap[0x13] = &Cpu::Mtlo;
    m_sec_opmap[0x14] = &Cpu::BadOp;   m_sec_opmap[0x15] = &Cpu::BadOp;
    m_sec_opmap[0x16] = &Cpu::BadOp;   m_sec_opmap[0x17] = &Cpu::BadOp;
    m_sec_opmap[0x18] = &Cpu::Mult;    m_sec_opmap[0x19] = &Cpu::Multu;
    m_sec_opmap[0x1a] = &Cpu::Div;     m_sec_opmap[0x1b] = &Cpu::Divu;
    m_sec_opmap[0x1c] = &Cpu::BadOp;   m_sec_opmap[0x1d] = &Cpu::BadOp;
    m_sec_opmap[0x1e] = &Cpu::BadOp;   m_sec_opmap[0x1f] = &Cpu::BadOp;

    m_sec_opmap[0x20] = &Cpu::Add;     m_sec_opmap[0x21] = &Cpu::Addu;
    m_sec_opmap[0x22] = &Cpu::Sub;     m_sec_opmap[0x23] = &Cpu::Subu;
    m_sec_opmap[0x24] = &Cpu::And;     m_sec_opmap[0x25] = &Cpu::Or;
    m_sec_opmap[0x26] = &Cpu::Xor;     m_sec_opmap[0x27] = &Cpu::Nor;
    m_sec_opmap[0x28] = &Cpu::BadOp;   m_sec_opmap[0x29] = &Cpu::BadOp;
    m_sec_opmap[0x2a] = &Cpu::Slt;     m_sec_opmap[0x2b] = &Cpu::Sltu;
    m_sec_opmap[0x2c] = &Cpu::BadOp;   m_sec_opmap[0x2d] = &Cpu::BadOp;
    m_sec_opmap[0x2e] = &Cpu::BadOp;   m_sec_opmap[0x2f] = &Cpu::BadOp;

    m_sec_opmap[0x30] = &Cpu::BadOp;   m_sec_opmap[0x31] = &Cpu::BadOp;
    m_sec_opmap[0x32] = &Cpu::BadOp;   m_sec_opmap[0x33] = &Cpu::BadOp;
    m_sec_opmap[0x34] = &Cpu::BadOp;   m_sec_opmap[0x35] = &Cpu::BadOp;
    m_sec_opmap[0x36] = &Cpu::BadOp;   m_sec_opmap[0x37] = &Cpu::BadOp;
    m_sec_opmap[0x38] = &Cpu::BadOp;   m_sec_opmap[0x39] = &Cpu::BadOp;
    m_sec_opmap[0x3a] = &Cpu::BadOp;   m_sec_opmap[0x3b] = &Cpu::BadOp;
    m_sec_opmap[0x3c] = &Cpu::BadOp;   m_sec_opmap[0x3d] = &Cpu::BadOp;
    m_sec_opmap[0x3e] = &Cpu::BadOp;   m_sec_opmap[0x3f] = &Cpu::BadOp;

    // Bcondz Op encoding
    m_bcondz_opmap[0x00] = &Cpu::Bltz;
    m_bcondz_opmap[0x01] = &Cpu::Bgez;
    m_bcondz_opmap[0x10] = &Cpu::Bltzal;
    m_bcondz_opmap[0x11] = &Cpu::Bgezal;
}


//================================================
// Indirect Ops
//================================================
/*
 * Opcode = 0x00
 * Use Instruction.funct as the new op code. Most of these instructions are
 * ALU R-Type instructions.
 */
u8 Cpu::Special(const Asm::Instruction& instr)
{
    return (this->*m_sec_opmap[instr.funct])(instr);
}

/*
 * Opcode = 0x01
 * Use Instruction.bcondz_op as new op code. These instructions are branch compare
 * with zero register.
 */
u8 Cpu::Bcondz(const Asm::Instruction& instr)
{
    // check if in map
    auto iter = m_bcondz_opmap.find(instr.bcondz_op);
    if (iter != m_bcondz_opmap.end()) {
        // found!
        return (this->*iter->second)(instr);
    } else {
        return BadOp(instr);
    }
}


//================================================
// Special Ops
//================================================
/*
 * Unknown Instruction. Triggers a "Reserved Instruction Exception" (excode = 0x0a)
 */
u8 Cpu::BadOp(const Asm::Instruction& instr)
{
    CPU_WARN(PSX_FMT("Unknown CPU instruction: op[0x{:02}] funct[0x{:02}] bcondz[0x{:02x}]", instr.op, instr.funct, instr.bcondz_op));
    PSX_ASSERT(0);
    return 0;
}

/*
 */
u8 Cpu::Syscall(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
    return 0;
}

u8 Cpu::Break(const Asm::Instruction& instr)
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
u8 Cpu::Addi(const Asm::Instruction& instr)
{
    u32 signed_imm32 = signExtendTo32(instr.imm16);
    u32 rs = m_regs.r[instr.rs];
    u32 res = signed_imm32 + rs;
    if (overflowed(res, signed_imm32, rs)) {
        // TRAP!
        Exception ex;
        ex.type = Exception::Type::Overflow;
        m_cop0.RaiseException(ex);
        return 0; // no modification
    } else {
        m_regs.r[instr.rt] = res;
        return instr.rt;
    }
}

/*
 * Addition Immediate (no trap)
 * opcode = 0x09
 * Format: ADDIU rt, rs, (sign-extended)imm16
 * Add rs to imm16 and store in rt. Do NOT TRAP on Two's-complement overflow.
 */
u8 Cpu::Addiu(const Asm::Instruction& instr)
{
    u32 signed_imm32 = signExtendTo32(instr.imm16);
    m_regs.r[instr.rt] = signed_imm32 + m_regs.r[instr.rs];
    return instr.rt;
}

/*
 * Set on Less Than Immediate
 * opcode = 0x0a
 * Format: SLTI rt, rs, (sign-extended)imm16
 * Check if rs is less than imm16. TRAP on overflow.
 */
u8 Cpu::Slti(const Asm::Instruction& instr)
{
    u32 signed_imm32 = signExtendTo32(instr.imm16);
    u32 rs = m_regs.r[instr.rs];
    u32 twos = twosComplement(signed_imm32);
    u32 res =  rs + twos;
    if (overflowed(res, rs, twos)) {
        // TRAP!
        Exception ex;
        ex.type = Exception::Type::Overflow;
        m_cop0.RaiseException(ex);
        return 0; // no modification
    } else {
        m_regs.r[instr.rt] = res & 0x8000'0000 ? 1 : 0;
        return instr.rt;
    }
}

/*
 * Set on Less Than Immediate (no trap)
 * opcode = 0x0b
 * Format: SLTIU rt, rs, (sign-extended)imm16
 * Check if rs is less than imm16. Do NOT TRAP on overflow.
 */
u8 Cpu::Sltiu(const Asm::Instruction& instr)
{
    u32 signed_imm32 = signExtendTo32(instr.imm16);
    u32 res = m_regs.r[instr.rs] + twosComplement(signed_imm32);
    m_regs.r[instr.rt] = res & 0x8000'0000 ? 1 : 0;
    return instr.rt;
}

/*
 * Bitwise AND Immediate
 * opcode = 0x0c
 * Format: ANDI rt, rs, (zero-extended)imm16
 * AND rs and imm16
 */
u8 Cpu::Andi(const Asm::Instruction& instr)
{
    m_regs.r[instr.rt] = m_regs.r[instr.rs] & instr.imm16;
    return instr.rt;
}

/*
 * Bitwise OR Immediate
 * opcode = 0x0d
 * Format: ORI rt, rs, (zero-extended)imm16
 * OR rs and imm16
 */
u8 Cpu::Ori(const Asm::Instruction& instr)
{
    m_regs.r[instr.rt] = m_regs.r[instr.rs] | instr.imm16;
    return instr.rt;
}

/*
 * Bitwise XOR Immediate
 * opcode = 0x0e
 * Format: XORI rt, rs, (zero-extended)imm16
 * XOR rs and imm16
 */
u8 Cpu::Xori(const Asm::Instruction& instr)
{
    m_regs.r[instr.rt] = m_regs.r[instr.rs] ^ instr.imm16;
    return instr.rt;
}

/*
 * Load Upper Immediate
 * opcode = 0x0f
 * Format: LUI rt, (zero-extended)imm16
 * Load imm16 into the upper 16 bits of rt.
 */
u8 Cpu::Lui(const Asm::Instruction& instr)
{
    m_regs.r[instr.rt] = (u32) (instr.imm16 << 16);
    return instr.rt;
}

// *** Three Operand Register-Type Ops ***
/*
 * Addition
 * funct = 0x20
 * Format: ADD rd, rs, rt
 * Add rs to rt and store in rd. TRAP on overflow.
 */
u8 Cpu::Add(const Asm::Instruction& instr)
{
    u32 rt = m_regs.r[instr.rt];
    u32 rs = m_regs.r[instr.rs];
    u32 res = rs + rt;
    if (overflowed(res, rt, rs)) {
        // TRAP!
        Exception ex;
        ex.type = Exception::Type::Overflow;
        m_cop0.RaiseException(ex);
        return 0;
    } else {
        m_regs.r[instr.rd] = res;
        return instr.rd;
    }
}

/*
 * Addition (no trap)
 * funct = 0x21
 * Format: ADDU rd, rs, rt
 * Add rs to rt and store in rd. Do NOT TRAP on overflow.
 */
u8 Cpu::Addu(const Asm::Instruction& instr)
{
    m_regs.r[instr.rd] = m_regs.r[instr.rs] + m_regs.r[instr.rt];
    return instr.rd;
}

/*
 * Subtraction
 * funct = 0x22
 * Format: SUB rd, rs, rt
 * Sub rt from rs and store in rd. TRAP on overflow.
 */
u8 Cpu::Sub(const Asm::Instruction& instr)
{
    u32 rt = twosComplement(m_regs.r[instr.rt]);
    u32 rs = m_regs.r[instr.rs];
    u32 res = rs + rt;
    if (overflowed(res, rt, rs)) {
        // TRAP!
        Exception ex;
        ex.type = Exception::Type::Overflow;
        m_cop0.RaiseException(ex);
        return 0; // no modification
    } else {
        m_regs.r[instr.rd] = res;
        return instr.rd;
    }
}

/*
 * Subtraction (no trap)
 * funct = 0x23
 * Format: SUBU rd, rs, rt
 * Sub rt from rs and store in rd. Do NOT TRAP on overflow.
 */
u8 Cpu::Subu(const Asm::Instruction& instr)
{
    m_regs.r[instr.rd] = m_regs.r[instr.rs] + twosComplement(m_regs.r[instr.rt]);
    return instr.rd;
}

/*
 * Set on Less Than
 * funct = 0x2a
 * Format: SLT rd, rs, rt
 * Set rd to 1 if rs less than rt. TRAP on overflow.
 */
u8 Cpu::Slt(const Asm::Instruction& instr)
{
    u32 rt = m_regs.r[instr.rt];
    u32 rs = m_regs.r[instr.rs];
    u32 twos = twosComplement(rt);
    u32 res =  rs + twos;
    if (overflowed(res, rs, twos)) {
        // TRAP!
        Exception ex;
        ex.type = Exception::Type::Overflow;
        m_cop0.RaiseException(ex);
        return 0; // no modification
    } else {
        m_regs.r[instr.rd] = res & 0x8000'0000 ? 1 : 0;
        return instr.rd;
    }
}

/*
 * Set on Less Than (no trap)
 * funct = 0x2b
 * Format: SLTU rd, rs, rt
 * Set rd to 1 if rs less than rt.
 */
u8 Cpu::Sltu(const Asm::Instruction& instr)
{
    u32 res = m_regs.r[instr.rs] + twosComplement(m_regs.r[instr.rt]);
    m_regs.r[instr.rd] = res & 0x8000'0000 ? 1 : 0;
    return instr.rd;
}

/*
 * Bitwise AND
 * funct = 0x24
 * Format: AND rd, rs, rt
 * AND rs and rt, store in rd.
 */
u8 Cpu::And(const Asm::Instruction& instr)
{
    m_regs.r[instr.rd] = m_regs.r[instr.rs] & m_regs.r[instr.rt];
    return instr.rd;
}

/*
 * Bitwise OR
 * funct = 0x25
 * Format: OR rd, rs, rt
 * OR rs and rt, store in rd.
 */
u8 Cpu::Or(const Asm::Instruction& instr)
{
    m_regs.r[instr.rd] = m_regs.r[instr.rs] | m_regs.r[instr.rt];
    return instr.rd;
}

/*
 * Bitwise XOR
 * funct = 0x26
 * Format: XOR rd, rs, rt
 * XOR rs and rt, store in rd.
 */
u8 Cpu::Xor(const Asm::Instruction& instr)
{
    m_regs.r[instr.rd] = m_regs.r[instr.rs] ^ m_regs.r[instr.rt];
    return instr.rd;
}

/*
 * Bitwise NOR
 * funct = 0x27
 * Format: NOR rd, rs, rt
 * NOR rs and rt, store in rd.
 */
u8 Cpu::Nor(const Asm::Instruction& instr)
{
    m_regs.r[instr.rd] = ~(m_regs.r[instr.rs] | m_regs.r[instr.rt]);
    return instr.rd;
}

// *** Shift Operations ***
/*
 * Shift Left Logical
 * funct = 0x00
 * Format: SLL rd, rt, shamt
 * Shift rt left by shamt, inserting zeroes into low order bits. Store in rd.
 */
u8 Cpu::Sll(const Asm::Instruction& instr)
{
    m_regs.r[instr.rd] = m_regs.r[instr.rt] << instr.shamt;
    return instr.rd;
}

/*
 * Shift Right Logical
 * funct = 0x02
 * Format: SRL rd, rt, shamt
 * Shift rt right by shamt, inserting zeroes into high order bits. Store in rd.
 */
u8 Cpu::Srl(const Asm::Instruction& instr)
{
    m_regs.r[instr.rd] = m_regs.r[instr.rt] >> instr.shamt;
    return instr.rd;
}

/*
 * Shift Right Arithmetic
 * funct = 0x03
 * Format: SRA rd, rt, shamt
 * Shift rt right by shamt, keeping sign of highest order bit. Store in rd.
 */
u8 Cpu::Sra(const Asm::Instruction& instr)
{
    bool msb = (m_regs.r[instr.rt] & 0x8000'0000) != 0;
    u32 upper_bits = (0xffff'ffff << (32 - instr.shamt));
    m_regs.r[instr.rd] = m_regs.r[instr.rt] >> instr.shamt;
    if (msb) {
        m_regs.r[instr.rd] |= upper_bits;
    }
    return instr.rd;
}

/*
 * Shift Left Logical Variable
 * funct = 0x04
 * Format: SLLV rd, rt, rs
 * Shift rt left by rs, inserting zeroes into low order bits. Store in rd.
 */
u8 Cpu::Sllv(const Asm::Instruction& instr)
{
    m_regs.r[instr.rd] = m_regs.r[instr.rt] << m_regs.r[instr.rs];
    return instr.rd;
}

/*
 * Shift Right Logical Variable
 * funct = 0x06
 * Format: SRLV rd, rt, rs
 * Shift rt Right by rs, inserting zeroes into high order bits. Store in rd.
 */
u8 Cpu::Srlv(const Asm::Instruction& instr)
{
    m_regs.r[instr.rd] = m_regs.r[instr.rt] >> m_regs.r[instr.rs];
    return instr.rd;
}

/*
 * Shift Right Arithmetic Variable
 * funct = 0x07
 * Format: SRAV rd, rt, rs
 * Shift rt Right by rs, keeping sign of highest order bit. Store in rd.
 */
u8 Cpu::Srav(const Asm::Instruction& instr)
{
    bool msb = (m_regs.r[instr.rt] & 0x8000'0000) != 0;
    u32 rs = m_regs.r[instr.rs] & 0x1f; // only 5 lsb
    u32 upper_bits = (0xffff'ffff << (32 - rs));
    m_regs.r[instr.rd] = m_regs.r[instr.rt] >> rs;
    if (msb) {
        m_regs.r[instr.rd] |= upper_bits;
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
u8 Cpu::Mult(const Asm::Instruction& instr)
{
    i64 rs64 = static_cast<i64>(signExtendTo64(m_regs.r[instr.rs]));
    i64 rt64 = static_cast<i64>(signExtendTo64(m_regs.r[instr.rt]));
    i64 res = rs64 * rt64;
    m_regs.hi = (res >> 32) & 0xffff'ffff;
    m_regs.lo = res & 0xffff'ffff;
    return 0;
}

/*
 * Multiply Unsigned
 * funct = 0x19
 * Format: MULTU rs, rt
 * Multiply rs and rt together as unsigned values and store 64 bit result 
 * into HI/LO registers.
 */
u8 Cpu::Multu(const Asm::Instruction& instr)
{
    u64 rs64 = static_cast<u64>(m_regs.r[instr.rs]);
    u64 rt64 = static_cast<u64>(m_regs.r[instr.rt]);
    u64 res = rs64 * rt64;
    m_regs.hi = (res >> 32) & 0xffff'ffff;
    m_regs.lo = res & 0xffff'ffff;
    return 0;
}

/*
 * Division
 * funct = 0x1a
 * Format: DIV rs, rt
 * Divide rs by rt as signed values and store quotient in LO and remainder in HI.
 */
u8 Cpu::Div(const Asm::Instruction& instr)
{
    i32 rs = static_cast<i32>(m_regs.r[instr.rs]);
    i32 rt = static_cast<i32>(m_regs.r[instr.rt]);
    if (rt != 0 && (rt != -1 && static_cast<u32>(rt) != 0x8000'0000)) {
        // normal case
        m_regs.hi = static_cast<u32>(rs % rt);
        m_regs.lo = static_cast<u32>(rs / rt);
    } else if (rt == 0) {
        // divide by zero special case
        m_regs.hi = static_cast<u32>(rs);
        if (rs >= 0) {
            m_regs.lo = static_cast<u32>(-1);
        } else {
            m_regs.lo = static_cast<u32>(1);
        }
    } else if (rt == -1 && static_cast<u32>(rs) == 0x8000'0000) {
        // special case
        m_regs.hi = 0;
        m_regs.lo = 0x8000'0000;
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
u8 Cpu::Divu(const Asm::Instruction& instr)
{
    u32 rs = m_regs.r[instr.rs];
    u32 rt = m_regs.r[instr.rt];
    if (rt != 0) {
        // normal case
        m_regs.hi = static_cast<u32>(rs % rt);
        m_regs.lo = static_cast<u32>(rs / rt);
    } else {
        m_regs.hi = static_cast<u32>(rs);
        m_regs.lo = 0xffff'ffff;
    }
    return 0;
}

/*
 * Move From HI
 * funct = 0x10
 * Format: MFHI rd
 * Move contents of Register HI to rd.
 */
u8 Cpu::Mfhi(const Asm::Instruction& instr)
{
    m_regs.r[instr.rd] = m_regs.hi;
    return 0;
}

/*
 * Move From LO
 * funct = 0x12
 * Format: MFLO rd
 * Move contents of Register LO to rd.
 */
u8 Cpu::Mflo(const Asm::Instruction& instr)
{
    m_regs.r[instr.rd] = m_regs.lo;
    return 0;
}

/*
 * Move To HI
 * funct = 0x11
 * Format: MTHI rd
 * Move contents of rd to Register HI
 */
u8 Cpu::Mthi(const Asm::Instruction& instr)
{
    m_regs.hi = m_regs.r[instr.rd];
    return 0;
}

/*
 * Move To LO
 * funct = 0x13
 * Format: MTLO rd
 * Move contents of rd to Register LO
 */
u8 Cpu::Mtlo(const Asm::Instruction& instr)
{
    m_regs.lo = m_regs.r[instr.rd];
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
u8 Cpu::Lb(const Asm::Instruction& instr)
{
    u32 addr = signExtendTo32(instr.imm16) + m_regs.r[instr.rs];
    u8 byte = m_bus->Read8(addr);
    // Load Delay
    m_lds.is_primed = true;
    m_lds.reg = instr.rt;
    m_lds.val = signExtendTo32(byte);
    return 0;
}

/*
 * Load Byte Unsigned
 * op = 0x24
 * Format: LBU rt, imm(rs)
 * Load byte from address at imm + rs and store into rt (zero extended).
 */
u8 Cpu::Lbu(const Asm::Instruction& instr)
{
    u32 addr = signExtendTo32(instr.imm16) + m_regs.r[instr.rs];
    u8 byte = m_bus->Read8(addr);
    // Load Delay!
    m_lds.is_primed = true;
    m_lds.reg = instr.rt;
    m_lds.val = static_cast<u32>(byte);
    return 0;
}

/*
 * Load Halfword
 * op = 0x21
 * Format: LH rt, imm(rs)
 * Load halfword from address at imm + rs and store into rt (sign extended).
 */
u8 Cpu::Lh(const Asm::Instruction& instr)
{
    u32 addr = signExtendTo32(instr.imm16) + m_regs.r[instr.rs];
    // check alignment, needs to be 2-aligned
    if (addr & 0x1) {
        Cpu::Exception e;
        e.type = Cpu::Exception::Type::AddrErrLoad;
        m_cop0.RaiseException(e);
    } else {
        u16 halfword = m_bus->Read16(addr);
        // Load Delay!
        m_lds.is_primed = true;
        m_lds.reg = instr.rt;
        m_lds.val = signExtendTo32(halfword);
    }
    return 0;
}

/*
 * Load Halfword Unsigned
 * op = 0x25
 * Format: LHU rt, imm(rs)
 * Load halfword from address at imm + rs and store into rt (zero extended).
 */
u8 Cpu::Lhu(const Asm::Instruction& instr)
{
    u32 addr = signExtendTo32(instr.imm16) + m_regs.r[instr.rs];
    // check alignment, needs to be 2-aligned
    if (addr & 0x1) {
        Cpu::Exception e;
        e.type = Cpu::Exception::Type::AddrErrLoad;
        m_cop0.RaiseException(e);
    } else {
        u16 halfword = m_bus->Read16(addr);
        // Load Delay!
        m_lds.is_primed = true;
        m_lds.reg = instr.rt;
        m_lds.val = static_cast<u32>(halfword);
    }
    return 0;
}

/*
 * Load Word
 * op = 0x23
 * Format: LW rt, imm(rs)
 * Load word from address at imm + rs and store into rt.
 */
u8 Cpu::Lw(const Asm::Instruction& instr)
{
    u32 addr = signExtendTo32(instr.imm16) + m_regs.r[instr.rs];
    // check alignment, needs to be 4-aligned
    if (addr & 0x3) {
        Cpu::Exception e;
        e.type = Cpu::Exception::Type::AddrErrLoad;
        m_cop0.RaiseException(e);
    } else {
        // Load Delay!
        m_lds.is_primed = true;
        m_lds.reg = instr.rt;
        m_lds.val = m_bus->Read32(addr);
    }
    return 0;
}

/*
 * Load Word Left
 * op = 0x22
 * Format: LWL rt, imm(rs)
 * Merge unaligned data (up to word boundary) into the MSB of the target register.
 */
u8 Cpu::Lwl(const Asm::Instruction& instr)
{
    u32 addr = signExtendTo32(instr.imm16) + m_regs.r[instr.rs];
    u32 shift_to_align = (addr & 0x3) << 3; // mult by 8
    // read from 4-aligned addr
    u32 data = m_bus->Read32(addr & ~0x3u);
    // if the previous instruction was a load instruction, we need to grab the value
    // from the load delay slot.
    u32 rt = m_lds.was_primed && m_lds.reg == instr.rt ? m_lds.val : m_regs.r[instr.rt];
    m_lds.val = (rt & (0x00ff'ffff >> shift_to_align))
              | (data << (24 - shift_to_align));
    m_lds.reg = instr.rt;
    m_lds.is_primed = true;
    return 0;
}

/*
 * Load Word Right
 * op = 0x26
 * Format: LWR rt, imm(rs)
 * Merge unaligned data (down to word boundary) into LSB of the target register.
 */
u8 Cpu::Lwr(const Asm::Instruction& instr)
{
    u32 addr = signExtendTo32(instr.imm16) + m_regs.r[instr.rs];
    u32 shift_to_align = (addr & 0x3) << 3; // mult by 8
    // read from 4-aligned addr
    u32 data = m_bus->Read32(addr & ~0x3u);
    // if the previous instruction was a load instruction, we need to grab the value
    // from the load delay slot.
    u32 rt = m_lds.was_primed && m_lds.reg == instr.rt ? m_lds.val : m_regs.r[instr.rt];
    u32 reg_mask = 0xffff'ff00 << (24 - shift_to_align);
    m_lds.val = (rt & reg_mask) | (data >> shift_to_align);
    m_lds.reg = instr.rt;
    m_lds.is_primed = true;
    return 0;
}

// *** Store ***
/*
 * Store Byte
 * op = 0x28
 * Format: SB rt, imm(rs)
 * Store the least significant byte in rt at address imm + base.
 */
u8 Cpu::Sb(const Asm::Instruction& instr)
{
    u32 addr = signExtendTo32(instr.imm16) + m_regs.r[instr.rs];
    u8 rt = m_regs.r[instr.rt] & 0xff;
    m_bus->Write8(rt, addr);
    return 0;
}

/*
 * Store Halfword
 * op = 0x29
 * Format: SH rt, imm(rs)
 * Store the least significant halfword in rt at address imm + base.
 */
u8 Cpu::Sh(const Asm::Instruction& instr)
{
    u32 addr = signExtendTo32(instr.imm16) + m_regs.r[instr.rs];
    // check if 2-aligned
    if (addr & 0x1) {
        // not aligned, throw exception
        Cpu::Exception e;
        e.type = Cpu::Exception::Type::AddrErrStore;
        m_cop0.RaiseException(e);
    } else {
        u16 rt = m_regs.r[instr.rt] & 0xffff;
        m_bus->Write16(rt, addr);
    }
    return 0;
}

/*
 * Store Word
 * op = 0x2b
 * Format: SW rt, imm(rs)
 * Store the word in rt at address imm + base.
 */
u8 Cpu::Sw(const Asm::Instruction& instr)
{
    u32 addr = signExtendTo32(instr.imm16) + m_regs.r[instr.rs];
    if (addr & 0x3) {
        // not aligned, throw exception
        Cpu::Exception e;
        e.type = Cpu::Exception::Type::AddrErrStore;
        m_cop0.RaiseException(e);
    } else {
        u32 rt = m_regs.r[instr.rt];
        m_bus->Write32(rt, addr);
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
u8 Cpu::Swl(const Asm::Instruction& instr)
{
    u32 addr = signExtendTo32(instr.imm16) + m_regs.r[instr.rs];
    u32 shift_to_align = (addr & 0x3) << 3; // mult by 8
    // read from 4-aligned addr
    u32 old_data = m_bus->Read32(addr & ~0x3u);
    u32 rt = m_regs.r[instr.rt];
    u32 mask = (0xffff'ff00 << shift_to_align);
    u32 new_data = (old_data & mask) | (rt >> (24 - shift_to_align));
    m_bus->Write32(new_data, addr & ~0x3u);
    return 0;
}

/*
 * Store Word Right
 * op = 0x2e
 * Format: SWR rt, imm(rs)
 * Merge the word stored in rt into the unaligned address up to the word
 * boundary.
 */
u8 Cpu::Swr(const Asm::Instruction& instr)
{
    u32 addr = signExtendTo32(instr.imm16) + m_regs.r[instr.rs];
    u32 shift_to_align = (addr & 0x3) << 3; // mult by 8
    // read from 4-aligned addr
    u32 old_data = m_bus->Read32(addr & ~0x3u);
    u32 rt = m_regs.r[instr.rt];
    u32 mask = (0x00ff'ffff >> (24 - shift_to_align));
    u32 new_data = (old_data & mask) | (rt << shift_to_align);
    m_bus->Write32(new_data, addr & ~0x3u);
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
u8 Cpu::J(const Asm::Instruction& instr)
{
    u32 target = instr.target << 2;
    target |= (m_regs.pc & 0xf000'0000);
    m_bds.pc = target;
    m_bds.is_primed = true;
    m_bds.take_branch = true;
    return 0;
}

/*
 * Jump and Link
 * op = 0x03
 * Format: JAL target
 * Jump to Target and store instruction following delay into R31.
 */
u8 Cpu::Jal(const Asm::Instruction& instr)
{
    u32 target = instr.target << 2;
    target |= (m_regs.pc & 0xf000'0000);
    m_regs.r[31] = m_regs.pc + 8; // return addr
    m_bds.pc = target;
    m_bds.is_primed = true;
    m_bds.take_branch = true;
    return 31;
}

/*
 * Jump Register
 * funct = 0x08
 * Format: JR rs
 * Jump to the address stored in rs.
 */
u8 Cpu::Jr(const Asm::Instruction& instr)
{
    u32 target = m_regs.r[instr.rs];
    // check 4-alignment
    if (target & 0x3) {
        Cpu::Exception e;
        e.type = Cpu::Exception::Type::AddrErrLoad;
        e.badv = target;
        m_cop0.RaiseException(e);
    } else {
        m_bds.pc = target;
        m_bds.take_branch = true;
    }
    m_bds.is_primed = true;
    return 0;
}

/*
 * Jump Register and Link
 * funct = 0x09
 * Format: JRAL rs, rd
 * Jump to the address stored in rs.
 */
u8 Cpu::Jalr(const Asm::Instruction& instr)
{
    u32 target = m_regs.r[instr.rs];
    // check 4-alignment
    if (target & 0x3) {
        Cpu::Exception e;
        e.type = Cpu::Exception::Type::AddrErrLoad;
        e.badv = target;
        m_cop0.RaiseException(e);
    } else {
        m_bds.pc = target;
        m_regs.r[31] = m_regs.r[instr.rd];
        m_bds.take_branch = true;
    }
    m_bds.is_primed = true;
    return 0;
}

// *** Branch instructions ***
/*
 * Branch if Equal
 * op = 0x04
 * Format: BEQ rs, rt, offset
 * Branch to pc + offset if rs equals rt.
 */
u8 Cpu::Beq(const Asm::Instruction& instr)
{
    if (m_regs.r[instr.rs] == m_regs.r[instr.rt]) {
        m_bds.take_branch = true;
        u32 offset = signExtendTo32(instr.imm16) << 2;
        m_bds.pc = m_regs.pc + offset;
    }
    m_bds.is_primed = true;
    return 0;
}

/*
 * Branch if Not Equal
 * op = 0x05
 * Format: BNE rs, rt, offset
 * Branch to pc + offset if rs does not equals rt.
 */
u8 Cpu::Bne(const Asm::Instruction& instr)
{
    if (m_regs.r[instr.rs] != m_regs.r[instr.rt]) {
        u32 offset = signExtendTo32(instr.imm16) << 2;
        m_bds.pc = m_regs.pc + offset;
        m_bds.take_branch = true;
    }
    m_bds.is_primed = true;
    return 0;
}

/*
 * Branch if less than or equal to zero.
 * op = 0x06
 * Format: BLEZ rs, offset
 * Branch to pc + offset if rs greater than zero.
 */
u8 Cpu::Blez(const Asm::Instruction& instr)
{
    i32 rs = static_cast<i32>(m_regs.r[instr.rs]);
    if (rs <= 0) {
        m_bds.take_branch = true;
        u32 offset = signExtendTo32(instr.imm16) << 2;
        m_bds.pc = m_regs.pc + offset;
    }
    m_bds.is_primed = true;
    return 0;
}

/*
 * Branch if greater than 0
 * op = 0x07
 * Format: BGTZ rs, offset
 * Branch to pc + offset if rs greater than zero.
 */
u8 Cpu::Bgtz(const Asm::Instruction& instr)
{
    i32 rs = static_cast<i32>(m_regs.r[instr.rs]);
    if (rs > 0) {
        m_bds.take_branch = true;
        u32 offset = signExtendTo32(instr.imm16) << 2;
        m_bds.pc = m_regs.pc + offset;
    }
    m_bds.is_primed = true;
    return 0;
}

// *** bcondz ***
/*
 * Branch if less than 0
 * bcondz = 0x00
 * Format: BLTZ rs, offset
 * Branch to pc + offset if rs less than zero.
 */
u8 Cpu::Bltz(const Asm::Instruction& instr)
{
    i32 rs = static_cast<i32>(m_regs.r[instr.rs]);
    if (rs < 0) {
        m_bds.take_branch = true;
        u32 offset = signExtendTo32(instr.imm16) << 2;
        m_bds.pc = m_regs.pc + offset;
    }
    m_bds.is_primed = true;
    return 0;
}

/*
 * Branch if greater than or equal to 0
 * bcondz = 0x01
 * Format: BGEZ rs, offset
 * Branch to pc + offset if rs greater than or equal to zero.
 */
u8 Cpu::Bgez(const Asm::Instruction& instr)
{
    i32 rs = static_cast<i32>(m_regs.r[instr.rs]);
    if (rs >= 0) {
        m_bds.take_branch = true;
        u32 offset = signExtendTo32(instr.imm16) << 2;
        m_bds.pc = m_regs.pc + offset;
    }
    m_bds.is_primed = true;
    return 0;
}

/*
 * Branch if less then zero and link
 * bcondz = 0x10
 * Format: BLTZAL rs, offset
 * Branch to pc + offset if rs less than zero and link.
 */
u8 Cpu::Bltzal(const Asm::Instruction& instr)
{
    i32 rs = static_cast<i32>(m_regs.r[instr.rs]);
    if (rs < 0) {
        m_bds.take_branch = true;
        u32 offset = signExtendTo32(instr.imm16) << 2;
        m_bds.pc = m_regs.pc + offset;
        m_regs.r[31] = m_regs.pc + 8;
    }
    m_bds.is_primed = true;
    return 0;
}

/*
 * Branch if greater or equal to zero and link
 * bcondz = 0x10
 * Format: BGEZAL rs, offset
 * Branch to pc + offset if rs greater or equal to zero and link.
 */
u8 Cpu::Bgezal(const Asm::Instruction& instr)
{
    i32 rs = static_cast<i32>(m_regs.r[instr.rs]);
    if (rs >= 0) {
        m_bds.take_branch = true;
        u32 offset = signExtendTo32(instr.imm16) << 2;
        m_bds.pc = m_regs.pc + offset;
        m_regs.r[31] = m_regs.pc + 8;
    }
    m_bds.is_primed = true;
    return 0;
}

//================================================
// Co-Processor Instructions
//================================================
// *** Cop General ***
u8 Cpu::Cop0(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
    return 0;
}

u8 Cpu::Cop1(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
    return 0;
}

u8 Cpu::Cop2(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
    return 0;
}

u8 Cpu::Cop3(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
    return 0;
}

// *** Cop Loads ***
u8 Cpu::LwC0(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
    return 0;
}

u8 Cpu::LwC1(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
    return 0;
}

u8 Cpu::LwC2(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
    return 0;
}

u8 Cpu::LwC3(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
    return 0;
}

// *** Cop Store ***
u8 Cpu::SwC0(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
    return 0;
}

u8 Cpu::SwC1(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
    return 0;
}

u8 Cpu::SwC2(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
    return 0;
}

u8 Cpu::SwC3(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
    return 0;
}

