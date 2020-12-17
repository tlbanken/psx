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
    m_sysRam.resize(2 * 1024 * 1024);
}

// reads
ASResult Ram::read8(u32 addr)
{
    u8 data = 0;
    auto [maddr, found] = mirrorAddr(addr);
    if (found) {
        PSX_ASSERT(maddr < m_sysRam.size());
        data = m_sysRam[maddr];
    }

    ASResult asres;
    asres.res.res8 = data;
    asres.found = found;
    return asres;
}

ASResult Ram::read16(u32 addr)
{
    u16 data = 0;
    auto [maddr, found] = mirrorAddr(addr);
    if (found) {
        PSX_ASSERT(maddr < m_sysRam.size());
        // create 16-bit halfword in little-endian format
        data = m_sysRam[maddr];
        data |= static_cast<u16>(m_sysRam[maddr+1]) << 8;
    }

    ASResult asres;
    asres.res.res16 = data;
    asres.found = found;
    return asres;
}

ASResult Ram::read32(u32 addr)
{
    u32 data = 0;
    auto [maddr, found] = mirrorAddr(addr);
    if (found) {
        PSX_ASSERT(maddr < m_sysRam.size());
        // create 32-bit word in little-endian format
        data = m_sysRam[maddr];
        data |= static_cast<u32>(m_sysRam[maddr+1]) << 8;
        data |= static_cast<u32>(m_sysRam[maddr+2]) << 16;
        data |= static_cast<u32>(m_sysRam[maddr+3]) << 24;
    }

    ASResult asres;
    asres.res.res32 = data;
    asres.found = found;
    return asres;
}


// writes
bool Ram::write8(u8 data, u32 addr)
{
    auto [maddr, found] = mirrorAddr(addr);
    if (found) {
        PSX_ASSERT(maddr < m_sysRam.size());
        m_sysRam[maddr] = data;
    }

    return found;
}

bool Ram::write16(u16 data, u32 addr)
{
    auto [maddr, found] = mirrorAddr(addr);
    if (found) {
        PSX_ASSERT(maddr < m_sysRam.size());
        // write 16-bit halfword in little-endian format
        m_sysRam[maddr] = static_cast<u8>(data & 0xff);
        m_sysRam[maddr + 1] = static_cast<u8>((data >> 8) & 0xff);
    }

    return found;
}

bool Ram::write32(u32 data, u32 addr)
{
    auto [maddr, found] = mirrorAddr(addr);
    if (found) {
        PSX_ASSERT(maddr < m_sysRam.size());
        // write 32-bit word in little-endian format
        m_sysRam[maddr] = static_cast<u8>(data & 0xff);
        m_sysRam[maddr + 1] = static_cast<u8>((data >> 8) & 0xff);
        m_sysRam[maddr + 2] = static_cast<u8>((data >> 16) & 0xff);
        m_sysRam[maddr + 3] = static_cast<u8>((data >> 24) & 0xff);
    }

    return found;
}

std::string Ram::getModuleLabel()
{
    return "RAM";
}

void Ram::onActive(bool *active)
{
    static u32 startLine = 0;
    static u32 lastLine = (u32)m_sysRam.size() >> 4;
    static bool findTarget = false;
    static u32 target = 0;

    const u32 numLines = 50;

    if (!ImGui::Begin("Ram Debug", active)) {
        ImGui::End();
        return;
    }

    //----------------------------------------------------
    // Buttons for Navigation
    //----------------------------------------------------
    if (ImGui::Button("Up")) {
        startLine = startLine == 0 ? 0 : startLine - 1;
    }
    ImGui::SameLine();
    if (ImGui::Button("Up+10")) {
        startLine = startLine < 10 ? 0 : startLine - 10;
    }
    ImGui::SameLine();
    if (ImGui::Button("Down")) {
        startLine = startLine == lastLine ? lastLine : startLine + 1;
    }
    ImGui::SameLine();
    if (ImGui::Button("Down+10")) {
        startLine = startLine >= lastLine - 10 ? lastLine : startLine + 10;
    }
    ImGui::SameLine();
    static char searchBuf[7] = "";
    auto inputFlags = ImGuiInputTextFlags_CharsHexadecimal;
    ImGui::InputTextWithHint("Goto Address", "Enter address (hex)", searchBuf, sizeof(searchBuf), inputFlags);
    u32 res = strToHex(searchBuf);
    if (target != res) {
        findTarget = true;
        target = res;
        startLine = (target >> 4);
    } else {
        findTarget = false;
    }
    //----------------------------------------------------

    //----------------------------------------------------
    // Hex Dump Region
    //----------------------------------------------------
    // set up hex dump
    ImGuiWindowFlags hexDumpFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    u32 endLine = startLine + numLines;
    if (endLine > lastLine) {
        endLine = lastLine;
    }

    if (ImGui::BeginChild("Hex Dump", ImVec2(0,0), false, hexDumpFlags)) {
        for (u32 line = startLine; line < endLine; line++) {
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
    /* PSX_ASSERT((size_t)addr + 16 <= m_sysRam.size()); */
    if ((size_t)addr + 16 > m_sysRam.size()) {
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
            , m_sysRam[addr+0], m_sysRam[addr+1], m_sysRam[addr+2], m_sysRam[addr+3]
            , m_sysRam[addr+4], m_sysRam[addr+5], m_sysRam[addr+6], m_sysRam[addr+7]
            , m_sysRam[addr+8], m_sysRam[addr+9], m_sysRam[addr+10], m_sysRam[addr+11]
            , m_sysRam[addr+12], m_sysRam[addr+13], m_sysRam[addr+14], m_sysRam[addr+15]
            , hexDumpChar(m_sysRam[addr+0]), hexDumpChar(m_sysRam[addr+1])
            , hexDumpChar(m_sysRam[addr+2]), hexDumpChar(m_sysRam[addr+3])
            , hexDumpChar(m_sysRam[addr+4]), hexDumpChar(m_sysRam[addr+5])
            , hexDumpChar(m_sysRam[addr+6]), hexDumpChar(m_sysRam[addr+7])
            , hexDumpChar(m_sysRam[addr+8]), hexDumpChar(m_sysRam[addr+9])
            , hexDumpChar(m_sysRam[addr+9]), hexDumpChar(m_sysRam[addr+11])
            , hexDumpChar(m_sysRam[addr+12]), hexDumpChar(m_sysRam[addr+13])
            , hexDumpChar(m_sysRam[addr+14]), hexDumpChar(m_sysRam[addr+15])
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
