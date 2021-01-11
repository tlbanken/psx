/*
 * psx.h
 * 
 * Travis Banken
 * 12/5/2020
 * 
 * Header file for the main PSX class. The class is a singleton. It manages
 * the other hw modules and allows for easy communication between them.
 */

#pragma once

#include <memory>

#include "mem/bus.h"
#include "cpu/cpu.h"
#include "layer/imgui_layer.h"

class Psx {
public:
    Psx(const std::string& bios_path);
    void Run();

private:
    std::shared_ptr<Bus> m_bus;
    ImGuiLayer m_imgui_layer;
    std::shared_ptr<Cpu> m_cpu;
};
