/*
 * timer.hh
 *
 * Travis Banken
 * 5/8/2022
 * 
 * Header for timers (Root Counters) for the PSX.
 * There are 3 timers:
 *   1F80110xh      Timer 0 Dotclock
 *   1F80111xh      Timer 1 Horizontal Retrace
 *   1F80112xh      Timer 2 1/8 system clock
 */
#pragma once

#include "util/psxutil.hh"

namespace Psx {
namespace Timer {

void Init();
void Reset();
void Step();
void OnActive(bool *active);

template<class T>
T Read(u32 addr);

template<class T>
void Write(T data, u32 addr);

} // end ns
}