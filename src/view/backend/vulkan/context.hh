/*
 * context.hh
 *
 * Travis Banken
 * 7/12/2021
 * 
 * Vulkan context for the graphics backend.
 */
#pragma once

#include <vector>

#include "view/backend/vulkan/includes.hh"

#include "util/psxutil.hh"
#include "view/backend/vulkan/device.hh"
#include "view/backend/vulkan/swapchain.hh"
#include "view/backend/vulkan/pipeline.hh"

namespace Psx {
namespace Vulkan {

class Context {
public:
    Context(std::vector<const char*> extensions, SDL_Window *window);
    ~Context();

    void InitializeImGui(SDL_Window *window);

private:
    VkInstance m_instance;
    VkDebugUtilsMessengerEXT m_debug_messenger;
    VkSurfaceKHR m_surface;
    Device m_device;
    Swapchain m_swapchain;
    VkRenderPass m_render_pass;
    Pipeline m_pipeline;
    VkDescriptorPool m_imgui_descriptor_pool;

    // private methods
    bool checkValidationLayerSupport(const std::vector<const char*>& layers_wanted);
    void setupDebugMessenger();
    void createInstance(std::vector<const char*> extensions);
    void createSurface(SDL_Window *window);
    void createRenderPass();
};

}// end ns
}
