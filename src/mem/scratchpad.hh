/*
 * scratchpad.h
 *
 * Travis Banken
 * 1/22/21
 *
 * The PSX does not use a Data Cache. Instead is uses scatchpad which acts
 * like a "fast ram".
 */

#pragma once

#include "util/psxutil.hh"

namespace Psx {
namespace Scratchpad {

void Init();
void Reset();

template<class T> T Read(u32 addr);
template<class T> void Write(T data, u32 addr);

void OnActive(bool *active);

}// end namespace
}
