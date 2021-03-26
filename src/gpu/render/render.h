/*
 * render.h
 *
 * Travis Banken
 * 3/13/2021
 *
 * Render backend abstraction layer.
 */

#pragma once

#include "util/psxutil.h"

namespace Psx {

class Renderer {
public:
    enum class BackendType {
        OpenGL,
        Software,
    };

    // always need a virtual destructor for polymorphism
    virtual ~Renderer() {}

    virtual BackendType GetBackendType() = 0;
    virtual void RenderSomething() = 0; // Only for debug
private:
};

}// end ns
