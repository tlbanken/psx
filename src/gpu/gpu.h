/*
 * gpu.h
 *
 * Travis Banken
 * 2/13/2021
 *
 * GPU for the PSX.
 */

#pragma once

#include "util/psxutil.h"

namespace Psx {
namespace Gpu {

void Init();
void Reset();
void Step();

template<class T> T Read(u32 addr);
template<class T> void Write(T data, u32 addr);
void DoDmaCmds(u32 addr);

void OnActive(bool *active);

} // end ns
}
