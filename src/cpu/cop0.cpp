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

#define SYSCTRL_INFO(msg) PSXLOG_INFO("System Control", msg);
#define SYSCTRL_WARN(msg) PSXLOG_WARN("System Control", msg);
#define SYSCTRL_ERROR(msg) PSXLOG_ERROR("System Control", msg);

/*
 * Raises the given exception. A handler will then be called and the
 * System Control Cop will update it's status registers.
 */
void SysControl::RaiseException(const Exception& ex)
{
    // TODO
    (void) ex;
    SYSCTRL_WARN("Exception Handler not implemented!");
}

