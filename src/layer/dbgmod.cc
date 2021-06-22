/*
 * dbgmod.cpp
 *
 * Travis Banken
 * 1/11/21
 *
 * Common Modules for use in the ImGui Debug Windows for the PSX.
 */

#include "dbgmod.hh"

#include <deque>

#include "imgui/imgui.h"

#include "core/globals.hh"
#include "cpu/cpu.hh"


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
    m_file_name.resize(64);
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
    if (ImGui::Button("Dump to File")) {
        ImGui::OpenPopup("Dump File");
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
    // Dump to file
    //----------------------------------------------------
    // Always center this window when appearing
    ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Dump File", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted("Enter Name of File");
        auto input_flags_modal = ImGuiInputTextFlags_EnterReturnsTrue;
        if (ImGui::InputTextWithHint("Dump File Name", "Enter File Name", m_file_name.data(), m_file_name.size(), input_flags_modal)) {
            if (m_file_name != "") {
                dumpToFile(mem);
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    }

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

void HexDump::dumpToFile(const std::vector<u8>& mem)
{
    DBG_INFO("Dump Mem to {}", m_file_name);
    // open file
    std::ofstream file;
    file.open(m_file_name, std::ios::binary);
    if (!file.is_open()) {
        DBG_ERROR("Failed to open {} for writing", m_file_name);
        throw std::runtime_error(PSX_FMT("Failed to open {} for writing", m_file_name));
    }

    // dump
    file.write((char*) mem.data(), (long) mem.size());

    file.close();
}

namespace Breakpoints {

#define REMOVE_BP(bp, vector) \
    (vector).erase(std::remove((vector).begin(), (vector).end(), (bp)), (vector).end())

namespace {
struct BreakPair {
    BrkType type;
    u32 addr;
    std::string msg;

    std::string ToString()
    {
        std::string type_str;
        if (type == BrkType::PCWatch) {
            type_str = "PC Breakpoint";
        } else if (type == BrkType::ReadWatch) {
            type_str = "Read Watchpoint";
        } else if (type == BrkType::WriteWatch) {
            type_str = "Write Watchpoint";
        } else if (type == BrkType::Forced) {
            type_str = "Forced Break";
            return PSX_FMT("Forced from {}", msg);
        }
        return PSX_FMT("0x{:08x} ({})", addr, type_str);
    }
};
struct State {
    std::vector<u32> pc_breakpoints;
    std::vector<u32> memw_breakpoints;
    std::vector<u32> memr_breakpoints;

    std::deque<BreakPair> break_queue;
}s;

// *** Adding through TextBox
bool textAddPC()
{
    static char s_search_buf[9] = {0};
    auto input_flags = ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue;
    if (ImGui::InputTextWithHint("Breakpoint##", "Enter address (hex)", s_search_buf, 9, input_flags)) {
        if (s_search_buf[0] != '\0') {
            u32 addr = static_cast<u32>(std::stol(s_search_buf, nullptr, 16));
            Set<BrkType::PCWatch>(addr);
            return true;
        }
    }
    return false;
}
bool textAddMW()
{
    static char s_search_buf[9] = {0};
    auto input_flags = ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue;
    if (ImGui::InputTextWithHint("Breakpoint##", "Enter address (hex)", s_search_buf, 9, input_flags)) {
        if (s_search_buf[0] != '\0') {
            u32 addr = static_cast<u32>(std::stol(s_search_buf, nullptr, 16));
            Set<BrkType::WriteWatch>(addr);
            return true;
        }
    }
    return false;
}
bool textAddMR()
{
    static char s_search_buf[9] = {0};
    auto input_flags = ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue;
    if (ImGui::InputTextWithHint("Breakpoint##", "Enter address (hex)", s_search_buf, 9, input_flags)) {
        if (s_search_buf[0] != '\0') {
            u32 addr = static_cast<u32>(std::stol(s_search_buf, nullptr, 16));
            Set<BrkType::ReadWatch>(addr);
            return true;
        }
    }
    return false;
}
}// end private namespace

void OnActive(bool *active)
{
    static bool add_pc = false;
    static bool add_mw = false;
    static bool add_mr = false;

    if (!ImGui::Begin("Breakpoints", active)) {
        ImGui::End();
        return;
    }

    constexpr int block_width = 220;
    // Display Breakpoints
    // PC
    ImGui::BeginChild("PC Breakpoints", ImVec2(block_width, 500));
    ImGui::TextUnformatted("PC Breakpoints");
    ImGui::Separator();
    int i = 1;
    std::vector<u32> pc_deletes;
    for (const auto& bp : s.pc_breakpoints) {
        ImGui::Text("%d. 0x%08x", i, bp);
        ImGui::SameLine();
        if (ImGui::Button(PSX_FMT("Delete##PC{}", i).c_str())) {
            pc_deletes.push_back(bp);
        }
        i++;
    }
    if (!add_pc && ImGui::Button("Add##PC")) {
        add_pc = true;
    }
    if (add_pc) {
        add_pc = !textAddPC();
    }
    ImGui::EndChild();
    ImGui::SameLine();
    // MEM WRITE
    ImGui::BeginChild("MW Breakpoints", ImVec2(block_width, 500));
    ImGui::TextUnformatted("MemWrite Watchpoints");
    ImGui::Separator();
    i = 1;
    std::vector<u32> memw_deletes;
    for (const auto& bp : s.memw_breakpoints) {
        ImGui::Text("%d. 0x%08x", i, bp);
        ImGui::SameLine();
        if (ImGui::Button(PSX_FMT("Delete##MW{}", i).c_str())) {
            memw_deletes.push_back(bp);
        }
        i++;
    }
    if (!add_mw && ImGui::Button("Add##Write")) {
        add_mw = true;
    }
    if (add_mw) {
        add_mw = !textAddMW();
    }
    ImGui::EndChild();
    ImGui::SameLine();
    // MEM READ
    ImGui::BeginChild("MR Breakpoints", ImVec2(block_width, 500));
    ImGui::TextUnformatted("MemRead Watchpoints");
    ImGui::Separator();
    i = 1;
    std::vector<u32> memr_deletes;
    for (const auto& bp : s.memr_breakpoints) {
        ImGui::Text("%d. 0x%08x", i, bp);
        ImGui::SameLine();
        if (ImGui::Button(PSX_FMT("Delete##MR{}", i).c_str())) {
            memr_deletes.push_back(bp);
        }
        i++;
    }
    if (!add_mr && ImGui::Button("Add##Read")) {
        add_mr = true;
    }
    if (add_mr) {
        add_mr = !textAddMR();
    }
    ImGui::EndChild();

    // Delete any if needed
    for (const auto& bp : pc_deletes) {
        DBG_INFO("Removing PC Breakpoint @ 0x{:08x}", bp);
        REMOVE_BP(bp, s.pc_breakpoints);
    }
    for (const auto& bp : memw_deletes) {
        DBG_INFO("Removing Mem Write Watchpoint @ 0x{:08x}", bp);
        REMOVE_BP(bp, s.memw_breakpoints);
    }
    for (const auto& bp : memr_deletes) {
        DBG_INFO("Removing Mem Read Watchpoint @ 0x{:08x}", bp);
        REMOVE_BP(bp, s.memr_breakpoints);
    }

    ImGui::End();
}

void OnUpdate()
{
    if (s.break_queue.size() > 0)
        ImGui::OpenPopup("Break");
    else
        return;

    // Always center this window when appearing
    ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Break", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        g_emu_state.paused = true;
        BreakPair bp = s.break_queue.front();
        ImGui::TextUnformatted(PSX_FMT("Break! [{}]", bp.ToString()).c_str());
        if (ImGui::Button("Close")) {
            DBG_INFO("Break! [{}]", bp.ToString());
            s.break_queue.pop_front();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void ForceBreak(const std::string& from)
{
    s.break_queue.push_back({BrkType::Forced, 0, from});
}

/*
 * Inform the breakpoint manager that the given address has been
 * reached/written/read.
 */
template <BrkType type>
void Saw(u32 addr)
{
    if constexpr (type == BrkType::PCWatch) {
        // pc breakpoint
        for (const auto& bp : s.pc_breakpoints) {
            if (addr == bp) {
                s.break_queue.push_back({BrkType::PCWatch, addr, ""});
            }
        }
    } else if constexpr (type == BrkType::WriteWatch) {
        // mem write watchpoint
        for (const auto& bp : s.memw_breakpoints) {
            if (addr == bp) {
                s.break_queue.push_back({BrkType::WriteWatch, addr, ""});
            }
        }
    } else if constexpr (type == BrkType::ReadWatch) {
        // mem read watchpoint
        for (const auto& bp : s.memr_breakpoints) {
            if (addr == bp) {
                s.break_queue.push_back({BrkType::ReadWatch, addr, ""});
            }
        }
    } 
}
// template impl needs to be visible to other cpp files to avoid compile errs
template void Saw<BrkType::PCWatch>(u32 addr);
template void Saw<BrkType::WriteWatch>(u32 addr);
template void Saw<BrkType::ReadWatch>(u32 addr);

bool ReadyToBreak()
{
    return s.break_queue.size() != 0;
}

// set breakpoint
template <BrkType type>
void Set(u32 addr)
{
    if constexpr (type == BrkType::PCWatch) {
        // pc breakpoint
        DBG_INFO("Setting PC Breakpoint @ 0x{:08x}", addr);
        s.pc_breakpoints.push_back(addr);
    } else if constexpr (type == BrkType::WriteWatch) {
        // mem write watchpoint
        DBG_INFO("Setting Mem Write Watchpoint @ 0x{:08x}", addr);
        s.memw_breakpoints.push_back(addr);
    } else if constexpr (type == BrkType::ReadWatch) {
        // mem read watchpoint
        DBG_INFO("Setting Mem Read Watchpoint @ 0x{:08x}", addr);
        s.memr_breakpoints.push_back(addr);
    } 
}
// template impl needs to be visible to other cpp files to avoid compile errs
template void Set<BrkType::PCWatch>(u32 addr);
template void Set<BrkType::WriteWatch>(u32 addr);
template void Set<BrkType::ReadWatch>(u32 addr);



}// end breakpoints namespace

}// end namespace
}
}
