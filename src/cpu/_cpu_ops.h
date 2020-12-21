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
void BadOp(const Asm::Instruction& instr);

// Extra Map
void Special(const Asm::Instruction& instr);
void Bcondz(const Asm::Instruction& instr);

//-------------------------------------------
// Computational Instructions
//-------------------------------------------
// ALU Immediate Ops
void Addi(const Asm::Instruction& instr);
void Addiu(const Asm::Instruction& instr);
void Slti(const Asm::Instruction& instr);
void Sltiu(const Asm::Instruction& instr);
void Andi(const Asm::Instruction& instr);
void Ori(const Asm::Instruction& instr);
void Xori(const Asm::Instruction& instr);
void Lui(const Asm::Instruction& instr);

// Three Operand Register-Type Ops
void Add(const Asm::Instruction& instr);
void Addu(const Asm::Instruction& instr);
void Sub(const Asm::Instruction& instr);
void Subu(const Asm::Instruction& instr);
void Slt(const Asm::Instruction& instr);
void Sltu(const Asm::Instruction& instr);
void And(const Asm::Instruction& instr);
void Or(const Asm::Instruction& instr);
void Xor(const Asm::Instruction& instr);
void Nor(const Asm::Instruction& instr);

// Shift Operations
void Sll(const Asm::Instruction& instr);
void Srl(const Asm::Instruction& instr);
void Sra(const Asm::Instruction& instr);
void Sllv(const Asm::Instruction& instr);
void Srlv(const Asm::Instruction& instr);
void Srav(const Asm::Instruction& instr);

// Multiply and Divide Operations
void Mult(const Asm::Instruction& instr);
void Multu(const Asm::Instruction& instr);
void Div(const Asm::Instruction& instr);
void Divu(const Asm::Instruction& instr);
void Mfhi(const Asm::Instruction& instr);
void Mflo(const Asm::Instruction& instr);
void Mthi(const Asm::Instruction& instr);
void Mtlo(const Asm::Instruction& instr);

//-------------------------------------------
// Load and Store Instructions
//-------------------------------------------
void Lb(const Asm::Instruction& instr);
void Lbu(const Asm::Instruction& instr);
void Lh(const Asm::Instruction& instr);
void Lhu(const Asm::Instruction& instr);
void Lw(const Asm::Instruction& instr);
void Lwl(const Asm::Instruction& instr);
void Lwr(const Asm::Instruction& instr);
void Sb(const Asm::Instruction& instr);
void Sh(const Asm::Instruction& instr);
void Sw(const Asm::Instruction& instr);
void Swl(const Asm::Instruction& instr);
void Swr(const Asm::Instruction& instr);

//-------------------------------------------
// Jump and Branch Instructions
//-------------------------------------------
// Jump instructions
void J(const Asm::Instruction& instr);
void Jal(const Asm::Instruction& instr);
void Jr(const Asm::Instruction& instr);
void Jalr(const Asm::Instruction& instr);

// Branch instructions
void Beq(const Asm::Instruction& instr);
void Bne(const Asm::Instruction& instr);
void Blez(const Asm::Instruction& instr);
void Bgtz(const Asm::Instruction& instr);

// bcondz
void Bltz(const Asm::Instruction& instr);
void Bgez(const Asm::Instruction& instr);
void Bltzal(const Asm::Instruction& instr);
void Bgezal(const Asm::Instruction& instr);

//-------------------------------------------
// Special
//-------------------------------------------
void Syscall(const Asm::Instruction& instr);
void Break(const Asm::Instruction& instr);

//-------------------------------------------
// Co-Processor Instructions
//-------------------------------------------
void Cop0(const Asm::Instruction& instr);
void Cop1(const Asm::Instruction& instr);
void Cop2(const Asm::Instruction& instr);
void Cop3(const Asm::Instruction& instr);

void LwC0(const Asm::Instruction& instr);
void LwC1(const Asm::Instruction& instr);
void LwC2(const Asm::Instruction& instr);
void LwC3(const Asm::Instruction& instr);

void SwC0(const Asm::Instruction& instr);
void SwC1(const Asm::Instruction& instr);
void SwC2(const Asm::Instruction& instr);
void SwC3(const Asm::Instruction& instr);


