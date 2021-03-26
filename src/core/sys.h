/*
 * sys.h
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

namespace Psx {

class System {
public:
    System(const std::string& bios_path, bool headless_mode);
    ~System();
    void Run();
    bool Step();
    static void Reset();

private:
    static System *m_instance;
    bool m_headless_mode;
    u64 m_clocks = 0;
};

}// end namespace
