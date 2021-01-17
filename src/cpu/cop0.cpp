/*
 * cop0.cpp
 *
 * Travis Banken
 * 12/20/2020
 *
 * Coprocessor 0 - System Control Coprocessor.
 * Exception/Interrupt handling.
 */

#include "cpu/cop0.h"

#include <string>
#include <map>

#include "util/psxutil.h"
#include "cpu/cpu.h"
#include "imgui/imgui.h"

#define COP0_INFO(...) PSXLOG_INFO("Cop0", __VA_ARGS__);
#define COP0_WARN(...) PSXLOG_WARN("Cop0", __VA_ARGS__);
#define COP0_ERROR(...) PSXLOG_ERROR("Cop0", __VA_ARGS__);

namespace {
using namespace Psx::Cop0;

const std::map<Psx::Cop0::Exception::Type, std::string> extype_to_str = {
    {Exception::Type::Interrupt, "INT"},    {Exception::Type::AddrErrLoad, "AEL"},
    {Exception::Type::AddrErrStore, "AES"}, {Exception::Type::IBusErr, "IBE"},
    {Exception::Type::DBusErr, "DBE"},      {Exception::Type::Syscall, "SYS"},
    {Exception::Type::Break, "BRK"},        {Exception::Type::ReservedInstr, "RI"},
    {Exception::Type::CopUnusable, "COP"},  {Exception::Type::Overflow, "OVF"},
};

struct State {
    struct Registers {
        u32 bpc = 0;     // Breakpoint on Exception (R/W)
        u32 bda = 0;     // Breakpoint on Data Access (R/W)
        u32 jmpdest = 0; // rand jump address (unknown use) (R)
        u32 dcic = 0;    // Breakpoint control (R/W)
        u32 badv = 0;    // Bad Virtual Address (R)
        u32 bdam = 0;    // Data Access Breakpoint mask (R/W)
        u32 bpcm = 0;    // Execute Breakpoint mask (R/W)
        u32 epc  = 0;    // Return Address from Trap (R)
        struct Status {  // status register (R/W)
            u32 raw = 0;
            // TODO add field getters/setters if needed
            std::string ToString()
            {
                u32 cur_int_enable  = (raw >> 0) & 0x1;
                u32 cur_ku_mode     = (raw >> 1) & 0x1;
                u32 prev_int_enable = (raw >> 2) & 0x1;
                u32 prev_ku_mode    = (raw >> 3) & 0x1;
                u32 old_int_enable  = (raw >> 4) & 0x1;
                u32 old_ku_mode     = (raw >> 5) & 0x1;
                u32 int_mask        = (raw >> 8) & 0xff;
                u32 iso_cache       = (raw >> 16) & 0x1;
                u32 swap_cache      = (raw >> 17) & 0x1;
                u32 cache_parz      = (raw >> 18) & 0x1;
                u32 last_load       = (raw >> 19) & 0x1;
                u32 cache_par_err   = (raw >> 20) & 0x1;
                u32 tlb_shutdown    = (raw >> 21) & 0x1;
                u32 bev             = (raw >> 22) & 0x1;
                u32 rev_end         = (raw >> 25) & 0x1;
                u32 cop0_enable     = (raw >> 28) & 0x1;
                u32 cop1_enable     = (raw >> 29) & 0x1;
                u32 cop2_enable     = (raw >> 30) & 0x1;
                u32 cop3_enable     = (raw >> 31) & 0x1;
                return PSX_FMT("Curr Int Enable      : {}\n"
                               "Curr K/U Mode        : {}\n"
                               "Prev Int Enable      : {}\n"
                               "Prev K/U Mode        : {}\n"
                               "Old  Int Enable      : {}\n"
                               "Old  K/U Mode        : {}\n"
                               "Interrupt Mask       : 0b{:08b}\n"
                               "Isolate Cache        : {}\n"
                               "Swapped Cache        : {}\n"
                               "Cache Parity zero    : {}\n"
                               "Last Load (CM)       : {}\n"
                               "Cache Parity Err     : {}\n"
                               "TLB Shutdown         : {}\n"
                               "Boot Ex Vectors (BEV): {}\n"
                               "Reverse Endianess    : {}\n"
                               "COP0 Enab for U mode : {}\n"
                               "COP1 Enable (N/A)    : {}\n"
                               "COP2 Enable (GTE)    : {}\n"
                               "COP3 Enable (N/A)    : {}\n"
                               ,cur_int_enable
                               ,cur_ku_mode
                               ,prev_int_enable
                               ,prev_ku_mode
                               ,old_int_enable
                               ,old_ku_mode
                               ,int_mask
                               ,iso_cache
                               ,swap_cache
                               ,cache_parz
                               ,last_load
                               ,cache_par_err
                               ,tlb_shutdown
                               ,bev
                               ,rev_end
                               ,cop0_enable
                               ,cop1_enable
                               ,cop2_enable
                               ,cop3_enable
                );
            }
        } sr;
        struct Cause {   // Cause Register (R (bits 8-9 W))
            u32 raw = 0;
            // field getters/setters
            void SetExType(Exception::Type t)
            {
                raw &= ~(0x1f << 2);
                raw |= static_cast<u32>(t) << 2;
            }
            Exception::Type GetExType()
            {
                return static_cast<Exception::Type>((raw >> 2) & 0x1f);
            }

            void SetSWIntPending(u32 val)
            {
                raw &= ~(0x3 << 8);
                raw |= (val & 0x3) << 8;
            }

            void SetCopNum(u32 cop_num)
            {
                raw &= ~(0x3 << 28);
                raw |= ((cop_num & 0x3) << 28);
            }

            void SetBD(bool bd)
            {
                raw &= ~(1 << 31);
                raw |= static_cast<u32>(bd) << 31;
            }
            bool GetBD()
            {
                return (raw & 0x8000'0000) != 0;
            }

            std::string ToString()
            {
                u32 int_pending = (raw >> 8) & 0xff;
                u32 cop_num = (raw >> 28) & 0x3;
                u32 bd = (raw >> 31) & 0x1;
                return PSX_FMT("Type        : {}\n"
                               "Int Pending : 0b{:08b}\n"
                               "COP Num     : {}\n"
                               "Branch Delay: {}\n"
                               ,extype_to_str.at(this->GetExType())
                               ,int_pending
                               ,cop_num
                               ,bd
                );
            }
        } cause;
        struct ProcID {  // Processor ID
            u32 raw = 0;
        } prid;
    } regs;
} s;


/*
 * Return, in string form, the last exception occured.
 */
std::string FmtLastException()
{
    return PSX_FMT("EX{{TYPE: {}, BADV: {:08x}, BD: {}, EPC: {:08x}}}",
            extype_to_str.at(s.regs.cause.GetExType()), s.regs.badv, s.regs.cause.GetBD(), s.regs.epc);
}
}// end namespace


// *** Public Functions ***
namespace Psx {
namespace Cop0 {

void Init()
{
    COP0_INFO("Initializing state");
}

/*
 * Reset the state of the coprocessor.
 */
void Reset()
{
    COP0_INFO("Resetting state");
    s.regs = {};
}

/*
 * Raises the given exception. A handler will then be called and the
 * System Control Cop will update it's status registers.
 */
void RaiseException(const Exception& ex)
{
    // update cause register
    s.regs.cause.SetExType(ex.type);
    s.regs.cause.SetBD(Cpu::InBranchDelaySlot());
    s.regs.cause.SetCopNum(ex.cop_num);

    // update badv
    s.regs.badv = ex.badv;

    // update epc
    s.regs.epc = Cpu::GetPC() + 4; // pc has already incremented
    if (s.regs.cause.GetExType() != Exception::Type::Interrupt && s.regs.cause.GetBD()) {
        COP0_WARN("Branch Delay Exception! Did anything break??");
        s.regs.epc -= 4;
    }

    COP0_WARN("Exception Handler not fully implemented!");
    COP0_INFO("{}", FmtLastException());
}

/*
 * Execute the given COP0 Command. Since the PSX does not have a TLB, only
 * one command is valid. This command is "Return from Exception" (RFE).
 */
void ExeCmd(u32 command)
{
    // only support 1 command: RFE (Return from Exception)
    if (command == 0x10) {
        // copy bits 2,3 into 0,1. Copy bits 4,5 into 2,3
        u32 bits_2_3 = (s.regs.sr.raw >> 2) & 0x3;
        u32 bits_4_5 = (s.regs.sr.raw >> 4) & 0x3;
        s.regs.sr.raw &= ~(0xf);
        s.regs.sr.raw |= bits_2_3 | (bits_4_5 << 2);
    } else {
        COP0_WARN("Command not supported: 0x{:08x}", command);
        // raise exception
        Exception e;
        e.type = Exception::Type::ReservedInstr;
        RaiseException(e);
    }
}

/*
 * Returns the value from the specified register.
 */
u32 Mf(u8 reg)
{
    u32 data = 0;
    switch (reg) {
    case  3: data = s.regs.bpc; break;
    case  5: data = s.regs.bda; break;
    case  6: data = s.regs.jmpdest; break;
    case  7: data = s.regs.dcic; break;
    case  8: data = s.regs.badv; break;
    case  9: data = s.regs.bdam; break;
    case 11: data = s.regs.bpcm; break;
    case 12: data = s.regs.sr.raw; break;
    case 13: data = s.regs.cause.raw; break;
    case 14: data = s.regs.epc; break;
    case 15: data = s.regs.prid.raw; break;
    // garbage registers
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
    case 26:
    case 27:
    case 28:
    case 29:
    case 30:
    case 31:
        COP0_WARN("MF trying to read from Garbage register [{}]", reg);
        data = s.regs.cause.raw ^ ~s.regs.epc ^ s.regs.sr.raw; // garbage (hopefully doesn't matter)
        break;
    default:
        // reserved instruction exception
        COP0_WARN("Invalid MF register access (R) [{}]", reg);
        Exception e;
        e.type = Exception::Type::ReservedInstr;
        RaiseException(e);
        break;
    }
    return data;
}

/*
 * Returns the value from the specified register.
 */
void Mt(u32 data, u8 reg)
{
    switch (reg) {
    case  3: s.regs.bpc       = data; break;
    case  5: s.regs.bda       = data; break;
    case  6: /*JMPDST Read Only*/     break;
    case  7: s.regs.dcic      = data; break;
    case  8: /*BADV Read Only*/       break;
    case  9: s.regs.bdam      = data; break;
    case 11: s.regs.bpcm      = data; break;
    case 12: s.regs.sr.raw    = data; break;
    case 13: s.regs.cause.SetSWIntPending((data >> 8) & 0x3); break; // only bits 8-9 are writable
    case 14: /*EPC Read Only*/        break;
    case 15: /*PRID Read Only*/       break;
    // garbage registers
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
    case 26:
    case 27:
    case 28:
    case 29:
    case 30:
    case 31:
        COP0_WARN("MT trying to write to Garbage register [{}]", reg);
        break;
    default:
        // reserved instruction exception
        COP0_WARN("Invalid MT register access (W) [{}]", reg);
        Exception e;
        e.type = Exception::Type::ReservedInstr;
        RaiseException(e);
        break;
    }
}

void OnActive(bool *active)
{
    if (!ImGui::Begin("COP0 Debug", active)) {
        ImGui::End();
        return;
    }

    //-----------------------
    // Registers Raw
    //-----------------------
    ImGui::BeginGroup();
    ImGui::TextUnformatted("Registers");
    ImGui::Separator();
    ImGui::TextUnformatted(PSX_FMT("R03: {:<8} = 0x{:08x}", "BPC", s.regs.bpc).c_str());
    ImGui::TextUnformatted(PSX_FMT("R05: {:<8} = 0x{:08x}", "BDA", s.regs.bda).c_str());
    ImGui::TextUnformatted(PSX_FMT("R06: {:<8} = 0x{:08x}", "JMPDST", s.regs.jmpdest).c_str());
    ImGui::TextUnformatted(PSX_FMT("R07: {:<8} = 0x{:08x}", "DCIC", s.regs.dcic).c_str());
    ImGui::TextUnformatted(PSX_FMT("R08: {:<8} = 0x{:08x}", "BADV", s.regs.badv).c_str());
    ImGui::TextUnformatted(PSX_FMT("R09: {:<8} = 0x{:08x}", "BDAM", s.regs.bdam).c_str());
    ImGui::TextUnformatted(PSX_FMT("R11: {:<8} = 0x{:08x}", "BPCM", s.regs.bpcm).c_str());
    ImGui::TextUnformatted(PSX_FMT("R12: {:<8} = 0x{:08x}", "STATUS", s.regs.sr.raw).c_str());
    ImGui::TextUnformatted(PSX_FMT("R13: {:<8} = 0x{:08x}", "CAUSE", s.regs.cause.raw).c_str());
    ImGui::TextUnformatted(PSX_FMT("R14: {:<8} = 0x{:08x}", "EPC", s.regs.epc).c_str());
    ImGui::TextUnformatted(PSX_FMT("R15: {:<8} = 0x{:08x}", "PRID", s.regs.prid.raw).c_str());
    ImGui::EndGroup();

    ImGui::SameLine();

    //-----------------------
    // Registers Formated
    //-----------------------
    ImGui::BeginGroup();
    ImGui::TextUnformatted("Formated Registers");
    ImGui::Separator();
    ImGui::BeginGroup(); // status
    ImGui::TextUnformatted("Status Register");
    ImGui::Separator();
    ImGui::TextUnformatted(s.regs.sr.ToString().c_str());
    ImGui::EndGroup(); // status
    ImGui::SameLine();
    ImGui::BeginGroup(); // cause
    ImGui::TextUnformatted("Cause Register");
    ImGui::Separator();
    ImGui::TextUnformatted(s.regs.cause.ToString().c_str());
    ImGui::EndGroup(); // cause
    ImGui::Separator();
    ImGui::EndGroup();

    ImGui::End();
}


}// end namespace
}
