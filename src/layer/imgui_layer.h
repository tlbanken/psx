/*
 * imgui_layer.h
 * 
 * Travis Banken
 * 12/10/2020
 * 
 * Layer over the Dear ImGui Framework.
 */

#pragma once

// #define GLFW_INCLUDE_VULKAN // must be defined before glfw3
#include <vulkan/vulkan.h>
#include "glfw/glfw3.h"

#include <string>

namespace Psx {
namespace ImGuiLayer {

enum class Style {
    Light,
    Dark,
};

void Init(ImGuiLayer::Style style = Style::Dark);
void Shutdown();
void OnUpdate();
bool ShouldStop();
void SetTitleExtra(const std::string& extra);

}// end namespace
}
