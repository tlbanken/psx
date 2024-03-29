/*
 * window.hh
 *
 * Travis Banken
 * 7/9/2021
 * 
 * Vulkan-backed window.
 */
#pragma once


#include "util/psxutil.hh"
#include "view/backend/vulkan/includes.hh"
#include "view/backend/vulkan/builder.hh"
#include "view/geometry.hh"

namespace Psx {
namespace Vulkan {


class Window {
public:
    Window(int width, int height, const std::string& title);
    ~Window();

    bool ShouldClose();
    void SetTitleExtra(const std::string& extra);
    void NewFrame();
    void Render();
    void OnUpdate();
    void DrawPolygon(const Geometry::Polygon& polygon);
    void Clear();

private:
    SDL_Window *m_window = nullptr;
    std::string m_title_base;
    Builder::WindowData *m_wd = nullptr;
    Builder::DeviceData *m_dd = nullptr;
    VkInstance m_instance;
    VkAllocationCallbacks *m_allocator_callbacks = nullptr;
    int m_win_height;
    int m_win_width;

    void drawTri(const Geometry::Polygon& tri);
    void drawQuad(const Geometry::Polygon& quad);
};

}// end ns
}