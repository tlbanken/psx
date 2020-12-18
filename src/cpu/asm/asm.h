/*
 * asm.h
 *
 * Travis Banken
 * 12/13/2020
 *
 * Header file for MIPS assembly tools.
 */

#pragma once

#include <string>

#include "util/psxutil.h"

namespace Asm {
    struct Instruction {
        // standard
        u8 op;
        union {
            u8 rt;
            u8 bcondz_op; // bcondz instructions
            u8 cop_branch_op; // cop instruction
        };
        union {
            u8 rs;
            u8 cop_op; // cop instructions
        };

        // i-type
        u16 imm16;

        // j-type
        u32 target;

        // r-type
        u8 rd;
        u8 shamt;
        u8 funct;

        // cop command
        u32 imm25;
    };

    Instruction DecodeRawInstr(u32 raw_instr);
    std::string DasmInstruction(u32 raw_instr);
    u32 AsmInstruction(const std::string& str_instr);
}

