/*
 * gpu.h
 *
 * Travis Banken
 * 2/13/2021
 *
 * GPU for the PSX.
 */

#pragma once

#include "util/psxutil.hh"

namespace Psx {
namespace Gpu {

void Init();
void Reset();
void RenderFrame();
bool Step();

template<class T> T Read(u32 addr);
template<class T> void Write(T data, u32 addr);
void DoDmaCmds(u32 addr);
void DoGP0Cmd(u32 cmd);

void OnActive(bool *active);

} // end ns
}
