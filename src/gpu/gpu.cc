/*
 * gpu.cpp
 *
 * Travis Banken
 * 2/13/2021
 *
 * GPU for the PSX.
 */

#include "gpu.hh"

#include <queue>

#include "imgui/imgui.h"

#include "mem/ram.hh"
#include "view/imgui/dbgmod.hh"

#define GPU_INFO(...) PSXLOG_INFO("GPU", __VA_ARGS__)
#define GPU_WARN(...) PSXLOG_WARN("GPU", __VA_ARGS__)
#define GPU_ERROR(...) PSXLOG_ERROR("GPU", __VA_ARGS__)
#define GPU_FATAL(...) GPU_ERROR(__VA_ARGS__); throw std::runtime_error(PSX_FMT(__VA_ARGS__))

// *** Private Data ***
namespace {
using namespace Psx;
enum class CmdType {
    Misc      = 0,
    Polygon   = 1,
    Line      = 2,
    Rectangle = 3,
    VVBlit    = 4, // VRAM-to-VRAM blit
    CVBlit    = 5, // CPU-to-VRAM blit
    VCBlit    = 6, // VRAM-to-CPU blit
    EnvCmds   = 7,
    None      = 0xff,
};

enum class Shading {
    Flat = 0,
    Gouraud = 1,
};

struct State {
    // status register
    u32 sr = 0;

    // current command being processed
    CmdType cmd = CmdType::None;

    struct DrawingEnv {
        // texture flip
        bool texture_rect_flip_x = false;
        bool texture_rect_flip_y = false;
        // texture window settings (in 8 px steps)
        u8 texture_win_mask_x = 0;
        u8 texture_win_mask_y = 0;
        u8 texture_win_offset_x = 0;
        u8 texture_win_offset_y = 0;

        struct DrawArea {
            u16 x = 0;
            // Old 160 pin GPU (1MB of VRAM)
            // New 208 pin GPU (2MB of VRAM)
            u16 y = 0; 
        } draw_area[2];

        // TODO: These are signed values!
        u16 draw_offset_x = 0;
        u16 draw_offset_y = 0;
    } env;

    struct Display {
        u16 start_x = 0;
        u16 start_y = 0;
        u16 range_x1 = 0;
        u16 range_x2 = 0;
        u16 range_y1 = 0;
        u16 range_y2 = 0;
    } display;

    struct RenderFlags {
        Shading shading;
        bool textured;
        bool transparent;
    } render_flags;

    // Command queue
    std::queue<u32> cmd_queue = {};

    // 1MB of vram
    std::vector<u8> vram;
}s;


// Prototypes
void handleGP0Cmd(u32 cmd);
void handleGP1Cmd(u32 word);
void handleMiscCmd(u32 word);
void handleEnvCmd(u32 word);
void handlePolygon(u32 word);
void softReset();
void resetCmdQueue();
void ackIrq();
void displayDisable(bool disable);
void dmaDirection(u32 dir);
void displayStart(u32 word);
void horzDisplayRange(u32 word);
void vertDisplayRange(u32 word);
void displayMode(u32 word);
void displayEnvInfo();
void displayStatusRegister();

} // end ns

// *** Public Data ***
namespace Psx {
namespace Gpu {

void Init()
{
    GPU_INFO("Initializing state");
    s.vram.resize(1 * 1024 * 1024);
    Util::SetBits(s.sr, 26, 3, 0x7);
}

void Reset()
{
    GPU_INFO("Resetting state");
    s = {};
    Util::SetBits(s.sr, 26, 3, 0x7);
    // vram was reset, so need to resize
    s.vram.resize(1 * 1024 * 1024);
}

void RenderFrame()
{
    // TODO Render Something??
}

/*
 * Performs one step in the gpu operation. Usually this entails processing the
 * next command in the queue. Returns true if new frame ready to render.
 */
bool Step()
{
    // if (s.cmd_queue.size() != 0) {
    //     // we still have commands to handle
    //     u32 word = s.cmd_queue.front();
    //     s.cmd_queue.pop();
    //     handleGP0Cmd(word);
    // }
    // if (s.cmd == CmdType::None) {
    //     // we are ready to receive command words
    //     // and we are ready for dma
    //     // and we are not busy
    //     Util::SetBits(s.sr, 26, 3, 0x7);
    // }

    // do something i guess :/
    // s.renderer->RenderSomething();

    static u32 count = 0;
    bool new_frame = false;
    // if (count > 2413 * 263) {
    if (count > 2413) {
        new_frame = true;
        count = 0;
    } else {
        count++;
        new_frame = false;
    }

    return false;
}

// *** Read ***
template<class T>
T Read(u32 addr)
{
    T data = 0;
    if constexpr (std::is_same_v<T, u8>) {
        // read8 (assuming little endian)
        PSX_ASSERT(0);
    } else if constexpr (std::is_same_v<T, u16>) {
        // read16 (assuming little endian)
        PSX_ASSERT(0);
    } else if constexpr (std::is_same_v<T, u32>) {
        // read32
        if (addr == 0x1f80'1810) {
            // responses from commands
            GPU_ERROR("!!! GPUREAD should return data from frame buffer !!!");
            data = 0; // TODO: Fix me
        } else {
            PSX_ASSERT(addr == 0x1f80'1814);
            // Status Register
            data = s.sr;
        }
    } else {
        static_assert(!std::is_same_v<T, T>);
    }
    return data;
}
// template impl needs to be visable to other cpp files to avoid compile err
template u8 Read<u8>(u32 addr);
template u16 Read<u16>(u32 addr);
template u32 Read<u32>(u32 addr);

// *** Write ***
template<class T>
void Write(T data, u32 addr)
{
    if constexpr (std::is_same_v<T, u8>) {
        // write8 (assuming little endian)
        PSX_ASSERT(0);
    } else if constexpr (std::is_same_v<T, u16>) {
        // write16 (assuming little endian)
        PSX_ASSERT(0);
    } else if constexpr (std::is_same_v<T, u32>) {
        // write32
        if (addr == 0x1f80'1810) {
            // GP0 (Rendering and VRAM access)
            // s.cmd_queue.push(data);
            handleGP0Cmd(data);
        } else {
            PSX_ASSERT(addr == 0x1f80'1814);
            // GP1 (Display Control and DMA control)
            handleGP1Cmd(data);
        }
    } else {
        static_assert(!std::is_same_v<T, T>);
    }
}
// template impl needs to be visable to other cpp files to avoid compile err
template void Write<u8>(u8 data, u32 addr);
template void Write<u16>(u16 data, u32 addr);
template void Write<u32>(u32 data, u32 addr);

/*
 * Perform Cmds from dma, starting at the given address in ram
 */
void DoDmaCmds(u32 addr)
{
    addr &= 0x1f'fffc;
    while (true) {
        // read header
        u32 header = Ram::Read<u32>(addr);

        // high byte contains size of packet
        u32 num_words = (header >> 24) & 0xff;

        while (num_words > 0) {
            addr = (addr + 4) & 0x1f'fffc;
            u32 cmd = Ram::Read<u32>(addr);
            GPU_INFO("LL COMMAND: 0x{:08x}", cmd);
            handleGP0Cmd(cmd);
            num_words--;
        }

        // are we done processing packets?
        if ((header & 0xff'ffff) == 0xff'ffff) {
            break;
        }

        // find next packet
        addr = header & 0x1f'fffc;
    }
}

/*
 * Called by gui for debug display.
 */
void OnActive(bool *active)
{
    if (!ImGui::Begin("GPU Debug", active)) {
        ImGui::End();
        return;
    }

    //-----------------------
    // Environment
    //-----------------------
    ImGui::BeginGroup();
    ImGui::TextUnformatted("Environment State");
    ImGui::Separator();
    displayEnvInfo();
    ImGui::EndGroup();

    ImGui::SameLine();

    ImGui::BeginGroup();
    ImGui::TextUnformatted("Status Register");
    ImGui::Separator();
    displayStatusRegister();
    ImGui::EndGroup();


    ImGui::End();
}

} // end ns
}

// *** Private Functions ***
namespace {

using namespace Psx;

inline void finishedCommand()
{
    s.cmd = CmdType::None;
}

/*
 * Handle the next gpu command. This acts as a state machine, a full gpu
 * cmd sequence will take multiple calls to this function.
 */
void handleGP0Cmd(u32 word)
{
    // If no cmd in progress, read the command
    if (s.cmd == CmdType::None) {
        s.cmd = static_cast<CmdType>((word >> 29) & 0x7);
    }
    switch (s.cmd) {
    case CmdType::Misc:
        handleMiscCmd(word);
        break;
    case CmdType::Polygon:
        handlePolygon(word);
        break;
    case CmdType::Line:
        PSX_ASSERT(0);
        break;
    case CmdType::Rectangle:
        PSX_ASSERT(0);
        break;
    case CmdType::VVBlit:
        PSX_ASSERT(0);
        break;
    case CmdType::CVBlit:
        PSX_ASSERT(0);
        break;
    case CmdType::VCBlit:
        PSX_ASSERT(0);
        break;
    case CmdType::EnvCmds:
        handleEnvCmd(word);
        break;
    default:
        GPU_FATAL("Unknown GP0 command: {:02x}", s.cmd);
    }
}

/*
 * Handle GP1 Command. These commands are executed immediately, rather than
 * through the command fifo.
 */
void handleGP1Cmd(u32 word)
{
    u8 cmd = (word >> 24) & 0xff;
    switch (cmd) {
    case 0x00: // reset
        softReset();
        break;
    case 0x01: // reset command buffer
        resetCmdQueue();
        break;
    case 0x02: // ack gpu interrupt
        ackIrq();
        break;
    case 0x03: // display enable
        displayDisable(word & 0x1);
        break;
    case 0x04: // dma direction / data request
        dmaDirection(word);
        break;
    case 0x05: // start of display area
        displayStart(word);
        break;
    case 0x06: // horizontal display range
        horzDisplayRange(word);
        break;
    case 0x07: // vertical display range
        vertDisplayRange(word);
        break;
    case 0x08: // display mode
        displayMode(word);
        break;
    default:
        GPU_FATAL("Unknown GP1 command: {:02x}", cmd);
    }
}

/*
 * Handle Misc Commands.
 */
void handleMiscCmd(u32 word)
{
    u8 cmd = (word >> 24) & 0xff;
    switch (cmd) {
    case 0x00: // NOP
        finishedCommand();
        // s.cmd = CmdType::None;
        break;
    case 0x01: // clear cache
        PSX_ASSERT(0);
        break;
    case 0x02: // quick rectangle fill
        PSX_ASSERT(0);
        break;
    default:
        GPU_FATAL("Unknown GPU Misc Command [{:02x}]", cmd);
    }
}

/*
 * Handle Environment commands.
 */
void handleEnvCmd(u32 word)
{
    u8 cmd = (word >> 24) & 0xff;
    switch (cmd) {
    case 0xe1: // Draw Mode setting (aka "Texpage")
        Util::SetBits(s.sr, 0, 10, (word >> 10) & 0x3ff);
        Util::SetBits(s.sr, 15, 1, (word >> 11) & 0x1);
        s.env.texture_rect_flip_x = (word >> 12) & 0x1;
        s.env.texture_rect_flip_y = (word >> 13) & 0x1;
        break;
    case 0xe2: // Texture Window Setting
        s.env.texture_win_mask_x = (word >> 0) & 0x1f;
        s.env.texture_win_mask_y = (word >> 5) & 0x1f;
        s.env.texture_win_offset_x = (word >> 10) & 0x1f;
        s.env.texture_win_offset_y = (word >> 15) & 0x1f;
        break;
    case 0xe3: // Drawing area (Top Left)
    case 0xe4: // Drawing area (Bottom Right)
    {
        u8 corner = cmd - 0xe3;
        s.env.draw_area[corner].x = (word >> 0) & 0x3ff;
        // TODO: if new gpu, y is 10 bits, if old, y is 9 bits
        s.env.draw_area[corner].y = (word >> 10) & 0x3ff;
        // s.env.draw_area[corner].y = (word >> 10) & 0x1ff;
        break;
    }
    case 0xe5: // Draw offset
        s.env.draw_offset_x = (word >> 0) & 0x7ff;
        s.env.draw_offset_y = (word >> 11) & 0x7ff;
        break;
    case 0xe6: // Mask Bit Setting
        Util::SetBits(s.sr, 11, 1, word & 0x1);
        Util::SetBits(s.sr, 12, 1, (word >> 1) & 0x1);
        break;
    default:
        GPU_FATAL("Unknown Environment Command [{:02x}]", cmd);
    }
    finishedCommand();
}

/*
 * Polygon Builder state machine.
 */
void handlePolygon(u32 word)
{
    static Gpu::Polygon s_polygon;
    static int s_words_left = 0;

    switch (s_words_left) {
    case 0: // start of new command
    {
        s_polygon.gouraud_shaded = Util::GetBits(word, 28, 1);
        s_polygon.num_vertices = Util::GetBits(word, 27, 1) ? 4 : 3;
        break;
    }
    case 1: // end of command
        // TODO
        s.cmd = CmdType::None;
        break;
    default:
        GPU_FATAL("Unexpected number of words left in Polygon Command: {}", s_words_left);
    }

    s_words_left--;
    PSX_ASSERT(0);
}

/*
 * Software reset.
 */
void softReset()
{
    GPU_INFO("Software Reset");
    // clear fifo
    resetCmdQueue();
    // ack irq (0)
    ackIrq();
    // display off (1)
    displayDisable(true);
    // dma off (0)
    dmaDirection(0);
    // display address (0)
    displayStart(0);
    // display x1,x2 (x1=200h, x2=200h+256*10)
    u32 word = 0;
    Util::SetBits(word, 0, 12, 0x200);
    Util::SetBits(word, 12, 12, 0x200 + (256 * 10));
    horzDisplayRange(word);
    // display y1,y2 (y1=010h, y2=010h+240)
    word = 0;
    Util::SetBits(word, 0, 12, 0x010);
    Util::SetBits(word, 12, 12, 0x010 + 240);
    vertDisplayRange(word);
    // display mode 320x200 NTSC (0)
    displayMode(0x0000'0001);
    // rendering attributes (0)
    handleEnvCmd(0xe100'0000);
    handleEnvCmd(0xe200'0000);
    handleEnvCmd(0xe300'0000);
    handleEnvCmd(0xe400'0000);
    handleEnvCmd(0xe500'0000);
    handleEnvCmd(0xe600'0000);
}

void resetCmdQueue()
{
    s.cmd_queue = {};
    s.cmd = CmdType::None;
}

void ackIrq()
{
    Util::SetBits(s.sr, 24, 1, 0);
}

void displayDisable(bool disable)
{
    Util::SetBits(s.sr, 23, 1, disable);
}

void dmaDirection(u32 dir)
{
    Util::SetBits(s.sr, 29, 2, dir);
}

void displayStart(u32 word)
{
    s.display.start_x = static_cast<u16>(Util::GetBits(word, 0, 10));
    s.display.start_y = static_cast<u16>(Util::GetBits(word, 10, 9));
}

void horzDisplayRange(u32 word)
{
    s.display.range_x1 = static_cast<u16>(Util::GetBits(word, 0, 12));
    s.display.range_x2 = static_cast<u16>(Util::GetBits(word, 12, 12));
}

void vertDisplayRange(u32 word)
{
    s.display.range_y1 = static_cast<u16>(Util::GetBits(word, 0, 10));
    s.display.range_y2 = static_cast<u16>(Util::GetBits(word, 10, 10));
}

void displayMode(u32 word)
{
    Util::SetBits(s.sr, 17, 2, word);
    Util::SetBits(s.sr, 19, 1, word >> 2);
    Util::SetBits(s.sr, 20, 1, word >> 3);
    Util::SetBits(s.sr, 21, 1, word >> 4);
    Util::SetBits(s.sr, 22, 1, word >> 5);
    Util::SetBits(s.sr, 16, 1, word >> 6);
    Util::SetBits(s.sr, 14, 1, word >> 7);
}

//+++++++++++++++++++++++++++++
// Debug Helpers
//+++++++++++++++++++++++++++++
#define DBG_DISPLAY(...) ImGui::TextUnformatted(PSX_FMT(__VA_ARGS__).c_str())
void displayEnvInfo()
{
    DBG_DISPLAY("Texture Rect Flip X: {}", s.env.texture_rect_flip_x);
    DBG_DISPLAY("Texture Rect Flip Y: {}", s.env.texture_rect_flip_y);
    DBG_DISPLAY("Texture Window Mask X: {}", s.env.texture_win_mask_x);
    DBG_DISPLAY("Texture Window Mask Y: {}", s.env.texture_win_mask_y);
    DBG_DISPLAY("Texture Window Offset X: {}", s.env.texture_win_offset_x);
    DBG_DISPLAY("Texture Window Offset Y: {}", s.env.texture_win_offset_y);
    DBG_DISPLAY("Draw Area Corner (Top Left) : ({}, {})", s.env.draw_area[0].x, s.env.draw_area[0].y);
    DBG_DISPLAY("Draw Area Corner (Bot Right): ({}, {})", s.env.draw_area[1].x, s.env.draw_area[1].y);
    DBG_DISPLAY("Draw Offset X: {}", s.env.draw_offset_x);
    DBG_DISPLAY("Draw Offset Y: {}", s.env.draw_offset_y);
}

void displayStatusRegister()
{
    DBG_DISPLAY("Status Raw                : 0x{:08x}", s.sr);
    DBG_DISPLAY("(0-3)   Texture Page X Base       : {}", Util::GetBits(s.sr, 0, 4));
    DBG_DISPLAY("(4)     Texture Page Y Base       : {}", Util::GetBits(s.sr, 4, 1));
    DBG_DISPLAY("(5-6)   Semi Transparency         : {}", Util::GetBits(s.sr, 5, 2));
    DBG_DISPLAY("(7-8)   Texture page colors       : {}", Util::GetBits(s.sr, 7, 2));
    DBG_DISPLAY("(9)     Dither 24bit to 15bit     : {}", Util::GetBits(s.sr, 9, 1));
    DBG_DISPLAY("(10)    Drawing to display area   : {}", Util::GetBits(s.sr, 10, 1));
    DBG_DISPLAY("(11)    Set Mask-bit when drawing pixels: {}", Util::GetBits(s.sr, 11, 1));
    DBG_DISPLAY("(12)    Draw Pixels               : {}", Util::GetBits(s.sr, 12, 1));
    DBG_DISPLAY("(13)    Interlace Field           : {}", Util::GetBits(s.sr, 13, 1));
    DBG_DISPLAY("(14)    Reverseflag               : {}", Util::GetBits(s.sr, 14, 1));
    DBG_DISPLAY("(15)    Texture Disable           : {}", Util::GetBits(s.sr, 15, 1));
    DBG_DISPLAY("(16)    Horizontal Resolution 2   : {}", Util::GetBits(s.sr, 16, 1));
    DBG_DISPLAY("(17-18) Horizontal Resolution 1   : {}", Util::GetBits(s.sr, 17, 2));
    DBG_DISPLAY("(19)    Vertical Resolution       : {}", Util::GetBits(s.sr, 19, 1));
    DBG_DISPLAY("(20)    Video Mode                : {}", Util::GetBits(s.sr, 20, 1));
    DBG_DISPLAY("(21)    Display Area Color Depth  : {}", Util::GetBits(s.sr, 21, 1));
    DBG_DISPLAY("(22)    Vertical Interlace        : {}", Util::GetBits(s.sr, 22, 1));
    DBG_DISPLAY("(23)    Display Enable            : {}", Util::GetBits(s.sr, 23, 1));
    DBG_DISPLAY("(24)    Interrupt Request (IRQ1)  : {}", Util::GetBits(s.sr, 24, 1));
    DBG_DISPLAY("(25)    DMA / Data Request        : {}", Util::GetBits(s.sr, 25, 1));
    DBG_DISPLAY("(26)    Ready to recieve Cmd Word : {}", Util::GetBits(s.sr, 26, 1));
    DBG_DISPLAY("(27)    Ready to send VRAM to CPU : {}", Util::GetBits(s.sr, 27, 1));
    DBG_DISPLAY("(28)    Ready to recieve DMA Block: {}", Util::GetBits(s.sr, 28, 1));
    DBG_DISPLAY("(29-30) DMA Direction             : {}", Util::GetBits(s.sr, 29, 2));
    DBG_DISPLAY("(31)    Drawing even/odd lines in interlace mode: {}", Util::GetBits(s.sr, 31, 1));
}

} // end private ns
