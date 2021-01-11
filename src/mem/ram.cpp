/*
 * ram.cpp
 * 
 * Travis Banken
 * 12/6/2020
 * 
 * Main Ram for the PSX.
 */

#include <vector>
#include <memory>

#include "imgui/imgui.h"

#include "mem/ram.h"

#define RAM_INFO(msg) PSXLOG_INFO("RAM", msg)
#define RAM_WARN(msg) PSXLOG_WARN("RAM", msg)
#define RAM_ERROR(msg) PSXLOG_ERROR("RAM", msg)

// protos
static u32 strToHex(const char *str);
static std::pair<u32, bool> mirrorAddr(u32 addr);



Ram::Ram()
{
    RAM_INFO("Initializing 2MB of System RAM");
    m_sysram.resize(2 * 1024 * 1024);
}

void Ram::Reset()
{
    RAM_INFO("Resetting RAM. Zeroing out memory.");
    for (auto& byte : m_sysram) {
        byte = 0x00;
    }
}

// reads
ASResult Ram::Read8(u32 addr)
{
    u8 data = 0;
    auto [maddr, found] = mirrorAddr(addr);
    if (found) {
        PSX_ASSERT(maddr < m_sysram.size());
        data = m_sysram[maddr];
    }

    ASResult asres;
    asres.res.res8 = data;
    asres.found = found;
    return asres;
}

ASResult Ram::Read16(u32 addr)
{
    u16 data = 0;
    auto [maddr, found] = mirrorAddr(addr);
    if (found) {
        PSX_ASSERT(maddr < m_sysram.size() - 2);
        // create 16-bit halfword in little-endian format
        data = m_sysram[maddr];
        data |= static_cast<u16>(m_sysram[maddr+1]) << 8;
    }

    ASResult asres;
    asres.res.res16 = data;
    asres.found = found;
    return asres;
}

ASResult Ram::Read32(u32 addr)
{
    u32 data = 0;
    auto [maddr, found] = mirrorAddr(addr);
    if (found) {
        PSX_ASSERT(maddr < m_sysram.size() - 4);
        // create 32-bit word in little-endian format
        data = m_sysram[maddr];
        data |= static_cast<u32>(m_sysram[maddr+1]) << 8;
        data |= static_cast<u32>(m_sysram[maddr+2]) << 16;
        data |= static_cast<u32>(m_sysram[maddr+3]) << 24;
    }

    ASResult asres;
    asres.res.res32 = data;
    asres.found = found;
    return asres;
}


// writes
bool Ram::Write8(u8 data, u32 addr)
{
    auto [maddr, found] = mirrorAddr(addr);
    if (found) {
        PSX_ASSERT(maddr < m_sysram.size());
        m_sysram[maddr] = data;
    }

    return found;
}

bool Ram::Write16(u16 data, u32 addr)
{
    auto [maddr, found] = mirrorAddr(addr);
    if (found) {
        PSX_ASSERT(maddr < m_sysram.size() - 2);
        // write 16-bit halfword in little-endian format
        m_sysram[maddr] = static_cast<u8>(data & 0xff);
        m_sysram[maddr + 1] = static_cast<u8>((data >> 8) & 0xff);
    }

    return found;
}

bool Ram::Write32(u32 data, u32 addr)
{
    auto [maddr, found] = mirrorAddr(addr);
    if (found) {
        PSX_ASSERT(maddr < m_sysram.size() - 4);
        // write 32-bit word in little-endian format
        m_sysram[maddr] = static_cast<u8>(data & 0xff);
        m_sysram[maddr + 1] = static_cast<u8>((data >> 8) & 0xff);
        m_sysram[maddr + 2] = static_cast<u8>((data >> 16) & 0xff);
        m_sysram[maddr + 3] = static_cast<u8>((data >> 24) & 0xff);
    }

    return found;
}

std::string Ram::GetModuleLabel()
{
    return "RAM";
}

void Ram::OnActive(bool *active)
{
    static u32 start_line = 0;
    static u32 last_line = (u32)m_sysram.size() >> 4;
    static bool find_target = false;
    static u32 target = 0;

    const u32 num_lines = 50;

    if (!ImGui::Begin("Ram Debug", active)) {
        ImGui::End();
        return;
    }

    //----------------------------------------------------
    // Buttons for Navigation
    //----------------------------------------------------
    if (ImGui::Button("Up")) {
        start_line = start_line == 0 ? 0 : start_line - 1;
    }
    ImGui::SameLine();
    if (ImGui::Button("Up+10")) {
        start_line = start_line < 10 ? 0 : start_line - 10;
    }
    ImGui::SameLine();
    if (ImGui::Button("Down")) {
        start_line = start_line == last_line ? last_line : start_line + 1;
    }
    ImGui::SameLine();
    if (ImGui::Button("Down+10")) {
        start_line = start_line >= last_line - 10 ? last_line : start_line + 10;
    }
    ImGui::SameLine();
    static char searchBuf[7] = "";
    auto input_flags = ImGuiInputTextFlags_CharsHexadecimal;
    ImGui::InputTextWithHint("Goto Address", "Enter address (hex)", searchBuf, sizeof(searchBuf), input_flags);
    u32 res = strToHex(searchBuf);
    if (target != res) {
        find_target = true;
        target = res;
        start_line = (target >> 4);
    } else {
        find_target = false;
    }
    //----------------------------------------------------

    //----------------------------------------------------
    // Hex Dump Region
    //----------------------------------------------------
    // set up hex dump
    ImGuiWindowFlags hex_dump_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    u32 endLine = start_line + num_lines;
    if (endLine > last_line) {
        endLine = last_line;
    }

    if (ImGui::BeginChild("Hex Dump", ImVec2(0,0), false, hex_dump_flags)) {
        for (u32 line = start_line; line < endLine; line++) {
            // color the line if it is our search target
            if (line != 0 && line == (target >> 4)) {
                ImGui::TextColored(ImVec4(0.8f,0.2f,0,1), "%s", hexDumpLine(line << 4).data());
            } else {
                ImGui::TextUnformatted(hexDumpLine(line << 4).data());
            }
        }
        ImGui::EndChild();
    }
    //----------------------------------------------------

    ImGui::End();
}

/*
 * Creates one line of hexdump output for a given address. Will proccess 16 bytes
 * of data starting at the given address.
 */
std::string Ram::hexDumpLine(u32 addr)
{
    /* PSX_ASSERT((size_t)addr + 16 <= m_sysram.size()); */
    if ((size_t)addr + 16 > m_sysram.size()) {
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
            , m_sysram[addr+0], m_sysram[addr+1], m_sysram[addr+2], m_sysram[addr+3]
            , m_sysram[addr+4], m_sysram[addr+5], m_sysram[addr+6], m_sysram[addr+7]
            , m_sysram[addr+8], m_sysram[addr+9], m_sysram[addr+10], m_sysram[addr+11]
            , m_sysram[addr+12], m_sysram[addr+13], m_sysram[addr+14], m_sysram[addr+15]
            , hexDumpChar(m_sysram[addr+0]), hexDumpChar(m_sysram[addr+1])
            , hexDumpChar(m_sysram[addr+2]), hexDumpChar(m_sysram[addr+3])
            , hexDumpChar(m_sysram[addr+4]), hexDumpChar(m_sysram[addr+5])
            , hexDumpChar(m_sysram[addr+6]), hexDumpChar(m_sysram[addr+7])
            , hexDumpChar(m_sysram[addr+8]), hexDumpChar(m_sysram[addr+9])
            , hexDumpChar(m_sysram[addr+9]), hexDumpChar(m_sysram[addr+11])
            , hexDumpChar(m_sysram[addr+12]), hexDumpChar(m_sysram[addr+13])
            , hexDumpChar(m_sysram[addr+14]), hexDumpChar(m_sysram[addr+15])
    );
    return dump;
}

//================================================
// Private Static Helpers
//================================================
static std::pair<u32, bool> mirrorAddr(u32 addr)
{
    // TODO: Maybe add support for exceptions when access beyond 512MB region?
    // Hopefully no bugs in ps1 games :/

    // KUSEG
    // first 64KB is kernel, rest is user mem
    if (addr < 0x0020'0000) {
        return {addr, true};
    }

    // KSEG0
    if (addr >= 0x8000'0000 && addr < 0x8020'0000) {
        return {addr - 0x8000'0000, true};
    }

    // KSEG1
    if (addr >= 0xa000'0000 && addr < 0xa020'0000) {
        return {addr - 0xa000'0000, true};
    }

    return {0, false};
}

// Very hacky string to hex converter
static u32 strToHex(const char *str)
{
    // assume string is a hex string
    auto ctoh = [] (char c) -> u32 {
        if (c <= '9') {
            return (u32) c - '0';
        }
        if (c <= 'F') {
            return (u32) (c - 'A') + 10;
        }
        if (c <= 'f') {
            return (u32) (c - 'a') + 10;
        }
        return 0;
    };

    u32 res = 0;
    for (size_t i = 0; str[i] != '\0'; i++) {
        res <<= 4;
        res += ctoh(str[i]);
    }
    return res;
}
