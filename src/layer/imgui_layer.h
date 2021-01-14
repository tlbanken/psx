/*
 * imgui_layer.h
 * 
 * Travis Banken
 * 12/10/2020
 * 
 * Layer over the Dear ImGui Framework.
 */

#pragma once

#include "glad/glad.h"
#include "glfw/glfw3.h" // MUST be included AFTER glad

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

}// end namespace
}
