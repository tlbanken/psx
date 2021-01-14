/*
 * dbgmod.h
 *
 * Travis Banken
 * 1/11/21
 *
 * Common Modules for use in the ImGui Debug Windows for the PSX.
 */

#pragma once

#include <vector>

#include "util/psxutil.h"

namespace Psx {
namespace ImGuiLayer {
namespace DbgMod {

class HexDump {
public:
    HexDump() = default;
    void Update(const std::vector<u8>& mem);
private:
    u32 m_start_line = 0;
    bool m_find_target = false;
    u32 m_target = 0;

    static const u32 m_num_lines = 50;
};


}// end namespace
}
}
