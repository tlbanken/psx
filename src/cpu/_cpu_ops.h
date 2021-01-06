/*
 * _cpu_ops.h
 *
 * Travis Banken
 * 12/18/2020
 *
 * Includes all op functions. Included in cpu.h.
 */

#ifdef _CPU_OPS_H
#error There should only be one include of this file!
#else
#define _CPU_OPS_H
#endif

// fallback
u8 BadOp(const Asm::Instruction& instr);

// Extra Map
u8 Special(const Asm::Instruction& instr);
u8 Bcondz(const Asm::Instruction& instr);

//-------------------------------------------
// Computational Instructions
//-------------------------------------------
// ALU Immediate Ops
u8 Addi(const Asm::Instruction& instr);
u8 Addiu(const Asm::Instruction& instr);
u8 Slti(const Asm::Instruction& instr);
u8 Sltiu(const Asm::Instruction& instr);
u8 Andi(const Asm::Instruction& instr);
u8 Ori(const Asm::Instruction& instr);
u8 Xori(const Asm::Instruction& instr);
u8 Lui(const Asm::Instruction& instr);

// Three Operand Register-Type Ops
u8 Add(const Asm::Instruction& instr);
u8 Addu(const Asm::Instruction& instr);
u8 Sub(const Asm::Instruction& instr);
u8 Subu(const Asm::Instruction& instr);
u8 Slt(const Asm::Instruction& instr);
u8 Sltu(const Asm::Instruction& instr);
u8 And(const Asm::Instruction& instr);
u8 Or(const Asm::Instruction& instr);
u8 Xor(const Asm::Instruction& instr);
u8 Nor(const Asm::Instruction& instr);

// Shift Operations
u8 Sll(const Asm::Instruction& instr);
u8 Srl(const Asm::Instruction& instr);
u8 Sra(const Asm::Instruction& instr);
u8 Sllv(const Asm::Instruction& instr);
u8 Srlv(const Asm::Instruction& instr);
u8 Srav(const Asm::Instruction& instr);

// Multiply and Divide Operations
u8 Mult(const Asm::Instruction& instr);
u8 Multu(const Asm::Instruction& instr);
u8 Div(const Asm::Instruction& instr);
u8 Divu(const Asm::Instruction& instr);
u8 Mfhi(const Asm::Instruction& instr);
u8 Mflo(const Asm::Instruction& instr);
u8 Mthi(const Asm::Instruction& instr);
u8 Mtlo(const Asm::Instruction& instr);

//-------------------------------------------
// Load and Store Instructions
//-------------------------------------------
u8 Lb(const Asm::Instruction& instr);
u8 Lbu(const Asm::Instruction& instr);
u8 Lh(const Asm::Instruction& instr);
u8 Lhu(const Asm::Instruction& instr);
u8 Lw(const Asm::Instruction& instr);
u8 Lwl(const Asm::Instruction& instr);
u8 Lwr(const Asm::Instruction& instr);
u8 Sb(const Asm::Instruction& instr);
u8 Sh(const Asm::Instruction& instr);
u8 Sw(const Asm::Instruction& instr);
u8 Swl(const Asm::Instruction& instr);
u8 Swr(const Asm::Instruction& instr);

//-------------------------------------------
// Jump and Branch Instructions
//-------------------------------------------
// Jump instructions
u8 J(const Asm::Instruction& instr);
u8 Jal(const Asm::Instruction& instr);
u8 Jr(const Asm::Instruction& instr);
u8 Jalr(const Asm::Instruction& instr);

// Branch instructions
u8 Beq(const Asm::Instruction& instr);
u8 Bne(const Asm::Instruction& instr);
u8 Blez(const Asm::Instruction& instr);
u8 Bgtz(const Asm::Instruction& instr);

// bcondz
u8 Bltz(const Asm::Instruction& instr);
u8 Bgez(const Asm::Instruction& instr);
u8 Bltzal(const Asm::Instruction& instr);
u8 Bgezal(const Asm::Instruction& instr);

//-------------------------------------------
// Special
//-------------------------------------------
u8 Syscall(const Asm::Instruction& instr);
u8 Break(const Asm::Instruction& instr);

//-------------------------------------------
// Co-Processor Instructions
//-------------------------------------------
u8 Cop0(const Asm::Instruction& instr);
u8 Cop1(const Asm::Instruction& instr);
u8 Cop2(const Asm::Instruction& instr);
u8 Cop3(const Asm::Instruction& instr);

u8 LwC0(const Asm::Instruction& instr);
u8 LwC1(const Asm::Instruction& instr);
u8 LwC2(const Asm::Instruction& instr);
u8 LwC3(const Asm::Instruction& instr);

u8 SwC0(const Asm::Instruction& instr);
u8 SwC1(const Asm::Instruction& instr);
u8 SwC2(const Asm::Instruction& instr);
u8 SwC3(const Asm::Instruction& instr);
