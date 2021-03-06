/*
 * psxtest_asm.cpp
 *
 * Travis Banken
 * 12/15/2020
 *
 * Tests for the assembler and disassembler.
 */

#include <iostream>

#include "fmt/core.h"

#include "util/psxutil.hh"
#include "util/psxlog.hh"
#include "cpu/asm/asm.hh"

#define LOG_DASM(instr) PSXLOG_INFO("Test-Dasm", "0x{:08x} -> [{}]", instr, Cpu::Asm::DasmInstruction(instr))
#define LOG_ASM(instr) PSXLOG_INFO("Test-Asm", "[{}] -> 0x{:08x}", instr, Cpu::Asm::AsmInstruction(instr))
#define LOG_COMBO(instr) {\
    u32 _i = Cpu::Asm::AsmInstruction(instr);\
    std::string _si = Cpu::Asm::DasmInstruction(_i);\
    PSXLOG_INFO("Test-Combo", "[{:<20}] -> 0x{:08x} -> [{}]", instr, _i, _si); \
}

using namespace Psx;

static void dasmTests()
{
    /*
    3c048000 -> lui $4,0x8000
    34844000 -> ori $4,$4,0x4000
    3c05b400 -> lui $5,0xb400
    34a503f8 -> ori $5,$5,0x3f8
    80860000 -> lb $6,0($4)
    10c00005 -> beq $6,$0,0x8000102c
    00000000 -> sll $0,$0,0x0
    a0a60000 -> sb $6,0($5)
    24840001 -> addiu $4,$4,1
    08000404 -> j 0x80001010
    00000000 -> sll $0,$0,0x0
    3c05bfbf -> lui $5,0xbfbf
    34a50004 -> ori $5,$5,0x4
    3c060000 -> lui $6,0x0
    34c6002a -> ori $6,$6,0x2a
    a0a60000 -> sb $6,0($5)
    */
    u32 instr = 0x3c048000; 
    LOG_DASM(instr);
    instr = 0x34844000; 
    LOG_DASM(instr);
    instr = 0x3c05b400; 
    LOG_DASM(instr);
    instr = 0x34a503f8; 
    LOG_DASM(instr);
    instr = 0x80860000; 
    LOG_DASM(instr);
    instr = 0x10c00005; 
    LOG_DASM(instr);
    instr = 0x00000000; 
    LOG_DASM(instr);
    instr = 0xa0a60000; 
    LOG_DASM(instr);
    instr = 0x24840001; 
    LOG_DASM(instr);
    instr = 0x08000404; 
    LOG_DASM(instr);
    instr = 0x00000000; 
    LOG_DASM(instr);
    instr = 0x3c05bfbf; 
    LOG_DASM(instr);
    instr = 0x34a50004; 
    LOG_DASM(instr);
    instr = 0x3c060000; 
    LOG_DASM(instr);
    instr = 0x34c6002a; 
    LOG_DASM(instr);
    instr = 0xa0a60000; 
    LOG_DASM(instr);
}

static void asmTests()
{
    /*
    3c048000 -> lui $4,0x8000
    34844000 -> ori $4,$4,0x4000
    3c05b400 -> lui $5,0xb400
    34a503f8 -> ori $5,$5,0x3f8
    80860000 -> lb $6,0($4)
    10c00005 -> beq $6,$0,0x8000102c
    00000000 -> sll $0,$0,0x0
    a0a60000 -> sb $6,0($5)
    24840001 -> addiu $4,$4,1
    08000404 -> j 0x80001010
    00000000 -> sll $0,$0,0x0
    3c05bfbf -> lui $5,0xbfbf
    34a50004 -> ori $5,$5,0x4
    3c060000 -> lui $6,0x0
    34c6002a -> ori $6,$6,0x2a
    a0a60000 -> sb $6,0($5)
    */
    PSXLOG_INFO("Test-Asm/Dasm", "Starting Assembler Tests");
    LOG_ASM("SLT R5 R1 R0");
    LOG_ASM("lui    R4 0x8000");
    LOG_ASM("OrI R4 R4 0x4000");
    LOG_ASM("LUI   R5 0xb400");
    LOG_ASM("Ori    R5 R5 0x3f8");
    LOG_ASM("LB     R6 0 R4");
    LOG_ASM("BEQ    R6 R0 5");
    LOG_ASM("sll R0 R0 0x0");
    LOG_ASM("sb R6 0 R5");
    LOG_ASM("addiu R4 R4 1");
    LOG_ASM("j      0x1010");
    LOG_ASM("sll    R0 R0 0x0");
    LOG_ASM("LUI    R5 0xbfbf");
    LOG_ASM("OrI    R5 R5 0x04");
    LOG_ASM("ORI    R6 R6 0x2a");
    LOG_ASM("sb     R6 0 R5");
}

static void comboTests()
{
    PSXLOG_INFO("Test-Asm/Dasm", "Starting Combo Tests");
    LOG_COMBO("ADDI R1 R0 13");
    LOG_COMBO("ADDIU R1 R0 10");
    LOG_COMBO("SLTI R1 R0 1");
    LOG_COMBO("SLTIU R1 R0 1");
    LOG_COMBO("ANDI R3 R10 0x1");
    LOG_COMBO("ORI R2 R13 0xa2");
    LOG_COMBO("XORI R4 R3 0xbe");
    LOG_COMBO("LUI R2 0xbefe");
    LOG_COMBO("ADD R4 R3 R1");
    LOG_COMBO("ADDU R4 R3 R1");
    LOG_COMBO("SUB R3 R3 R1");
    LOG_COMBO("SUBU R4 R3 R10");
    LOG_COMBO("SLT R5 R1 R0");
    LOG_COMBO("MULT R1 R0");
    LOG_COMBO("MULTU R1 R0");
    LOG_COMBO("DIV R1 R0");
    LOG_COMBO("DIVU R1 R0");
    LOG_COMBO("LB R1 -1 R0");
    LOG_COMBO("LBU R1 32 R0");
    LOG_COMBO("LH R1 32 R0");
    LOG_COMBO("LHU R4 32 R2");
    LOG_COMBO("LW R4 32 R2");
    LOG_COMBO("LWR R4 -2 R2");
    LOG_COMBO("LWL R10 32 R2");
    LOG_COMBO("SB R1 -1 R0");
    LOG_COMBO("SH R1 2 R0");
    LOG_COMBO("SW R6 89 R12");
    LOG_COMBO("SWR R14 -29 R7");
    LOG_COMBO("SWL R1 132 R20");
    LOG_COMBO("J 0x1230");
    LOG_COMBO("JAL 0x1324");
    LOG_COMBO("JR R2");
    LOG_COMBO("JALR R1 R2");
}

namespace Psx {
namespace Test {

void AsmDasmTests()
{
    std::cout << PSX_FANCYTITLE("ASM/DASM TESTS");
    PSXLOG_INFO("Test-Asm/Dasm", "Starting Disassembler tests");
    dasmTests();
    asmTests();
    comboTests();
}

}// end namespace
}
