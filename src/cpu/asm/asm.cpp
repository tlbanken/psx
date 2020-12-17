/*
 * asm.cpp
 *
 * Travis Banken
 * 12/13/2020
 *
 * Functions related to assembly and disasembly of MIPS instructions.
 *
 * ========================
 * <Modified MIPS Form>
 * ========================
 * Instruction:
 * ADD  R0 R1 R3
 *
 * Cop Instructions:
 * CopN [CopInstr] [Registers..]
 * Cop0 MF R0 R2
 *
 * Notes:
 * No commas (use whitespace), one instruction per line, labels start with ':',
 * no data sections, limited instruction support, no data support.
 * ========================
 */

#include <map>

#include "cpu/asm/asm.h"
#include "util/psxutil.h"

static const std::string primaryOpMap[] = {
    /*00*/ "???",  /*01*/ "BCONDZ",/*02*/ "J",    /*03*/ "JAL",
    /*04*/ "BEQ",  /*05*/ "BNE",   /*06*/ "BLEZ", /*07*/ "BGTZ",
    /*08*/ "ADDI", /*09*/ "ADDIU",  /*0a*/ "STLI", /*0b*/ "SLTIU",
    /*0c*/ "ANDI", /*0d*/ "ORI",   /*0e*/ "XORI", /*0f*/ "LUI",

    /*10*/ "COP0", /*11*/ "COP1", /*12*/ "COP2", /*13*/ "COP3",
    /*14*/ "???",  /*15*/ "???",  /*16*/ "???",  /*17*/ "???",
    /*18*/ "???",  /*19*/ "???",  /*1a*/ "???",  /*1b*/ "???",
    /*1c*/ "???",  /*1d*/ "???",  /*1e*/ "???",  /*1f*/ "???",

    /*20*/ "LB",   /*21*/ "LH",   /*22*/ "LWL",  /*23*/ "LW",
    /*24*/ "LBU",  /*25*/ "LHU",  /*26*/ "LWR",  /*27*/ "???",
    /*28*/ "SB",   /*29*/ "SH",   /*2a*/ "SWL",  /*2b*/ "SW",
    /*2c*/ "???",  /*2d*/ "???",  /*2e*/ "SWR",  /*2f*/ "???",

    /*30*/ "LWC0", /*31*/ "LWC1", /*32*/ "LWC2", /*33*/ "LWC3",
    /*34*/ "???",  /*35*/ "???",  /*36*/ "???",  /*37*/ "???",
    /*38*/ "SWC0", /*39*/ "SWC1", /*3a*/ "SWC2", /*3b*/ "SWC3",
    /*3c*/ "???",  /*3d*/ "???",  /*3e*/ "???",  /*3f*/ "???"
};

static const std::string secondaryOpMap[] = {
    /*00*/ "SLL",     /*01*/ "???",   /*02*/ "SRL",  /*03*/ "SRA",
    /*04*/ "SLLV",    /*05*/ "???",   /*06*/ "SRLV", /*07*/ "SRAV",
    /*08*/ "JR",      /*09*/ "JALR",  /*0a*/ "???",  /*0b*/ "???",
    /*0c*/ "SYSCALL", /*0d*/ "BREAK", /*0e*/ "???",  /*0f*/ "???",

    /*10*/ "MFHI", /*11*/ "MTHI", /*12*/ "MFLO", /*13*/ "MTLO",
    /*14*/ "???",  /*15*/ "???",  /*16*/ "???",  /*17*/ "???",
    /*18*/ "MULT", /*19*/ "MULTU",/*1a*/ "DIV",  /*1b*/ "DIVU",
    /*1c*/ "???",  /*1d*/ "???",  /*1e*/ "???",  /*1f*/ "???",

    /*20*/ "ADD",  /*21*/ "ADDU", /*22*/ "SUB",  /*23*/ "SUBU",
    /*24*/ "SUBU", /*25*/ "AND",  /*26*/ "OR",   /*27*/ "XOR",
    /*28*/ "SB",   /*29*/ "SH",   /*2a*/ "SWL",  /*2b*/ "SW",
    /*2c*/ "???",  /*2d*/ "???",  /*2e*/ "SWR",  /*2f*/ "???",

    /*30*/ "???",  /*31*/ "???",  /*32*/ "???",  /*33*/ "???",
    /*34*/ "???",  /*35*/ "???",  /*36*/ "???",  /*37*/ "???",
    /*38*/ "???",  /*39*/ "???",  /*3a*/ "???",  /*3b*/ "???",
    /*3c*/ "???",  /*3d*/ "???",  /*3e*/ "???",  /*3f*/ "???"
};

static const std::map<std::string, u8> primaryStrToOp = {
    {primaryOpMap[0x00], 0x00}, {primaryOpMap[0x01], 0x01},
    {primaryOpMap[0x02], 0x02}, {primaryOpMap[0x03], 0x03},
    {primaryOpMap[0x04], 0x04}, {primaryOpMap[0x05], 0x05},
    {primaryOpMap[0x06], 0x06}, {primaryOpMap[0x07], 0x07},
    {primaryOpMap[0x08], 0x08}, {primaryOpMap[0x09], 0x09},
    {primaryOpMap[0x0a], 0x0a}, {primaryOpMap[0x0b], 0x0b},
    {primaryOpMap[0x0c], 0x0c}, {primaryOpMap[0x0d], 0x0d},
    {primaryOpMap[0x0e], 0x0e}, {primaryOpMap[0x0f], 0x0f},

    {primaryOpMap[0x10], 0x10}, {primaryOpMap[0x11], 0x11},
    {primaryOpMap[0x12], 0x12}, {primaryOpMap[0x13], 0x13},
    {primaryOpMap[0x14], 0x14}, {primaryOpMap[0x15], 0x15},
    {primaryOpMap[0x16], 0x16}, {primaryOpMap[0x17], 0x17},
    {primaryOpMap[0x18], 0x18}, {primaryOpMap[0x19], 0x19},
    {primaryOpMap[0x1a], 0x1a}, {primaryOpMap[0x1b], 0x1b},
    {primaryOpMap[0x1c], 0x1c}, {primaryOpMap[0x1d], 0x1d},
    {primaryOpMap[0x1e], 0x1e}, {primaryOpMap[0x1f], 0x1f},

    {primaryOpMap[0x20], 0x20}, {primaryOpMap[0x21], 0x21},
    {primaryOpMap[0x22], 0x22}, {primaryOpMap[0x23], 0x23},
    {primaryOpMap[0x24], 0x24}, {primaryOpMap[0x25], 0x25},
    {primaryOpMap[0x26], 0x26}, {primaryOpMap[0x27], 0x27},
    {primaryOpMap[0x28], 0x28}, {primaryOpMap[0x29], 0x29},
    {primaryOpMap[0x2a], 0x2a}, {primaryOpMap[0x2b], 0x2b},
    {primaryOpMap[0x2c], 0x2c}, {primaryOpMap[0x2d], 0x2d},
    {primaryOpMap[0x2e], 0x2e}, {primaryOpMap[0x2f], 0x2f},

    {primaryOpMap[0x30], 0x30}, {primaryOpMap[0x31], 0x31},
    {primaryOpMap[0x32], 0x32}, {primaryOpMap[0x33], 0x33},
    {primaryOpMap[0x34], 0x34}, {primaryOpMap[0x35], 0x35},
    {primaryOpMap[0x36], 0x36}, {primaryOpMap[0x37], 0x37},
    {primaryOpMap[0x38], 0x38}, {primaryOpMap[0x39], 0x39},
    {primaryOpMap[0x3a], 0x3a}, {primaryOpMap[0x3b], 0x3b},
    {primaryOpMap[0x3c], 0x3c}, {primaryOpMap[0x3d], 0x3d},
    {primaryOpMap[0x3e], 0x3e}, {primaryOpMap[0x3f], 0x3f},
};

static const std::map<std::string, u8> secondaryStrToOp = {
    {secondaryOpMap[0x00], 0x00}, {secondaryOpMap[0x01], 0x01},
    {secondaryOpMap[0x02], 0x02}, {secondaryOpMap[0x03], 0x03},
    {secondaryOpMap[0x04], 0x04}, {secondaryOpMap[0x05], 0x05},
    {secondaryOpMap[0x06], 0x06}, {secondaryOpMap[0x07], 0x07},
    {secondaryOpMap[0x08], 0x08}, {secondaryOpMap[0x09], 0x09},
    {secondaryOpMap[0x0a], 0x0a}, {secondaryOpMap[0x0b], 0x0b},
    {secondaryOpMap[0x0c], 0x0c}, {secondaryOpMap[0x0d], 0x0d},
    {secondaryOpMap[0x0e], 0x0e}, {secondaryOpMap[0x0f], 0x0f},

    {secondaryOpMap[0x10], 0x10}, {secondaryOpMap[0x11], 0x11},
    {secondaryOpMap[0x12], 0x12}, {secondaryOpMap[0x13], 0x13},
    {secondaryOpMap[0x14], 0x14}, {secondaryOpMap[0x15], 0x15},
    {secondaryOpMap[0x16], 0x16}, {secondaryOpMap[0x17], 0x17},
    {secondaryOpMap[0x18], 0x18}, {secondaryOpMap[0x19], 0x19},
    {secondaryOpMap[0x1a], 0x1a}, {secondaryOpMap[0x1b], 0x1b},
    {secondaryOpMap[0x1c], 0x1c}, {secondaryOpMap[0x1d], 0x1d},
    {secondaryOpMap[0x1e], 0x1e}, {secondaryOpMap[0x1f], 0x1f},

    {secondaryOpMap[0x20], 0x20}, {secondaryOpMap[0x21], 0x21},
    {secondaryOpMap[0x22], 0x22}, {secondaryOpMap[0x23], 0x23},
    {secondaryOpMap[0x24], 0x24}, {secondaryOpMap[0x25], 0x25},
    {secondaryOpMap[0x26], 0x26}, {secondaryOpMap[0x27], 0x27},
    {secondaryOpMap[0x28], 0x28}, {secondaryOpMap[0x29], 0x29},
    {secondaryOpMap[0x2a], 0x2a}, {secondaryOpMap[0x2b], 0x2b},
    {secondaryOpMap[0x2c], 0x2c}, {secondaryOpMap[0x2d], 0x2d},
    {secondaryOpMap[0x2e], 0x2e}, {secondaryOpMap[0x2f], 0x2f},

    {secondaryOpMap[0x30], 0x30}, {secondaryOpMap[0x31], 0x31},
    {secondaryOpMap[0x32], 0x32}, {secondaryOpMap[0x33], 0x33},
    {secondaryOpMap[0x34], 0x34}, {secondaryOpMap[0x35], 0x35},
    {secondaryOpMap[0x36], 0x36}, {secondaryOpMap[0x37], 0x37},
    {secondaryOpMap[0x38], 0x38}, {secondaryOpMap[0x39], 0x39},
    {secondaryOpMap[0x3a], 0x3a}, {secondaryOpMap[0x3b], 0x3b},
    {secondaryOpMap[0x3c], 0x3c}, {secondaryOpMap[0x3d], 0x3d},
    {secondaryOpMap[0x3e], 0x3e}, {secondaryOpMap[0x3f], 0x3f},
};

static const std::map<std::string, u8> bcondzStrToOp = {
    {{"BLTZ", 0x00}, {"BGEZ", 0x01}, {"BLTZAL", 0x10}, {"BGEZAL", 0x11}}
};

/*
 * Converts a bcondz Op to its name.
 */
static std::string bcondzOpToStr(u8 bcondzOp)
{
    std::string res;
    switch (bcondzOp) {
    case 0x00:
        res = "BLTZ";
        break;
    case 0x01:
        res = "BGEZ";
        break;
    case 0x10:
        res = "BLTZAL";
        break;
    case 0x11:
        res = "BGEZAL";
        break;
    default:
        res = "???";
    }
    return res;
}


/*
 * Disassmble the given instruction into a coprocessor instruction string.
 */
static std::string copDasm(const Asm::Instruction& instr)
{
    std::string res("");
    if ((instr.copOp & 0x10) != 0) {
        if ((instr.copOp & 0x08) != 0) {
            // Branch
            std::string name = (instr.rt & 0x1) == 0 ? "BCF" : "BCT";
            res = PSX_FMT("{} {:+d}", name, (i16) instr.imm16);
        } else {
            // Moves
            switch (instr.copOp) {
            case 0x00: // MFCn
                res = PSX_FMT("MF R{}, R{}_data", instr.rt, instr.rd);
                break;
            case 0x02: // CFCn
                res = PSX_FMT("CF R{}, R{}_ctrl", instr.rt, instr.rd);
                break;
            case 0x04: // MTCn
                res = PSX_FMT("MT R{}, R{}_data", instr.rt, instr.rd);
                break;
            case 0x06: // CTCn
                res = PSX_FMT("CT R{}, R{}_ctrl", instr.rt, instr.rd);
                break;
            default:
                res = "???";
                break;
            }
        }
    } else {
        // Cop Command
        res = PSX_FMT("CMD#{:07x}", instr.imm25);
    }
    return res;
}

/*
 * Disassemble primary Op instructions (opcode != 0x00).
 */
static std::string primOpDasm(const Asm::Instruction& instr)
{
    std::string res;
    switch (instr.op) {
    // Jump Instructions
    case 0x02: // J
    case 0x03: // JAL
        res = PSX_FMT("0x[PC]{:07x}", instr.target << 2);
        break;
    // Branch Instructions
    case 0x04: // BEQ
    case 0x05: // BNE
        res = PSX_FMT("R{}, R{}, {:+d}", instr.rs, instr.rt, (i16)instr.imm16);
        break;
    case 0x06: // BLEZ
    case 0x07: // BGTZ
        res = PSX_FMT("R{}, {:+d}", instr.rs, (i16)instr.imm16);
        break;
    // ALU Imm
    case 0x09: // ADDIU
    case 0x0b: // SLTIU
    case 0x08: // ADDI
    case 0x0a: // SLTI
        res = PSX_FMT("R{}, R{}, {:d}", instr.rs, instr.rt, (i16)instr.imm16);
        break;
    case 0x0c: // ANDI
    case 0x0d: // ORI
    case 0x0e: // XORI
        res = PSX_FMT("R{}, R{}, 0x{:04x}", instr.rs, instr.rt, instr.imm16);
        break;
    // load imm
    case 0x0f: // LUI
        res = PSX_FMT("R{}, 0x{:04x}", instr.rt, instr.imm16);
        break;
    // Coprocessor
    case 0x10: // COP0
    case 0x11: // COP1
    case 0x12: // COP2
    case 0x13: // COP3
        res = copDasm(instr);
        break;
    // Load/Store
    case 0x20: // LB
    case 0x21: // LH
    case 0x22: // LWL
    case 0x23: // LW
    case 0x24: // LBU
    case 0x25: // LHU
    case 0x26: // LWR
    case 0x28: // SB
    case 0x29: // SH
    case 0x2a: // SWL
    case 0x2b: // SW
    case 0x2e: // SWR
        res = PSX_FMT("R{}, {:d}(R{})", instr.rt, (i16)instr.imm16, instr.rs);
        break;
    // Load/Store Coprocessor
    case 0x30: // LWC0
    case 0x31: // LWC1
    case 0x32: // LWC2
    case 0x33: // LWC3
    case 0x38: // SWC0
    case 0x39: // SWC1
    case 0x3a: // SWC2
    case 0x3b: // SWC3
        res = PSX_FMT("CopR{}, {:d}(R{})", instr.rt, (i16)instr.imm16, instr.rs);
        break;
    default:
        res = "";
        break;
    }

    return res;
}

/*
 * Disassemble Secondary instructions (opcode == 0x00).
 */
static std::string secOpDasm(const Asm::Instruction& instr)
{
    std::string res("");
    switch (instr.funct) {
    // Shifts
    case 0x00: // SLL
    case 0x02: // SRL
    case 0x03: // SRA
    case 0x04: // SLLV
    case 0x06: // SRLV
    case 0x07: // SRAV
        res = PSX_FMT("R{}, R{}, {:d}", instr.rd, instr.rt, instr.shamt);
        break;
    // Jump Registers
    case 0x08: // JR
        res = PSX_FMT("R{}", instr.rs);
        break;
    case 0x09: // JALR
        res = PSX_FMT("R{}, R{}", instr.rs, instr.rd);
        break;
    // SPECIAL
    case 0x0c: // SYSCALL
    case 0x0d: // BREAK
        res = "";
        break;
    // Move Hi/Lo
    case 0x10: // MFHI
    case 0x11: // MTHI
    case 0x12: // MFLO
    case 0x13: // MTLO
        res = PSX_FMT("R{}", instr.rd);
        break;
    // MULT/DIV
    case 0x18: // MULT
    case 0x19: // MULTU
    case 0x1a: // DIV
    case 0x1b: // DIVU
        res = PSX_FMT("R{}, R{}", instr.rs, instr.rt);
        break;
    // ALU Ops
    case 0x20: // ADD
    case 0x21: // ADDU
    case 0x22: // SUB
    case 0x23: // SUBU
    case 0x24: // AND
    case 0x25: // OR
    case 0x26: // XOR
    case 0x27: // NOR
    case 0x2a: // SLT
    case 0x2b: // SLTU
        res = PSX_FMT("R{}, R{}, R{}", instr.rd, instr.rs, instr.rt);
        break;
    default:
        res = "";
        break;
    }

    return res;
}

namespace Asm {
    /*
     * Assemble the given instruction. The instruction should be one instruction
     * in the Modified MIPS Form specified above. Returns the resulting binary
     * form.
     */
    u32 asmInstruction(const std::string& strInstr)
    {
        return 0;
    }

    /*
     * Disassemble the instruction and return a string formatted in the 
     * Modified MIPS Form specified above.
     */
    std::string dasmInstruction(u32 rawInstr)
    {
        std::string strInstr;
        const Asm::Instruction instr = decodeRawInstr(rawInstr);
        if (instr.op == 0x01) {
            // BCONDZ
            strInstr = PSX_FMT("{:<8}R{}, {:+d}", bcondzOpToStr(instr.bcondzOp), instr.rs, (i16)instr.imm16);
        } else if (instr.op == 0x00) {
            // SPECIAL
            // look-up in map
            strInstr = PSX_FMT("{:<8}{}", secondaryOpMap[instr.funct], secOpDasm(instr));
        } else {
            // NORMAL
            strInstr = PSX_FMT("{:<8}{}", primaryOpMap[instr.op], primOpDasm(instr));
        }
        return strInstr;
    }

    /*
     * Transforms a raw 32-bit instruction into a structured instruction for easy
     * manipulation.
     */
    Asm::Instruction decodeRawInstr(u32 rawInstr)
    {
        Asm::Instruction instr;
        instr.op = (rawInstr >> 26) & 0x3f; // 26-31
        instr.rs = (rawInstr >> 21) & 0x1f; // 21-25
        instr.rt = (rawInstr >> 16) & 0x1f; // 16-20
        instr.rd = (rawInstr >> 11) & 0x1f; // 11-15
        instr.shamt = (rawInstr >> 6) & 0x1f; // 6-10
        instr.funct = (rawInstr >> 0) & 0x3f; // 0-5

        instr.imm16 = (rawInstr >> 0) & 0xffff; // 0-15
        
        instr.target = (rawInstr >> 0) & 0x3ff'ffff; // 0-25

        instr.imm25 = (rawInstr >> 0) & 0x1ff'ffff; // 0-24

        return instr;
    }
}

