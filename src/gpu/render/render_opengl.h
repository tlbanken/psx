/*
 * render_opengl.h
 *
 * Travis Banken
 * 3/13/2021
 *
 * OpenGL rendering backend.
 */

#pragma once

#include "render.h"

namespace Psx {


class OpenGL final : public Renderer {
public:
    OpenGL();
    ~OpenGL() {}

    BackendType GetBackendType() override;
    void RenderSomething() override;
private:
};

}//end ns
