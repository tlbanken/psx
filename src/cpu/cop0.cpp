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

#define COP0_INFO(...) PSXLOG_INFO("Cop0", __VA_ARGS__);
#define COP0_WARN(...) PSXLOG_WARN("Cop0", __VA_ARGS__);
#define COP0_ERROR(...) PSXLOG_ERROR("Cop0", __VA_ARGS__);

namespace {
using namespace Psx::Cop0;
struct State {
    // Registers
    struct CauseReg {
        Exception::Type ex_type;
        u8 int_pending = 0;
        u8 cop_num = 0;
        bool branch_delay = false;
    } cause; // r13
    u32 epc = 0;
    u32 bad_vaddr = 0;
} s;

const std::map<Psx::Cop0::Exception::Type, std::string> extype_to_str = {
    {Exception::Type::Interrupt, "INT"},    {Exception::Type::AddrErrLoad, "AEL"},
    {Exception::Type::AddrErrStore, "AES"}, {Exception::Type::IBusErr, "IBE"},
    {Exception::Type::DBusErr, "DBE"},      {Exception::Type::Syscall, "SYS"},
    {Exception::Type::Break, "BRK"},        {Exception::Type::ReservedInstr, "RI"},
    {Exception::Type::CopUnusable, "COP"},  {Exception::Type::Overflow, "OVF"},
};

/*
 * Return, in string form, the last exception occured.
 */
std::string FmtLastException()
{
    return PSX_FMT("EX{{TYPE: {}, BADV: {:08x}, BD: {}, EPC: {:08x}}}",
            extype_to_str.at(s.cause.ex_type), s.bad_vaddr, s.cause.branch_delay, s.epc);
}
}// end namespace




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
    s.cause = {};
    s.epc = 0;
    s.bad_vaddr = 0;
}

/*
 * Raises the given exception. A handler will then be called and the
 * System Control Cop will update it's status registers.
 */
void RaiseException(const Exception& ex)
{
    // update cause register
    s.cause.ex_type = ex.type;
    s.cause.branch_delay = Cpu::InBranchDelaySlot();
    s.cause.cop_num = ex.cop_num;

    // update badv
    s.bad_vaddr = ex.badv;

    // update epc
    s.epc = Cpu::GetPC() + 4; // pc has already incremented
    if (s.cause.ex_type != Exception::Type::Interrupt && s.cause.branch_delay) {
        COP0_WARN("Branch Delay Exception! Did anything break??");
        s.epc -= 4;
    }

    COP0_WARN("Exception Handler not fully implemented!");
    COP0_INFO("{}", FmtLastException());
}

}// end namespace
}
