/*
 * cpu.cpp
 *
 * Travis Banken
 * 12/13/2020
 *
 * CPU for the PSX. The Playstation uses a R3000A MIPS processor.
 */

#include <random>

#include "imgui/imgui.h"

#include "cpu/cpu.h"
#include "core/globals.h"

#define CPU_INFO(msg) PSXLOG_INFO("CPU", msg)
#define CPU_WARN(msg) PSXLOG_WARN("CPU", msg)
#define CPU_ERROR(msg) PSXLOG_ERROR("CPU", msg)

// simple constructor
Cpu::Cpu(std::shared_ptr<Bus> bus)
    : m_bus(bus)
{
    CPU_INFO("Initializing CPU");
    buildOpMaps();
}

/*
 * Execute one instruction. On average, takes 1 psx clock cycle.
 */
void Cpu::Step()
{
    // fetch instruction
    u32 raw_instr = m_bus->Read32(m_regs.pc);

    // decode
    Asm::Instruction instr = Asm::DecodeRawInstr(raw_instr);

    // execute
    (this->*m_prim_opmap[instr.op])(instr);

    // increment pc
    m_regs.pc += 4;

    // zero register should always be zero
    m_regs.r[0] = 0;
}

/*
 * Set the cpu's program counter to the specified address.
 */
void Cpu::SetPC(u32 addr)
{
    m_regs.pc = addr;
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

    u32 prePC = pc_region > m_regs.pc ? 0 : m_regs.pc - pc_region;
    u32 postPC = m_regs.pc + pc_region;


    //-------------------------------------
    // Instruction Disassembly
    //-------------------------------------
    ImGuiWindowFlags dasm_window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    ImGui::BeginGroup();
    ImGui::BeginChild("Instruction Disassembly", ImVec2(300,0), false, dasm_window_flags);
    ImGui::TextUnformatted("Instruction Disassembly");
    for (u32 addr = prePC; addr <= postPC; addr += 4) {
        u32 instr = m_bus->Read32(addr);
        if (addr == m_regs.pc) {
            ImGui::TextUnformatted(PSX_FMT("{:08x}  | {}", addr, Asm::DasmInstruction(instr)).data());
        } else {
            ImGui::TextColored(ImVec4(0.6f,0.6f,0.6f,1), "%08x  | %s", addr, 
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
void Cpu::Special(const Asm::Instruction& instr)
{
    (this->*m_sec_opmap[instr.funct])(instr);
}

/*
 * Opcode = 0x01
 * Use Instruction.bcondz_op as new op code. These instructions are branch compare
 * with zero register.
 */
void Cpu::Bcondz(const Asm::Instruction& instr)
{
    // check if in map
    auto iter = m_bcondz_opmap.find(instr.bcondz_op);
    if (iter != m_bcondz_opmap.end()) {
        // found!
        (this->*iter->second)(instr);
    } else {
        BadOp(instr);
    }
}


//================================================
// Special Ops
//================================================
/*
 * Unknown Instruction. Triggers a "Reserved Instruction Exception" (excode = 0x0a)
 */
void Cpu::BadOp(const Asm::Instruction& instr)
{
    CPU_WARN(PSX_FMT("Unknown CPU instruction: op[0x{:02}] funct[0x{:02}] bcondz[0x{:02x}]", instr.op, instr.funct, instr.bcondz_op));
    PSX_ASSERT(0);
    (void) instr;
}

/*
 */
void Cpu::Syscall(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::Break(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

//================================================
// Computational Instructions
//================================================
// *** Immediate ALU Ops ***
void Cpu::Addi(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::Addiu(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::Slti(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::Sltiu(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::Andi(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::Ori(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::Xori(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::Lui(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

// *** Three Operand Register-Type Ops ***
void Cpu::Add(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::Addu(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::Sub(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::Subu(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::Slt(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::Sltu(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::And(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::Or(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::Xor(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::Nor(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

// *** Shift Operations ***
void Cpu::Sll(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::Srl(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::Sra(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::Sllv(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::Srlv(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::Srav(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

// *** Multiply and Divide Operations ***
void Cpu::Mult(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::Multu(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::Div(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::Divu(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::Mfhi(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::Mflo(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::Mthi(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::Mtlo(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

//================================================
// Load and Store Instructions
//================================================
// *** Load ***
void Cpu::Lb(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::Lbu(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::Lh(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::Lhu(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::Lw(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::Lwl(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::Lwr(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

// *** Store ***
void Cpu::Sb(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::Sh(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::Sw(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::Swl(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::Swr(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

//================================================
// Jump and Branch Instructions
//================================================
// *** Jump instructions ***
void Cpu::J(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::Jal(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::Jr(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::Jalr(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

// *** Branch instructions ***
void Cpu::Beq(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::Bne(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::Blez(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::Bgtz(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

// *** bcondz ***
void Cpu::Bltz(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::Bgez(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::Bltzal(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::Bgezal(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

//================================================
// Co-Processor Instructions
//================================================
// *** Cop General ***
void Cpu::Cop0(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::Cop1(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::Cop2(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::Cop3(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

// *** Cop Loads ***
void Cpu::LwC0(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::LwC1(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::LwC2(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::LwC3(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

// *** Cop Store ***
void Cpu::SwC0(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::SwC1(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::SwC2(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

void Cpu::SwC3(const Asm::Instruction& instr)
{
    PSX_ASSERT(0);
    (void) instr;
}

