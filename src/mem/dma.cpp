/*
 * dma.cpp
 *
 * Travis Banken
 * 1/23/2021
 *
 * Handles the DMA channels on the PSX.
 */

#include "dma.h"

#include "imgui/imgui.h"

#define DMA_INFO(...) PSXLOG_INFO("Dma", __VA_ARGS__)
#define DMA_WARN(...) PSXLOG_WARN("Dma", __VA_ARGS__)
#define DMA_ERROR(...) PSXLOG_ERROR("Dma", __VA_ARGS__)

namespace {

struct State {
    struct Registers {
        u32 dpcr = 0; // dma control reg
        u32 dicr = 0; // dma interrupt reg
        struct ChannelReg {
            u32 madr = 0; // dma base addr (R/W)
            u32 bcr  = 0; // dma block control (R/W)
            u32 chcr = 0; // channel control (R/W)

            std::string ToString()
            {
                u32 smadr = madr & 0x00ff'ffff;
                u32 sbcr = bcr & 0x0000'ffff;
                // chcr
                u32 trans_dir = (chcr >> 0) & 0x1;
                u32 mem_step = (chcr >> 1) & 0x1;
                u32 chop_enab = (chcr >> 8) & 0x1;
                u32 sync_mode = (chcr >> 9) & 0x3;
                u32 chop_dma_wins = (chcr >> 16) & 0x7;
                u32 chop_cpu_wins = (chcr >> 20) & 0x7;
                u32 start_busy = (chcr >> 24) & 0x1;
                u32 start_trigger = (chcr >> 28) & 0x1;

                return PSX_FMT("DMA Base Addr       : 0x{:08x}\n"
                               "DMA Block Control   : {}\n"
                               "--- Channel Control ---\n"
                               "Transfer Direction  : {}\n"
                               "Memory Addr Step    : {}\n"
                               "Chopping Enable     : {}\n"
                               "Sync Mode           : {}\n"
                               "Chopping DMA WinSize: {}\n"
                               "Chopping CPU WinSize: {}\n"
                               "Start/Busy          : {}\n"
                               "Start/Trigger       : {}\n"
                               ,smadr
                               ,sbcr
                               ,trans_dir
                               ,mem_step
                               ,chop_enab
                               ,sync_mode
                               ,chop_dma_wins
                               ,chop_cpu_wins
                               ,start_busy
                               ,start_trigger
                );
            }
        } channels[7];

        std::string ToString()
        {
            u32 dpcr_dma0_prio = (dpcr >> 0) & 0x7;
            u32 dpcr_dma0_enab = (dpcr >> 3) & 0x1;
            u32 dpcr_dma1_prio = (dpcr >> 4) & 0x7;
            u32 dpcr_dma1_enab = (dpcr >> 7) & 0x1;
            u32 dpcr_dma2_prio = (dpcr >> 8) & 0x7;
            u32 dpcr_dma2_enab = (dpcr >> 11) & 0x1;
            u32 dpcr_dma3_prio = (dpcr >> 12) & 0x7;
            u32 dpcr_dma3_enab = (dpcr >> 15) & 0x1;
            u32 dpcr_dma4_prio = (dpcr >> 16) & 0x7;
            u32 dpcr_dma4_enab = (dpcr >> 19) & 0x1;
            u32 dpcr_dma5_prio = (dpcr >> 20) & 0x7;
            u32 dpcr_dma5_enab = (dpcr >> 23) & 0x1;
            u32 dpcr_dma6_prio = (dpcr >> 24) & 0x7;
            u32 dpcr_dma6_enab = (dpcr >> 27) & 0x1;
            u32 dicr_force_irq = (dicr >> 15) & 0x1;
            u32 dicr_irq_enab  = (dicr >> 16) & 0x7f;
            u32 dicr_irq_master_enab  = (dicr >> 23) & 0x1;
            u32 dicr_irq_flags  = (dicr >> 24) & 0x7f;
            u32 dicr_irq_master_flags  = (dicr >> 31) & 0x1;

            return PSX_FMT("--- DMA Control Register ---\n"
                           "DMA0 Priority     : {}\n"
                           "DMA0 Master Enable: {}\n"
                           "DMA1 Priority     : {}\n"
                           "DMA1 Master Enable: {}\n"
                           "DMA2 Priority     : {}\n"
                           "DMA2 Master Enable: {}\n"
                           "DMA3 Priority     : {}\n"
                           "DMA3 Master Enable: {}\n"
                           "DMA4 Priority     : {}\n"
                           "DMA4 Master Enable: {}\n"
                           "DMA5 Priority     : {}\n"
                           "DMA5 Master Enable: {}\n"
                           "DMA6 Priority     : {}\n"
                           "DMA6 Master Enable: {}\n"
                           "--- DMA Interrupt Register ---\n"
                           "Force IRQ        : {}\n"
                           "IRQ Enable DMA0-6: 0b{:07b}\n"
                           "Master Enable    : {}\n"
                           "IRQ Flags DMA0-6 : 0b{:07b}\n"
                           "Master Flag      : {}\n"
                           ,dpcr_dma0_prio
                           ,dpcr_dma0_enab
                           ,dpcr_dma1_prio
                           ,dpcr_dma1_enab
                           ,dpcr_dma2_prio
                           ,dpcr_dma2_enab
                           ,dpcr_dma3_prio
                           ,dpcr_dma3_enab
                           ,dpcr_dma4_prio
                           ,dpcr_dma4_enab
                           ,dpcr_dma5_prio
                           ,dpcr_dma5_enab
                           ,dpcr_dma6_prio
                           ,dpcr_dma6_enab
                           ,dicr_force_irq
                           ,dicr_irq_enab
                           ,dicr_irq_master_enab
                           ,dicr_irq_flags
                           ,dicr_irq_master_flags
            );
        }
    } regs;
} s;

// Protos
u32* getRegRef(u32 addr);

}// end ns


namespace Psx {
namespace Dma {

void Init()
{
    DMA_INFO("Initializing state");
    s.regs = {};
}

void Reset()
{
    DMA_INFO("Resetting state");
    s.regs = {};
}

// *** Read ***
template<class T>
T Read(u32 addr)
{
    T data = 0;
    u32 *reg_ptr = getRegRef(addr);
    if constexpr (std::is_same_v<T, u8>) {
        // read8 (assuming little endian)
        PSX_ASSERT(0);
    } else if constexpr (std::is_same_v<T, u16>) {
        // read16 (assuming little endian)
        PSX_ASSERT(0);
    } else if constexpr (std::is_same_v<T, u32>) {
        // read32
        data = *reg_ptr;
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
    u32 *reg_ptr = getRegRef(addr);
    if constexpr (std::is_same_v<T, u8>) {
        // write8 (assuming little endian)
        PSX_ASSERT(0);
    } else if constexpr (std::is_same_v<T, u16>) {
        // write16 (assuming little endian)
        PSX_ASSERT(0);
    } else if constexpr (std::is_same_v<T, u32>) {
        // write32
        // special case for Interrupt Register
        if (addr == 0x1f80'10f4) {
            // bit 31 is read only
            s.regs.dicr = data & 0x7fff'ffff;
            // set bit 31
            s.regs.dicr &= ~0x8000'0000;
            bool bit15 = (s.regs.dicr >> 15) & 0x1;
            bool bit23 = (s.regs.dicr >> 23) & 0x1;
            bool bit16_22 = (s.regs.dicr >> 16) & 0x7f;
            bool bit24_30 = (s.regs.dicr >> 24) & 0x7f;
            s.regs.dicr |= (bit15 || (bit23 && bit16_22 && bit24_30)) ? 0x8000'0000 : 0;
        } else {
            *reg_ptr = data;
        }
    } else {
        static_assert(!std::is_same_v<T, T>);
    }
}
// template impl needs to be visable to other cpp files to avoid compile err
template void Write<u8>(u8 data, u32 addr);
template void Write<u16>(u16 data, u32 addr);
template void Write<u32>(u32 data, u32 addr);

void OnActive(bool *active)
{
    if (!ImGui::Begin("DMA Debug", active)) {
        ImGui::End();
        return;
    }

    //-----------------------
    // Registers Raw
    //-----------------------
    ImGui::BeginGroup();
    ImGui::TextUnformatted("Registers");
    ImGui::Separator();
    ImGui::TextUnformatted(PSX_FMT("{:<8} = 0x{:08x}", "DPCR", s.regs.dpcr).c_str());
    ImGui::TextUnformatted(PSX_FMT("{:<8} = 0x{:08x}", "DICR", s.regs.dicr).c_str());
    for (int i = 0; i < 7; i++) {
        ImGui::TextUnformatted(PSX_FMT("{:<8} = 0x{:08x}", PSX_FMT("D{}_MADR", i), s.regs.channels[i].madr).c_str());
        ImGui::TextUnformatted(PSX_FMT("{:<8} = 0x{:08x}", PSX_FMT("D{}_BCR", i), s.regs.channels[i].bcr).c_str());
        ImGui::TextUnformatted(PSX_FMT("{:<8} = 0x{:08x}", PSX_FMT("D{}_CHCR", i), s.regs.channels[i].chcr).c_str());
    }
    ImGui::EndGroup();

    ImGui::SameLine();

    //-----------------------
    // Registers Formated
    //-----------------------
    ImGui::BeginGroup();
    ImGui::TextUnformatted("Formated Registers");
    ImGui::Separator();
    ImGui::BeginGroup();
    ImGui::TextUnformatted(s.regs.ToString().c_str());
    ImGui::EndGroup();
    ImGui::SameLine();
    // channels
    ImGui::BeginGroup();
    ImGui::TextUnformatted("DMA0 -- MDECin");
    ImGui::TextUnformatted(s.regs.channels[0].ToString().c_str());
    ImGui::Separator();
    ImGui::TextUnformatted("DMA1 -- MDECout");
    ImGui::TextUnformatted(s.regs.channels[1].ToString().c_str());
    ImGui::EndGroup();
    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::TextUnformatted("DMA2 -- GPU");
    ImGui::TextUnformatted(s.regs.channels[2].ToString().c_str());
    ImGui::Separator();
    ImGui::TextUnformatted("DMA3 -- CDROM");
    ImGui::TextUnformatted(s.regs.channels[3].ToString().c_str());
    ImGui::EndGroup();
    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::TextUnformatted("DMA4 -- SPU");
    ImGui::TextUnformatted(s.regs.channels[4].ToString().c_str());
    ImGui::Separator();
    ImGui::TextUnformatted("DMA5 -- PIO");
    ImGui::TextUnformatted(s.regs.channels[5].ToString().c_str());
    ImGui::EndGroup();
    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::TextUnformatted("DMA6 -- OTC");
    ImGui::TextUnformatted(s.regs.channels[6].ToString().c_str());
    ImGui::EndGroup();
    ImGui::EndGroup();

    ImGui::End();
}


}// end ns
}

namespace {

/*
 * Returns a pointer to the register which corresponds to the given address.
 */
u32* getRegRef(u32 addr)
{
    bool is_ctrl = addr == 0x1f80'10f0;
    bool is_intreg = addr == 0x1f80'10f4;
    u32 chnum = ((addr >> 4) & 0xf) - 0x8;
    bool is_madr = (addr & 0xf) == 0;
    bool is_bcr  = (addr & 0xf) == 4;
    bool is_chcr = (addr & 0xf) == 8;

    u32 *reg_ptr = nullptr;
    if (is_ctrl) {
        reg_ptr = &s.regs.dpcr;
    } else if (is_intreg) {
        reg_ptr = &s.regs.dicr;
    } else {
        if (is_madr) {
            reg_ptr = &s.regs.channels[chnum].madr;
        } else if (is_bcr) {
            reg_ptr = &s.regs.channels[chnum].bcr;
        } else if (is_chcr) {
            reg_ptr = &s.regs.channels[chnum].madr;
        }
    }

    PSX_ASSERT(reg_ptr != nullptr && PSX_FMT("Invalid Address [0x{:08x}]", addr).c_str());
    return reg_ptr;
}

}// end ns
