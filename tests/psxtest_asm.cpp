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

#include "util/psxutil.h"
#include "util/psxlog.h"
#include "cpu/asm/asm.h"

#define LOG_DASM(instr) PSXLOG_INFO("Test-Dasm", PSX_FMT("0x{:08x} -> [{}]", instr, Asm::dasmInstruction(instr)))

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

namespace psxtest {
    void asmTests()
    {
        std::cout << PSX_FANCYTITLE("ASM/DASM TESTS");
        PSXLOG_INFO("Test-Asm/Dasm", "Starting Disassemble tests");
        dasmTests();
    }
}
