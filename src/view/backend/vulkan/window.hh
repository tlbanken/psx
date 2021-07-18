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
#include "view/backend/vulkan/context.hh"

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

private:
    GLFWwindow *m_window;
    std::string m_title_base;
    std::unique_ptr<Context> m_context;
};

}// end ns
}