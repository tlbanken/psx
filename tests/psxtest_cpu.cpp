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
#include "mem/ram.h"

#define TCPU_INFO(msg) PSXLOG_INFO("Test-CPU", msg)
#define TCPU_WARN(msg) PSXLOG_WARN("Test-CPU", msg)
#define TCPU_ERROR(msg) PSXLOG_ERROR("Test-CPU", msg)

// helper macros
#define EXE_INSTR(instr) TCPU_INFO(instr); cpu.ExecuteInstruction(Asm::AsmInstruction(instr))
#define EXE_LW_INSTR(instr) \
{\
    TCPU_INFO(instr);\
    bus->Write32(Asm::AsmInstruction(instr), 0x100);\
    bus->Write32(0, 0x104);\
    cpu.SetPC(0x100);\
    cpu.Step();\
    cpu.Step();\
}

#define EXE_TWO_INSTRS(i1, i2) \
{\
    TCPU_INFO(PSX_FMT("{} -> {}", i1, i2));\
    bus->Write32(Asm::AsmInstruction(i1), 0x1000);\
    bus->Write32(Asm::AsmInstruction(i2), 0x1004);\
    cpu.SetPC(0x1000);\
    cpu.Step();\
    cpu.Step();\
}

#define EXE_BD_INSTR(i) \
    EXE_INSTR(i);\
    cpu.Step();



static void aluiTests()
{
    TCPU_INFO("** Starting ALU Immediate Tests -----------------------");
    // setup hardware
    std::shared_ptr<Bus> bus(new Bus());
    Cpu cpu(bus);

    std::string instr;
    //========================
    // addi
    //========================
    instr = "ADDI R1 R0 13";
    EXE_INSTR(instr);
    assert(cpu.GetR(1) == 13);
    instr = "ADDI R2 R1 20";
    EXE_INSTR(instr);
    assert(cpu.GetR(2) == 33);
    instr = "ADDI R3 R1 0xffff"; // sub 1
    EXE_INSTR(instr);
    assert(cpu.GetR(3) == 12);
    instr = "ADDI R3 R1 0xffff"; // sub 1
    EXE_INSTR(instr);
    assert(cpu.GetR(3) == 12);

    //========================
    // addiu
    //========================
    cpu.Reset();
    instr = "ADDIU R1 R0 10";
    EXE_INSTR(instr);
    assert(cpu.GetR(1) == 10);
    instr = "ADDIU R2 R1 20";
    EXE_INSTR(instr);
    assert(cpu.GetR(2) == 30);
    instr = "ADDIU R3 R1 0xffff"; // sub 1
    EXE_INSTR(instr);
    assert(cpu.GetR(3) == 9);
    instr = "ADDIU R3 R1 -1"; // sub 1
    EXE_INSTR(instr);
    assert(cpu.GetR(3) == 9);

    //========================
    // slti
    //========================
    cpu.Reset();
    instr = "SLTI R1 R0 1";
    EXE_INSTR(instr);
    assert(cpu.GetR(1) == 1);
    instr = "SLTI R2 R1 1";
    EXE_INSTR(instr);
    assert(cpu.GetR(2) == 0);
    instr = "SLTI R3 R1 0";
    EXE_INSTR(instr);
    assert(cpu.GetR(3) == 0);

    //========================
    // slti
    //========================
    cpu.Reset();
    instr = "SLTIU R1 R0 1";
    EXE_INSTR(instr);
    assert(cpu.GetR(1) == 1);
    instr = "SLTIU R2 R1 1";
    EXE_INSTR(instr);
    assert(cpu.GetR(2) == 0);
    instr = "SLTIU R3 R1 0";
    EXE_INSTR(instr);
    assert(cpu.GetR(3) == 0);

    //========================
    // andi
    //========================
    cpu.Reset();
    instr = "ADDI R1 R0 0x1";
    cpu.ExecuteInstruction(Asm::AsmInstruction(instr));
    assert(cpu.GetR(1) == 1);
    instr = "ANDI R2 R1 0x1";
    EXE_INSTR(instr);
    assert(cpu.GetR(2) == 0x1);
    instr = "ANDI R3 R0 0x1";
    EXE_INSTR(instr);
    assert(cpu.GetR(3) == 0x0);
    instr = "ANDI R4 R1 0xffff";
    EXE_INSTR(instr);
    assert(cpu.GetR(4) == 0x1);

    //========================
    // ori
    //========================
    cpu.Reset();
    instr = "ADDI R1 R0 0x1";
    EXE_INSTR(instr);
    assert(cpu.GetR(1) == 1);
    instr = "ORI R2 R1 0x1";
    EXE_INSTR(instr);
    assert(cpu.GetR(2) == 0x1);
    instr = "ORI R3 R0 0x1";
    EXE_INSTR(instr);
    assert(cpu.GetR(3) == 0x1);
    instr = "ORI R4 R1 0xffff";
    EXE_INSTR(instr);
    assert(cpu.GetR(4) == 0xffff);
    instr = "ORI R5 R0 0x0";
    EXE_INSTR(instr);
    assert(cpu.GetR(5) == 0x0);

    //========================
    // xori
    //========================
    cpu.Reset();
    instr = "ADDI R1 R0 0x1";
    EXE_INSTR(instr);
    assert(cpu.GetR(1) == 1);
    instr = "XORI R2 R1 0x1";
    EXE_INSTR(instr);
    assert(cpu.GetR(2) == 0x0);
    instr = "XORI R3 R1 0x1";
    EXE_INSTR(instr);
    assert(cpu.GetR(3) == 0x0);
    instr = "XORI R4 R1 0xffff";
    EXE_INSTR(instr);
    assert(cpu.GetR(4) == 0xfffe);
    instr = "XORI R5 R0 0x0";
    EXE_INSTR(instr);
    assert(cpu.GetR(5) == 0x0);

    //========================
    // lui
    //========================
    cpu.Reset();
    instr = "LUI R1 0x0000";
    EXE_INSTR(instr);
    assert(cpu.GetR(1) == 0x0);
    instr = "LUI R2 0xffff";
    EXE_INSTR(instr);
    assert(cpu.GetR(2) == 0xffff'0000);
    instr = "LUI R2 0xbeef";
    EXE_INSTR(instr);
    assert(cpu.GetR(2) == 0xbeef'0000);
}

static void alurTests()
{
    TCPU_INFO("** Starting ALU R-Type Instruction Tests --------------");
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
    EXE_INSTR(instr);
    assert(cpu.GetR(2) == 20);
    // test trap
    cpu.SetR(3, 0x7fff'ffff);
    instr = "ADD R4 R3 R1";
    EXE_INSTR(instr);
    assert(cpu.GetR(4) == 0);

    //========================
    // ADDU
    //========================
    cpu.Reset();
    cpu.SetR(1, 1);
    instr = "ADDU R2 R1 R1";
    EXE_INSTR(instr);
    assert(cpu.GetR(2) == 2);
    instr = "ADDU R3 R0 R2";
    EXE_INSTR(instr);
    assert(cpu.GetR(3) == 2);
    // test no trap
    cpu.SetR(4, 0x7fff'ffff);
    instr = "ADDU R5 R4 R1";
    EXE_INSTR(instr);
    assert(cpu.GetR(5) == 0x8000'0000);

    //========================
    // SUB
    //========================
    cpu.Reset();
    cpu.SetR(1, 1);
    instr = "SUB R2 R0 R1";
    EXE_INSTR(instr);
    assert(cpu.GetR(2) == 0xffff'ffff);
    cpu.SetR(3, 10);
    cpu.SetR(4, 7);
    instr = "SUB R5 R3 R4";
    EXE_INSTR(instr);
    assert(cpu.GetR(5) == 3);
    // test trap
    cpu.SetR(6, 0x8000'0000);
    instr = "SUB R7 R6 R1";
    EXE_INSTR(instr);
    assert(cpu.GetR(7) == 0);

    //========================
    // SUBU
    //========================
    cpu.Reset();
    cpu.SetR(1, 1);
    instr = "SUBU R2 R0 R1";
    EXE_INSTR(instr);
    assert(cpu.GetR(2) == 0xffff'ffff);
    cpu.SetR(3, 10);
    cpu.SetR(4, 7);
    instr = "SUBU R5 R3 R4";
    EXE_INSTR(instr);
    assert(cpu.GetR(5) == 3);
    // test no trap
    cpu.SetR(6, 0x8000'0000);
    instr = "SUBU R7 R6 R1";
    EXE_INSTR(instr);
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
    EXE_INSTR(instr);
    assert(cpu.GetR(5) == 0);
    instr = "SLT R5 R1 R2";
    EXE_INSTR(instr);
    assert(cpu.GetR(5) == 0);
    instr = "SLT R5 R1 R3";
    EXE_INSTR(instr);
    assert(cpu.GetR(5) == 1);
    instr = "SLT R5 R1 R4";
    EXE_INSTR(instr);
    assert(cpu.GetR(5) == 0);
    instr = "SLT R5 R4 R2";
    EXE_INSTR(instr);
    assert(cpu.GetR(5) == 1);
    // test TRAP
    cpu.SetR(6, 0x8000'0000);
    instr = "SLT R7 R6 R1";
    EXE_INSTR(instr);
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
    EXE_INSTR(instr);
    assert(cpu.GetR(5) == 0);
    instr = "SLTU R5 R1 R2";
    EXE_INSTR(instr);
    assert(cpu.GetR(5) == 0);
    instr = "SLTU R5 R1 R3";
    EXE_INSTR(instr);
    assert(cpu.GetR(5) == 1);
    instr = "SLTU R5 R1 R4";
    EXE_INSTR(instr);
    assert(cpu.GetR(5) == 0);
    instr = "SLTU R5 R4 R2";
    EXE_INSTR(instr);
    assert(cpu.GetR(5) == 1);
    // test no trap
    cpu.SetR(6, 0x8000'0000);
    instr = "SLTU R7 R6 R1";
    EXE_INSTR(instr);
    assert(cpu.GetR(7) == 0);

    //========================
    // AND
    //========================
    cpu.Reset();
    cpu.SetR(1, 0xff);
    cpu.SetR(2, 0xaa);
    instr = "AND R3 R1 R2";
    EXE_INSTR(instr);
    assert(cpu.GetR(3) == 0xaa);
    cpu.SetR(3, 0x00);
    instr = "AND R4 R3 R2";
    EXE_INSTR(instr);
    assert(cpu.GetR(4) == 0x00);

    //========================
    // OR
    //========================
    cpu.Reset();
    cpu.SetR(1, 0xff);
    cpu.SetR(2, 0xaa);
    instr = "OR R3 R1 R2";
    EXE_INSTR(instr);
    assert(cpu.GetR(3) == 0xff);
    cpu.SetR(3, 0x00);
    instr = "OR R4 R3 R2";
    EXE_INSTR(instr);
    assert(cpu.GetR(4) == 0xaa);

    //========================
    // XOR
    //========================
    cpu.Reset();
    cpu.SetR(1, 0xff);
    cpu.SetR(2, 0xaa);
    instr = "XOR R3 R1 R2";
    EXE_INSTR(instr);
    assert(cpu.GetR(3) == (0xaa ^ 0xff));
    instr = "XOR R4 R3 R3";
    EXE_INSTR(instr);
    assert(cpu.GetR(4) == 0x00);

    //========================
    // NOR
    //========================
    cpu.Reset();
    cpu.SetR(1, 0xff);
    cpu.SetR(2, 0xaa);
    instr = "NOR R3 R1 R2";
    EXE_INSTR(instr);
    assert(cpu.GetR(3) == (u32) ~(0xaa | 0xff));
    instr = "NOR R4 R1 R1";
    EXE_INSTR(instr);
    assert(cpu.GetR(4) == 0xffff'ff00);
}

static void shiftTests()
{
    TCPU_INFO("** Starting Shift Tests -------------------------------");
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
    EXE_INSTR(instr);
    assert(cpu.GetR(2) == 2);
    instr = "SLL R3, R2, 2";
    EXE_INSTR(instr);
    assert(cpu.GetR(3) == 8);
    instr = "SLL R4, R3, 0";
    EXE_INSTR(instr);
    assert(cpu.GetR(4) == 8);

    //========================
    // SRL
    //========================
    cpu.Reset();
    cpu.SetR(1, 8);
    instr = "SRL R2, R1, 1";
    EXE_INSTR(instr);
    assert(cpu.GetR(2) == 4);
    instr = "SRL R3, R2, 2";
    EXE_INSTR(instr);
    assert(cpu.GetR(3) == 1);
    cpu.SetR(4, 0x8000'0000);
    instr = "SRL R5, R4, 16";
    EXE_INSTR(instr);
    assert(cpu.GetR(5) == 0x0000'8000);

    //========================
    // SRA
    //========================
    cpu.Reset();
    cpu.SetR(1, 8);
    instr = "SRA R2, R1, 1";
    EXE_INSTR(instr);
    assert(cpu.GetR(2) == 4);
    instr = "SRA R3, R2, 2";
    EXE_INSTR(instr);
    assert(cpu.GetR(3) == 1);
    cpu.SetR(4, 0x8000'0000);
    instr = "SRA R5, R4, 16";
    EXE_INSTR(instr);
    assert(cpu.GetR(5) == 0xffff'8000);

    //========================
    // SLLV
    //========================
    cpu.Reset();
    cpu.SetR(1, 1);
    cpu.SetR(10, 1);
    instr = "SLLV R2, R1, R10";
    EXE_INSTR(instr);
    assert(cpu.GetR(2) == 2);
    cpu.SetR(10, 2);
    instr = "SLLV R3, R2, R10";
    EXE_INSTR(instr);
    assert(cpu.GetR(3) == 8);
    cpu.SetR(10, 0);
    instr = "SLLV R4, R3, R10";
    EXE_INSTR(instr);
    assert(cpu.GetR(4) == 8);

    //========================
    // SRLV
    //========================
    cpu.Reset();
    cpu.SetR(1, 8);
    cpu.SetR(10, 1);
    instr = "SRLV R2, R1, R10";
    EXE_INSTR(instr);
    assert(cpu.GetR(2) == 4);
    cpu.SetR(10, 2);
    instr = "SRLV R3, R2, R10";
    EXE_INSTR(instr);
    assert(cpu.GetR(3) == 1);
    cpu.SetR(4, 0x8000'0000);
    cpu.SetR(10, 16);
    instr = "SRLV R5, R4, R10";
    EXE_INSTR(instr);
    assert(cpu.GetR(5) == 0x0000'8000);

    //========================
    // SRAV
    //========================
    cpu.Reset();
    cpu.SetR(1, 8);
    cpu.SetR(10, 1);
    instr = "SRAV R2, R1, R10";
    EXE_INSTR(instr);
    assert(cpu.GetR(2) == 4);
    cpu.SetR(10, 2);
    instr = "SRAV R3, R2, R10";
    EXE_INSTR(instr);
    assert(cpu.GetR(3) == 1);
    cpu.SetR(4, 0x8000'0000);
    cpu.SetR(10, 16);
    instr = "SRAV R5, R4, R10";
    EXE_INSTR(instr);
    assert(cpu.GetR(5) == 0xffff'8000);
}

void hiloTests()
{
    TCPU_INFO("** Starting HiLo Tests --------------------------------");
    std::string instr;
    // setup hardware
    std::shared_ptr<Bus> bus(new Bus);
    Cpu cpu(bus);
    u64 res = 0;

    //========================
    // MULT
    //========================
    cpu.Reset();
    cpu.SetR(1, 10);
    cpu.SetR(2, 3);
    instr = "MULT R1, R2";
    EXE_INSTR(instr);
    res = (static_cast<u64>(cpu.GetHI()) << 32) | cpu.GetLO();
    assert(res == 30);
    cpu.SetR(1, 0xffff'ffff); // -1
    cpu.SetR(2, 3);
    instr = "MULT R1, R2";
    EXE_INSTR(instr);
    res = (static_cast<u64>(cpu.GetHI()) << 32) | cpu.GetLO();
    assert(res == 0xffff'ffff'ffff'fffd); // -3
    instr = "MULT R0, R2";
    EXE_INSTR(instr);
    res = (static_cast<u64>(cpu.GetHI()) << 32) | cpu.GetLO();
    assert(res == 0);

    //========================
    // MULTU
    //========================
    cpu.Reset();
    cpu.SetR(1, 10);
    cpu.SetR(2, 3);
    instr = "MULTU R1, R2";
    EXE_INSTR(instr);
    res = (static_cast<u64>(cpu.GetHI()) << 32) | cpu.GetLO();
    assert(res == 30);
    cpu.SetR(1, 0xffff'ffff);
    cpu.SetR(2, 3);
    instr = "MULTU R1, R2";
    EXE_INSTR(instr);
    res = (static_cast<u64>(cpu.GetHI()) << 32) | cpu.GetLO();
    assert(res == 0x0000'0002'ffff'fffd); // since unsigned, NOT -3
    instr = "MULTU R0, R2";
    EXE_INSTR(instr);
    res = (static_cast<u64>(cpu.GetHI()) << 32) | cpu.GetLO();
    assert(res == 0);

    //========================
    // DIV
    //========================
    cpu.Reset();
    cpu.SetR(1, 24);
    cpu.SetR(2, 6);
    instr = "DIV R1, R2";
    EXE_INSTR(instr);
    assert(cpu.GetLO() == 4); // quotient
    assert(cpu.GetHI() == 0); // remainder
    cpu.SetR(1, 23);
    cpu.SetR(2, 6);
    instr = "DIV R1, R2";
    EXE_INSTR(instr);
    assert(cpu.GetLO() == 3); // quotient
    assert(cpu.GetHI() == 5); // remainder
    cpu.SetR(1, 23);
    cpu.SetR(2, 29);
    instr = "DIV R1, R2";
    EXE_INSTR(instr);
    assert(cpu.GetLO() == 0); // quotient
    assert(cpu.GetHI() == 23); // remainder
    cpu.SetR(1, (u32)-4);
    cpu.SetR(2, (u32)2);
    instr = "DIV R1, R2";
    EXE_INSTR(instr);
    assert(cpu.GetLO() == (u32)-2); // quotient
    assert(cpu.GetHI() == 0); // remainder
    // Divide positive num by 0
    cpu.SetR(1, 1);
    instr = "DIV R1, R0";
    EXE_INSTR(instr);
    assert(cpu.GetLO() == (u32)-1); // quotient
    assert(cpu.GetHI() == cpu.GetR(1)); // remainder
    // Divide negative num by 0
    cpu.SetR(1, (u32) -1);
    instr = "DIV R1, R0";
    EXE_INSTR(instr);
    assert(cpu.GetLO() == 0x1); // quotient
    assert(cpu.GetHI() == cpu.GetR(1)); // remainder
    // 0x8000'0000 / -1
    cpu.SetR(1, 0x8000'0000);
    cpu.SetR(2, (u32)-1);
    instr = "DIV R1, R2";
    EXE_INSTR(instr);
    assert(cpu.GetLO() == 0x8000'0000); // quotient
    assert(cpu.GetHI() == 0); // remainder

    //========================
    // DIVU
    //========================
    cpu.Reset();
    cpu.SetR(1, 24);
    cpu.SetR(2, 6);
    instr = "DIVU R1, R2";
    EXE_INSTR(instr);
    assert(cpu.GetLO() == 4); // quotient
    assert(cpu.GetHI() == 0); // remainder
    cpu.SetR(1, 23);
    cpu.SetR(2, 6);
    instr = "DIVU R1, R2";
    EXE_INSTR(instr);
    assert(cpu.GetLO() == 3); // quotient
    assert(cpu.GetHI() == 5); // remainder
    cpu.SetR(1, 23);
    cpu.SetR(2, 29);
    instr = "DIVU R1, R2";
    EXE_INSTR(instr);
    assert(cpu.GetLO() == 0); // quotient
    assert(cpu.GetHI() == 23); // remainder
    cpu.SetR(1, (u32)-4);
    cpu.SetR(2, (u32)2);
    instr = "DIVU R1, R2";
    EXE_INSTR(instr);
    assert(cpu.GetLO() == (u32)-4 >> 1); // quotient
    assert(cpu.GetHI() == 0); // remainder
    // Divide positive num by 0
    cpu.SetR(1, 1);
    instr = "DIVU R1, R0";
    EXE_INSTR(instr);
    assert(cpu.GetLO() == (u32)-1); // quotient
    assert(cpu.GetHI() == cpu.GetR(1)); // remainder

    //========================
    // Move HI/LO
    //========================
    cpu.Reset();
    // move to
    cpu.SetR(1, 10);
    instr = "MTHI R1";
    EXE_INSTR(instr);
    assert(cpu.GetHI() == cpu.GetR(1));
    cpu.SetR(1, 132);
    instr = "MTLO R1";
    EXE_INSTR(instr);
    assert(cpu.GetLO() == cpu.GetR(1));
    // move from
    cpu.SetHI(100);
    instr = "MFHI R1";
    EXE_INSTR(instr);
    assert(cpu.GetHI() == cpu.GetR(1));
    cpu.SetLO(9123);
    instr = "MFLO R1";
    EXE_INSTR(instr);
    assert(cpu.GetLO() == cpu.GetR(1));
}

static void loadTests()
{
    TCPU_INFO("** Starting LOAD Tests --------------------------------");
    std::string instr;
    // setup hardware
    std::shared_ptr<Bus> bus(new Bus);
    std::shared_ptr<Ram> ram(new Ram);
    bus->AddAddressSpace(ram, BusPriority::First);
    Cpu cpu(bus);

    //========================
    // LB
    //========================
    cpu.Reset();
    bus->Write8(0x7e, 0x0000'0000);
    EXE_LW_INSTR("LB R1 0 R0");
    assert(cpu.GetR(1) == 0x0000'0007e);
    bus->Write8(0xbe, 0x0000'0000);
    EXE_LW_INSTR("LB R1 0 R0");
    assert(cpu.GetR(1) == 0xffff'ffbe);
    bus->Write8(0x42, 0x0000'0001);
    cpu.SetR(2, 2);
    EXE_LW_INSTR("LB R1 -1 R2");
    assert(cpu.GetR(1) == 0x42);
    instr = "LB R2 1 R0";
    EXE_LW_INSTR("LB R2 1 R0");
    assert(cpu.GetR(2) == 0x42);

    //========================
    // LBU
    //========================
    cpu.Reset();
    bus->Write8(0x7e, 0x0000'0000);
    EXE_LW_INSTR("LBU R1 0 R0");
    assert(cpu.GetR(1) == 0x0000'0007e);
    bus->Write8(0xbe, 0x0000'0000);
    EXE_LW_INSTR("LBU R1 0 R0");
    assert(cpu.GetR(1) == 0x0000'00be);
    bus->Write8(0x42, 0x0000'0001);
    cpu.SetR(2, 2);
    EXE_LW_INSTR("LBU R1 -1 R2");
    assert(cpu.GetR(1) == 0x42);
    EXE_LW_INSTR("LBU R2 1 R0");
    assert(cpu.GetR(2) == 0x42);

    //========================
    // LH
    //========================
    cpu.Reset();
    bus->Write16(0x7bef, 0x0000'0000);
    EXE_LW_INSTR("LH R1 0 R0");
    assert(cpu.GetR(1) == 0x0000'7bef);
    bus->Write16(0x8bef, 0x0000'0000);
    EXE_LW_INSTR("LH R1 0 R0");
    assert(cpu.GetR(1) == 0xffff'8bef);
    bus->Write16(0x0042, 0x0000'0002);
    cpu.SetR(2, 3);
    EXE_LW_INSTR("LH R1 -1 R2");
    assert(cpu.GetR(1) == 0x42);
    EXE_LW_INSTR("LH R2 2 R0");
    assert(cpu.GetR(2) == 0x42);
    // test mis-align exception
    bus->Write16(0xbeef, 0x0000'0007);
    cpu.SetR(4, 7);
    EXE_LW_INSTR("LH R20 0 R4");
    assert(cpu.GetR(20) == 0x0000'0000);

    //========================
    // LHU
    //========================
    cpu.Reset();
    bus->Write16(0x7bef, 0x0000'0000);
    EXE_LW_INSTR("LHU R1 0 R0");
    assert(cpu.GetR(1) == 0x0000'7bef);
    bus->Write16(0x8bef, 0x0000'0000);
    EXE_LW_INSTR("LHU R1 0 R0");
    assert(cpu.GetR(1) == 0x0000'8bef);
    bus->Write16(0x0042, 0x0000'0002);
    cpu.SetR(2, 3);
    EXE_LW_INSTR("LHU R1 -1 R2");
    assert(cpu.GetR(1) == 0x42);
    EXE_LW_INSTR("LHU R2 2 R0");
    assert(cpu.GetR(2) == 0x42);
    // test mis-align exception
    bus->Write16(0xbeef, 0x0000'0007);
    cpu.SetR(4, 7);
    EXE_LW_INSTR("LHU R20 0 R4");
    assert(cpu.GetR(20) == 0x0000'0000);

    //========================
    // LW
    //========================
    cpu.Reset();
    bus->Write32(0xbeef'abee, 0x0000'0000);
    EXE_LW_INSTR("LW R1 0 R0");
    assert(cpu.GetR(1) == 0xbeef'abee);
    bus->Write32(0x90023e, 0x0000'0004);
    cpu.SetR(2, 5);
    EXE_LW_INSTR("LW R1 -1 R2");
    assert(cpu.GetR(1) == 0x90023e);
    cpu.SetR(2, 3);
    EXE_LW_INSTR("LW R1 1 R2");
    assert(cpu.GetR(1) == 0x90023e);
    // test mis-align exception
    cpu.SetR(5, 0xa);
    bus->Write32(0xbeef'0042, 0x0000'000a);
    EXE_LW_INSTR("LW R20 0 R5");
    assert(cpu.GetR(20) == 0x0000'0000);
    bus->Write32(0xbeef'0042, 0x0000'0009);
    EXE_LW_INSTR("LW R21 -1 R5");
    assert(cpu.GetR(21) == 0x0000'0000);

    //========================
    // LWL
    //========================
    cpu.Reset();
    bus->Write32(0xbeef'abee, 0x0000'0000);
    bus->Write32(0xffff'ffff, 0x0000'0004);
    EXE_LW_INSTR("LWL R1 0 R0");
    assert(cpu.GetR(1) == 0xee00'0000);
    EXE_LW_INSTR("LWL R2 1 R0");
    assert(cpu.GetR(2) == 0xabee'0000);
    EXE_LW_INSTR("LWL R3 2 R0");
    assert(cpu.GetR(3) == 0xefab'ee00);
    EXE_LW_INSTR("LWL R4 3 R0");
    assert(cpu.GetR(4) == 0xbeef'abee);
    // test merge
    cpu.SetR(1, 0xcccc'cccc);
    bus->Write32(0x3333'3333, 0x0000'0000);
    EXE_LW_INSTR("LWL R1 0 R0");
    assert(cpu.GetR(1) == 0x33cc'cccc);
    cpu.SetR(1, 0xcccc'cccc);
    EXE_LW_INSTR("LWL R1 1 R0");
    assert(cpu.GetR(1) == 0x3333'cccc);
    cpu.SetR(1, 0xcccc'cccc);
    EXE_LW_INSTR("LWL R1 2 R0");
    assert(cpu.GetR(1) == 0x3333'33cc);
    cpu.SetR(1, 0xcccc'cccc);
    EXE_LW_INSTR("LWL R1 3 R0");
    assert(cpu.GetR(1) == 0x3333'3333);

    //========================
    // LWR
    //========================
    cpu.Reset();
    bus->Write32(0xdead'beef, 0x0000'0000);
    bus->Write32(0xffff'ffff, 0x0000'0004);
    EXE_LW_INSTR("LWR R1 0 R0");
    assert(cpu.GetR(1) == 0xdead'beef);
    EXE_LW_INSTR("LWR R2 1 R0");
    assert(cpu.GetR(2) == 0x00de'adbe);
    EXE_LW_INSTR("LWR R3 2 R0");
    assert(cpu.GetR(3) == 0x0000'dead);
    EXE_LW_INSTR("LWR R4 3 R0");
    assert(cpu.GetR(4) == 0x0000'00de);
    // test merge
    cpu.SetR(1, 0xcccc'cccc);
    bus->Write32(0x3333'3333, 0x0000'0000);
    EXE_LW_INSTR("LWR R1 0 R0");
    assert(cpu.GetR(1) == 0x3333'3333);
    cpu.SetR(1, 0xcccc'cccc);
    EXE_LW_INSTR("LWR R1 1 R0");
    assert(cpu.GetR(1) == 0xcc33'3333);
    cpu.SetR(1, 0xcccc'cccc);
    EXE_LW_INSTR("LWR R1 2 R0");
    assert(cpu.GetR(1) == 0xcccc'3333);
    cpu.SetR(1, 0xcccc'cccc);
    EXE_LW_INSTR("LWR R1 3 R0");
    assert(cpu.GetR(1) == 0xcccc'cc33);
}

static void loadDelayTests()
{
    TCPU_INFO("** Starting Load-Delay Tests --------------------------");
    std::string instr;
    // setup hardware
    std::shared_ptr<Bus> bus(new Bus);
    std::shared_ptr<Ram> ram(new Ram);
    bus->AddAddressSpace(ram, BusPriority::First);
    Cpu cpu(bus);

    //========================
    // Load Delays (Standard)
    //========================
    cpu.Reset();
    bus->Write8(5, 0x0000'0000);
    EXE_TWO_INSTRS("LB R1 0 R0", "ADD R20 R0 R1");
    assert(cpu.GetR(20) == 0);
    assert(cpu.GetR(1) == 5);
    cpu.SetR(1, 0);
    EXE_TWO_INSTRS("LBU R1 0 R0", "ADD R20 R0 R1");
    assert(cpu.GetR(20) == 0);
    assert(cpu.GetR(1) == 5);
    cpu.SetR(1, 0);
    EXE_TWO_INSTRS("LH R1 0 R0", "ADD R20 R0 R1");
    assert(cpu.GetR(20) == 0);
    assert(cpu.GetR(1) == 5);
    cpu.SetR(1, 0);
    EXE_TWO_INSTRS("LHU R1 0 R0", "ADD R20 R0 R1");
    assert(cpu.GetR(20) == 0);
    assert(cpu.GetR(1) == 5);
    cpu.SetR(1, 0);
    EXE_TWO_INSTRS("LW R1 0 R0", "ADD R20 R0 R1");
    assert(cpu.GetR(20) == 0);
    assert(cpu.GetR(1) == 5);
    cpu.SetR(1, 0);
    EXE_TWO_INSTRS("LWR R1 0 R0", "ADD R20 R0 R1");
    assert(cpu.GetR(20) == 0);
    assert(cpu.GetR(1) == 5);
    cpu.SetR(1, 0);
    EXE_TWO_INSTRS("LWL R1 0 R0", "ADD R20 R0 R1");
    assert(cpu.GetR(20) == 0);
    cpu.SetR(1, 0);

    //========================
    // Load Delays (Next instruction writes to Load register)
    //========================
    cpu.Reset();
    cpu.SetR(1, 10);
    bus->Write8(5, 0x0000'0000);
    EXE_TWO_INSTRS("LB R20 0 R0", "ADD R20 R0 R1");
    assert(cpu.GetR(20) == 10); // ADD should win the race
    EXE_TWO_INSTRS("LBU R20 0 R0", "ADD R20 R0 R1");
    assert(cpu.GetR(20) == 10); // ADD should win the race
    EXE_TWO_INSTRS("LH R20 0 R0", "ADD R20 R0 R1");
    assert(cpu.GetR(20) == 10); // ADD should win the race
    EXE_TWO_INSTRS("LHU R20 0 R0", "ADD R20 R0 R1");
    assert(cpu.GetR(20) == 10); // ADD should win the race
    EXE_TWO_INSTRS("LW R20 0 R0", "ADD R20 R0 R1");
    assert(cpu.GetR(20) == 10); // ADD should win the race
    EXE_TWO_INSTRS("LWR R20 0 R0", "ADD R20 R0 R1");
    assert(cpu.GetR(20) == 10); // ADD should win the race
    EXE_TWO_INSTRS("LWL R20 0 R0", "ADD R20 R0 R1");
    assert(cpu.GetR(20) == 10); // ADD should win the race

    //========================
    // Load Delays (Two unaligned reads)
    //========================
    cpu.Reset();
    cpu.SetR(1, 0x1); // unaligned
    bus->Write32(0xdead'beef, 0x0000'0001);
    EXE_TWO_INSTRS("LWR R20 0 R1", "LWL R20 3 R1");
    assert(cpu.GetR(20) == 0xdead'beef);
    cpu.SetR(20, 0);
    EXE_TWO_INSTRS("LWL R20 3 R1", "LWR R20 0 R1");
    assert(cpu.GetR(20) == 0xdead'beef);
    cpu.SetR(1, 0x2); // unaligned
    bus->Write32(0xdead'beef, 0x0000'0002);
    EXE_TWO_INSTRS("LWR R20 0 R1", "LWL R20 3 R1");
    assert(cpu.GetR(20) == 0xdead'beef);
    cpu.SetR(20, 0);
    EXE_TWO_INSTRS("LWL R20 3 R1", "LWR R20 0 R1");
    assert(cpu.GetR(20) == 0xdead'beef);
    cpu.SetR(1, 0x3); // unaligned
    bus->Write32(0xdead'beef, 0x0000'0003);
    EXE_TWO_INSTRS("LWR R20 0 R1", "LWL R20 3 R1");
    assert(cpu.GetR(20) == 0xdead'beef);
    cpu.SetR(20, 0);
    EXE_TWO_INSTRS("LWL R20 3 R1", "LWR R20 0 R1");
    assert(cpu.GetR(20) == 0xdead'beef);
}

static void storeTests()
{
    TCPU_INFO("** Starting Store Instruction Tests -------------------");
    // setup hardware
    std::shared_ptr<Bus> bus(new Bus);
    std::shared_ptr<Ram> ram(new Ram);
    bus->AddAddressSpace(ram, BusPriority::First);
    Cpu cpu(bus);

    //========================
    // SB
    //========================
    cpu.Reset();
    cpu.SetR(1, 125);
    EXE_INSTR("SB R1 0 R0");
    assert(bus->Read8(0x0000'0000) == 125);
    EXE_INSTR("SB R1 5 R0");
    assert(bus->Read8(0x0000'0005) == 125);
    cpu.SetR(2, 12);
    EXE_INSTR("SB R1 -5 R2");
    assert(bus->Read8(0x0000'0007) == 125);

    //========================
    // SH
    //========================
    cpu.Reset();
    bus->Reset();
    cpu.SetR(1, 1234);
    EXE_INSTR("SH R1 0 R0");
    assert(bus->Read16(0x0000'0000) == 1234);
    EXE_INSTR("SH R1 4 R0");
    assert(bus->Read16(0x0000'0004) == 1234);
    cpu.SetR(2, 12);
    EXE_INSTR("SH R1 -2 R2");
    assert(bus->Read16(0x0000'000a) == 1234);
    // test unaligned store
    cpu.SetR(2, 0x11);
    EXE_INSTR("SH R1 0 R2");
    assert(bus->Read16(0x0000'0011) == 0);

    //========================
    // SW
    //========================
    cpu.Reset();
    bus->Reset();
    cpu.SetR(1, 0x1234'1234);
    EXE_INSTR("SW R1 0 R0");
    assert(bus->Read32(0x0000'0000) == 0x1234'1234);
    EXE_INSTR("SW R1 4 R0");
    assert(bus->Read32(0x0000'0004) == 0x1234'1234);
    cpu.SetR(2, 10);
    EXE_INSTR("SW R1 -2 R2");
    assert(bus->Read32(0x0000'0008) == 0x1234'1234);
    // test unaligned store
    cpu.SetR(2, 0x100);
    EXE_INSTR("SW R1 1 R2");
    assert(bus->Read32(0x0000'0101) == 0);
    EXE_INSTR("SW R1 2 R2");
    assert(bus->Read32(0x0000'0102) == 0);
    EXE_INSTR("SW R1 3 R2");
    assert(bus->Read32(0x0000'0103) == 0);

    //========================
    // SWL
    //========================
    cpu.Reset();
    bus->Reset();
    cpu.SetR(1, 0xdead'beef);
    EXE_INSTR("SWL R1 0 R0");
    assert(bus->Read32(0) == 0x0000'00de);
    EXE_INSTR("SWL R1 5 R0");
    assert(bus->Read32(4) == 0x0000'dead);
    EXE_INSTR("SWL R1 10 R0");
    assert(bus->Read32(8) == 0x00de'adbe);
    EXE_INSTR("SWL R1 15 R0");
    assert(bus->Read32(12) == 0xdead'beef);

    //========================
    // SWR
    //========================
    cpu.Reset();
    bus->Reset();
    cpu.SetR(1, 0xdead'beef);
    EXE_INSTR("SWR R1 0 R0");
    assert(bus->Read32(0) == 0xdead'beef);
    EXE_INSTR("SWR R1 5 R0");
    assert(bus->Read32(4) == 0xadbe'ef00);
    EXE_INSTR("SWR R1 10 R0");
    assert(bus->Read32(8) == 0xbeef'0000);
    EXE_INSTR("SWR R1 15 R0");
    assert(bus->Read32(12) == 0xef00'0000);

    //========================
    // SWL + SWR
    //========================
    cpu.Reset();
    bus->Reset();
    cpu.SetR(10, 13);
    cpu.SetR(1, 0xdead'beef);
    EXE_INSTR("SWL R1 3 R10");
    EXE_INSTR("SWR R1 0 R10");
    assert(bus->Read32(13) == 0xdead'beef);
}

static void jumpTests()
{
    TCPU_INFO("** Starting Jump Instruction Tests --------------------");
    // setup hardware
    std::shared_ptr<Bus> bus(new Bus);
    std::shared_ptr<Ram> ram(new Ram);
    bus->AddAddressSpace(ram, BusPriority::First);
    Cpu cpu(bus);

    //========================
    // J
    //========================
    cpu.Reset();
    bus->Reset();
    cpu.SetPC(0);
    EXE_BD_INSTR("J 0x100");
    assert(cpu.GetPC() == 0x100);
    cpu.SetPC(0xb000'3003);
    EXE_BD_INSTR("J 0x100");
    assert(cpu.GetPC() == 0xb000'0100);
    cpu.SetPC(0);
    // test branch delay
    EXE_TWO_INSTRS("J 0x100", "ADDI R10 R0 11");
    assert(cpu.GetPC() == 0x100);
    assert(cpu.GetR(10) == 11);

    //========================
    // JAL
    //========================
    cpu.Reset();
    bus->Reset();
    cpu.SetPC(0);
    EXE_BD_INSTR("JAL 0x100");
    assert(cpu.GetPC() == 0x100);
    assert(cpu.GetR(31) == 0x8);
    cpu.SetPC(0xb000'3000);
    EXE_BD_INSTR("JAL 0x100");
    assert(cpu.GetPC() == 0xb000'0100);
    assert(cpu.GetR(31) == 0xb000'3008);
    // test branch delay
    EXE_TWO_INSTRS("JAL 0x100", "ADDI R10 R0 11");
    assert(cpu.GetPC() == 0x100);
    assert(cpu.GetR(10) == 11);

    //========================
    // JR
    //========================
    cpu.Reset();
    bus->Reset();
    cpu.SetPC(0);
    cpu.SetR(1, 0x100);
    EXE_BD_INSTR("JR R1");
    assert(cpu.GetPC() == 0x100);
    cpu.SetPC(0xb000'3003);
    EXE_BD_INSTR("JR R1");
    assert(cpu.GetPC() == 0x0000'0100);
    // test branch delay
    EXE_TWO_INSTRS("JR R1", "ADDI R10 R0 11");
    assert(cpu.GetPC() == 0x100);
    assert(cpu.GetR(10) == 11);

    //========================
    // JALR
    //========================
    cpu.Reset();
    bus->Reset();
    cpu.SetPC(0);
    cpu.SetR(1, 0x100);
    cpu.SetR(2, 0xbe0);
    EXE_BD_INSTR("JALR R1 R2");
    assert(cpu.GetPC() == 0x100);
    assert(cpu.GetR(31) == 0xbe0);
    cpu.SetPC(0xb000'3000);
    EXE_BD_INSTR("JALR R1 R2");
    assert(cpu.GetPC() == 0x100);
    assert(cpu.GetR(31) == 0xbe0);
    // test branch delay
    EXE_TWO_INSTRS("JALR R1 R2", "ADDI R10 R0 11");
    assert(cpu.GetPC() == 0x100);
    assert(cpu.GetR(10) == 11);
}

static void branchTests()
{
    TCPU_INFO("** Starting Branch Instruction Tests ------------------");
    // setup hardware
    std::shared_ptr<Bus> bus(new Bus);
    std::shared_ptr<Ram> ram(new Ram);
    bus->AddAddressSpace(ram, BusPriority::First);
    Cpu cpu(bus);

    //========================
    // BEQ
    //========================
    cpu.Reset();
    cpu.SetR(1, 10);
    cpu.SetPC(0x100);
    EXE_BD_INSTR("BEQ R1 R1 2");
    assert(cpu.GetPC() == 0x108);
    cpu.SetPC(0x100);
    EXE_BD_INSTR("BEQ R1 R1 -1");
    assert(cpu.GetPC() == 0x0fc);
    cpu.SetPC(0x100);
    EXE_BD_INSTR("BEQ R1 R0 2");
    assert(cpu.GetPC() == 0x104);
    // test branch delay
    EXE_TWO_INSTRS("BEQ R1 R1 4", "ADDI R10 R0 11");
    assert(cpu.GetPC() == 0x1010);
    assert(cpu.GetR(10) == 11);

    //========================
    // BNE
    //========================
    cpu.Reset();
    cpu.SetR(1, 10);
    cpu.SetPC(0x100);
    EXE_BD_INSTR("BNE R1 R0 2");
    assert(cpu.GetPC() == 0x108);
    cpu.SetPC(0x100);
    EXE_BD_INSTR("BNE R1 R0 -1");
    assert(cpu.GetPC() == 0x0fc);
    cpu.SetPC(0x100);
    EXE_BD_INSTR("BNE R1 R1 2");
    assert(cpu.GetPC() == 0x104);
    // test branch delay
    EXE_TWO_INSTRS("BNE R1 R0 4", "ADDI R10 R0 11");
    assert(cpu.GetPC() == 0x1010);
    assert(cpu.GetR(10) == 11);

    //========================
    // BLEZ
    //========================
    cpu.Reset();
    cpu.SetR(1, 0xffff'ffff);
    cpu.SetPC(0x100);
    EXE_BD_INSTR("BLEZ R1 2");
    assert(cpu.GetPC() == 0x108);
    cpu.SetR(1, 0);
    cpu.SetPC(0x100);
    EXE_BD_INSTR("BLEZ R1 -1");
    assert(cpu.GetPC() == 0x0fc);
    cpu.SetR(1, 1);
    cpu.SetPC(0x100);
    EXE_BD_INSTR("BLEZ R1 2");
    assert(cpu.GetPC() == 0x104);
    // test branch delay
    EXE_TWO_INSTRS("BLEZ R0 4", "ADDI R10 R0 11");
    assert(cpu.GetPC() == 0x1010);
    assert(cpu.GetR(10) == 11);

    //========================
    // BGTZ
    //========================
    cpu.Reset();
    cpu.SetR(1, 10);
    cpu.SetPC(0x100);
    EXE_BD_INSTR("BGTZ R1 2");
    assert(cpu.GetPC() == 0x108);
    cpu.SetPC(0x100);
    EXE_BD_INSTR("BGTZ R1 -1");
    assert(cpu.GetPC() == 0x0fc);
    cpu.SetR(1, 0);
    cpu.SetPC(0x100);
    EXE_BD_INSTR("BGTZ R1 2");
    assert(cpu.GetPC() == 0x104);
    cpu.SetR(1, (u32)-1);
    cpu.SetPC(0x100);
    EXE_BD_INSTR("BGTZ R1 2");
    assert(cpu.GetPC() == 0x104);
    // test branch delay
    cpu.SetR(1, 1);
    EXE_TWO_INSTRS("BGTZ R1 4", "ADDI R10 R0 11");
    assert(cpu.GetPC() == 0x1010);
    assert(cpu.GetR(10) == 11);

    //========================
    // BLTZ
    //========================
    cpu.Reset();
    cpu.SetR(1, (u32)-10);
    cpu.SetPC(0x100);
    EXE_BD_INSTR("BLTZ R1 2");
    assert(cpu.GetPC() == 0x108);
    cpu.SetPC(0x100);
    EXE_BD_INSTR("BLTZ R1 -1");
    assert(cpu.GetPC() == 0x0fc);
    cpu.SetR(1, 0);
    cpu.SetPC(0x100);
    EXE_BD_INSTR("BLTZ R1 2");
    assert(cpu.GetPC() == 0x104);
    cpu.SetR(1, 1);
    cpu.SetPC(0x100);
    EXE_BD_INSTR("BLTZ R1 2");
    assert(cpu.GetPC() == 0x104);
    // test branch delay
    cpu.SetR(1, (u32)-1);
    EXE_TWO_INSTRS("BLTZ R1 4", "ADDI R10 R0 11");
    assert(cpu.GetPC() == 0x1010);
    assert(cpu.GetR(10) == 11);

    //========================
    // BGEZ
    //========================
    cpu.Reset();
    cpu.SetR(1, 10);
    cpu.SetPC(0x100);
    EXE_BD_INSTR("BGEZ R1 2");
    assert(cpu.GetPC() == 0x108);
    cpu.SetPC(0x100);
    EXE_BD_INSTR("BGEZ R1 -1");
    assert(cpu.GetPC() == 0x0fc);
    cpu.SetR(1, 0);
    cpu.SetPC(0x100);
    EXE_BD_INSTR("BGEZ R1 2");
    assert(cpu.GetPC() == 0x108);
    cpu.SetR(1, (u32)-1);
    cpu.SetPC(0x100);
    EXE_BD_INSTR("BGEZ R1 2");
    assert(cpu.GetPC() == 0x104);
    // test branch delay
    cpu.SetR(1, 1);
    EXE_TWO_INSTRS("BGEZ R1 4", "ADDI R10 R0 11");
    assert(cpu.GetPC() == 0x1010);
    assert(cpu.GetR(10) == 11);

    //========================
    // BLTZAL
    //========================
    cpu.Reset();
    cpu.SetR(1, (u32)-1);
    cpu.SetR(31, 0);
    cpu.SetPC(0x100);
    EXE_BD_INSTR("BLTZAL R1 4");
    assert(cpu.GetPC() == 0x110);
    assert(cpu.GetR(31) == 0x108);
    cpu.SetR(31, 0);
    cpu.SetPC(0x100);
    EXE_BD_INSTR("BLTZAL R1 -1");
    assert(cpu.GetPC() == 0x0fc);
    assert(cpu.GetR(31) == 0x108);
    cpu.SetR(31, 0);
    cpu.SetR(1, 0);
    cpu.SetPC(0x100);
    EXE_BD_INSTR("BLTZAL R1 4");
    assert(cpu.GetPC() == 0x104);
    assert(cpu.GetR(31) == 0);
    cpu.SetR(31, 0);
    cpu.SetR(1, 1);
    cpu.SetPC(0x100);
    EXE_BD_INSTR("BLTZAL R1 4");
    assert(cpu.GetPC() == 0x104);
    assert(cpu.GetR(31) == 0);
    // test branch delay
    cpu.SetR(31, 0);
    cpu.SetR(1, (u32)-1);
    EXE_TWO_INSTRS("BLTZAL R1 4", "ADDI R10 R0 11");
    assert(cpu.GetPC() == 0x1010);
    assert(cpu.GetR(10) == 11);
    assert(cpu.GetR(31) == 0x1008);

    //========================
    // BGEZAL
    //========================
    cpu.Reset();
    cpu.SetR(1, 10);
    cpu.SetR(31, 0);
    cpu.SetPC(0x100);
    EXE_BD_INSTR("BGEZAL R1 4");
    assert(cpu.GetPC() == 0x110);
    assert(cpu.GetR(31) == 0x108);
    cpu.SetR(31, 0);
    cpu.SetPC(0x100);
    EXE_BD_INSTR("BGEZAL R1 -1");
    assert(cpu.GetPC() == 0x0fc);
    assert(cpu.GetR(31) == 0x108);
    cpu.SetR(31, 0);
    cpu.SetR(1, 0);
    cpu.SetPC(0x100);
    EXE_BD_INSTR("BGEZAL R1 4");
    assert(cpu.GetPC() == 0x110);
    assert(cpu.GetR(31) == 0x108);
    cpu.SetR(31, 0);
    cpu.SetR(1, (u32)-1);
    cpu.SetPC(0x100);
    EXE_BD_INSTR("BGEZAL R1 4");
    assert(cpu.GetPC() == 0x104);
    assert(cpu.GetR(31) == 0);
    // test branch delay
    cpu.SetR(31, 0);
    cpu.SetR(1, 1);
    EXE_TWO_INSTRS("BGEZAL R1 4", "ADDI R10 R0 11");
    assert(cpu.GetPC() == 0x1010);
    assert(cpu.GetR(10) == 11);
    assert(cpu.GetR(31) == 0x1008);
}

namespace psxtest {
    void CpuTests()
    {
        std::cout << PSX_FANCYTITLE("CPU TESTS");
        aluiTests();
        alurTests();
        shiftTests();
        hiloTests();
        loadTests();
        loadDelayTests();
        storeTests();
        jumpTests();
        branchTests();
    }
}
