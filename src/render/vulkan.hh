/*
 * vulkan.hh
 *
 * Travis Banken
 * 6/21/21
 *
 * Vulkan backend for renderering.
 */

#pragma once

#include <vector>

#include "imgui/imgui_impl_vulkan.h"

namespace Psx {
namespace Vulkan {
    
void Init(std::vector<const char *> extensions);
void Shutdown();
VkRenderPass GetRenderPass();
void SetupImGui();

}// end ns
}
