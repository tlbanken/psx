/*
 * view.hh
 * 
 * Travis Banken
 * 7/5/2021
 * 
 * View for the PSX emulator.
 */
#pragma once

#include "util/psxutil.hh"

namespace Psx {
namespace View {

void Init();
void Shutdown();
bool ShouldClose();
void SetTitleExtra(const std::string& extra);
void OnUpdate();

} // end ns
}