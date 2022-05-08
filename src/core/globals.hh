/*
 * globals.h
 *
 * Travis Banken
 * 12/17/2020
 *
 * Global State for the PSX.
 */

#pragma once

#include "util/psxutil.hh"

// Struct containing flags for the current emulation state. This allows any
// function to control the flow of the emulation. This includes pausing and
// steping through instructions.
struct EmuState {
    bool paused = false;
    u32 step_count = 0;
};

extern EmuState g_emu_state;
