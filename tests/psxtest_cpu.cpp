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
#include "cpu/cpu.h"
#include "mem/bus.h"

#define TCPU_INFO(msg) PSXLOG_INFO("Test-CPU", msg)
#define TCPU_WARN(msg) PSXLOG_WARN("Test-CPU", msg)
#define TCPU_ERROR(msg) PSXLOG_ERROR("Test-CPU", msg)

static void aluiTests()
{
    TCPU_INFO("Starting ALU Immediate Tests");
    // setup hardware
    std::shared_ptr<Bus> bus(new Bus());
    Cpu cpu(bus);

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
    // setup hardware
    std::shared_ptr<Bus> bus(new Bus);
    Cpu cpu(bus);
    
    std::string instr;
    //========================
    // ADD
    //========================
    cpu.Reset();
    cpu.SetR(1, 10);
    instr = "ADD R2 R1 R1";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(2) == 20);
    // test trap
    cpu.SetR(3, 0x7fff'ffff);
    instr = "ADD R4 R3 R1";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(4) == 0);

    //========================
    // ADDU
    //========================
    cpu.Reset();
    cpu.SetR(1, 1);
    instr = "ADDU R2 R1 R1";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(2) == 2);
    instr = "ADDU R3 R0 R2";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(3) == 2);
    // test no trap
    cpu.SetR(4, 0x7fff'ffff);
    instr = "ADDU R5 R4 R1";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(5) == 0x8000'0000);

    //========================
    // SUB
    //========================
    cpu.Reset();
    cpu.SetR(1, 1);
    instr = "SUB R2 R0 R1";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(2) == 0xffff'ffff);
    cpu.SetR(3, 10);
    cpu.SetR(4, 7);
    instr = "SUB R5 R3 R4";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(5) == 3);
    // test trap
    cpu.SetR(6, 0x8000'0000);
    instr = "SUB R7 R6 R1";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(7) == 0);

    //========================
    // SUBU
    //========================
    cpu.Reset();
    cpu.SetR(1, 1);
    instr = "SUBU R2 R0 R1";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(2) == 0xffff'ffff);
    cpu.SetR(3, 10);
    cpu.SetR(4, 7);
    instr = "SUBU R5 R3 R4";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(5) == 3);
    // test no trap
    cpu.SetR(6, 0x8000'0000);
    instr = "SUBU R7 R6 R1";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(7) == 0x7fff'ffff);


    //========================
    // SLT
    //========================
    cpu.Reset();
    cpu.SetR(1, 10);
    cpu.SetR(2, 5);
    cpu.SetR(3, 20);
    cpu.SetR(4, 0xffff'ffff);
    instr = "SLT R5 R1 R1";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(5) == 0);
    instr = "SLT R5 R1 R2";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(5) == 0);
    instr = "SLT R5 R1 R3";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(5) == 1);
    instr = "SLT R5 R1 R4";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(5) == 0);
    instr = "SLT R5 R4 R2";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(5) == 1);
    // test TRAP
    cpu.SetR(6, 0x8000'0000);
    instr = "SLT R7 R6 R1";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(7) == 0);

    //========================
    // SLT
    //========================
    cpu.Reset();
    cpu.SetR(1, 10);
    cpu.SetR(2, 5);
    cpu.SetR(3, 20);
    cpu.SetR(4, 0xffff'ffff);
    instr = "SLTU R5 R1 R1";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(5) == 0);
    instr = "SLTU R5 R1 R2";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(5) == 0);
    instr = "SLTU R5 R1 R3";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(5) == 1);
    instr = "SLTU R5 R1 R4";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(5) == 0);
    instr = "SLTU R5 R4 R2";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(5) == 1);
    // test no trap
    cpu.SetR(6, 0x8000'0000);
    instr = "SLTU R7 R6 R1";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(7) == 0);

    //========================
    // AND
    //========================
    cpu.Reset();
    cpu.SetR(1, 0xff);
    cpu.SetR(2, 0xaa);
    instr = "AND R3 R1 R2";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(3) == 0xaa);
    cpu.SetR(3, 0x00);
    instr = "AND R4 R3 R2";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(4) == 0x00);

    //========================
    // OR
    //========================
    cpu.Reset();
    cpu.SetR(1, 0xff);
    cpu.SetR(2, 0xaa);
    instr = "OR R3 R1 R2";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(3) == 0xff);
    cpu.SetR(3, 0x00);
    instr = "OR R4 R3 R2";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(4) == 0xaa);

    //========================
    // XOR
    //========================
    cpu.Reset();
    cpu.SetR(1, 0xff);
    cpu.SetR(2, 0xaa);
    instr = "XOR R3 R1 R2";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(3) == (0xaa ^ 0xff));
    instr = "XOR R4 R3 R3";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(4) == 0x00);

    //========================
    // NOR
    //========================
    cpu.Reset();
    cpu.SetR(1, 0xff);
    cpu.SetR(2, 0xaa);
    instr = "NOR R3 R1 R2";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(3) == (u32) ~(0xaa | 0xff));
    instr = "NOR R4 R1 R1";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(4) == 0xffff'ff00);
}

static void shiftTests()
{
    TCPU_INFO("Starting Shift Tests");
    std::string instr;
    // setup hardware
    std::shared_ptr<Bus> bus(new Bus);
    Cpu cpu(bus);

    //========================
    // SLL
    //========================
    cpu.Reset();
    cpu.SetR(1, 1);
    instr = "SLL R2, R1, 1";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(2) == 2);
    instr = "SLL R3, R2, 2";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(3) == 8);
    instr = "SLL R4, R3, 0";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(4) == 8);

    //========================
    // SRL
    //========================
    cpu.Reset();
    cpu.SetR(1, 8);
    instr = "SRL R2, R1, 1";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(2) == 4);
    instr = "SRL R3, R2, 2";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(3) == 1);
    cpu.SetR(4, 0x8000'0000);
    instr = "SRL R5, R4, 16";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(5) == 0x0000'8000);

    //========================
    // SRA
    //========================
    cpu.Reset();
    cpu.SetR(1, 8);
    instr = "SRA R2, R1, 1";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(2) == 4);
    instr = "SRA R3, R2, 2";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(3) == 1);
    cpu.SetR(4, 0x8000'0000);
    instr = "SRA R5, R4, 16";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(5) == 0xffff'8000);

    //========================
    // SLLV
    //========================
    cpu.Reset();
    cpu.SetR(1, 1);
    cpu.SetR(10, 1);
    instr = "SLLV R2, R1, R10";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(2) == 2);
    cpu.SetR(10, 2);
    instr = "SLLV R3, R2, R10";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(3) == 8);
    cpu.SetR(10, 0);
    instr = "SLLV R4, R3, R10";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(4) == 8);

    //========================
    // SRLV
    //========================
    cpu.Reset();
    cpu.SetR(1, 8);
    cpu.SetR(10, 1);
    instr = "SRLV R2, R1, R10";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(2) == 4);
    cpu.SetR(10, 2);
    instr = "SRLV R3, R2, R10";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(3) == 1);
    cpu.SetR(4, 0x8000'0000);
    cpu.SetR(10, 16);
    instr = "SRLV R5, R4, R10";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(5) == 0x0000'8000);

    //========================
    // SRAV
    //========================
    cpu.Reset();
    cpu.SetR(1, 8);
    cpu.SetR(10, 1);
    instr = "SRAV R2, R1, R10";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(2) == 4);
    cpu.SetR(10, 2);
    instr = "SRAV R3, R2, R10";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(3) == 1);
    cpu.SetR(4, 0x8000'0000);
    cpu.SetR(10, 16);
    instr = "SRAV R5, R4, R10";
    TCPU_INFO(instr);
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(5) == 0xffff'8000);
}

namespace psxtest {
    void CpuTests()
    {
        std::cout << PSX_FANCYTITLE("CPU TESTS");
        aluiTests();
        alurTests();
        shiftTests();
    }
}
