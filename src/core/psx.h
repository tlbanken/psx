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
#include "cpu/cpu.h"
#include "layer/imgui_layer.h"

class Psx {
private:
    std::shared_ptr<Bus> m_bus;
    ImGuiLayer m_imgui_layer;
    std::shared_ptr<Cpu> m_cpu;

public:
    Psx();

    void Run();
};
