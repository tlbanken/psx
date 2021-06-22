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
 * SW   R0 20 R3
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
#include <exception>
#include <istream>
#include <vector>
#include <iostream>
#include <sstream>

#include "cpu/asm/asm.hh"
#include "util/psxutil.hh"

// helpers for forming instructions
#define SET_INSTR(instr, val) (u32)(instr) | (u32)(val)
#define SET_OP(instr, op)               SET_INSTR(instr, (u32)((op) & 0x3f) << 26)
#define SET_RS(instr, rs)               SET_INSTR(instr, (u32)((rs) & 0x1f) << 21)
#define SET_RT(instr, rt)               SET_INSTR(instr, (u32)((rt) & 0x1f) << 16)
#define SET_RD(instr, rd)               SET_INSTR(instr, (u32)((rd) & 0x1f) << 11)
#define SET_SHAMT(instr, shamt)         SET_INSTR(instr, (u32)((shamt) & 0x1f) << 6)
#define SET_FUNCT(instr, funct)         SET_INSTR(instr, (u32)((funct) & 0x3f) << 0)
#define SET_IMM16(instr, imm16)         SET_INSTR(instr, (u32)((imm16) & 0xffff) << 0)
#define SET_TARGET(instr, target)       SET_INSTR(instr, (u32)((target) & 0x3ff'ffff) << 0)
#define SET_IMM25(instr, imm25)         SET_INSTR(instr, (u32)((imm25) & 0x1ff'ffff) << 0)
#define SET_BCONDZOP(instr, bcondzop)   SET_RT(instr, bcondzop)
#define SET_COPOP(instr, copop)         SET_RS(instr, copop)
#define SET_COPBRANCHOP(instr, cbo)     SET_RT(instr, cbo)

using namespace Psx::Cpu;

static const std::string prim_op_map[] = {
    /*00*/ "???",  /*01*/ "BCONDZ",/*02*/ "J",    /*03*/ "JAL",
    /*04*/ "BEQ",  /*05*/ "BNE",   /*06*/ "BLEZ", /*07*/ "BGTZ",
    /*08*/ "ADDI", /*09*/ "ADDIU",  /*0a*/ "SLTI", /*0b*/ "SLTIU",
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

static const std::string sec_op_map[] = {
    /*00*/ "SLL",     /*01*/ "???",   /*02*/ "SRL",  /*03*/ "SRA",
    /*04*/ "SLLV",    /*05*/ "???",   /*06*/ "SRLV", /*07*/ "SRAV",
    /*08*/ "JR",      /*09*/ "JALR",  /*0a*/ "???",  /*0b*/ "???",
    /*0c*/ "SYSCALL", /*0d*/ "BREAK", /*0e*/ "???",  /*0f*/ "???",

    /*10*/ "MFHI", /*11*/ "MTHI", /*12*/ "MFLO", /*13*/ "MTLO",
    /*14*/ "???",  /*15*/ "???",  /*16*/ "???",  /*17*/ "???",
    /*18*/ "MULT", /*19*/ "MULTU",/*1a*/ "DIV",  /*1b*/ "DIVU",
    /*1c*/ "???",  /*1d*/ "???",  /*1e*/ "???",  /*1f*/ "???",

    /*20*/ "ADD",  /*21*/ "ADDU", /*22*/ "SUB",  /*23*/ "SUBU",
    /*24*/ "AND",  /*25*/ "OR",   /*26*/ "XOR",  /*27*/ "NOR",
    /*28*/ "???",  /*29*/ "???",  /*2a*/ "SLT",  /*2b*/ "SLTU",
    /*2c*/ "???",  /*2d*/ "???",  /*2e*/ "???",  /*2f*/ "???",

    /*30*/ "???",  /*31*/ "???",  /*32*/ "???",  /*33*/ "???",
    /*34*/ "???",  /*35*/ "???",  /*36*/ "???",  /*37*/ "???",
    /*38*/ "???",  /*39*/ "???",  /*3a*/ "???",  /*3b*/ "???",
    /*3c*/ "???",  /*3d*/ "???",  /*3e*/ "???",  /*3f*/ "???"
};

static const std::map<std::string, u8> prim_str_to_op = {
    {prim_op_map[0x00], 0x00}, {prim_op_map[0x01], 0x01},
    {prim_op_map[0x02], 0x02}, {prim_op_map[0x03], 0x03},
    {prim_op_map[0x04], 0x04}, {prim_op_map[0x05], 0x05},
    {prim_op_map[0x06], 0x06}, {prim_op_map[0x07], 0x07},
    {prim_op_map[0x08], 0x08}, {prim_op_map[0x09], 0x09},
    {prim_op_map[0x0a], 0x0a}, {prim_op_map[0x0b], 0x0b},
    {prim_op_map[0x0c], 0x0c}, {prim_op_map[0x0d], 0x0d},
    {prim_op_map[0x0e], 0x0e}, {prim_op_map[0x0f], 0x0f},

    {prim_op_map[0x10], 0x10}, {prim_op_map[0x11], 0x11},
    {prim_op_map[0x12], 0x12}, {prim_op_map[0x13], 0x13},
    {prim_op_map[0x14], 0x14}, {prim_op_map[0x15], 0x15},
    {prim_op_map[0x16], 0x16}, {prim_op_map[0x17], 0x17},
    {prim_op_map[0x18], 0x18}, {prim_op_map[0x19], 0x19},
    {prim_op_map[0x1a], 0x1a}, {prim_op_map[0x1b], 0x1b},
    {prim_op_map[0x1c], 0x1c}, {prim_op_map[0x1d], 0x1d},
    {prim_op_map[0x1e], 0x1e}, {prim_op_map[0x1f], 0x1f},

    {prim_op_map[0x20], 0x20}, {prim_op_map[0x21], 0x21},
    {prim_op_map[0x22], 0x22}, {prim_op_map[0x23], 0x23},
    {prim_op_map[0x24], 0x24}, {prim_op_map[0x25], 0x25},
    {prim_op_map[0x26], 0x26}, {prim_op_map[0x27], 0x27},
    {prim_op_map[0x28], 0x28}, {prim_op_map[0x29], 0x29},
    {prim_op_map[0x2a], 0x2a}, {prim_op_map[0x2b], 0x2b},
    {prim_op_map[0x2c], 0x2c}, {prim_op_map[0x2d], 0x2d},
    {prim_op_map[0x2e], 0x2e}, {prim_op_map[0x2f], 0x2f},

    {prim_op_map[0x30], 0x30}, {prim_op_map[0x31], 0x31},
    {prim_op_map[0x32], 0x32}, {prim_op_map[0x33], 0x33},
    {prim_op_map[0x34], 0x34}, {prim_op_map[0x35], 0x35},
    {prim_op_map[0x36], 0x36}, {prim_op_map[0x37], 0x37},
    {prim_op_map[0x38], 0x38}, {prim_op_map[0x39], 0x39},
    {prim_op_map[0x3a], 0x3a}, {prim_op_map[0x3b], 0x3b},
    {prim_op_map[0x3c], 0x3c}, {prim_op_map[0x3d], 0x3d},
    {prim_op_map[0x3e], 0x3e}, {prim_op_map[0x3f], 0x3f},
};

static const std::map<std::string, u8> sec_str_to_op = {
    {sec_op_map[0x00], 0x00}, {sec_op_map[0x01], 0x01},
    {sec_op_map[0x02], 0x02}, {sec_op_map[0x03], 0x03},
    {sec_op_map[0x04], 0x04}, {sec_op_map[0x05], 0x05},
    {sec_op_map[0x06], 0x06}, {sec_op_map[0x07], 0x07},
    {sec_op_map[0x08], 0x08}, {sec_op_map[0x09], 0x09},
    {sec_op_map[0x0a], 0x0a}, {sec_op_map[0x0b], 0x0b},
    {sec_op_map[0x0c], 0x0c}, {sec_op_map[0x0d], 0x0d},
    {sec_op_map[0x0e], 0x0e}, {sec_op_map[0x0f], 0x0f},

    {sec_op_map[0x10], 0x10}, {sec_op_map[0x11], 0x11},
    {sec_op_map[0x12], 0x12}, {sec_op_map[0x13], 0x13},
    {sec_op_map[0x14], 0x14}, {sec_op_map[0x15], 0x15},
    {sec_op_map[0x16], 0x16}, {sec_op_map[0x17], 0x17},
    {sec_op_map[0x18], 0x18}, {sec_op_map[0x19], 0x19},
    {sec_op_map[0x1a], 0x1a}, {sec_op_map[0x1b], 0x1b},
    {sec_op_map[0x1c], 0x1c}, {sec_op_map[0x1d], 0x1d},
    {sec_op_map[0x1e], 0x1e}, {sec_op_map[0x1f], 0x1f},

    {sec_op_map[0x20], 0x20}, {sec_op_map[0x21], 0x21},
    {sec_op_map[0x22], 0x22}, {sec_op_map[0x23], 0x23},
    {sec_op_map[0x24], 0x24}, {sec_op_map[0x25], 0x25},
    {sec_op_map[0x26], 0x26}, {sec_op_map[0x27], 0x27},
    {sec_op_map[0x28], 0x28}, {sec_op_map[0x29], 0x29},
    {sec_op_map[0x2a], 0x2a}, {sec_op_map[0x2b], 0x2b},
    {sec_op_map[0x2c], 0x2c}, {sec_op_map[0x2d], 0x2d},
    {sec_op_map[0x2e], 0x2e}, {sec_op_map[0x2f], 0x2f},

    {sec_op_map[0x30], 0x30}, {sec_op_map[0x31], 0x31},
    {sec_op_map[0x32], 0x32}, {sec_op_map[0x33], 0x33},
    {sec_op_map[0x34], 0x34}, {sec_op_map[0x35], 0x35},
    {sec_op_map[0x36], 0x36}, {sec_op_map[0x37], 0x37},
    {sec_op_map[0x38], 0x38}, {sec_op_map[0x39], 0x39},
    {sec_op_map[0x3a], 0x3a}, {sec_op_map[0x3b], 0x3b},
    {sec_op_map[0x3c], 0x3c}, {sec_op_map[0x3d], 0x3d},
    {sec_op_map[0x3e], 0x3e}, {sec_op_map[0x3f], 0x3f},
};

static const std::map<std::string, u8> bcondz_str_to_op = {
    {{"BLTZ", 0x00}, {"BGEZ", 0x01}, {"BLTZAL", 0x10}, {"BGEZAL", 0x11}}
};

/*
 * Converts a bcondz Op to its name.
 */
static std::string bcondzOpToStr(u8 bcondz_op)
{
    std::string res;
    switch (bcondz_op) {
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
    if ((instr.cop_op & 0x10) == 0) {
        if ((instr.cop_op & 0x08) != 0) {
            // Branch
            std::string name = (instr.cop_branch_op & 0x1) == 0 ? "BCF" : "BCT";
            res = PSX_FMT("{} {:+d}", name, (i16) instr.imm16);
        } else {
            // Moves
            switch (instr.cop_op) {
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
 * Assemble the tokens for a coprocessor instruction.
 */
static u32 copAsm(const std::vector<std::string>& tokens)
{
    u32 raw_instr = 0;
    std::string op = tokens.at(1);
    for (auto& c : op) {
        c = (char)std::toupper(c);
    }

    if (op.substr(0, 3) == "CMD") {
        // command
        raw_instr = SET_COPOP(raw_instr, 0x10);
        u32 imm25 = (u32)std::stoi(op.substr(4), nullptr, 0);
        raw_instr = SET_IMM25(raw_instr, imm25);
    } else if (op == "BCF" || op == "BCT") {
        // branches
        u32 cbo = op == "BCF" ? 0x00 : 0x01;
        raw_instr = SET_COPOP(raw_instr, 0x08);
        raw_instr = SET_COPBRANCHOP(raw_instr, cbo);
        u32 imm16 = (u32)std::stoi(tokens.at(2), nullptr, 0);
        raw_instr = SET_IMM16(raw_instr, imm16);
    } else {
        // Moves
        u32 cop_op = 0;
        if (op == "MF") {
            cop_op = 0x00;
        } else if (op == "CF") {
            cop_op = 0x02;
        } else if (op == "MT") {
            cop_op = 0x04;
        } else if (op == "CT") {
            cop_op = 0x06;
        } else {
            throw std::runtime_error(PSX_FMT("COP ASM: Unknown COP OP [{}]", op));
        }

        raw_instr = SET_COPOP(raw_instr, cop_op);
        PSX_ASSERT(tokens.at(2)[0] == 'R');
        u32 rt = (u32)std::stoi(tokens.at(2).substr(1), nullptr, 0);
        raw_instr = SET_RT(raw_instr, rt);
        PSX_ASSERT(tokens.at(3)[0] == 'R');
        u32 rd = (u32)std::stoi(tokens.at(3).substr(1), nullptr, 0);
        raw_instr = SET_RD(raw_instr, rd);
    }
    return raw_instr;
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
        res = PSX_FMT("R{}, R{}, {:d}", instr.rt, instr.rs, (i16)instr.imm16);
        break;
    case 0x0c: // ANDI
    case 0x0d: // ORI
    case 0x0e: // XORI
        res = PSX_FMT("R{}, R{}, 0x{:04x}", instr.rt, instr.rs, instr.imm16);
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
        res = PSX_FMT("R{}, R{}, {:d}", instr.rd, instr.rt, instr.shamt);
        break;
    case 0x04: // SLLV
    case 0x06: // SRLV
    case 0x07: // SRAV
        res = PSX_FMT("R{}, R{}, R{}", instr.rd, instr.rs, instr.rt);
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

/*
 * Assemble primary Op instructions (opcode != 0x00).
 */
static u32 primOpAsm(u32 op, const std::vector<std::string>& tokens)
{
    u32 raw_instr = 0;
    raw_instr = SET_OP(raw_instr, op);
    u32 target = 0;
    u32 rt, rs;
    u32 imm16;
    switch (op) {
    // Jump Instructions
    case 0x02: // J
    case 0x03: // JAL
        // FORM: J target
        target = (u32)std::stoi(tokens.at(1), nullptr, 0);
        target >>= 2;
        raw_instr = SET_TARGET(raw_instr, target);
        break;
    // Branch Instructions
    case 0x04: // BEQ
    case 0x05: // BNE
        // FORM: BEQ rs, rt, imm16
        // rs
        PSX_ASSERT(tokens.at(1)[0] == 'R');
        rs = (u32)std::stoi(tokens.at(1).substr(1), nullptr, 0);
        raw_instr = SET_RS(raw_instr, rs);
        // rt
        PSX_ASSERT(tokens.at(2)[0] == 'R');
        rt = (u32)std::stoi(tokens.at(2).substr(1), nullptr, 0);
        raw_instr = SET_RT(raw_instr, rt);
        // imm16
        imm16 = (u32)std::stoi(tokens.at(3), nullptr, 0);
        raw_instr = SET_IMM16(raw_instr, imm16);
        break;
    case 0x06: // BLEZ
    case 0x07: // BGTZ
        // FORM: BLEZ rs, imm16
        // rs
        PSX_ASSERT(tokens.at(1)[0] == 'R');
        rs = (u32)std::stoi(tokens.at(1).substr(1), nullptr, 0);
        raw_instr = SET_RS(raw_instr, rs);
        // imm16
        imm16 = (u32)std::stoi(tokens.at(2), nullptr, 0);
        raw_instr = SET_IMM16(raw_instr, imm16);
        break;
    // ALU Imm
    case 0x09: // ADDIU
    case 0x0b: // SLTIU
    case 0x08: // ADDI
    case 0x0a: // SLTI
        // FORM: ADDIU rt, rs, imm16
        // rt
        PSX_ASSERT(tokens.at(1)[0] == 'R');
        rt = (u32)std::stoi(tokens.at(1).substr(1), nullptr, 0);
        raw_instr = SET_RT(raw_instr, rt);
        // rs
        PSX_ASSERT(tokens.at(2)[0] == 'R');
        rs = (u32)std::stoi(tokens.at(2).substr(1), nullptr, 0);
        raw_instr = SET_RS(raw_instr, rs);
        // imm16
        imm16 = (u32)std::stoi(tokens.at(3), nullptr, 0);
        raw_instr = SET_IMM16(raw_instr, imm16);
        break;
    case 0x0c: // ANDI
    case 0x0d: // ORI
    case 0x0e: // XORI
        // FORM: ANDI rt, rs, imm16
        // rt
        PSX_ASSERT(tokens.at(1)[0] == 'R');
        rt = (u32)std::stoi(tokens.at(1).substr(1), nullptr, 0);
        raw_instr = SET_RT(raw_instr, rt);
        // rs
        PSX_ASSERT(tokens.at(2)[0] == 'R');
        rs = (u32)std::stoi(tokens.at(2).substr(1), nullptr, 0);
        raw_instr = SET_RS(raw_instr, rs);
        // imm16
        imm16 = (u32)std::stoi(tokens.at(3), nullptr, 0);
        raw_instr = SET_IMM16(raw_instr, imm16);
        break;
    // load imm
    case 0x0f: // LUI
        // FORM: LUI rt, imm16
        // rt
        PSX_ASSERT(tokens.at(1)[0] == 'R');
        rt = (u32)std::stoi(tokens.at(1).substr(1), nullptr, 0);
        raw_instr = SET_RT(raw_instr, rt);
        // imm16
        imm16 = (u32)std::stoi(tokens.at(2), nullptr, 0);
        raw_instr = SET_IMM16(raw_instr, imm16);
        break;
    // Coprocessor
    case 0x10: // COP0
    case 0x11: // COP1
    case 0x12: // COP2
    case 0x13: // COP3
        // res = copDasm(instr);
        // COP0 [COP_OP] ...
        raw_instr |= copAsm(tokens);
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
        // FORM: SW rt imm16 rs
        // rt
        PSX_ASSERT(tokens.at(1)[0] == 'R');
        rt = (u32)std::stoi(tokens.at(1).substr(1), nullptr, 0);
        raw_instr = SET_RT(raw_instr, rt);
        // imm16
        imm16 = (u32)std::stoi(tokens.at(2), nullptr, 0);
        raw_instr = SET_IMM16(raw_instr, imm16);
        // rs
        PSX_ASSERT(tokens.at(1)[0] == 'R');
        rs = (u32)std::stoi(tokens.at(3).substr(1), nullptr, 0);
        raw_instr = SET_RS(raw_instr, rs);
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
        // res = PSX_FMT("CopR{}, {:d}(R{})", instr.rt, (i16)instr.imm16, instr.rs);
        // FORM: LWC0 CopRt imm16 rs
        // rt
        PSX_ASSERT(tokens.at(1)[3] == 'R');
        rt = (u32)std::stoi(tokens.at(1).substr(4), nullptr, 0);
        raw_instr = SET_RT(raw_instr, rt);
        // imm16
        imm16 = (u32)std::stoi(tokens.at(2), nullptr, 0);
        raw_instr = SET_IMM16(raw_instr, imm16);
        // rs
        PSX_ASSERT(tokens.at(1)[0] == 'R');
        rs = (u32)std::stoi(tokens.at(3).substr(1), nullptr, 0);
        raw_instr = SET_RS(raw_instr, rs);
        break;
    default:
        throw std::runtime_error("Failed to assemble: Unknown Instruction");
        break;
    }

    return raw_instr;
}

/*
 * Assemble the instruction from the given tokens. The assumption is that the
 * opcode is equal to zero.
 */
static u32 secOpAsm(u32 funct, const std::vector<std::string>& tokens)
{
    u32 raw_instr = 0;
    raw_instr = SET_OP(raw_instr, 0x00);
    raw_instr = SET_FUNCT(raw_instr, funct);
    u32 rd, rs, rt, shamt;
    switch (funct) {
    // Shifts
    case 0x00: // SLL
    case 0x02: // SRL
    case 0x03: // SRA
        // FORM: SLL rd rt shamt
        // rd
        PSX_ASSERT(tokens.at(1)[0] == 'R');
        rd = (u32)std::stoi(tokens.at(1).substr(1), nullptr, 0);
        raw_instr = SET_RD(raw_instr, rd);
        // rt
        PSX_ASSERT(tokens.at(2)[0] == 'R');
        rt = (u32)std::stoi(tokens.at(2).substr(1), nullptr, 0);
        raw_instr = SET_RT(raw_instr, rt);
        // shamt
        shamt = (u32)std::stoi(tokens.at(3), nullptr, 0);
        raw_instr = SET_SHAMT(raw_instr, shamt);
        break;
    case 0x04: // SLLV
    case 0x06: // SRLV
    case 0x07: // SRAV
        // FORM: SLL rd rt rs
        // rd
        PSX_ASSERT(tokens.at(1)[0] == 'R');
        rd = (u32)std::stoi(tokens.at(1).substr(1), nullptr, 0);
        raw_instr = SET_RD(raw_instr, rd);
        // rt
        PSX_ASSERT(tokens.at(2)[0] == 'R');
        rt = (u32)std::stoi(tokens.at(2).substr(1), nullptr, 0);
        raw_instr = SET_RT(raw_instr, rt);
        // rs
        PSX_ASSERT(tokens.at(3)[0] == 'R');
        rs = (u32)std::stoi(tokens.at(3).substr(1), nullptr, 0);
        raw_instr = SET_RS(raw_instr, rs);
        break;
    // Jump Registers
    case 0x08: // JR
        // FORM: JR rs
        // rs
        PSX_ASSERT(tokens.at(1)[0] == 'R');
        rs = (u32)std::stoi(tokens.at(1).substr(1), nullptr, 0);
        raw_instr = SET_RS(raw_instr, rs);
        break;
    case 0x09: // JALR
        // FORM: JALR rs rd
        // rs
        PSX_ASSERT(tokens.at(1)[0] == 'R');
        rs = (u32)std::stoi(tokens.at(1).substr(1), nullptr, 0);
        raw_instr = SET_RS(raw_instr, rs);
        // rd
        PSX_ASSERT(tokens.at(2)[0] == 'R');
        rd = (u32)std::stoi(tokens.at(2).substr(1), nullptr, 0);
        raw_instr = SET_RD(raw_instr, rd);
        break;
    // SPECIAL
    case 0x0c: // SYSCALL
    case 0x0d: // BREAK
        // FORM: SYSCALL
        // nothing to do
        break;
    // Move Hi/Lo
    case 0x10: // MFHI
    case 0x11: // MTHI
    case 0x12: // MFLO
    case 0x13: // MTLO
        // res = PSX_FMT("R{}", instr.rd);
        // FORM: MFHI rd
        // rd
        PSX_ASSERT(tokens.at(1)[0] == 'R');
        rd = (u32)std::stoi(tokens.at(1).substr(1), nullptr, 0);
        raw_instr = SET_RD(raw_instr, rd);
        break;
    // MULT/DIV
    case 0x18: // MULT
    case 0x19: // MULTU
    case 0x1a: // DIV
    case 0x1b: // DIVU
        // FORM: MULT rs rt
        // rs
        PSX_ASSERT(tokens.at(1)[0] == 'R');
        rs = (u32)std::stoi(tokens.at(1).substr(1), nullptr, 0);
        raw_instr = SET_RS(raw_instr, rs);
        // rt
        PSX_ASSERT(tokens.at(2)[0] == 'R');
        rt = (u32)std::stoi(tokens.at(2).substr(1), nullptr, 0);
        raw_instr = SET_RT(raw_instr, rt);
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
        // FORM: ADD rd rs rt
        // rd
        PSX_ASSERT(tokens.at(1)[0] == 'R');
        rd = (u32)std::stoi(tokens.at(1).substr(1), nullptr, 0);
        raw_instr = SET_RD(raw_instr, rd);
        // rs
        PSX_ASSERT(tokens.at(2)[0] == 'R');
        rs = (u32)std::stoi(tokens.at(2).substr(1), nullptr, 0);
        raw_instr = SET_RS(raw_instr, rs);
        // rt
        PSX_ASSERT(tokens.at(3)[0] == 'R');
        rt = (u32)std::stoi(tokens.at(3).substr(1), nullptr, 0);
        raw_instr = SET_RT(raw_instr, rt);
        break;
    default:
        throw std::runtime_error(PSX_FMT("Failed to assemble: Unknown FUNCT [{:}]", funct));
        break;
    }
    return raw_instr;
}



namespace Psx {
namespace Cpu {
namespace Asm {

/*
 * Assemble the given instruction. The instruction should be one instruction
 * in the Modified MIPS Form specified above. Returns the resulting binary
 * form.
 */
u32 AsmInstruction(const std::string& str_instr)
{
    // break up into tokens
    std::istringstream iss(str_instr);
    std::vector<std::string> tokens{std::istream_iterator<std::string>{iss},
                                    std::istream_iterator<std::string>{}};

    // check if empty string
    if (tokens.size() == 0) {
        return 0x0000'0000;
    }
    for (auto& c : tokens[0]) {
        c = (char)std::toupper(c);
    }

    // get opcode and build instruction
    u32 raw_instr = 0;
    auto prim_iter = prim_str_to_op.find(tokens.at(0));
    auto sec_iter = sec_str_to_op.find(tokens.at(0));
    auto bcondz_iter = bcondz_str_to_op.find(tokens.at(0));
    if (prim_iter != prim_str_to_op.end()) {
        u32 op = prim_iter->second;
        raw_instr = primOpAsm(op, tokens);
    } else if (sec_iter != sec_str_to_op.end()) {
        // SPECIAL
        // opcode == 0
        u32 funct = sec_iter->second;
        raw_instr = secOpAsm(funct, tokens);
    } else if (bcondz_iter != bcondz_str_to_op.end()) {
        // bcondz
        // opcode == 1
        raw_instr = SET_OP(raw_instr, 0x01);
        raw_instr = SET_BCONDZOP(raw_instr, bcondz_iter->second);
        // rs
        PSX_ASSERT(tokens.at(1)[0] == 'R');
        u32 rs = (u32)std::stoi(tokens.at(1).substr(1), nullptr, 0);
        raw_instr = SET_RS(raw_instr, rs);
        // imm16
        u32 imm16 = (u32)std::stoi(tokens.at(2), nullptr, 0);
        raw_instr = SET_IMM16(raw_instr, imm16);
    } else {
        // Unknown instruction
        throw std::runtime_error(PSX_FMT("Unknown Instruction: [{}]", str_instr));
    }
    return raw_instr;
}

/*
 * Disassemble the instruction and return a string formatted in the
 * Modified MIPS Form specified above.
 */
std::string DasmInstruction(u32 raw_instr)
{
    std::string str_instr;
    const Asm::Instruction instr = Asm::DecodeRawInstr(raw_instr);
    if (instr.op == 0x01) {
        // BCONDZ
        str_instr = PSX_FMT("{:<8}R{}, {:+d}", bcondzOpToStr(instr.bcondz_op), instr.rs, (i16)instr.imm16);
    } else if (instr.op == 0x00) {
        // SPECIAL
        // look-up in map
        str_instr = PSX_FMT("{:<8}{}", sec_op_map[instr.funct], secOpDasm(instr));
    } else {
        // NORMAL
        str_instr = PSX_FMT("{:<8}{}", prim_op_map[instr.op], primOpDasm(instr));
    }
    return str_instr;
}

/*
 * Transforms a raw 32-bit instruction into a structured instruction for easy
 * manipulation.
 */
Asm::Instruction DecodeRawInstr(u32 raw_instr)
{
    Asm::Instruction instr;
    instr.op = (raw_instr >> 26) & 0x3f; // 26-31
    instr.rs = (raw_instr >> 21) & 0x1f; // 21-25
    instr.rt = (raw_instr >> 16) & 0x1f; // 16-20
    instr.rd = (raw_instr >> 11) & 0x1f; // 11-15
    instr.shamt = (raw_instr >> 6) & 0x1f; // 6-10
    instr.funct = (raw_instr >> 0) & 0x3f; // 0-5

    instr.imm16 = (raw_instr >> 0) & 0xffff; // 0-15

    instr.target = (raw_instr >> 0) & 0x3ff'ffff; // 0-25

    instr.imm25 = (raw_instr >> 0) & 0x1ff'ffff; // 0-24

    return instr;
}

}// end namespace
}
}

