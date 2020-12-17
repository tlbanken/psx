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
}

/*
 * Execute one instruction. Should take about 1 clock cycle.
 */
void Cpu::step()
{
    // fetch instruction
    u32 instr = m_bus->read32(m_regs.pc);

    // decode

    // execute
    // Do some random writes to registers
    u32 val = (u32) rand() & 0xffff'ffff;
    u32 reg = (u32) rand() % 32;
    m_regs.r[reg] = val;


    // increment pc
    m_regs.pc += 4;

    // zero register should always be zero
    m_regs.r[0] = 0;
}

/*
 * Set the cpu's program counter to the specified address.
 */
void Cpu::setPC(u32 addr)
{
    m_regs.pc = addr;
}

/*
 * Update function for ImGui.
 */
void Cpu::onActive(bool *active)
{
    const u32 pcRegion = (10 << 2);

    if (!ImGui::Begin("CPU Debug", active)) {
        ImGui::End();
        return;
    }

    //-------------------------------------
    // Step Buttons
    //-------------------------------------
    if (!g_emuState.paused) {
        if (ImGui::Button("Pause")) {
            CPU_INFO("Pausing Emulation");
            // Pause Emulation
            g_emuState.paused = true;
        }
    } else {
        if (ImGui::Button("Continue")) {
            CPU_INFO("Continuing Emulation");
            // Play Emulation
            g_emuState.paused = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Step")) {
            // Step Mode
            g_emuState.stepInstr = true;
        }
    }

    //-------------------------------------

    u32 prePC = pcRegion > m_regs.pc ? 0 : m_regs.pc - pcRegion;
    u32 postPC = m_regs.pc + pcRegion;


    //-------------------------------------
    // Instruction Disassembly
    //-------------------------------------
    ImGuiWindowFlags dasmWindowFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    ImGui::BeginGroup();
    ImGui::BeginChild("Instruction Disassembly", ImVec2(300,0), false, dasmWindowFlags);
    ImGui::TextUnformatted("Instruction Disassembly");
    for (u32 addr = prePC; addr <= postPC; addr += 4) {
        u32 instr = m_bus->read32(addr);
        if (addr == m_regs.pc) {
            ImGui::TextUnformatted(PSX_FMT("{:08x}  | {}", addr, Asm::dasmInstruction(instr)).data());
        } else {
            ImGui::TextColored(ImVec4(0.6f,0.6f,0.6f,1), "%08x  | %s", addr, 
                Asm::dasmInstruction(instr).data());
        }
    }
    ImGui::EndChild();
    //-------------------------------------

    ImGui::SameLine();

    //-------------------------------------
    // Registers
    //-------------------------------------
    // if (ImGui::BeginChild("Registers", ImVec2(0,0), true, dasmWindowFlags)) {
    ImGui::BeginChild("Registers", ImVec2(0,0), false, dasmWindowFlags);
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
std::string Cpu::getModuleLabel()
{
    return "CPU";
}
