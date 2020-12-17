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
            u8 bcondzOp; // bcondz instructions
        };
        union {
            u8 rs;
            u8 copOp; // cop instructions
        };

        // i-type
        u16 imm16;

        // j-type
        u32 target;

        // r-type
        union {
            u8 rd;
            u8 copBranchOp; // cop instruction
        };
        u8 shamt;
        u8 funct;

        // cop command
        u32 imm25;
    };

    Instruction decodeRawInstr(u32 rawInstr);
    std::string dasmInstruction(u32 rawInstr);
    u32 asmInstruction(const std::string& strInstr);
}

