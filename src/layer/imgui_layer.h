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
        virtual std::string getModuleLabel() = 0;
        virtual void onActive(bool *active) = 0;
    };

    enum class Style {
        Dark,
        Light,
    };

    ImGuiLayer();
    ImGuiLayer(ImGuiLayer::Style style);
    ~ImGuiLayer();

    void addDbgModule(std::shared_ptr<ImGuiLayer::DbgModule> module);
    void onUpdate();
    bool shouldStop();

private:
    struct DbgModEntry {
        std::shared_ptr<DbgModule> module;
        bool active;
    };

    GLFWwindow *m_window = nullptr;
    std::vector<DbgModEntry> m_modEntries;

    void init();
    void newFrame();
    void render();
};
