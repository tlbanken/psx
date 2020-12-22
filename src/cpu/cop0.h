/*
 * cop0.h
 *
 * Travis Banken
 * 12/20/2020
 *
 * Coprocessor 0 - System Control Coprocessor.
 * Exception/Interrupt handling.
 */

#pragma once

#include "util/psxutil.h"
#include "mem/bus.h"


struct Exception {
    enum class Type {
        Interrupt     = 0x00,
        AddrErrLoad   = 0x04,
        AddrErrStore  = 0x05,
        IBusErr       = 0x06,
        DBusErr       = 0x07,
        Syscall       = 0x08,
        Break         = 0x09,
        ReservedInstr = 0x0a,
        CopUnusable   = 0x0b,
        Overflow      = 0x0c,
    } type;
    u32 badv = 0;
};

class SysControl {
public:
    void RaiseException(const Exception& ex);
private:
};


