/*
 * psx.h
 * 
 * Travis Banken
 * 12/5/2020
 * 
 * Header file for the main PSX class.
 */

#pragma once

#include <memory>

#include "mem/bus.h"
#include "layer/imgui_layer.h"

class Psx {
private:
    std::shared_ptr<Bus> m_bus;
    ImGuiLayer m_imguiLayer;

public:
    Psx();

    void run();
};
