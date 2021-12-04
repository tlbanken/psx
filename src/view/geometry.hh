/*
 * geometry.hh
 *
 * Travis Banken
 * 11/26/2021
 *
 * Geometry for the PSX.
 */

#pragma once

#include "util/psxutil.hh"

namespace Psx {
namespace Geometry {

struct Color {
    u32 red = 0;
    u32 green = 0;
    u32 blue = 0;

    // contructors
    Color() {}
    Color (u32 r, u32 g, u32 b)
        : red(r), green(g), blue(b) {}
    Color(u32 raw_rgb)
    {
        red   = (raw_rgb >> 0) & 0xff;
        green = (raw_rgb >> 8) & 0xff;
        blue  = (raw_rgb >> 16) & 0xff;
    }
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
    bool textured = false;
    bool transparent = false;
    bool blend_texture = false;
};

} // end ns
}