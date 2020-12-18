/*
 * imgui_layer.h
 * 
 * Travis Banken
 * 12/10/2020
 * 
 * Layer over the Dear ImGui Framework.
 */

#pragma once

#include <vector>
#include <string>
#include <memory>

#include "glad/glad.h"
#include "glfw/glfw3.h" // MUST be included AFTER glad

class ImGuiLayer {
public:
    class DbgModule {
    protected:
        virtual ~DbgModule();
    public:
        virtual std::string GetModuleLabel() = 0;
        virtual void OnActive(bool *active) = 0;
    };

    enum class Style {
        Dark,
        Light,
    };

    ImGuiLayer();
    ImGuiLayer(ImGuiLayer::Style style);
    ~ImGuiLayer();

    void AddDbgModule(std::shared_ptr<ImGuiLayer::DbgModule> module);
    void OnUpdate();
    bool ShouldStop();

private:
    struct DbgModEntry {
        std::shared_ptr<DbgModule> module;
        bool active;
    };

    GLFWwindow *m_window = nullptr;
    std::vector<DbgModEntry> m_mod_entries;

    void init();
    void newFrame();
    void render();
};
