/*
 * psxtest_cpu.cpp
 *
 * Travis Banken
 * 12/21/2020
 *
 * Tests for the CPU on the PSX.
 */

#include <iostream>
#include <memory>

#include "util/psxutil.h"
#include "util/psxlog.h"
#include "cpu/cop0.h"
#include "cpu/cpu.h"
#include "mem/bus.h"

#define TCPU_INFO(msg) PSXLOG_INFO("Test-CPU", msg)
#define TCPU_WARN(msg) PSXLOG_WARN("Test-CPU", msg)
#define TCPU_ERROR(msg) PSXLOG_ERROR("Test-CPU", msg)

static void aluiTests()
{
    TCPU_INFO("Starting ALU Immediate Tests");
    // setup hardware
    std::shared_ptr<SysControl> cop0;
    std::shared_ptr<Bus> bus;
    Cpu cpu(bus, cop0);

    std::string instr;
    //========================
    // addi
    //========================
    instr = "ADDI R1 R0 13";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(1) == 13);
    instr = "ADDI R2 R1 20";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(2) == 33);
    instr = "ADDI R3 R1 0xffff"; // sub 1
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(3) == 12);
    instr = "ADDI R3 R1 0xffff"; // sub 1
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(3) == 12);

    //========================
    // addiu
    //========================
    cpu.Reset();
    instr = "ADDIU R1 R0 10";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(1) == 10);
    instr = "ADDIU R2 R1 20";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(2) == 30);
    instr = "ADDIU R3 R1 0xffff"; // sub 1
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(3) == 9);
    instr = "ADDIU R3 R1 -1"; // sub 1
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(3) == 9);

    //========================
    // slti
    //========================
    cpu.Reset();
    instr = "SLTI R1 R0 1";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(1) == 1);
    instr = "SLTI R2 R1 1";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(2) == 0);
    instr = "SLTI R3 R1 0";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(3) == 0);

    //========================
    // slti
    //========================
    cpu.Reset();
    instr = "SLTIU R1 R0 1";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(1) == 1);
    instr = "SLTIU R2 R1 1";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(2) == 0);
    instr = "SLTIU R3 R1 0";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(3) == 0);

    //========================
    // andi
    //========================
    cpu.Reset();
    instr = "ADDI R1 R0 0x1";
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(1) == 1);
    instr = "ANDI R2 R1 0x1";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(2) == 0x1);
    instr = "ANDI R3 R0 0x1";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(3) == 0x0);
    instr = "ANDI R4 R1 0xffff";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(4) == 0x1);

    //========================
    // ori
    //========================
    cpu.Reset();
    instr = "ADDI R1 R0 0x1";
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(1) == 1);
    instr = "ORI R2 R1 0x1";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(2) == 0x1);
    instr = "ORI R3 R0 0x1";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(3) == 0x1);
    instr = "ORI R4 R1 0xffff";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(4) == 0xffff);
    instr = "ORI R5 R0 0x0";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(5) == 0x0);

    //========================
    // xori
    //========================
    cpu.Reset();
    instr = "ADDI R1 R0 0x1";
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(1) == 1);
    instr = "XORI R2 R1 0x1";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(2) == 0x0);
    instr = "XORI R3 R1 0x1";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(3) == 0x0);
    instr = "XORI R4 R1 0xffff";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(4) == 0xfffe);
    instr = "XORI R5 R0 0x0";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(5) == 0x0);

    //========================
    // lui
    //========================
    cpu.Reset();
    instr = "LUI R1 0x0000";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(1) == 0x0);
    instr = "LUI R2 0xffff";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(2) == 0xffff'0000);
    instr = "LUI R2 0xbeef";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(2) == 0xbeef'0000);
}

static void alurTests()
{
    TCPU_INFO("Starting ALU R-Type Instruction Tests");
}

namespace psxtest {
    void CpuTests()
    {
        std::cout << PSX_FANCYTITLE("CPU TESTS");
        aluiTests();
        alurTests();
    }
}