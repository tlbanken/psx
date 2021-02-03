/*
 * dma.cpp
 *
 * Travis Banken
 * 1/23/2021
 *
 * Handles the DMA channels on the PSX.
 */

#pragma once

#include "util/psxutil.h"


namespace Psx {
namespace Dma {

void Init();
void Reset();
void OnActive(bool *active);

template<class T> T Read(u32 addr);
template<class T> void Write(T data, u32 addr);

}// end ns
}
