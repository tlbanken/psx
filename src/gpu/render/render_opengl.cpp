/*
 * render_opengl.cpp
 *
 * Travis Banken
 * 3/13/2021
 *
 * OpenGL rendering backend.
 */

#include "render_opengl.h"

#include "glad/glad.h"

#define ROPENGL_INFO(...) PSXLOG_INFO("OpenGL-Render", __VA_ARGS__)
#define ROPENGL_WARN(...) PSXLOG_WARN("OpenGL-Render", __VA_ARGS__)
#define ROPENGL_ERROR(...) PSXLOG_ERROR("OpenGL-Render", __VA_ARGS__)
#define ROPENGL_FATAL(...) GPU_ERROR(__VA_ARGS__); throw std::runtime_error(PSX_FMT(__VA_ARGS__))

namespace Psx {

OpenGL::OpenGL()
{
    ROPENGL_INFO("Initializing OpengGL: {}", glGetString(GL_VERSION));
    
}

void OpenGL::RenderSomething()
{
    ROPENGL_INFO("Don't mind me, just doing some rendererin' :/");
}

Renderer::BackendType OpenGL::GetBackendType()
{
    return Renderer::BackendType::OpenGL;
}

}//end ns