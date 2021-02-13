/*
 * psxtest_Cpu::cpp
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
#include "core/sys.h"
#include "mem/bus.h"
#include "mem/ram.h"

#define TCPU_INFO(...) PSXLOG_INFO("Test-CPU", __VA_ARGS__)
#define TCPU_WARN(...) PSXLOG_WARN("Test-CPU", __VA_ARGS__)
#define TCPU_ERROR(...) PSXLOG_ERROR("Test-CPU", __VA_ARGS__)

using namespace Psx;

// helper macros
#define EXE_INSTR(instr) TCPU_INFO(instr); Cpu::ExecuteInstruction(Cpu::Asm::AsmInstruction(instr))
#define EXE_LW_INSTR(instr) \
{\
    TCPU_INFO(instr);\
    Bus::Write<u32>(Cpu::Asm::AsmInstruction(instr), 0x100);\
    Bus::Write<u32>(0, 0x104);\
    Cpu::SetPC(0x100);\
    Cpu::Step();\
    Cpu::Step();\
}

#define EXE_TWO_INSTRS(i1, i2) \
{\
    TCPU_INFO("{} -> {}", i1, i2);\
    Bus::Write<u32>(Cpu::Asm::AsmInstruction(i1), 0x1000);\
    Bus::Write<u32>(Cpu::Asm::AsmInstruction(i2), 0x1004);\
    Cpu::SetPC(0x1000);\
    Cpu::Step();\
    Cpu::Step();\
}

#define EXE_TWO_LW_INSTRS(i1, i2) \
{\
    TCPU_INFO("{} -> {}", i1, i2);\
    Bus::Write<u32>(Cpu::Asm::AsmInstruction(i1), 0x1000);\
    Bus::Write<u32>(Cpu::Asm::AsmInstruction(i2), 0x1004);\
    Cpu::SetPC(0x1000);\
    Cpu::Step();\
    Cpu::Step();\
    Cpu::Step();\
}

#define EXE_BD_INSTR(i) \
    EXE_INSTR(i);\
    Cpu::Step();



static void aluiTests()
{
    TCPU_INFO("** Starting ALU Immediate Tests -----------------------");
    // setup hardware
    System::Reset();

    std::string instr;
    //========================
    // addi
    //========================
    EXE_INSTR("ADDI R1 R0 13");
    assert(Cpu::GetR(1) == 13);
    EXE_INSTR("ADDI R2 R1 20");
    assert(Cpu::GetR(2) == 33);
    EXE_INSTR("ADDI R3 R1 0xffff"); // sub 1
    assert(Cpu::GetR(3) == 12);
    EXE_INSTR("ADDI R3 R1 0xffff"); // sub 1
    assert(Cpu::GetR(3) == 12);

    //========================
    // addiu
    //========================
    System::Reset();
    instr = "ADDIU R1 R0 10";
    EXE_INSTR(instr);
    assert(Cpu::GetR(1) == 10);
    instr = "ADDIU R2 R1 20";
    EXE_INSTR(instr);
    assert(Cpu::GetR(2) == 30);
    instr = "ADDIU R3 R1 0xffff"; // sub 1
    EXE_INSTR(instr);
    assert(Cpu::GetR(3) == 9);
    instr = "ADDIU R3 R1 -1"; // sub 1
    EXE_INSTR(instr);
    assert(Cpu::GetR(3) == 9);

    //========================
    // slti
    //========================
    System::Reset();
    instr = "SLTI R1 R0 1";
    EXE_INSTR(instr);
    assert(Cpu::GetR(1) == 1);
    instr = "SLTI R2 R1 1";
    EXE_INSTR(instr);
    assert(Cpu::GetR(2) == 0);
    instr = "SLTI R3 R1 0";
    EXE_INSTR(instr);
    assert(Cpu::GetR(3) == 0);

    //========================
    // slti
    //========================
    System::Reset();
    instr = "SLTIU R1 R0 1";
    EXE_INSTR(instr);
    assert(Cpu::GetR(1) == 1);
    instr = "SLTIU R2 R1 1";
    EXE_INSTR(instr);
    assert(Cpu::GetR(2) == 0);
    instr = "SLTIU R3 R1 0";
    EXE_INSTR(instr);
    assert(Cpu::GetR(3) == 0);

    //========================
    // andi
    //========================
    System::Reset();
    instr = "ADDI R1 R0 0x1";
    Cpu::ExecuteInstruction(Cpu::Asm::AsmInstruction(instr));
    assert(Cpu::GetR(1) == 1);
    instr = "ANDI R2 R1 0x1";
    EXE_INSTR(instr);
    assert(Cpu::GetR(2) == 0x1);
    instr = "ANDI R3 R0 0x1";
    EXE_INSTR(instr);
    assert(Cpu::GetR(3) == 0x0);
    instr = "ANDI R4 R1 0xffff";
    EXE_INSTR(instr);
    assert(Cpu::GetR(4) == 0x1);

    //========================
    // ori
    //========================
    System::Reset();
    instr = "ADDI R1 R0 0x1";
    EXE_INSTR(instr);
    assert(Cpu::GetR(1) == 1);
    instr = "ORI R2 R1 0x1";
    EXE_INSTR(instr);
    assert(Cpu::GetR(2) == 0x1);
    instr = "ORI R3 R0 0x1";
    EXE_INSTR(instr);
    assert(Cpu::GetR(3) == 0x1);
    instr = "ORI R4 R1 0xffff";
    EXE_INSTR(instr);
    assert(Cpu::GetR(4) == 0xffff);
    instr = "ORI R5 R0 0x0";
    EXE_INSTR(instr);
    assert(Cpu::GetR(5) == 0x0);

    //========================
    // xori
    //========================
    System::Reset();
    instr = "ADDI R1 R0 0x1";
    EXE_INSTR(instr);
    assert(Cpu::GetR(1) == 1);
    instr = "XORI R2 R1 0x1";
    EXE_INSTR(instr);
    assert(Cpu::GetR(2) == 0x0);
    instr = "XORI R3 R1 0x1";
    EXE_INSTR(instr);
    assert(Cpu::GetR(3) == 0x0);
    instr = "XORI R4 R1 0xffff";
    EXE_INSTR(instr);
    assert(Cpu::GetR(4) == 0xfffe);
    instr = "XORI R5 R0 0x0";
    EXE_INSTR(instr);
    assert(Cpu::GetR(5) == 0x0);

    //========================
    // lui
    //========================
    System::Reset();
    instr = "LUI R1 0x0000";
    EXE_INSTR(instr);
    assert(Cpu::GetR(1) == 0x0);
    instr = "LUI R2 0xffff";
    EXE_INSTR(instr);
    assert(Cpu::GetR(2) == 0xffff'0000);
    instr = "LUI R2 0xbeef";
    EXE_INSTR(instr);
    assert(Cpu::GetR(2) == 0xbeef'0000);
}

static void alurTests()
{
    TCPU_INFO("** Starting ALU R-Type Instruction Tests --------------");
    // setup hardware
    System::Reset();
    
    std::string instr;
    //========================
    // ADD
    //========================
    System::Reset();
    Cpu::SetR(1, 10);
    instr = "ADD R2 R1 R1";
    EXE_INSTR(instr);
    assert(Cpu::GetR(2) == 20);
    // test trap
    Cpu::SetR(3, 0x7fff'ffff);
    instr = "ADD R4 R3 R1";
    EXE_INSTR(instr);
    assert(Cpu::GetR(4) == 0);

    //========================
    // ADDU
    //========================
    System::Reset();
    Cpu::SetR(1, 1);
    instr = "ADDU R2 R1 R1";
    EXE_INSTR(instr);
    assert(Cpu::GetR(2) == 2);
    instr = "ADDU R3 R0 R2";
    EXE_INSTR(instr);
    assert(Cpu::GetR(3) == 2);
    // test no trap
    Cpu::SetR(4, 0x7fff'ffff);
    instr = "ADDU R5 R4 R1";
    EXE_INSTR(instr);
    assert(Cpu::GetR(5) == 0x8000'0000);

    //========================
    // SUB
    //========================
    System::Reset();
    Cpu::SetR(1, 1);
    instr = "SUB R2 R0 R1";
    EXE_INSTR(instr);
    assert(Cpu::GetR(2) == 0xffff'ffff);
    Cpu::SetR(3, 10);
    Cpu::SetR(4, 7);
    instr = "SUB R5 R3 R4";
    EXE_INSTR(instr);
    assert(Cpu::GetR(5) == 3);
    // test trap
    Cpu::SetR(6, 0x8000'0000);
    instr = "SUB R7 R6 R1";
    EXE_INSTR(instr);
    assert(Cpu::GetR(7) == 0);

    //========================
    // SUBU
    //========================
    System::Reset();
    Cpu::SetR(1, 1);
    instr = "SUBU R2 R0 R1";
    EXE_INSTR(instr);
    assert(Cpu::GetR(2) == 0xffff'ffff);
    Cpu::SetR(3, 10);
    Cpu::SetR(4, 7);
    instr = "SUBU R5 R3 R4";
    EXE_INSTR(instr);
    assert(Cpu::GetR(5) == 3);
    // test no trap
    Cpu::SetR(6, 0x8000'0000);
    instr = "SUBU R7 R6 R1";
    EXE_INSTR(instr);
    assert(Cpu::GetR(7) == 0x7fff'ffff);


    //========================
    // SLT
    //========================
    System::Reset();
    Cpu::SetR(1, 10);
    Cpu::SetR(2, 5);
    Cpu::SetR(3, 20);
    Cpu::SetR(4, 0xffff'ffff);
    instr = "SLT R5 R1 R1";
    EXE_INSTR(instr);
    assert(Cpu::GetR(5) == 0);
    instr = "SLT R5 R1 R2";
    EXE_INSTR(instr);
    assert(Cpu::GetR(5) == 0);
    instr = "SLT R5 R1 R3";
    EXE_INSTR(instr);
    assert(Cpu::GetR(5) == 1);
    instr = "SLT R5 R1 R4";
    EXE_INSTR(instr);
    assert(Cpu::GetR(5) == 0);
    instr = "SLT R5 R4 R2";
    EXE_INSTR(instr);
    assert(Cpu::GetR(5) == 1);
    // test TRAP
    Cpu::SetR(6, 0x8000'0000);
    instr = "SLT R7 R6 R1";
    EXE_INSTR(instr);
    assert(Cpu::GetR(7) == 0);

    //========================
    // SLT
    //========================
    System::Reset();
    Cpu::SetR(1, 10);
    Cpu::SetR(2, 5);
    Cpu::SetR(3, 20);
    Cpu::SetR(4, 0xffff'ffff);
    instr = "SLTU R5 R1 R1";
    EXE_INSTR(instr);
    assert(Cpu::GetR(5) == 0);
    instr = "SLTU R5 R1 R2";
    EXE_INSTR(instr);
    assert(Cpu::GetR(5) == 0);
    instr = "SLTU R5 R1 R3";
    EXE_INSTR(instr);
    assert(Cpu::GetR(5) == 1);
    instr = "SLTU R5 R1 R4";
    EXE_INSTR(instr);
    assert(Cpu::GetR(5) == 0);
    instr = "SLTU R5 R4 R2";
    EXE_INSTR(instr);
    assert(Cpu::GetR(5) == 1);
    // test no trap
    Cpu::SetR(6, 0x8000'0000);
    instr = "SLTU R7 R6 R1";
    EXE_INSTR(instr);
    assert(Cpu::GetR(7) == 0);

    //========================
    // AND
    //========================
    System::Reset();
    Cpu::SetR(1, 0xff);
    Cpu::SetR(2, 0xaa);
    instr = "AND R3 R1 R2";
    EXE_INSTR(instr);
    assert(Cpu::GetR(3) == 0xaa);
    Cpu::SetR(3, 0x00);
    instr = "AND R4 R3 R2";
    EXE_INSTR(instr);
    assert(Cpu::GetR(4) == 0x00);

    //========================
    // OR
    //========================
    System::Reset();
    Cpu::SetR(1, 0xff);
    Cpu::SetR(2, 0xaa);
    instr = "OR R3 R1 R2";
    EXE_INSTR(instr);
    assert(Cpu::GetR(3) == 0xff);
    Cpu::SetR(3, 0x00);
    instr = "OR R4 R3 R2";
    EXE_INSTR(instr);
    assert(Cpu::GetR(4) == 0xaa);

    //========================
    // XOR
    //========================
    System::Reset();
    Cpu::SetR(1, 0xff);
    Cpu::SetR(2, 0xaa);
    instr = "XOR R3 R1 R2";
    EXE_INSTR(instr);
    assert(Cpu::GetR(3) == (0xaa ^ 0xff));
    instr = "XOR R4 R3 R3";
    EXE_INSTR(instr);
    assert(Cpu::GetR(4) == 0x00);

    //========================
    // NOR
    //========================
    System::Reset();
    Cpu::SetR(1, 0xff);
    Cpu::SetR(2, 0xaa);
    instr = "NOR R3 R1 R2";
    EXE_INSTR(instr);
    assert(Cpu::GetR(3) == (u32) ~(0xaa | 0xff));
    instr = "NOR R4 R1 R1";
    EXE_INSTR(instr);
    assert(Cpu::GetR(4) == 0xffff'ff00);
}

static void shiftTests()
{
    TCPU_INFO("** Starting Shift Tests -------------------------------");
    std::string instr;
    // setup hardware
    System::Reset();

    //========================
    // SLL
    //========================
    System::Reset();
    Cpu::SetR(1, 1);
    instr = "SLL R2, R1, 1";
    EXE_INSTR(instr);
    assert(Cpu::GetR(2) == 2);
    instr = "SLL R3, R2, 2";
    EXE_INSTR(instr);
    assert(Cpu::GetR(3) == 8);
    instr = "SLL R4, R3, 0";
    EXE_INSTR(instr);
    assert(Cpu::GetR(4) == 8);

    //========================
    // SRL
    //========================
    System::Reset();
    Cpu::SetR(1, 8);
    instr = "SRL R2, R1, 1";
    EXE_INSTR(instr);
    assert(Cpu::GetR(2) == 4);
    instr = "SRL R3, R2, 2";
    EXE_INSTR(instr);
    assert(Cpu::GetR(3) == 1);
    Cpu::SetR(4, 0x8000'0000);
    instr = "SRL R5, R4, 16";
    EXE_INSTR(instr);
    assert(Cpu::GetR(5) == 0x0000'8000);

    //========================
    // SRA
    //========================
    System::Reset();
    Cpu::SetR(1, 8);
    instr = "SRA R2, R1, 1";
    EXE_INSTR(instr);
    assert(Cpu::GetR(2) == 4);
    instr = "SRA R3, R2, 2";
    EXE_INSTR(instr);
    assert(Cpu::GetR(3) == 1);
    Cpu::SetR(4, 0x8000'0000);
    instr = "SRA R5, R4, 16";
    EXE_INSTR(instr);
    assert(Cpu::GetR(5) == 0xffff'8000);

    //========================
    // SLLV
    //========================
    System::Reset();
    Cpu::SetR(1, 1);
    Cpu::SetR(10, 1);
    instr = "SLLV R2, R1, R10";
    EXE_INSTR(instr);
    assert(Cpu::GetR(2) == 2);
    Cpu::SetR(10, 2);
    instr = "SLLV R3, R2, R10";
    EXE_INSTR(instr);
    assert(Cpu::GetR(3) == 8);
    Cpu::SetR(10, 0);
    instr = "SLLV R4, R3, R10";
    EXE_INSTR(instr);
    assert(Cpu::GetR(4) == 8);

    //========================
    // SRLV
    //========================
    System::Reset();
    Cpu::SetR(1, 8);
    Cpu::SetR(10, 1);
    instr = "SRLV R2, R1, R10";
    EXE_INSTR(instr);
    assert(Cpu::GetR(2) == 4);
    Cpu::SetR(10, 2);
    instr = "SRLV R3, R2, R10";
    EXE_INSTR(instr);
    assert(Cpu::GetR(3) == 1);
    Cpu::SetR(4, 0x8000'0000);
    Cpu::SetR(10, 16);
    instr = "SRLV R5, R4, R10";
    EXE_INSTR(instr);
    assert(Cpu::GetR(5) == 0x0000'8000);

    //========================
    // SRAV
    //========================
    System::Reset();
    Cpu::SetR(1, 8);
    Cpu::SetR(10, 1);
    instr = "SRAV R2, R1, R10";
    EXE_INSTR(instr);
    assert(Cpu::GetR(2) == 4);
    Cpu::SetR(10, 2);
    instr = "SRAV R3, R2, R10";
    EXE_INSTR(instr);
    assert(Cpu::GetR(3) == 1);
    Cpu::SetR(4, 0x8000'0000);
    Cpu::SetR(10, 16);
    instr = "SRAV R5, R4, R10";
    EXE_INSTR(instr);
    assert(Cpu::GetR(5) == 0xffff'8000);
}

void hiloTests()
{
    TCPU_INFO("** Starting HiLo Tests --------------------------------");
    std::string instr;
    // setup hardware
    System::Reset();
    u64 res = 0;

    //========================
    // MULT
    //========================
    System::Reset();
    Cpu::SetR(1, 10);
    Cpu::SetR(2, 3);
    instr = "MULT R1, R2";
    EXE_INSTR(instr);
    res = (static_cast<u64>(Cpu::GetHI()) << 32) | Cpu::GetLO();
    assert(res == 30);
    Cpu::SetR(1, 0xffff'ffff); // -1
    Cpu::SetR(2, 3);
    instr = "MULT R1, R2";
    EXE_INSTR(instr);
    res = (static_cast<u64>(Cpu::GetHI()) << 32) | Cpu::GetLO();
    assert(res == 0xffff'ffff'ffff'fffd); // -3
    instr = "MULT R0, R2";
    EXE_INSTR(instr);
    res = (static_cast<u64>(Cpu::GetHI()) << 32) | Cpu::GetLO();
    assert(res == 0);

    //========================
    // MULTU
    //========================
    System::Reset();
    Cpu::SetR(1, 10);
    Cpu::SetR(2, 3);
    instr = "MULTU R1, R2";
    EXE_INSTR(instr);
    res = (static_cast<u64>(Cpu::GetHI()) << 32) | Cpu::GetLO();
    assert(res == 30);
    Cpu::SetR(1, 0xffff'ffff);
    Cpu::SetR(2, 3);
    instr = "MULTU R1, R2";
    EXE_INSTR(instr);
    res = (static_cast<u64>(Cpu::GetHI()) << 32) | Cpu::GetLO();
    assert(res == 0x0000'0002'ffff'fffd); // since unsigned, NOT -3
    instr = "MULTU R0, R2";
    EXE_INSTR(instr);
    res = (static_cast<u64>(Cpu::GetHI()) << 32) | Cpu::GetLO();
    assert(res == 0);

    //========================
    // DIV
    //========================
    System::Reset();
    Cpu::SetR(1, 24);
    Cpu::SetR(2, 6);
    instr = "DIV R1, R2";
    EXE_INSTR(instr);
    assert(Cpu::GetLO() == 4); // quotient
    assert(Cpu::GetHI() == 0); // remainder
    Cpu::SetR(1, 23);
    Cpu::SetR(2, 6);
    instr = "DIV R1, R2";
    EXE_INSTR(instr);
    assert(Cpu::GetLO() == 3); // quotient
    assert(Cpu::GetHI() == 5); // remainder
    Cpu::SetR(1, 23);
    Cpu::SetR(2, 29);
    instr = "DIV R1, R2";
    EXE_INSTR(instr);
    assert(Cpu::GetLO() == 0); // quotient
    assert(Cpu::GetHI() == 23); // remainder
    Cpu::SetR(1, (u32)-4);
    Cpu::SetR(2, (u32)2);
    instr = "DIV R1, R2";
    EXE_INSTR(instr);
    assert(Cpu::GetLO() == (u32)-2); // quotient
    assert(Cpu::GetHI() == 0); // remainder
    // Divide positive num by 0
    Cpu::SetR(1, 1);
    instr = "DIV R1, R0";
    EXE_INSTR(instr);
    assert(Cpu::GetLO() == (u32)-1); // quotient
    assert(Cpu::GetHI() == Cpu::GetR(1)); // remainder
    // Divide negative num by 0
    Cpu::SetR(1, (u32) -1);
    instr = "DIV R1, R0";
    EXE_INSTR(instr);
    assert(Cpu::GetLO() == 0x1); // quotient
    assert(Cpu::GetHI() == Cpu::GetR(1)); // remainder
    // 0x8000'0000 / -1
    Cpu::SetR(1, 0x8000'0000);
    Cpu::SetR(2, (u32)-1);
    instr = "DIV R1, R2";
    EXE_INSTR(instr);
    assert(Cpu::GetLO() == 0x8000'0000); // quotient
    assert(Cpu::GetHI() == 0); // remainder

    //========================
    // DIVU
    //========================
    System::Reset();
    Cpu::SetR(1, 24);
    Cpu::SetR(2, 6);
    instr = "DIVU R1, R2";
    EXE_INSTR(instr);
    assert(Cpu::GetLO() == 4); // quotient
    assert(Cpu::GetHI() == 0); // remainder
    Cpu::SetR(1, 23);
    Cpu::SetR(2, 6);
    instr = "DIVU R1, R2";
    EXE_INSTR(instr);
    assert(Cpu::GetLO() == 3); // quotient
    assert(Cpu::GetHI() == 5); // remainder
    Cpu::SetR(1, 23);
    Cpu::SetR(2, 29);
    instr = "DIVU R1, R2";
    EXE_INSTR(instr);
    assert(Cpu::GetLO() == 0); // quotient
    assert(Cpu::GetHI() == 23); // remainder
    Cpu::SetR(1, (u32)-4);
    Cpu::SetR(2, (u32)2);
    instr = "DIVU R1, R2";
    EXE_INSTR(instr);
    assert(Cpu::GetLO() == (u32)-4 >> 1); // quotient
    assert(Cpu::GetHI() == 0); // remainder
    // Divide positive num by 0
    Cpu::SetR(1, 1);
    instr = "DIVU R1, R0";
    EXE_INSTR(instr);
    assert(Cpu::GetLO() == (u32)-1); // quotient
    assert(Cpu::GetHI() == Cpu::GetR(1)); // remainder

    //========================
    // Move HI/LO
    //========================
    System::Reset();
    // move to
    Cpu::SetR(1, 10);
    instr = "MTHI R1";
    EXE_INSTR(instr);
    assert(Cpu::GetHI() == Cpu::GetR(1));
    Cpu::SetR(1, 132);
    instr = "MTLO R1";
    EXE_INSTR(instr);
    assert(Cpu::GetLO() == Cpu::GetR(1));
    // move from
    Cpu::SetHI(100);
    instr = "MFHI R1";
    EXE_INSTR(instr);
    assert(Cpu::GetHI() == Cpu::GetR(1));
    Cpu::SetLO(9123);
    instr = "MFLO R1";
    EXE_INSTR(instr);
    assert(Cpu::GetLO() == Cpu::GetR(1));
}

static void loadTests()
{
    TCPU_INFO("** Starting LOAD Tests --------------------------------");
    std::string instr;
    // setup hardware
    System::Reset();

    //========================
    // LB
    //========================
    System::Reset();
    Bus::Write<u8>(0x7e, 0x0000'0000);
    EXE_LW_INSTR("LB R1 0 R0");
    assert(Cpu::GetR(1) == 0x0000'0007e);
    Bus::Write<u8>(0xbe, 0x0000'0000);
    EXE_LW_INSTR("LB R1 0 R0");
    assert(Cpu::GetR(1) == 0xffff'ffbe);
    Bus::Write<u8>(0x42, 0x0000'0001);
    Cpu::SetR(2, 2);
    EXE_LW_INSTR("LB R1 -1 R2");
    assert(Cpu::GetR(1) == 0x42);
    instr = "LB R2 1 R0";
    EXE_LW_INSTR("LB R2 1 R0");
    assert(Cpu::GetR(2) == 0x42);

    //========================
    // LBU
    //========================
    System::Reset();
    Bus::Write<u8>(0x7e, 0x0000'0000);
    EXE_LW_INSTR("LBU R1 0 R0");
    assert(Cpu::GetR(1) == 0x0000'0007e);
    Bus::Write<u8>(0xbe, 0x0000'0000);
    EXE_LW_INSTR("LBU R1 0 R0");
    assert(Cpu::GetR(1) == 0x0000'00be);
    Bus::Write<u8>(0x42, 0x0000'0001);
    Cpu::SetR(2, 2);
    EXE_LW_INSTR("LBU R1 -1 R2");
    assert(Cpu::GetR(1) == 0x42);
    EXE_LW_INSTR("LBU R2 1 R0");
    assert(Cpu::GetR(2) == 0x42);

    //========================
    // LH
    //========================
    System::Reset();
    Bus::Write<u16>(0x7bef, 0x0000'0000);
    EXE_LW_INSTR("LH R1 0 R0");
    assert(Cpu::GetR(1) == 0x0000'7bef);
    Bus::Write<u16>(0x8bef, 0x0000'0000);
    EXE_LW_INSTR("LH R1 0 R0");
    assert(Cpu::GetR(1) == 0xffff'8bef);
    Bus::Write<u16>(0x0042, 0x0000'0002);
    Cpu::SetR(2, 3);
    EXE_LW_INSTR("LH R1 -1 R2");
    assert(Cpu::GetR(1) == 0x42);
    EXE_LW_INSTR("LH R2 2 R0");
    assert(Cpu::GetR(2) == 0x42);
    // test mis-align exception
    Bus::Write<u16>(0xbeef, 0x0000'0007);
    Cpu::SetR(4, 7);
    EXE_LW_INSTR("LH R20 0 R4");
    assert(Cpu::GetR(20) == 0x0000'0000);

    //========================
    // LHU
    //========================
    System::Reset();
    Bus::Write<u16>(0x7bef, 0x0000'0000);
    EXE_LW_INSTR("LHU R1 0 R0");
    assert(Cpu::GetR(1) == 0x0000'7bef);
    Bus::Write<u16>(0x8bef, 0x0000'0000);
    EXE_LW_INSTR("LHU R1 0 R0");
    assert(Cpu::GetR(1) == 0x0000'8bef);
    Bus::Write<u16>(0x0042, 0x0000'0002);
    Cpu::SetR(2, 3);
    EXE_LW_INSTR("LHU R1 -1 R2");
    assert(Cpu::GetR(1) == 0x42);
    EXE_LW_INSTR("LHU R2 2 R0");
    assert(Cpu::GetR(2) == 0x42);
    // test mis-align exception
    Bus::Write<u16>(0xbeef, 0x0000'0007);
    Cpu::SetR(4, 7);
    EXE_LW_INSTR("LHU R20 0 R4");
    assert(Cpu::GetR(20) == 0x0000'0000);

    //========================
    // LW
    //========================
    System::Reset();
    Bus::Write<u32>(0xbeef'abee, 0x0000'0000);
    EXE_LW_INSTR("LW R1 0 R0");
    assert(Cpu::GetR(1) == 0xbeef'abee);
    Bus::Write<u32>(0x90023e, 0x0000'0004);
    Cpu::SetR(2, 5);
    EXE_LW_INSTR("LW R1 -1 R2");
    assert(Cpu::GetR(1) == 0x90023e);
    Cpu::SetR(2, 3);
    EXE_LW_INSTR("LW R1 1 R2");
    assert(Cpu::GetR(1) == 0x90023e);
    // test mis-align exception
    Cpu::SetR(5, 0xa);
    Bus::Write<u32>(0xbeef'0042, 0x0000'000a);
    EXE_LW_INSTR("LW R20 0 R5");
    assert(Cpu::GetR(20) == 0x0000'0000);
    Bus::Write<u32>(0xbeef'0042, 0x0000'0009);
    EXE_LW_INSTR("LW R21 -1 R5");
    assert(Cpu::GetR(21) == 0x0000'0000);

    //========================
    // LWL
    //========================
    System::Reset();
    Bus::Write<u32>(0xbeef'abee, 0x0000'0000);
    Bus::Write<u32>(0xffff'ffff, 0x0000'0004);
    EXE_LW_INSTR("LWL R1 0 R0");
    assert(Cpu::GetR(1) == 0xee00'0000);
    EXE_LW_INSTR("LWL R2 1 R0");
    assert(Cpu::GetR(2) == 0xabee'0000);
    EXE_LW_INSTR("LWL R3 2 R0");
    assert(Cpu::GetR(3) == 0xefab'ee00);
    EXE_LW_INSTR("LWL R4 3 R0");
    assert(Cpu::GetR(4) == 0xbeef'abee);
    // test merge
    Cpu::SetR(1, 0xcccc'cccc);
    Bus::Write<u32>(0x3333'3333, 0x0000'0000);
    EXE_LW_INSTR("LWL R1 0 R0");
    assert(Cpu::GetR(1) == 0x33cc'cccc);
    Cpu::SetR(1, 0xcccc'cccc);
    EXE_LW_INSTR("LWL R1 1 R0");
    assert(Cpu::GetR(1) == 0x3333'cccc);
    Cpu::SetR(1, 0xcccc'cccc);
    EXE_LW_INSTR("LWL R1 2 R0");
    assert(Cpu::GetR(1) == 0x3333'33cc);
    Cpu::SetR(1, 0xcccc'cccc);
    EXE_LW_INSTR("LWL R1 3 R0");
    assert(Cpu::GetR(1) == 0x3333'3333);

    //========================
    // LWR
    //========================
    System::Reset();
    Bus::Write<u32>(0xdead'beef, 0x0000'0000);
    Bus::Write<u32>(0xffff'ffff, 0x0000'0004);
    EXE_LW_INSTR("LWR R1 0 R0");
    assert(Cpu::GetR(1) == 0xdead'beef);
    EXE_LW_INSTR("LWR R2 1 R0");
    assert(Cpu::GetR(2) == 0x00de'adbe);
    EXE_LW_INSTR("LWR R3 2 R0");
    assert(Cpu::GetR(3) == 0x0000'dead);
    EXE_LW_INSTR("LWR R4 3 R0");
    assert(Cpu::GetR(4) == 0x0000'00de);
    // test merge
    Cpu::SetR(1, 0xcccc'cccc);
    Bus::Write<u32>(0x3333'3333, 0x0000'0000);
    EXE_LW_INSTR("LWR R1 0 R0");
    assert(Cpu::GetR(1) == 0x3333'3333);
    Cpu::SetR(1, 0xcccc'cccc);
    EXE_LW_INSTR("LWR R1 1 R0");
    assert(Cpu::GetR(1) == 0xcc33'3333);
    Cpu::SetR(1, 0xcccc'cccc);
    EXE_LW_INSTR("LWR R1 2 R0");
    assert(Cpu::GetR(1) == 0xcccc'3333);
    Cpu::SetR(1, 0xcccc'cccc);
    EXE_LW_INSTR("LWR R1 3 R0");
    assert(Cpu::GetR(1) == 0xcccc'cc33);
}

static void loadDelayTests()
{
    TCPU_INFO("** Starting Load-Delay Tests --------------------------");
    std::string instr;
    // setup hardware
    System::Reset();

    //========================
    // Load Delays (Standard)
    //========================
    System::Reset();
    Bus::Write<u8>(5, 0x0000'0000);
    EXE_TWO_LW_INSTRS("LB R1 0 R0", "ADD R20 R0 R1");
    assert(Cpu::GetR(20) == 0);
    assert(Cpu::GetR(1) == 5);
    Cpu::SetR(1, 0);
    EXE_TWO_LW_INSTRS("LBU R1 0 R0", "ADD R20 R0 R1");
    assert(Cpu::GetR(20) == 0);
    assert(Cpu::GetR(1) == 5);
    Cpu::SetR(1, 0);
    EXE_TWO_LW_INSTRS("LH R1 0 R0", "ADD R20 R0 R1");
    assert(Cpu::GetR(20) == 0);
    assert(Cpu::GetR(1) == 5);
    Cpu::SetR(1, 0);
    EXE_TWO_LW_INSTRS("LHU R1 0 R0", "ADD R20 R0 R1");
    assert(Cpu::GetR(20) == 0);
    assert(Cpu::GetR(1) == 5);
    Cpu::SetR(1, 0);
    EXE_TWO_LW_INSTRS("LW R1 0 R0", "ADD R20 R0 R1");
    assert(Cpu::GetR(20) == 0);
    assert(Cpu::GetR(1) == 5);
    Cpu::SetR(1, 0);
    EXE_TWO_LW_INSTRS("LWR R1 0 R0", "ADD R20 R0 R1");
    assert(Cpu::GetR(20) == 0);
    assert(Cpu::GetR(1) == 5);
    Cpu::SetR(1, 0);
    EXE_TWO_LW_INSTRS("LWL R1 0 R0", "ADD R20 R0 R1");
    assert(Cpu::GetR(20) == 0);
    Cpu::SetR(1, 0);

    //========================
    // Load Delays (Next instruction writes to Load register)
    //========================
    System::Reset();
    Cpu::SetR(1, 10);
    Bus::Write<u8>(5, 0x0000'0000);
    EXE_TWO_LW_INSTRS("LB R20 0 R0", "ADD R20 R0 R1");
    assert(Cpu::GetR(20) == 10); // ADD should win the race
    EXE_TWO_LW_INSTRS("LBU R20 0 R0", "ADD R20 R0 R1");
    assert(Cpu::GetR(20) == 10); // ADD should win the race
    EXE_TWO_LW_INSTRS("LH R20 0 R0", "ADD R20 R0 R1");
    assert(Cpu::GetR(20) == 10); // ADD should win the race
    EXE_TWO_LW_INSTRS("LHU R20 0 R0", "ADD R20 R0 R1");
    assert(Cpu::GetR(20) == 10); // ADD should win the race
    EXE_TWO_LW_INSTRS("LW R20 0 R0", "ADD R20 R0 R1");
    assert(Cpu::GetR(20) == 10); // ADD should win the race
    EXE_TWO_LW_INSTRS("LWR R20 0 R0", "ADD R20 R0 R1");
    assert(Cpu::GetR(20) == 10); // ADD should win the race
    EXE_TWO_LW_INSTRS("LWL R20 0 R0", "ADD R20 R0 R1");
    assert(Cpu::GetR(20) == 10); // ADD should win the race

    //========================
    // Load Delays (Two unaligned reads)
    //========================
    System::Reset();
    Cpu::SetR(1, 0x1); // unaligned
    Bus::Write<u32>(0xdead'beef, 0x0000'0001);
    EXE_TWO_LW_INSTRS("LWR R20 0 R1", "LWL R20 3 R1");
    assert(Cpu::GetR(20) == 0xdead'beef);
    Cpu::SetR(20, 0);
    EXE_TWO_LW_INSTRS("LWL R20 3 R1", "LWR R20 0 R1");
    assert(Cpu::GetR(20) == 0xdead'beef);
    Cpu::SetR(1, 0x2); // unaligned
    Bus::Write<u32>(0xdead'beef, 0x0000'0002);
    EXE_TWO_LW_INSTRS("LWR R20 0 R1", "LWL R20 3 R1");
    assert(Cpu::GetR(20) == 0xdead'beef);
    Cpu::SetR(20, 0);
    EXE_TWO_LW_INSTRS("LWL R20 3 R1", "LWR R20 0 R1");
    assert(Cpu::GetR(20) == 0xdead'beef);
    Cpu::SetR(1, 0x3); // unaligned
    Bus::Write<u32>(0xdead'beef, 0x0000'0003);
    EXE_TWO_LW_INSTRS("LWR R20 0 R1", "LWL R20 3 R1");
    assert(Cpu::GetR(20) == 0xdead'beef);
    Cpu::SetR(20, 0);
    EXE_TWO_LW_INSTRS("LWL R20 3 R1", "LWR R20 0 R1");
    assert(Cpu::GetR(20) == 0xdead'beef);
}

static void storeTests()
{
    TCPU_INFO("** Starting Store Instruction Tests -------------------");
    // setup hardware
    System::Reset();

    //========================
    // SB
    //========================
    System::Reset();
    Cpu::SetR(1, 125);
    EXE_INSTR("SB R1 0 R0");
    assert(Bus::Read<u8>(0x0000'0000) == 125);
    EXE_INSTR("SB R1 5 R0");
    assert(Bus::Read<u8>(0x0000'0005) == 125);
    Cpu::SetR(2, 12);
    EXE_INSTR("SB R1 -5 R2");
    assert(Bus::Read<u8>(0x0000'0007) == 125);

    //========================
    // SH
    //========================
    System::Reset();
    Cpu::SetR(1, 1234);
    EXE_INSTR("SH R1 0 R0");
    assert(Bus::Read<u16>(0x0000'0000) == 1234);
    EXE_INSTR("SH R1 4 R0");
    assert(Bus::Read<u16>(0x0000'0004) == 1234);
    Cpu::SetR(2, 12);
    EXE_INSTR("SH R1 -2 R2");
    assert(Bus::Read<u16>(0x0000'000a) == 1234);
    // test unaligned store
    Cpu::SetR(2, 0x11);
    EXE_INSTR("SH R1 0 R2");
    assert(Bus::Read<u16>(0x0000'0011) == 0);

    //========================
    // SW
    //========================
    System::Reset();
    Cpu::SetR(1, 0x1234'1234);
    EXE_INSTR("SW R1 0 R0");
    assert(Bus::Read<u32>(0x0000'0000) == 0x1234'1234);
    EXE_INSTR("SW R1 4 R0");
    assert(Bus::Read<u32>(0x0000'0004) == 0x1234'1234);
    Cpu::SetR(2, 10);
    EXE_INSTR("SW R1 -2 R2");
    assert(Bus::Read<u32>(0x0000'0008) == 0x1234'1234);
    // test unaligned store
    Cpu::SetR(2, 0x100);
    EXE_INSTR("SW R1 1 R2");
    assert(Bus::Read<u32>(0x0000'0101) == 0);
    EXE_INSTR("SW R1 2 R2");
    assert(Bus::Read<u32>(0x0000'0102) == 0);
    EXE_INSTR("SW R1 3 R2");
    assert(Bus::Read<u32>(0x0000'0103) == 0);

    //========================
    // SWL
    //========================
    System::Reset();
    Cpu::SetR(1, 0xdead'beef);
    EXE_INSTR("SWL R1 0 R0");
    assert(Bus::Read<u32>(0) == 0x0000'00de);
    EXE_INSTR("SWL R1 5 R0");
    assert(Bus::Read<u32>(4) == 0x0000'dead);
    EXE_INSTR("SWL R1 10 R0");
    assert(Bus::Read<u32>(8) == 0x00de'adbe);
    EXE_INSTR("SWL R1 15 R0");
    assert(Bus::Read<u32>(12) == 0xdead'beef);

    //========================
    // SWR
    //========================
    System::Reset();
    Cpu::SetR(1, 0xdead'beef);
    EXE_INSTR("SWR R1 0 R0");
    assert(Bus::Read<u32>(0) == 0xdead'beef);
    EXE_INSTR("SWR R1 5 R0");
    assert(Bus::Read<u32>(4) == 0xadbe'ef00);
    EXE_INSTR("SWR R1 10 R0");
    assert(Bus::Read<u32>(8) == 0xbeef'0000);
    EXE_INSTR("SWR R1 15 R0");
    assert(Bus::Read<u32>(12) == 0xef00'0000);

    //========================
    // SWL + SWR
    //========================
    System::Reset();
    Cpu::SetR(10, 13);
    Cpu::SetR(1, 0xdead'beef);
    EXE_INSTR("SWL R1 3 R10");
    EXE_INSTR("SWR R1 0 R10");
    assert(Bus::Read<u32>(13) == 0xdead'beef);
}

static void jumpTests()
{
    TCPU_INFO("** Starting Jump Instruction Tests --------------------");
    // setup hardware
    System::Reset();

    //========================
    // J
    //========================
    System::Reset();
    Cpu::SetPC(0);
    EXE_BD_INSTR("J 0x100");
    assert(Cpu::GetPC() == 0x100);
    Cpu::SetPC(0xb000'3003);
    EXE_BD_INSTR("J 0x100");
    assert(Cpu::GetPC() == 0xb000'0100);
    Cpu::SetPC(0);
    // test branch delay
    EXE_TWO_INSTRS("J 0x100", "ADDI R10 R0 11");
    assert(Cpu::GetPC() == 0x100);
    assert(Cpu::GetR(10) == 11);

    //========================
    // JAL
    //========================
    System::Reset();
    Cpu::SetPC(0);
    EXE_BD_INSTR("JAL 0x100");
    assert(Cpu::GetPC() == 0x100);
    assert(Cpu::GetR(31) == 0x4);
    Cpu::SetPC(0xb000'3000);
    EXE_BD_INSTR("JAL 0x100");
    assert(Cpu::GetPC() == 0xb000'0100);
    assert(Cpu::GetR(31) == 0xb000'3004);
    // test branch delay
    EXE_TWO_INSTRS("JAL 0x100", "ADDI R10 R0 11");
    assert(Cpu::GetPC() == 0x100);
    assert(Cpu::GetR(10) == 11);

    //========================
    // JR
    //========================
    System::Reset();
    Cpu::SetPC(0);
    Cpu::SetR(1, 0x100);
    EXE_BD_INSTR("JR R1");
    assert(Cpu::GetPC() == 0x100);
    Cpu::SetPC(0xb000'3003);
    EXE_BD_INSTR("JR R1");
    assert(Cpu::GetPC() == 0x0000'0100);
    // test branch delay
    EXE_TWO_INSTRS("JR R1", "ADDI R10 R0 11");
    assert(Cpu::GetPC() == 0x100);
    assert(Cpu::GetR(10) == 11);

    //========================
    // JALR
    //========================
    System::Reset();
    Cpu::SetPC(0);
    Cpu::SetR(1, 0x100);
    Cpu::SetR(2, 0xbe0);
    EXE_BD_INSTR("JALR R1 R2");
    assert(Cpu::GetPC() == 0x100);
    assert(Cpu::GetR(2) == 0x4);
    Cpu::SetPC(0xb000'3000);
    EXE_BD_INSTR("JALR R1 R2");
    assert(Cpu::GetPC() == 0x100);
    assert(Cpu::GetR(2) == 0xb000'3004);
    // test branch delay
    EXE_TWO_INSTRS("JALR R1 R2", "ADDI R10 R0 11");
    assert(Cpu::GetPC() == 0x100);
    assert(Cpu::GetR(10) == 11);
}

static void branchTests()
{
    TCPU_INFO("** Starting Branch Instruction Tests ------------------");
    // setup hardware
    System::Reset();

    //========================
    // BEQ
    //========================
    System::Reset();
    Cpu::SetR(1, 10);
    Cpu::SetPC(0x100);
    EXE_BD_INSTR("BEQ R1 R1 2");
    assert(Cpu::GetPC() == 0x108);
    Cpu::SetPC(0x100);
    EXE_BD_INSTR("BEQ R1 R1 -1");
    assert(Cpu::GetPC() == 0x0fc);
    Cpu::SetPC(0x100);
    EXE_BD_INSTR("BEQ R1 R0 2");
    assert(Cpu::GetPC() == 0x104);
    // test branch delay
    EXE_TWO_INSTRS("BEQ R1 R1 4", "ADDI R10 R0 11");
    assert(Cpu::GetPC() == 0x1010 + 4);
    assert(Cpu::GetR(10) == 11);

    //========================
    // BNE
    //========================
    System::Reset();
    Cpu::SetR(1, 10);
    Cpu::SetPC(0x100);
    EXE_BD_INSTR("BNE R1 R0 2");
    assert(Cpu::GetPC() == 0x108);
    Cpu::SetPC(0x100);
    EXE_BD_INSTR("BNE R1 R0 -1");
    assert(Cpu::GetPC() == 0x0fc);
    Cpu::SetPC(0x100);
    EXE_BD_INSTR("BNE R1 R1 2");
    assert(Cpu::GetPC() == 0x104);
    // test branch delay
    EXE_TWO_INSTRS("BNE R1 R0 4", "ADDI R10 R0 11");
    assert(Cpu::GetPC() == 0x1010 + 4);
    assert(Cpu::GetR(10) == 11);

    //========================
    // BLEZ
    //========================
    System::Reset();
    Cpu::SetR(1, 0xffff'ffff);
    Cpu::SetPC(0x100);
    EXE_BD_INSTR("BLEZ R1 2");
    assert(Cpu::GetPC() == 0x108);
    Cpu::SetR(1, 0);
    Cpu::SetPC(0x100);
    EXE_BD_INSTR("BLEZ R1 -1");
    assert(Cpu::GetPC() == 0x0fc);
    Cpu::SetR(1, 1);
    Cpu::SetPC(0x100);
    EXE_BD_INSTR("BLEZ R1 2");
    assert(Cpu::GetPC() == 0x104);
    // test branch delay
    EXE_TWO_INSTRS("BLEZ R0 4", "ADDI R10 R0 11");
    assert(Cpu::GetPC() == 0x1010 + 4);
    assert(Cpu::GetR(10) == 11);

    //========================
    // BGTZ
    //========================
    System::Reset();
    Cpu::SetR(1, 10);
    Cpu::SetPC(0x100);
    EXE_BD_INSTR("BGTZ R1 2");
    assert(Cpu::GetPC() == 0x108);
    Cpu::SetPC(0x100);
    EXE_BD_INSTR("BGTZ R1 -1");
    assert(Cpu::GetPC() == 0x0fc);
    Cpu::SetR(1, 0);
    Cpu::SetPC(0x100);
    EXE_BD_INSTR("BGTZ R1 2");
    assert(Cpu::GetPC() == 0x104);
    Cpu::SetR(1, (u32)-1);
    Cpu::SetPC(0x100);
    EXE_BD_INSTR("BGTZ R1 2");
    assert(Cpu::GetPC() == 0x104);
    // test branch delay
    Cpu::SetR(1, 1);
    EXE_TWO_INSTRS("BGTZ R1 4", "ADDI R10 R0 11");
    assert(Cpu::GetPC() == 0x1010 + 4);
    assert(Cpu::GetR(10) == 11);

    //========================
    // BLTZ
    //========================
    System::Reset();
    Cpu::SetR(1, (u32)-10);
    Cpu::SetPC(0x100);
    EXE_BD_INSTR("BLTZ R1 2");
    assert(Cpu::GetPC() == 0x108);
    Cpu::SetPC(0x100);
    EXE_BD_INSTR("BLTZ R1 -1");
    assert(Cpu::GetPC() == 0x0fc);
    Cpu::SetR(1, 0);
    Cpu::SetPC(0x100);
    EXE_BD_INSTR("BLTZ R1 2");
    assert(Cpu::GetPC() == 0x104);
    Cpu::SetR(1, 1);
    Cpu::SetPC(0x100);
    EXE_BD_INSTR("BLTZ R1 2");
    assert(Cpu::GetPC() == 0x104);
    // test branch delay
    Cpu::SetR(1, (u32)-1);
    EXE_TWO_INSTRS("BLTZ R1 4", "ADDI R10 R0 11");
    assert(Cpu::GetPC() == 0x1010 + 4);
    assert(Cpu::GetR(10) == 11);

    //========================
    // BGEZ
    //========================
    System::Reset();
    Cpu::SetR(1, 10);
    Cpu::SetPC(0x100);
    EXE_BD_INSTR("BGEZ R1 2");
    assert(Cpu::GetPC() == 0x108);
    Cpu::SetPC(0x100);
    EXE_BD_INSTR("BGEZ R1 -1");
    assert(Cpu::GetPC() == 0x0fc);
    Cpu::SetR(1, 0);
    Cpu::SetPC(0x100);
    EXE_BD_INSTR("BGEZ R1 2");
    assert(Cpu::GetPC() == 0x108);
    Cpu::SetR(1, (u32)-1);
    Cpu::SetPC(0x100);
    EXE_BD_INSTR("BGEZ R1 2");
    assert(Cpu::GetPC() == 0x104);
    // test branch delay
    Cpu::SetR(1, 1);
    EXE_TWO_INSTRS("BGEZ R1 4", "ADDI R10 R0 11");
    assert(Cpu::GetPC() == 0x1010 + 4);
    assert(Cpu::GetR(10) == 11);

    //========================
    // BLTZAL
    //========================
    System::Reset();
    Cpu::SetR(1, (u32)-1);
    Cpu::SetR(31, 0);
    Cpu::SetPC(0x100);
    EXE_BD_INSTR("BLTZAL R1 4");
    assert(Cpu::GetPC() == 0x110);
    assert(Cpu::GetR(31) == 0x108);
    Cpu::SetR(31, 0);
    Cpu::SetPC(0x100);
    EXE_BD_INSTR("BLTZAL R1 -1");
    assert(Cpu::GetPC() == 0x0fc);
    assert(Cpu::GetR(31) == 0x108);
    Cpu::SetR(31, 0);
    Cpu::SetR(1, 0);
    Cpu::SetPC(0x100);
    EXE_BD_INSTR("BLTZAL R1 4");
    assert(Cpu::GetPC() == 0x104);
    assert(Cpu::GetR(31) == 0);
    Cpu::SetR(31, 0);
    Cpu::SetR(1, 1);
    Cpu::SetPC(0x100);
    EXE_BD_INSTR("BLTZAL R1 4");
    assert(Cpu::GetPC() == 0x104);
    assert(Cpu::GetR(31) == 0);
    // test branch delay
    Cpu::SetR(31, 0);
    Cpu::SetR(1, (u32)-1);
    EXE_TWO_INSTRS("BLTZAL R1 4", "ADDI R10 R0 11");
    assert(Cpu::GetPC() == 0x1010 + 4);
    assert(Cpu::GetR(10) == 11);
    assert(Cpu::GetR(31) == 0x1008 + 4);

    //========================
    // BGEZAL
    //========================
    System::Reset();
    Cpu::SetR(1, 10);
    Cpu::SetR(31, 0);
    Cpu::SetPC(0x100);
    EXE_BD_INSTR("BGEZAL R1 4");
    assert(Cpu::GetPC() == 0x110);
    assert(Cpu::GetR(31) == 0x108);
    Cpu::SetR(31, 0);
    Cpu::SetPC(0x100);
    EXE_BD_INSTR("BGEZAL R1 -1");
    assert(Cpu::GetPC() == 0x0fc);
    assert(Cpu::GetR(31) == 0x108);
    Cpu::SetR(31, 0);
    Cpu::SetR(1, 0);
    Cpu::SetPC(0x100);
    EXE_BD_INSTR("BGEZAL R1 4");
    assert(Cpu::GetPC() == 0x110);
    assert(Cpu::GetR(31) == 0x108);
    Cpu::SetR(31, 0);
    Cpu::SetR(1, (u32)-1);
    Cpu::SetPC(0x100);
    EXE_BD_INSTR("BGEZAL R1 4");
    assert(Cpu::GetPC() == 0x104);
    assert(Cpu::GetR(31) == 0);
    // test branch delay
    Cpu::SetR(31, 0);
    Cpu::SetR(1, 1);
    EXE_TWO_INSTRS("BGEZAL R1 4", "ADDI R10 R0 11");
    assert(Cpu::GetPC() == 0x1010 + 4);
    assert(Cpu::GetR(10) == 11);
    assert(Cpu::GetR(31) == 0x1008 + 4);
}

namespace Psx {
namespace Test {

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

}//end namespace
}
