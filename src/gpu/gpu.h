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

struct Color {
    u32 red = 0;
    u32 green = 0;
    u32 blue = 0;
};

struct Vertex {
    i16 x = 0;
    i16 y = 0;
    Color color;
};

struct Polygon {
    Vertex vertices[4];
    u8 num_vertices = 0;
    bool gouraud_shaded = false;
};

void Init();
void Reset();
void RenderFrame();
bool Step();

template<class T> T Read(u32 addr);
template<class T> void Write(T data, u32 addr);
void DoDmaCmds(u32 addr);

void OnActive(bool *active);

} // end ns
}
