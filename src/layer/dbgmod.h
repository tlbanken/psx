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
    HexDump();
    void Update(const std::vector<u8>& mem);
private:
    u32 m_start_line = 0;
    bool m_find_target = false;
    u32 m_target = 0;
    std::string m_search_buf;
    u32 m_search_res = 0;

    static const u32 m_num_lines = 50;
};

namespace Breakpoints {

void OnActive(bool *active);
void OnUpdate();
bool Exists();
void Break(u32 pc);
bool ShouldBreakPC(u32 pc);
void PopupAddPCBreakpoint();
void SetPCBreakpoint(u32 pc);

}// end namespace (breakpoint)


}// end namespace
}
}
