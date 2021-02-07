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
    void dumpToFile(const std::vector<u8>& mem);
    u32 m_start_line = 0;
    bool m_find_target = false;
    u32 m_target = 0;
    std::string m_search_buf;
    u32 m_search_res = 0;
    std::string m_file_name;

    static const u32 m_num_lines = 50;
};

namespace Breakpoints {

enum class BrkType {
    WriteWatch,
    ReadWatch,
    PCWatch
};

void OnActive(bool *active);
void OnUpdate();
template <BrkType type> void Saw(u32 addr);
bool ReadyToBreak();
template <BrkType type> void Set(u32 addr);

}// end namespace (breakpoint)


}// end namespace
}
}
