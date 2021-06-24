/*
 * vulkan.cc
 *
 * Travis Banken
 * 6/21/21
 *
 * Vulkan backend for rendering.
 */

#include "vulkan.hh"

#include <vulkan/vulkan.h>
#include "glfw/glfw3.h"

#include "util/psxutil.hh"
#include "render/vulkan_instance.hh"
#include "layer/imgui_layer.hh"

#define VULKAN_INFO(...) PSXLOG_INFO("Vulkan Backend", __VA_ARGS__)
#define VULKAN_WARN(...) PSXLOG_WARN("Vulkan Backend", __VA_ARGS__)
#define VULKAN_ERROR(...) PSXLOG_ERROR("Vulkan Backend", __VA_ARGS__)
#define VULKAN_FATAL(...) VULKAN_ERROR(__VA_ARGS__); throw std::runtime_error(PSX_FMT(__VA_ARGS__))

namespace {

struct State {
    Psx::Vulkan::Instance::Details ideets;
} s;

// protos

}// end private ns

namespace Psx {
namespace Vulkan {

/*
 * Initializes Vulkan as a backend to the PSX.
 */
void Init(std::vector<const char *> extensions)
{
    VULKAN_INFO("Initializing Vulkan backend");
    s.ideets = Instance::Create(extensions);
}

/*
 * Shutdown backend and release resources.
 */
void Shutdown()
{
    VULKAN_INFO("Shutting down Vulkan backend");
    Instance::Destroy(s.ideets);
}

}// end ns
}

// PRIVATE METHODS
namespace {

}// end private ns
