/*
 * dma.cpp
 *
 * Travis Banken
 * 1/23/2021
 *
 * Handles the DMA channels on the PSX.
 */

#include "dma.hh"

#include <vector>
#include <queue>

#include "imgui/imgui.h"

#include "mem/ram.hh"
#include "gpu/gpu.hh"

#define DMA_INFO(...) PSXLOG_INFO("Dma", __VA_ARGS__)
#define DMA_WARN(...) PSXLOG_WARN("Dma", __VA_ARGS__)
#define DMA_ERROR(...) PSXLOG_ERROR("Dma", __VA_ARGS__)
#define DMA_FATAL(...) DMA_ERROR(__VA_ARGS__); throw std::runtime_error(PSX_FMT(__VA_ARGS__))

namespace {

struct State {
    struct Registers {
        u32 dpcr = 0x07654321; // dma control reg
        u32 dicr = 0; // dma interrupt reg
        struct ChannelReg {
            u32 madr = 0; // dma base addr (R/W)
            u32 bcr  = 0; // dma block control (R/W)
            u32 chcr = 0; // channel control (R/W)

            std::string ToString()
            {
                u32 smadr = madr & 0x00ff'ffff;
                u32 num_blocks = bcr & 0x0000'ffff;
                u32 block_size = (bcr >> 16) & 0x0000'ffff;
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
                               "Number of Blocks    : {}\n"
                               "Block size          : {}\n"
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
                               ,num_blocks
                               ,block_size
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

    // dma queue
    std::queue<u8> dma_queue;
} s;

enum class Channel {
    Ch0 = 0,
    Ch1 = 1,
    Ch2 = 2,
    Ch3 = 3,
    Ch4 = 4,
    Ch5 = 5,
    Ch6 = 6,
};

// Protos
u32* getRegRef(u32 addr);
void doDma(uint channel);
bool dmaReady(u32 channel);
// template<class channel> void doBlockDma(uint chnum);

}// end ns


namespace Psx {
namespace Dma {

void Init()
{
    DMA_INFO("Initializing state");
    Reset();
}

void Reset()
{
    DMA_INFO("Resetting state");
    s.regs = {};
}

void Step()
{

    // queue system
    if (s.dma_queue.size() > 0) {
        u8 channel = s.dma_queue.front();

        // if master bit not enabled, wait until it is
        if (!Util::GetBits(s.regs.dpcr, 3u + ((uint)channel << 2), 1)) {
            return;
        }

        DMA_INFO("[CH{}] Starting DMA", channel);
        if (Util::GetBits(s.regs.channels[channel].chcr, 8, 1)) {
            DMA_ERROR("No support for DMA Chopping mode");
            PSX_ASSERT(0);
        }

        // clear start/trigger bit
        Util::SetBits(s.regs.channels[channel].chcr, 28, 1, 0);
        doDma(channel);
        // clear start/busy bit
        Util::SetBits(s.regs.channels[channel].chcr, 24, 1, 0);

        // TODO: Do we clear after dma is complete?
        Util::SetBits(s.regs.dpcr, 3u + ((uint)channel << 2), 1, 0);

        // TODO: do stuff with interrupt bits
        DMA_INFO("[CH{}] Finished DMA", channel);

        s.dma_queue.pop();
    }

    // TODO handle priority
    // TODO: maybe switch to queue system to improve performance
    // for (u32 channel = 0; channel < 7; channel++) {
    //     if (dmaReady(channel)) {
    //         DMA_INFO("[CH{}] Starting DMA", channel);
    //         if (Util::GetBits(s.regs.channels[channel].chcr, 8, 1)) {
    //             DMA_ERROR("No support for DMA Chopping mode");
    //             PSX_ASSERT(0);
    //         }

    //         // clear start/trigger bit
    //         Util::SetBits(s.regs.channels[channel].chcr, 28, 1, 0);
    //         doDma(channel);
    //         // clear start/busy bit
    //         Util::SetBits(s.regs.channels[channel].chcr, 24, 1, 0);

    //         // TODO: Do we clear after dma is complete?
    //         Util::SetBits(s.regs.dpcr, 3u + (channel << 2), 1, 0);

    //         // TODO: do stuff with interrupt bits
    //         DMA_INFO("[CH{}] Finished DMA", channel);
    //         break; // only do at most 1 dma request
    //     }
    // }
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
            // if 
            if (addr != 0x1f80'10f0 && addr != 0x1f80'10f4) {
                u32 chnum = ((addr >> 4) & 0xf) - 0x8;
                if (dmaReady(chnum)) {
                    s.dma_queue.push(chnum);
                }
            }
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

using namespace Psx;

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
        PSX_ASSERT(chnum < 7);
        if (is_madr) {
            reg_ptr = &s.regs.channels[chnum].madr;
        } else if (is_bcr) {
            reg_ptr = &s.regs.channels[chnum].bcr;
        } else if (is_chcr) {
            reg_ptr = &s.regs.channels[chnum].chcr;
        }
    }

    PSX_ASSERT(reg_ptr != nullptr);
    return reg_ptr;
}

// TODO: we need to handle sync-mode 1 (request mode)
template<Channel channel>
void doBlockDma()
{
    constexpr uint chnum = static_cast<uint>(channel);
    bool dir_from_ram = s.regs.channels[chnum].chcr & 0x1;
    bool increment = !Util::GetBits(s.regs.channels[chnum].chcr, 1, 1);
    u32 num_words = s.regs.channels[chnum].bcr & 0xffff;
    if (num_words == 0) num_words = 0x1'0000;
    u32 words_remaining = num_words;
    u32 base_addr = s.regs.channels[chnum].madr & 0x00ff'ffff;

    if constexpr (channel == Channel::Ch0) {
        PSX_ASSERT(0);
    } else if constexpr (channel == Channel::Ch1) {
        PSX_ASSERT(0);
    } else if constexpr (channel == Channel::Ch2) {
        // TODO
        PSX_ASSERT(dir_from_ram);
        u32 addr = base_addr;
        while (words_remaining > 0) {
            // TODO: where word?
            u32 word = Ram::Read<u32>(addr);
            Gpu::DoGP0Cmd(word);
            if (increment) {
                addr += 4;
            } else {
                addr -= 4;
            }
            words_remaining--;
        }
    } else if constexpr (channel == Channel::Ch3) {
        PSX_ASSERT(0);
    } else if constexpr (channel == Channel::Ch4) {
        PSX_ASSERT(0);
    } else if constexpr (channel == Channel::Ch5) {
        PSX_ASSERT(0);
    } else if constexpr (channel == Channel::Ch6) {
        // OTC (ordering table)
        if (dir_from_ram) {
            PSX_ASSERT(0);
        } else {
            u32 addr = base_addr;
            DMA_INFO("Transfering {} words to RAM @ 0x{:08x}", words_remaining, addr);
            // direction: to ram
            while (words_remaining > 0) {
                u32 cur_addr = addr & 0x1f'fffc; // size of ram (aligned)
                // for OTC, step is always -4
                addr -= 4;

                u32 val = words_remaining == 1 ? 0x00ff'ffff : addr & 0x1f'ffff;
                Ram::Write<u32>(val, cur_addr);
                words_remaining -= 1;
            }
        }
    } 
}

/*
 * Perform DMA operation on the given channel.
 */
void doDma(uint channel)
{
    switch (channel) {
    case 0: // mdec in
        PSX_ASSERT(0);
        break;
    case 1: // mdec out
        PSX_ASSERT(0);
        break;
    case 2: // gpu
    {
        u32 sync_mode = Util::GetBits(s.regs.channels[2].chcr, 9, 2);

        if (sync_mode == 0 || sync_mode == 1) {
            DMA_INFO("DMA MODE BLOCK");
            doBlockDma<Channel::Ch2>();
        } else {
            DMA_INFO("DMA MODE LL");
            PSX_ASSERT(sync_mode == 2);
            bool dir_from_ram = s.regs.channels[2].chcr & 0x1;
            u32 base_addr = s.regs.channels[2].madr & 0x00ff'ffff;
            if (dir_from_ram) {
                Gpu::DoDmaCmds(base_addr);
            } else {
                PSX_ASSERT(0);
            }
        }

        break;
    }
    case 3: // cdrom
        PSX_ASSERT(0);
        break;
    case 4: // spu
        DMA_ERROR("!!! No support for CH4 - SPU !!!");
        break;
    case 5: // pio
        PSX_ASSERT(0);
        break;
    case 6: // otc (ordering table in ram)
        // sync-mode should be 0
        PSX_ASSERT(Util::GetBits(s.regs.channels[6].chcr, 9, 2) == 0);
        doBlockDma<Channel::Ch6>();
        break;
    default:
        DMA_FATAL("Unknown DMA Channel: {}");
    }
}

// return 1 if channel is ready for transfer, 0 otherwise
bool dmaReady(u32 channel)
{
    // a dma transfer will occur if the following conditions are met:
    //  1. Master enable must be set (dpcr)
    //  2. Start Bit must be set (chcr.24)
    //  3. If syncmode == 0; then trigger bit must be set (chcr.28)
    // if (!Util::GetBits(s.regs.dpcr, 3u + (channel << 2), 1)) {
    //     return false;
    // }

    if (!Util::GetBits(s.regs.channels[channel].chcr, 24, 1)) {
        return false;
    }

    if (!Util::GetBits(s.regs.channels[channel].chcr, 9, 2)
        && !Util::GetBits(s.regs.channels[channel].chcr, 28, 1)) {
        return false;
    }

    return true;
}

}// end ns
