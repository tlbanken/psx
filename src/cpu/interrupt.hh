/*
 * interrupt.hh
 *
 * Travis Banken
 * 5/8/2022
 * 
 * Interrupts middle man for the PSX.
 */
#pragma once

#include "util/psxutil.hh"

namespace Psx {
namespace Interrupt {

enum Type {
    Vblank     = 1 << 0,
    Gpu        = 1 << 1,
    CdRom      = 1 << 2,
    Dma        = 1 << 3,
    Timer0     = 1 << 4,
    Timer1     = 1 << 5,
    Timer2     = 1 << 6,
    Controller = 1 << 7,
    MemCard    = Controller,
    Sio        = 1 << 8,
    Spu        = 1 << 9,
    Lightpen   = 1 << 10,
};

void Init();
void Reset();
void Step();
void Signal(Type itype);
void OnActive(bool *active);
template<class T> T Read(u32 addr);
template<class T> void Write(T data, u32 addr);

} // end ns
}
