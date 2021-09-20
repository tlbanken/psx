/*
 * renderer.hh
 *
 * Travis Banken
 * 7/12/2021
 * 
 * Vulkan renderer backend.
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

class Renderer {
public:
    Renderer(std::vector<const char*> extensions, SDL_Window *window);
    ~Renderer();

    void InitializeImGui(SDL_Window *window);
    void DrawFrame();

private:
    VkInstance m_instance;
    VkDebugUtilsMessengerEXT m_debug_messenger;
    VkSurfaceKHR m_surface;
    Device m_device;
    Swapchain m_swapchain;
    VkRenderPass m_render_pass;
    Pipeline m_pipeline;
    VkDescriptorPool m_imgui_descriptor_pool;
    VkCommandPool m_command_pool;
    std::vector<VkCommandBuffer> m_command_buffers;
    VkSemaphore m_image_available_sem;
    VkSemaphore m_render_finished_sem;

    // private methods
    bool checkValidationLayerSupport(const std::vector<const char*>& layers_wanted);
    void setupDebugMessenger();
    void createInstance(std::vector<const char*> extensions);
    void createSurface(SDL_Window *window);
    void createRenderPass();
    void createCommandPool();
    void createCommandBuffers();
    void createSemaphores();
};

}// end ns
}
