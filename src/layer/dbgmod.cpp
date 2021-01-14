/*
 * dbgmod.cpp
 *
 * Travis Banken
 * 1/11/21
 *
 * Common Modules for use in the ImGui Debug Windows for the PSX.
 */

#include "dbgmod.h"

#include "imgui/imgui.h"
namespace {
/*
 * Creates one line of hexdump output for a given address. Will proccess 16 bytes
 * of data starting at the given address.
 */
std::string hexDumpLine(u32 addr, const std::vector<u8>& mem)
{
    /* PSX_ASSERT((size_t)addr + 16 <= mem.size()); */
    if ((size_t)addr + 16 > mem.size()) {
        return "";
    }

    // helper lambda
    auto hexDumpChar = [] (u8 byte) {return (byte >= '!' && byte <= '~') ? byte : '.';};

    // format 16 bytes "hexdump -C" style
    std::string dump = PSX_FMT(
            "{:08x}  "
            "{:02x} {:02x} {:02x} {:02x} "
            "{:02x} {:02x} {:02x} {:02x}  "
            "{:02x} {:02x} {:02x} {:02x} "
            "{:02x} {:02x} {:02x} {:02x}  "
            "|{:c}{:c}{:c}{:c}{:c}{:c}{:c}{:c}"
            "{:c}{:c}{:c}{:c}{:c}{:c}{:c}{:c}|"
            , addr
            , mem[addr+0], mem[addr+1], mem[addr+2], mem[addr+3]
            , mem[addr+4], mem[addr+5], mem[addr+6], mem[addr+7]
            , mem[addr+8], mem[addr+9], mem[addr+10], mem[addr+11]
            , mem[addr+12], mem[addr+13], mem[addr+14], mem[addr+15]
            , hexDumpChar(mem[addr+0]), hexDumpChar(mem[addr+1])
            , hexDumpChar(mem[addr+2]), hexDumpChar(mem[addr+3])
            , hexDumpChar(mem[addr+4]), hexDumpChar(mem[addr+5])
            , hexDumpChar(mem[addr+6]), hexDumpChar(mem[addr+7])
            , hexDumpChar(mem[addr+8]), hexDumpChar(mem[addr+9])
            , hexDumpChar(mem[addr+10]), hexDumpChar(mem[addr+11])
            , hexDumpChar(mem[addr+12]), hexDumpChar(mem[addr+13])
            , hexDumpChar(mem[addr+14]), hexDumpChar(mem[addr+15])
    );
    return dump;
}

}// end namespace

namespace Psx {
namespace ImGuiLayer {
namespace DbgMod {

void HexDump::Update(const std::vector<u8>& mem)
{
    u32 last_line = (u32)mem.size() >> 4;

    //----------------------------------------------------
    // Buttons for Navigation
    //----------------------------------------------------
    if (ImGui::Button("Up")) {
        m_start_line = m_start_line == 0 ? 0 : m_start_line - 1;
    }
    ImGui::SameLine();
    if (ImGui::Button("Up+10")) {
        m_start_line = m_start_line < 10 ? 0 : m_start_line - 10;
    }
    ImGui::SameLine();
    if (ImGui::Button("Down")) {
        m_start_line = m_start_line == last_line ? last_line : m_start_line + 1;
    }
    ImGui::SameLine();
    if (ImGui::Button("Down+10")) {
        m_start_line = m_start_line >= last_line - 10 ? last_line : m_start_line + 10;
    }
    ImGui::SameLine();
    static char searchBuf[7] = "";
    auto input_flags = ImGuiInputTextFlags_CharsHexadecimal;
    ImGui::InputTextWithHint("Goto Address", "Enter address (hex)", searchBuf, sizeof(searchBuf), input_flags);
    u32 res = 0;
    if (searchBuf[0] != '\0') {
        res = static_cast<u32>(std::stoi(searchBuf, nullptr, 16));
    }
    if (m_target != res) {
        m_find_target = true;
        m_target = res;
        m_start_line = (m_target >> 4);
    } else {
        m_find_target = false;
    }
    //----------------------------------------------------

    //----------------------------------------------------
    // Hex Dump Region
    //----------------------------------------------------
    // set up hex dump
    ImGuiWindowFlags hex_dump_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    u32 endLine = m_start_line + m_num_lines;
    if (endLine > last_line) {
        endLine = last_line;
    }

    ImGui::BeginChild("Hex Dump", ImVec2(0,0), false, hex_dump_flags);
        for (u32 line = m_start_line; line < endLine; line++) {
            // color the line if it is our search m_target
            if (line != 0 && line == (m_target >> 4)) {
                ImGui::TextColored(ImVec4(0.8f,0.2f,0,1), "%s", hexDumpLine(line << 4, mem).data());
            } else {
                ImGui::TextUnformatted(hexDumpLine(line << 4, mem).data());
            }
        }
    ImGui::EndChild();
    //----------------------------------------------------

}

}// end namespace
}
}
