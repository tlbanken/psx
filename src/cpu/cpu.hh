/*
 * cpu.h
 *
 * Travis Banken
 * 12/13/2020
 *
 * Header for the CPU for the PSX. The Playstation uses a R3000A MIPS processor.
 */

#pragma once

#include <map>

#include "util/psxutil.hh"
#include "mem/bus.hh"
#include "cpu/asm/asm.hh"
#include "layer/imgui_layer.hh"


namespace Psx {
namespace Cpu {
#include "cpu/_cpu_ops.hh"

void Init();
void Reset();

// functions
void Step();
void SetPC(u32 addr);
u32 GetPC();
u32 GetR(size_t r);
void SetR(size_t r, u32 val);
u32 GetHI();
void SetHI(u32 val);
u32 GetLO();
void SetLO(u32 val);
u8 ExecuteInstruction(u32 raw_instr);
bool InBranchDelaySlot();

// DbgModule Functions
void OnActive(bool *active);



}// end namspace
}
