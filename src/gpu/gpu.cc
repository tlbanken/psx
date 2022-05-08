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
#include "view/geometry.hh"
#include "view/view.hh"

#define GPU_INFO(...) PSXLOG_INFO("GPU", __VA_ARGS__)
#define GPU_WARN(...) PSXLOG_WARN("GPU", __VA_ARGS__)
#define GPU_ERROR(...) PSXLOG_ERROR("GPU", __VA_ARGS__)
#define GPU_FATAL(...) GPU_ERROR(__VA_ARGS__); throw std::runtime_error(PSX_FMT(__VA_ARGS__))

#define GPU_CMD_NONE 0xff

namespace Psx {
namespace Gpu {
// *** Private Data ***
namespace {
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

// Polygon states
enum class Gp0State {
    Ready,
    Color,
    Vertex,
    Texture,
    LoadImageCoord,
    LoadImageSize,
    LoadImageData,
};

struct PolyConfig {
    u8 num_vertices = 0;
    // TODO
};

struct State {
    // status register
    u32 sr = 0;

    // current command being processed
    u8 cmd = GPU_CMD_NONE;
    Gp0State gp0_state = Gp0State::Ready;

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

    // 1MB of vram
    std::vector<u8> vram;
}s;


// Prototypes
void handleGP1Cmd(u32 word);
void copyRectangleCpuToVram(u32 word);
void copyRectangleVramToCpu(u32 word);
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
void finishedCommand();
// poly commands
void handleMonoPoly(u32 word, const PolyConfig& config);
void handleShadedPoly(u32 word, const PolyConfig& config);
void handleMonoTexturedPoly(u32 word, const PolyConfig& config);
void handleShadedTexturedPoly(u32 word, const PolyConfig& config);

} // end ns

// *** Public ***
void Init()
{
    GPU_INFO("Initializing state");
    s.vram.resize(1 * 1024 * 1024);
    Util::SetBits(s.sr, 26, 3, 0x7);
}

void Reset()
{
    GPU_INFO("Resetting state");
    // handlePolygon(0, true);
    s.gp0_state = Gp0State::Ready;
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
            GPU_INFO("Direct GP0Cmd: {:08x}", data);
            DoGP0Cmd(data);
        } else {
            PSX_ASSERT(addr == 0x1f80'1814);
            // GP1 (Display Control and DMA control)
            GPU_INFO("Direct GP1Cmd: {:08x}", data);
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
            DoGP0Cmd(cmd);
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

/*
 * Handle the next gpu command. This acts as a state machine, a full gpu
 * cmd sequence will take multiple calls to this function.
 */
void DoGP0Cmd(u32 word)
{
    // If no cmd in progress, read the command
    if (s.cmd == GPU_CMD_NONE) {
        s.cmd = Util::GetBits(word, 24, 8);
    }
    switch (s.cmd) {
    case 0x00: // NOP
        finishedCommand();
        break;
    case 0x01: // clear cache
        // TODO
        GPU_WARN("Cache not supported! Skipping clear cache command...");
        finishedCommand();
        break;
    case 0x02: // quick rectangle fill
        PSX_ASSERT(0);
        break;
    case 0xa0: // copy rectangle (CPU TO VRAM)
        // TODO: need to build a state machine for the image transfer cmd
        copyRectangleCpuToVram(word);
        break;
    case 0xc0:
        // TODO: need to build a state machine for the image transfer cmd
        copyRectangleVramToCpu(word);
        break;
    case 0x1f: // interrupt request
        GPU_ERROR("No support for interrupt request!");
        finishedCommand();
        PSX_ASSERT(0);
        break;
    // ** MONOCHROME POLYS **
    case 0x20: // Monochrome three-point polygon, opaque
        PSX_ASSERT(0);
        break;
    case 0x22: // Monochrome three-point polygon, semi-transparent
        PSX_ASSERT(0);
        break;
    case 0x28: // Monochrome four-point polygon, opaque
        handleMonoPoly(word, {.num_vertices = 4});
        break;
    case 0x2a: // Monochrome four-point polygon, semi-transparent
        PSX_ASSERT(0);
        break;
    // ** TEXTURED POLYS **
    case 0x24: // Textured three-point polygon, opaque, texture-blending
        PSX_ASSERT(0);
        break;
    case 0x25: // Textured three-point polygon, opaque, raw-texture
        PSX_ASSERT(0);
        break;
    case 0x26: // Textured three-point polygon, semi-transparent, texture-blending
        PSX_ASSERT(0);
        break;
    case 0x27: // Textured three-point polygon, semi-transparent, raw-texture
        PSX_ASSERT(0);
        break;
    case 0x2c: // Textured four-point polygon, opaque, texture-blending
        // TODO: config needs work
        handleMonoTexturedPoly(word, {.num_vertices = 4});
        break;
    case 0x2d: // Textured four-point polygon, opaque, raw-texture
        PSX_ASSERT(0);
        break;
    case 0x2e: // Textured four-point polygon, semi-transparent, texture-blending
        PSX_ASSERT(0);
        break;
    case 0x2f: // Textured four-point polygon, semi-transparent, raw-texture
        PSX_ASSERT(0);
        break;
    // ** SHADED POLYS **
    case 0x30: // Shaded three-point polygon, opaque
        handleShadedPoly(word, {.num_vertices = 3});
        break;
    case 0x32: // Shaded three-point polygon, semi-transparent
        PSX_ASSERT(0);
        break;
    case 0x38: // Shaded four-point polygon, opaque
        handleShadedPoly(word, {.num_vertices = 4});
        break;
    case 0x3a: // Shaded four-point polygon, semi-transparent
        PSX_ASSERT(0);
        break;
    case 0x34: // Shaded Textured three-point polygon, opaque, texture-blending
        PSX_ASSERT(0);
        break;
    case 0x36: // Shaded Textured three-point polygon, semi-transparent, tex-blend
        PSX_ASSERT(0);
        break;
    case 0x3c: // Shaded Textured four-point polygon, opaque, texture-blending
        PSX_ASSERT(0);
        break;
    case 0x3e: // Shaded Textured four-point polygon, semi-transparent, tex-blend
        PSX_ASSERT(0);
        break;
    // ** ENV CMDS **
    case 0xe1: // Draw Mode setting (aka "Texpage")
        Util::SetBits(s.sr, 0, 10, (word >> 10) & 0x3ff);
        Util::SetBits(s.sr, 15, 1, (word >> 11) & 0x1);
        s.env.texture_rect_flip_x = (word >> 12) & 0x1;
        s.env.texture_rect_flip_y = (word >> 13) & 0x1;
        finishedCommand();
        break;
    case 0xe2: // Texture Window Setting
        s.env.texture_win_mask_x = (word >> 0) & 0x1f;
        s.env.texture_win_mask_y = (word >> 5) & 0x1f;
        s.env.texture_win_offset_x = (word >> 10) & 0x1f;
        s.env.texture_win_offset_y = (word >> 15) & 0x1f;
        finishedCommand();
        break;
    case 0xe3: // Drawing area (Top Left)
    case 0xe4: // Drawing area (Bottom Right)
    {
        u8 corner = s.cmd - 0xe3;
        s.env.draw_area[corner].x = (word >> 0) & 0x3ff;
        // TODO: if new gpu, y is 10 bits, if old, y is 9 bits
        s.env.draw_area[corner].y = (word >> 10) & 0x3ff;
        // s.env.draw_area[corner].y = (word >> 10) & 0x1ff;
        finishedCommand();
        break;
    }
    case 0xe5: // Draw offset
        s.env.draw_offset_x = (word >> 0) & 0x7ff;
        s.env.draw_offset_y = (word >> 11) & 0x7ff;
        finishedCommand();
        break;
    case 0xe6: // Mask Bit Setting
        Util::SetBits(s.sr, 11, 1, word & 0x1);
        Util::SetBits(s.sr, 12, 1, (word >> 1) & 0x1);
        finishedCommand();
        break;
    default:
        GPU_FATAL("Unknown GP0 command: {:02x} from word [{:08x}]", s.cmd, word);
    }
}

// *** Private Functions ***
namespace {

inline void finishedCommand()
{
    s.cmd = GPU_CMD_NONE;
    s.gp0_state = Gp0State::Ready;
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

void handleMonoPoly(u32 word, const PolyConfig& config)
{
    static Geometry::Polygon s_poly;
    static Geometry::Color s_mono_col;

    switch (s.gp0_state) {
    case Gp0State::Ready:
        GPU_INFO("Starting Mono Polygon command");
        s_mono_col = Geometry::Color(word);
        s.gp0_state = Gp0State::Vertex;
        break;
    case Gp0State::Vertex:
        s_poly.vertices[s_poly.num_vertices++] = Geometry::Vertex(word, s_mono_col);
        // stay in vertex state
        break;
    default:
        GPU_FATAL("Unknown Polygon State: {}", (int)s.gp0_state);
    }
    if (s_poly.num_vertices == config.num_vertices) {
        // draw!
        Psx::View::DrawPolygon(s_poly);
        s_poly = {};
        finishedCommand();
        GPU_INFO("Finished Mono Polygon command");
    }
}

void handleShadedPoly(u32 word, const PolyConfig& config)
{
    static Geometry::Polygon s_poly;

    switch (s.gp0_state) {
    case Gp0State::Ready:
        GPU_INFO("Starting Shaded Polygon command");
        s.gp0_state = Gp0State::Vertex;
        // TODO first color
        break;
    case Gp0State::Vertex:
        s_poly.vertices[s_poly.num_vertices].x = (word >>  0) & 0xffff;
        s_poly.vertices[s_poly.num_vertices].y = (word >> 16) & 0xffff;
        s_poly.num_vertices++;
        s.gp0_state = Gp0State::Color;
        break;
    case Gp0State::Color:
        s_poly.vertices[s_poly.num_vertices].color = Geometry::Color(word);
        s.gp0_state = Gp0State::Vertex;
        break;
    default:
        GPU_FATAL("Unknown Polygon State: {}", (int)s.gp0_state);
    }
    if (s_poly.num_vertices == config.num_vertices) {
        // draw!
        Psx::View::DrawPolygon(s_poly);
        s_poly = {};
        finishedCommand();
        GPU_INFO("Finished Shaded Polygon command");
    }
}

void handleMonoTexturedPoly(u32 word, const PolyConfig& config)
{
    static Geometry::Polygon s_poly;
    static Geometry::Color s_mono_col;

    switch (s.gp0_state) {
    case Gp0State::Ready:
        GPU_INFO("Starting Mono Textured Polygon command");
        s_mono_col = Geometry::Color(word);
        s.gp0_state = Gp0State::Vertex;
        break;
    case Gp0State::Vertex:
        s_poly.vertices[s_poly.num_vertices] = Geometry::Vertex(word, s_mono_col);
        // stay in vertex state
        break;
    case Gp0State::Texture:
        // TODO
        s_poly.num_vertices++;
        break;
    default:
        GPU_FATAL("Unknown Polygon State: {}", (int)s.gp0_state);
    }
    if (s_poly.num_vertices == config.num_vertices) {
        // draw!
        Psx::View::DrawPolygon(s_poly);
        s_poly = {};
        finishedCommand();
        GPU_INFO("Finished Mono Textured Polygon command");
    } 
}

void handleShadedTexturedPoly(u32 word, const PolyConfig& config)
{
    static Geometry::Polygon s_poly;

    switch (s.gp0_state) {
    case Gp0State::Ready:
        GPU_INFO("Starting Shaded Textured Polygon command");
        s.gp0_state = Gp0State::Vertex;
        // TODO first color
        break;
    case Gp0State::Vertex:
        s_poly.vertices[s_poly.num_vertices].x = (word >>  0) & 0xffff;
        s_poly.vertices[s_poly.num_vertices].y = (word >> 16) & 0xffff;
        s.gp0_state = Gp0State::Color;
        break;
    case Gp0State::Color:
        s_poly.vertices[s_poly.num_vertices].color = Geometry::Color(word);
        s.gp0_state = Gp0State::Vertex;
        break;
    case Gp0State::Texture:
        // TODO
        s_poly.num_vertices++;
        break;
    default:
        GPU_FATAL("Unknown Polygon State: {}", (int)s.gp0_state);
    }
    if (s_poly.num_vertices == config.num_vertices) {
        // draw!
        Psx::View::DrawPolygon(s_poly);
        s_poly = {};
        finishedCommand();
        GPU_INFO("Finished Shaded Textured Polygon command");
    }
}

// Load Image Command State Machine
void copyRectangleCpuToVram(u32 word)
{
    // GP0(A0h) - Copy Rectangle (CPU to VRAM)
    //   1st  Command           (Cc000000h)
    //   2nd  Destination Coord (YyyyXxxxh)  ;Xpos counted in halfwords
    //   3rd  Width+Height      (YsizXsizh)  ;Xsiz counted in halfwords
    //   ...  Data              (...)      <--- usually transferred via DMA

    // just implementing something to consume commands
    static u32 s_data_left = 0;
    switch (s.gp0_state) {
    case Gp0State::Ready:
        GPU_WARN("Load Image (CPU TO VRAM) command not supported!");
        GPU_INFO("Load Image (CPU TO VRAM) START!");
        s.gp0_state = Gp0State::LoadImageCoord;
        break;
    case Gp0State::LoadImageCoord:
        s.gp0_state = Gp0State::LoadImageSize;
        break;
    case Gp0State::LoadImageSize:
    {
        u32 w = word & 0xffff;
        u32 h = (word >> 16) & 0xffff;
        u32 img_size = w * h;
        if (img_size % 2 == 1) {
            // if img size is odd, round up
            img_size++;
        }
        s_data_left = img_size / 2;
        GPU_INFO("Load Image (CPU TO VRAM) ready to transfer {} words!", s_data_left);
        s.gp0_state = Gp0State::LoadImageData;
        break;
    }
    case Gp0State::LoadImageData:
        s_data_left--;
            GPU_ERROR("Load Image (CPU TO VRAM) {} LEFT!", s_data_left);
        if (s_data_left == 0) {
            finishedCommand();
            GPU_INFO("Load Image (CPU TO VRAM) END!");
        } 
        break;
    default:
        GPU_FATAL("Invalid Image Load State: {}", (int)s.gp0_state);
    }
}

void copyRectangleVramToCpu(u32 word)
{
    // GP0(C0h) - Copy Rectangle (VRAM to CPU)
    //   1st  Command           (Cc000000h) ;
    //   2nd  Source Coord      (YyyyXxxxh) ; write to GP0 port (as usually)
    //   3rd  Width+Height      (YsizXsizh) ;
    //   ...  Data              (...)       ;<--- read from GPUREAD port (or via DMA)
    // TODO
    switch (s.gp0_state) {
    case Gp0State::Ready:
        GPU_WARN("Load Image (VRAM TO CPU) command not supported!");
        s.gp0_state = Gp0State::LoadImageCoord;
        break;
    case Gp0State::LoadImageCoord:
        // TODO
        s.gp0_state = Gp0State::LoadImageSize;
        break;
    case Gp0State::LoadImageSize:
        // TODO
        finishedCommand();
        break;
    default:
        GPU_FATAL("Invalid Image Load State: {}", (int)s.gp0_state);
    }
}

/*
 * Software reset.
 */
void softReset()
{
    GPU_INFO("Software Reset");
    // clear fifo and reset state machines
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
    DoGP0Cmd(0xe100'0000);
    DoGP0Cmd(0xe200'0000);
    DoGP0Cmd(0xe300'0000);
    DoGP0Cmd(0xe400'0000);
    DoGP0Cmd(0xe500'0000);
    DoGP0Cmd(0xe600'0000);
}

void resetCmdQueue()
{
    finishedCommand();
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

    // TODO force bit 20 to 0 to get around having to implement interlaced
    // support for now. NEED to fix this later!
    // Util::SetBits(s.sr, 19, 1, word >> 2);
    Util::SetBits(s.sr, 19, 1, 0);

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
} // end ns
}