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

#include "core/globals.h"
#include "cpu/cpu.h"


#define DBG_INFO(...) PSXLOG_INFO("Debug", __VA_ARGS__)
#define DBG_WARN(...) PSXLOG_WARN("Debug", __VA_ARGS__)
#define DBG_ERROR(...) PSXLOG_ERROR("Debug", __VA_ARGS__)

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
HexDump::HexDump()
{
    m_search_buf.resize(8, 0);
}

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
    auto input_flags = ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue;
    if (ImGui::InputTextWithHint("Goto Address", "Enter address (hex)", m_search_buf.data(), m_search_buf.size(), input_flags)) {
        if (m_search_buf.c_str()[0] != '\0') {
            m_search_res = static_cast<u32>(std::stol(m_search_buf.data(), nullptr, 16));
        }
    }
    if (m_target != m_search_res) {
        m_find_target = true;
        m_target = m_search_res;
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

namespace Breakpoints {


namespace {
    std::vector<u32> pc_breakpoints;
    std::vector<u32> memw_breakpoints;
    std::vector<u32> memr_breakpoints;

    bool is_breaked = false;
    u32 last_break_addr = 0;
}

//bool Exists()
//{
//    return pc_breakpoints.size() != 0;
//}

void OnActive(bool *active)
{
    if (!ImGui::Begin("Breakpoints", active)) {
        ImGui::End();
        return;
    }

    PopupAddPCBreakpoint();

    ImGui::TextUnformatted("Active PC Breakpoints");
    int i = 1;
    for (const auto& bp : pc_breakpoints) {
        ImGui::Text("%d. 0x%08x", i, bp);
        i++;
    }

    ImGui::End();
}

bool ShouldBreakPC(u32 pc)
{
    for (const auto& bp : pc_breakpoints) {
        if (pc == bp) {
            return true;
        }
    }
    return false;
}

/*
 * Returns true if the current address matches any of the currently set
 * memory write breakpoints.
 */
bool ShouldBreakMemW(u32 addr)
{
    for (const auto& bp : memw_breakpoints) {
        if (addr == bp) {
            return true;
        }
    }
    return false;
}

/*
 * Returns true if the current address matches any of the currently set
 * memory read breakpoints.
 */
bool ShouldBreakMemR(u32 addr)
{
    for (const auto& bp : memr_breakpoints) {
        if (addr == bp) {
            return true;
        }
    }
    return false;
}

void BreakPC(u32 pc)
{
    g_emu_state.paused = true;
    last_break_addr = pc;
    is_breaked = true;

    pc_breakpoints.erase(std::remove(pc_breakpoints.begin(), pc_breakpoints.end(), pc), pc_breakpoints.end());
}

void BreakMemW(u32 addr)
{
    g_emu_state.paused = true;
    last_break_addr = addr;
    is_breaked = true;

    memw_breakpoints.erase(std::remove(memw_breakpoints.begin(), memw_breakpoints.end(), addr), memw_breakpoints.end());
}

void BreakMemR(u32 addr)
{
    g_emu_state.paused = true;
    last_break_addr = addr;
    is_breaked = true;

    memr_breakpoints.erase(std::remove(memr_breakpoints.begin(), memr_breakpoints.end(), addr), memr_breakpoints.end());
}

void OnUpdate()
{
    if (is_breaked)
        ImGui::OpenPopup("Break");

    // Always center this window when appearing
    ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Break", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Break on 0x%08x!", last_break_addr);
        if (ImGui::Button("Close")) {
            is_breaked = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void SetPCBreakpoint(u32 pc)
{
    DBG_INFO("Setting PC Breakpoint @ 0x{:08x}", pc);
    pc_breakpoints.push_back(pc);
}

/*
 * Set a breakpoint when the given memory address is written to.
 */
void SetMemWBreakpoint(u32 addr)
{
    DBG_INFO("Setting Mem Write Breakpoint @ 0x{:08x}", addr);
    memw_breakpoints.push_back(addr);
}

/*
 * Set a breakpoint when the given memory address is written to.
 */
void SetMemRBreakpoint(u32 addr)
{
    DBG_INFO("Setting Mem Read Breakpoint @ 0x{:08x}", addr);
    memr_breakpoints.push_back(addr);
}

void PopupAddPCBreakpoint()
{
    if (ImGui::Button("Add PC Breakpoint###")) {
        ImGui::OpenPopup("Add PC Breakpoint");
    }
    // Always center this window when appearing
    ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Add PC Breakpoint", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted("Enter an address to break on...");
        static char s_search_buf[9] = {0};
        auto input_flags = ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue;
        if (ImGui::InputTextWithHint("PC Breakpoint", "Enter address (hex)", s_search_buf, 9, input_flags)) {
            if (s_search_buf[0] != '\0') {
                u32 pc = static_cast<u32>(std::stol(s_search_buf, nullptr, 16));
                SetPCBreakpoint(pc);
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    }
}

}// end breakpoints namespace

}// end namespace
}
}
