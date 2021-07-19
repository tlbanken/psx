/*
 * imgui_layer.hh
 * 
 * Travis Banken
 * 12/10/2020
 * 
 * Layer over the Dear ImGui Framework.
 */

#pragma once

#include <string>

namespace Psx {
namespace View {
namespace ImGuiLayer {

enum class Style {
    Light,
    Dark,
};

void Init(Style style = Style::Dark);
void Shutdown();
void OnUpdate();
// bool ShouldStop();
// void SetTitleExtra(const std::string& extra);

}// end namespace
}
}
