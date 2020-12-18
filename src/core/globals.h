/*
 * globals.h
 *
 * Travis Banken
 * 12/17/2020
 *
 * Global State for the PSX.
 */

#pragma once

#include "util/psxutil.h"

// Struct containing flags for the current emulation state. This allows any
// function to control the flow of the emulation. This includes pausing and
// steping through instructions.
struct EmuState {
    bool paused = false;
    bool step_instr = false;
};

extern EmuState g_emu_state;
