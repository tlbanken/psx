/*
 * interrupt.cc
 *
 * Travis Banken
 * 5/8/2022
 * 
 * Interrupts middle man for the PSX.
 */

#include "interrupt.hh"

#include "util/psxutil.hh"
#include "cpu/cop0.hh"
#include "imgui/imgui.h"

#define INTERRUPT_INFO(...) PSXLOG_INFO("INTERRUPT", __VA_ARGS__)
#define INTERRUPT_WARN(...) PSXLOG_WARN("INTERRUPT", __VA_ARGS__)
#define INTERRUPT_ERROR(...) PSXLOG_ERROR("INTERRUPT", __VA_ARGS__)
#define INTERRUPT_FATAL(...) INTERRUPT_ERROR(__VA_ARGS__); throw std::runtime_error(PSX_FMT(__VA_ARGS__))

#define I_STAT_ADDR 0x1F80'1070
#define I_MASK_ADDR 0x1F80'1074

namespace Psx {
namespace Interrupt {

// Private state
namespace {

struct State {
    u32 i_stat = 0;
    u32 i_mask = 0;
} s;

// protos
void PrettyIReg(u16 reg);
} // end private ns

void Init()
{
    INTERRUPT_INFO("Intializing Interrupts");
    s = {};
}

void Reset()
{
    INTERRUPT_INFO("Resetting Interrupts");
    s = {};
}

void Step()
{
    if ((s.i_stat & s.i_mask) != 0) {
        // raise cop0 int exception
        Cop0::Exception e = {.type = Cop0::Exception::Type::Interrupt};
        Cop0::RaiseException(e);
        // TODO how to reset to avoid spam??
    }
}

void Signal(Type itype)
{
    s.i_stat |= (u32)itype;
}

// *** Read ***
template<class T>
T Read(u32 addr)
{
    T data;
    switch (addr) {
    case I_STAT_ADDR:
        data = (T) s.i_stat;
        break;
    case I_MASK_ADDR:
        data = (T) s.i_mask;
        break;
    default:
        INTERRUPT_FATAL("Address [{:08x}] not part of interrupt space!", addr);
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
    switch (addr) {
    case I_STAT_ADDR:
        s.i_stat &= (u32) data;
        break;
    case I_MASK_ADDR:
        s.i_mask = (u32) data;
        break;
    default:
        INTERRUPT_FATAL("Address [{:08x}] not part of interrupt space!", addr);
    }
}
// template impl needs to be visable to other cpp files to avoid compile err
template void Write<u8>(u8 data, u32 addr);
template void Write<u16>(u16 data, u32 addr);
template void Write<u32>(u32 data, u32 addr);

void OnActive(bool *active)
{
    if (!ImGui::Begin("Interrupts Debug", active)) {
        ImGui::End();
        return;
    }

    ImGui::TextUnformatted("Raw Registers");
    ImGui::Separator();
    ImGui::BeginGroup();
        ImGui::TextUnformatted(PSX_FMT("I_STAT: 0x{:08x}", s.i_stat).c_str());
        ImGui::SameLine();
        ImGui::TextUnformatted(PSX_FMT("I_MASK: 0x{:08x}", s.i_mask).c_str());
    ImGui::EndGroup();
    ImGui::Separator();
    ImGui::TextUnformatted("Pretty Registers");
    ImGui::Separator();
    ImGui::BeginGroup();
        ImGui::BeginGroup();
            ImGui::TextUnformatted("I_STAT");
            PrettyIReg(s.i_stat);
        ImGui::EndGroup();
        ImGui::SameLine();
        ImGui::BeginGroup();
            ImGui::TextUnformatted("I_MASK");
            PrettyIReg(s.i_mask);
        ImGui::EndGroup();
    ImGui::EndGroup();

    ImGui::End();
}

namespace {
void PrettyIReg(u16 reg)
{
    auto pbool = [&](Type t) {
        return reg & t ? "SET" : "CLEAR";
    };
    ImGui::TextUnformatted(PSX_FMT("Vblank    : {}", pbool(Type::Vblank)).c_str());
    ImGui::TextUnformatted(PSX_FMT("Gpu       : {}", pbool(Type::Gpu)).c_str());
    ImGui::TextUnformatted(PSX_FMT("CdRom     : {}", pbool(Type::CdRom)).c_str());
    ImGui::TextUnformatted(PSX_FMT("Dma       : {}", pbool(Type::Dma)).c_str());
    ImGui::TextUnformatted(PSX_FMT("Timer0    : {}", pbool(Type::Timer0)).c_str());
    ImGui::TextUnformatted(PSX_FMT("Timer1    : {}", pbool(Type::Timer1)).c_str());
    ImGui::TextUnformatted(PSX_FMT("Timer2    : {}", pbool(Type::Timer2)).c_str());
    ImGui::TextUnformatted(PSX_FMT("Controller: {}", pbool(Type::Controller)).c_str());
    ImGui::TextUnformatted(PSX_FMT("MemCard   : {}", pbool(Type::MemCard)).c_str());
    ImGui::TextUnformatted(PSX_FMT("Sio       : {}", pbool(Type::Sio)).c_str());
    ImGui::TextUnformatted(PSX_FMT("Spu       : {}", pbool(Type::Spu)).c_str());
    ImGui::TextUnformatted(PSX_FMT("Lightpen  : {}", pbool(Type::Lightpen)).c_str());
}

} // end private ns
} // end ns
}