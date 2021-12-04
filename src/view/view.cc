/*
 * view.cc
 * 
 * Travis Banken
 * 7/5/2021
 * 
 * View for the PSX emulator.
 */

#include "view.hh"

#include "view/backend/vulkan/window.hh"
#include "view/geometry.hh"

#define VIEW_INFO(...) PSXLOG_INFO("View", __VA_ARGS__)
#define VIEW_WARN(...) PSXLOG_WARN("View", __VA_ARGS__)
#define VIEW_ERROR(...) PSXLOG_ERROR("View", __VA_ARGS__)
#define VIEW_FATAL(...) VIEW_ERROR(__VA_ARGS__); throw std::runtime_error(PSX_FMT(__VA_ARGS__))

// window size
#define WINDOW_H 1280
#define WINDOW_W 720

namespace {

struct State {
    Psx::Vulkan::Window *window = nullptr;
    const std::string title_base = "PSX Emulator";
}s;

} // end private ns


namespace Psx {
namespace View {

/*
 * Initializes the view for the psx. This consists of the window, render backend,
 * and ImGui debug layer.
 */
void Init()
{
    VIEW_INFO("Initializing view");

    // create window
    s.window = new Psx::Vulkan::Window(WINDOW_H, WINDOW_W, s.title_base.c_str());
}

void Shutdown()
{
    delete s.window;
}

bool ShouldClose()
{
    return s.window->ShouldClose();
}

void SetTitleExtra(const std::string& extra)
{
    s.window->SetTitleExtra(extra);
}

void OnUpdate()
{
    s.window->OnUpdate();
}

void DrawPolygon(const Geometry::Polygon& polygon)
{
    s.window->DrawPolygon(polygon);
}

} // end ns
}
