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
#include "view/geometry.hh"

namespace Psx {
namespace View {

void Init();
void Shutdown();
bool ShouldClose();
void SetTitleExtra(const std::string& extra);
void OnUpdate();
void DrawPolygon(const Geometry::Polygon& polygon);
void Clear();

} // end ns
}