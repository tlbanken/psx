/*
 * cop0.cpp
 *
 * Travis Banken
 * 12/20/2020
 *
 * Coprocessor 0 - System Control Coprocessor.
 * Exception/Interrupt handling.
 */

#include "cpu/cpu.h"

#define COP0_INFO(msg) PSXLOG_INFO("Cop0", msg);
#define COP0_WARN(msg) PSXLOG_WARN("Cop0", msg);
#define COP0_ERROR(msg) PSXLOG_ERROR("Cop0", msg);

static const std::map<Cpu::Exception::Type, std::string> extype_to_str = {
    {Cpu::Exception::Type::Interrupt, "INT"},    {Cpu::Exception::Type::AddrErrLoad, "AEL"},
    {Cpu::Exception::Type::AddrErrStore, "AES"}, {Cpu::Exception::Type::IBusErr, "IBE"},
    {Cpu::Exception::Type::DBusErr, "DBE"},      {Cpu::Exception::Type::Syscall, "SYS"},
    {Cpu::Exception::Type::Break, "BRK"},        {Cpu::Exception::Type::ReservedInstr, "RI"},
    {Cpu::Exception::Type::CopUnusable, "COP"},  {Cpu::Exception::Type::Overflow, "OVF"},
};

/*
 * Raises the given exception. A handler will then be called and the
 * System Control Cop will update it's status registers.
 */
void Cpu::Cop0::RaiseException(const Exception& ex)
{
    // update cause register
    m_cause.ex_type = ex.type;
    m_cause.branch_delay = ex.on_branch;
    m_cause.cop_num = ex.cop_num;

    // update badv
    m_bad_vaddr = ex.badv;

    // update epc
    m_epc = m_cpu.GetPC() - 4; // pc has already incremented
    if (m_cause.ex_type != Exception::Type::Interrupt && m_cause.branch_delay) {
        COP0_WARN("Branch Delay Exception! Did anything break??");
        // unsure if this is correct?
        m_epc += 4;
    }

    COP0_WARN("Exception Handler not fully implemented!");
    COP0_INFO(FmtLastException());
}

/*
 * Reset the state of the coprocessor.
 */
void Cpu::Cop0::Reset()
{
    CauseReg new_cr;
    m_cause = new_cr;
    m_epc = 0;
    m_bad_vaddr = 0;
}

/*
 * Return, in string form, the last exception occured.
 */
std::string Cpu::Cop0::FmtLastException()
{
    return PSX_FMT("EX{{TYPE: {}, BADV: {:08x}, BD: {}, EPC: {:08x}}}",
            extype_to_str.at(m_cause.ex_type), m_bad_vaddr, m_cause.branch_delay, m_epc);
}

