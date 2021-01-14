/*
 * cop0.h
 *
 * Travis Banken
 * 1/12/21
 *
 * Coprocessor 0 - System Control
 * Handles Exceptions/Interrupts
 */

#pragma once

#include "util/psxutil.h"

namespace Psx {
namespace Cop0 {

//class Cpu final : public ImGuiLayer::DbgModule {
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
    u8 cop_num = 0;
};

//class Cpu final : public ImGuiLayer::DbgModule {
void Init();
void Reset();
void RaiseException(const Exception& ex);

}// end namespace
}
