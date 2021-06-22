/*
 * memcontrol.h
 *
 * Travis Banken
 * 1/13/21
 *
 * Memory Control Registers.
 */

#pragma once

#include "util/psxutil.hh"

namespace Psx {
namespace MemControl {

void Init();
void Reset();
void OnActive(bool *active);

template<class T>
void Write(T data, u32 addr);

template<class T>
T Read(u32 addr);


}// end namespace
}
