/*
 * timer.cc
 *
 * Travis Banken
 * 5/8/2022
 * 
 * Timers (Root Counters) for the PSX.
 * There are 3 timers:
 *   1F80110xh      Timer 0 Dotclock
 *   1F80111xh      Timer 1 Horizontal Retrace
 *   1F80112xh      Timer 2 1/8 system clock
 */

#include "timer.hh"

#include "imgui/imgui.h"

#include "util/psxutil.hh"
#include "util/psxlog.hh"
#include "cpu/interrupt.hh"
#include "view/imgui/dbgmod.hh"

#define TIMER_INFO(...) PSXLOG_INFO("TIMER", __VA_ARGS__)
#define TIMER_WARN(...) PSXLOG_WARN("TIMER", __VA_ARGS__)
#define TIMER_ERROR(...) PSXLOG_ERROR("TIMER", __VA_ARGS__)
#define TIMER_FATAL(...) TIMER_ERROR(__VA_ARGS__); throw std::runtime_error(PSX_FMT(__VA_ARGS__))

#define NUM_TIMERS 3

namespace Psx {
namespace Timer {

// private state vars
namespace {

enum TimerNum {
    Dotclock = 0,
    HorzRetrace = 1,
    SysClock = 2
};

struct State {
    u16 timer_val[NUM_TIMERS] = {0};
    u16 timer_target[NUM_TIMERS] = {0};
    union TimerModeReg {
        //   0     Synchronization Enable (0=Free Run, 1=Synchronize via Bit1-2)
        //   1-2   Synchronization Mode   (0-3, see lists below)
        //          Synchronization Modes for Counter 0:
        //            0 = Pause counter during Hblank(s)
        //            1 = Reset counter to 0000h at Hblank(s)
        //            2 = Reset counter to 0000h at Hblank(s) and pause outside of Hblank
        //            3 = Pause until Hblank occurs once, then switch to Free Run
        //          Synchronization Modes for Counter 1:
        //            Same as above, but using Vblank instead of Hblank
        //          Synchronization Modes for Counter 2:
        //            0 or 3 = Stop counter at current value (forever, no h/v-blank start)
        //            1 or 2 = Free Run (same as when Synchronization Disabled)
        //   3     Reset counter to 0000h  (0=After Counter=FFFFh, 1=After Counter=Target)
        //   4     IRQ when Counter=Target (0=Disable, 1=Enable)
        //   5     IRQ when Counter=FFFFh  (0=Disable, 1=Enable)
        //   6     IRQ Once/Repeat Mode    (0=One-shot, 1=Repeatedly)
        //   7     IRQ Pulse/Toggle Mode   (0=Short Bit10=0 Pulse, 1=Toggle Bit10 on/off)
        //   8-9   Clock Source (0-3, see list below)
        //          Counter 0:  0 or 2 = System Clock,  1 or 3 = Dotclock
        //          Counter 1:  0 or 2 = System Clock,  1 or 3 = Hblank
        //          Counter 2:  0 or 1 = System Clock,  2 or 3 = System Clock/8
        //   10    Interrupt Request       (0=Yes, 1=No) (Set after Writing)    (W=1) (R)
        //   11    Reached Target Value    (0=No, 1=Yes) (Reset after Reading)        (R)
        //   12    Reached FFFFh Value     (0=No, 1=Yes) (Reset after Reading)        (R)
        //   13-15 Unknown (seems to be always zero)
        //   16-31 Garbage (next opcode)
        struct {
            // hope this works :/
            u16 sync_enable: 1; // 0
            u16 sync_mode: 2; // 1-2
            u16 reset_mode: 1; // 3
            u16 irq_on_target: 1; // 4
            u16 irq_on_ffff: 1; // 5
            u16 irq_repeat: 1; // 6
            u16 irq_toggle: 1; // 7
            u16 clock_src: 2; // 8-9
            u16 irq_disabled: 1; // 10
            u16 reached_target: 1; // 11
            u16 reached_ffff: 1; // 12
            u16 reserved: 3; // 13-15
        } fields;
        u16 raw;
    } timer_mode[NUM_TIMERS];
} s;

// protos
void debugPrintTimerMode(u32 timer_num);
void sysClockStep();
void horzRetraceStep();
void dotClockStep();

} // end private ns

void Init()
{
    TIMER_INFO("Initializing timers");
    s = {};
    static_assert(sizeof(State::TimerModeReg) == sizeof(u16));
}

void Reset()
{
    TIMER_INFO("Resetting timers");
    s = {};
}

void Step()
{
    // Dotclock step
    dotClockStep();

    // Horizontal Retrace step
    horzRetraceStep();

    // System Clock step
    sysClockStep();

    for (u32 timer_num = 0; timer_num < NUM_TIMERS; timer_num++) {
        u16 reset_target = s.timer_mode[timer_num].fields.reset_mode == 0 
                           ? 0xffff : s.timer_target[timer_num];
        if (s.timer_val[timer_num] == reset_target) {
            s.timer_val[timer_num] = 0;
            // TODO irqs
            if (s.timer_mode[timer_num].fields.irq_disabled == 0) {
                Interrupt::Type itype;
                switch (timer_num) {
                case TimerNum::Dotclock: itype = Interrupt::Type::Timer0; break;
                case TimerNum::HorzRetrace: itype = Interrupt::Type::Timer1; break;
                case TimerNum::SysClock: itype = Interrupt::Type::Timer2; break;
                default: PSX_ASSERT(0);
                }
                Interrupt::Signal(itype);
            }
            
            // reached target/ffff
            if (reset_target == 0xffff) {
                s.timer_mode[timer_num].fields.reached_ffff = 1;
            } else {
                s.timer_mode[timer_num].fields.reached_target = 1;
            }
        }
    }
}

// *** Read ***
template<class T>
T Read(u32 addr)
{
    u32 timer_num = (addr >> 4) & 0x3;
    PSX_ASSERT(timer_num < NUM_TIMERS);
    u32 offset = addr & 0xf;
    T data = 0;

    switch (offset) {
    case 0x00: // value
        data = (T)s.timer_val[timer_num];
        break;
    case 0x04: // mode
        data = (T)s.timer_mode[timer_num].raw;
        // reset some state
        s.timer_mode[timer_num].fields.reached_ffff = 0;
        s.timer_mode[timer_num].fields.reached_target = 0;
        break;
    case 0x08: // target
        data = (T)s.timer_target[timer_num];
        break;
    default:
        TIMER_FATAL("Unknown timer register address [{:08x}], offset: {:02x}", addr, offset);
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
    u32 timer_num = (addr >> 4) & 0x3;
    PSX_ASSERT(timer_num < NUM_TIMERS);
    u32 offset = addr & 0xf;

    switch (offset) {
    case 0x00: // value
        s.timer_val[timer_num] = (u16) data;
        break;
    case 0x04: // mode
        s.timer_mode[timer_num].raw = (u16) data;
        // reset timer val
        s.timer_val[timer_num] = 0;
        break;
    case 0x08: // target
        s.timer_target[timer_num] = (u16) data;
        break;
    default:
        TIMER_FATAL("Unknown timer register address [{:08x}], offset: {:02x}", addr, offset);
    }
}
// template impl needs to be visable to other cpp files to avoid compile err
template void Write<u8>(u8 data, u32 addr);
template void Write<u16>(u16 data, u32 addr);
template void Write<u32>(u32 data, u32 addr);

void OnActive(bool *active)
{
    if (!ImGui::Begin("Timers Debug", active)) {
        ImGui::End();
        return;
    }

    ImGui::TextUnformatted("Registers");
    ImGui::Separator();
    ImGui::BeginGroup();
        ImGui::BeginGroup();
            // Value
            ImGui::Text("Timer Value");
            ImGui::TextUnformatted(PSX_FMT("{:<12} = 0x{:04x} ({:05})", "DotClock", s.timer_val[TimerNum::Dotclock], s.timer_val[TimerNum::Dotclock]).c_str());
            ImGui::TextUnformatted(PSX_FMT("{:<12} = 0x{:04x} ({:05})", "HorzRetrace", s.timer_val[TimerNum::HorzRetrace], s.timer_val[TimerNum::HorzRetrace]).c_str());
            ImGui::TextUnformatted(PSX_FMT("{:<12} = 0x{:04x} ({:05})", "SysClock", s.timer_val[TimerNum::SysClock], s.timer_val[TimerNum::SysClock]).c_str());
        ImGui::EndGroup();
        ImGui::SameLine();
        ImGui::BeginGroup();
            // Mode
            ImGui::Text("Timer Mode");
            ImGui::TextUnformatted(PSX_FMT("{:<12} = 0x{:04x} ({:05})", "DotClock", s.timer_mode[TimerNum::Dotclock].raw, s.timer_mode[TimerNum::Dotclock].raw).c_str());
            ImGui::TextUnformatted(PSX_FMT("{:<12} = 0x{:04x} ({:05})", "HorzRetrace", s.timer_mode[TimerNum::HorzRetrace].raw, s.timer_mode[TimerNum::HorzRetrace].raw).c_str());
            ImGui::TextUnformatted(PSX_FMT("{:<12} = 0x{:04x} ({:05})", "SysClock", s.timer_mode[TimerNum::SysClock].raw, s.timer_mode[TimerNum::SysClock].raw).c_str());
            // ...
        ImGui::EndGroup();
        ImGui::SameLine();
        ImGui::BeginGroup();
            // Target
            ImGui::Text("Timer Target");
            ImGui::TextUnformatted(PSX_FMT("{:<12} = 0x{:04x} ({:05})", "DotClock", s.timer_target[TimerNum::Dotclock], s.timer_target[TimerNum::Dotclock]).c_str());
            ImGui::TextUnformatted(PSX_FMT("{:<12} = 0x{:04x} ({:05})", "HorzRetrace", s.timer_target[TimerNum::HorzRetrace], s.timer_target[TimerNum::HorzRetrace]).c_str());
            ImGui::TextUnformatted(PSX_FMT("{:<12} = 0x{:04x} ({:05})", "SysClock", s.timer_target[TimerNum::SysClock], s.timer_target[TimerNum::SysClock]).c_str());
            // ...
        ImGui::EndGroup();
    ImGui::EndGroup();
    ImGui::Separator();
    ImGui::TextUnformatted("Timer Mode Breakdown");
    ImGui::BeginGroup();
        for (u32 i = 0; i < NUM_TIMERS; i++) {
            ImGui::BeginGroup();
                debugPrintTimerMode(i);
            ImGui::EndGroup();
            ImGui::SameLine();
        }
    ImGui::EndGroup();

    ImGui::End();
}

// Private helpers
namespace {

void debugPrintTimerMode(u32 timer_num)
{
    const State::TimerModeReg *m = &s.timer_mode[timer_num];
    std::string timer_num_str;
    if (timer_num == (u32)TimerNum::Dotclock) {
        timer_num_str = "DotClock";
    } else if (timer_num == (u32)TimerNum::HorzRetrace) {
        timer_num_str = "HorzRetrace";
    } else {
        timer_num_str = "SysClock";
    }
    ImGui::TextUnformatted(PSX_FMT("** {} **", timer_num_str).c_str());
    ImGui::TextUnformatted(PSX_FMT("Sync Enable    : {}", m->fields.sync_enable).c_str());
    ImGui::TextUnformatted(PSX_FMT("Sync Mode      : {}", m->fields.sync_mode).c_str());
    ImGui::TextUnformatted(PSX_FMT("Reset Mode     : {}", m->fields.reset_mode).c_str());
    ImGui::TextUnformatted(PSX_FMT("IRQ On Target  : {}", m->fields.irq_on_target).c_str());
    ImGui::TextUnformatted(PSX_FMT("IRQ On 0xffff  : {}", m->fields.irq_on_ffff).c_str());
    ImGui::TextUnformatted(PSX_FMT("IRQ Repeat     : {}", m->fields.irq_repeat).c_str());
    ImGui::TextUnformatted(PSX_FMT("IRQ Toggle     : {}", m->fields.irq_toggle).c_str());
    ImGui::TextUnformatted(PSX_FMT("Clock Src      : {}", m->fields.clock_src).c_str());
    ImGui::TextUnformatted(PSX_FMT("IRQ Disable    : {}", m->fields.irq_disabled).c_str());
    ImGui::TextUnformatted(PSX_FMT("Reached Target : {}", m->fields.reached_target).c_str());
    ImGui::TextUnformatted(PSX_FMT("Reached 0xffff : {}", m->fields.reached_ffff).c_str());
}

void sysClockStep()
{
    const State::TimerModeReg *sysclk_mode = &s.timer_mode[TimerNum::SysClock];
    static u32 s_cpu_clock = 0;
    // step every clock or once every 8 clocks...
    const u32 clocks_before_count = sysclk_mode->fields.clock_src == 0 
                                 || sysclk_mode->fields.clock_src == 1 
                                  ? 0 : 8;
    if (s_cpu_clock++ < clocks_before_count) {
        return;
    } 
    s_cpu_clock = 0;
    if (sysclk_mode->fields.sync_enable && 
       (sysclk_mode->fields.sync_mode == 0 || 
        sysclk_mode->fields.sync_mode == 3)) {
        // no counting
        return;
    } 
    s.timer_val[TimerNum::SysClock]++;
}

void horzRetraceStep()
{
    // TODO idk if this timing is good
    static u32 s_cpu_clock = 0;
    const u32 cpu_clocks_per_frame = 33'868'800 / 60;
    const u32 scanlines_per_frame = 263; // NTSC
    const u16 clk_src = s.timer_mode[TimerNum::HorzRetrace].fields.clock_src;
    const u32 clocks_before_count = clk_src == 0 || clk_src == 2 
                                    ? 0 : (cpu_clocks_per_frame / scanlines_per_frame);
    if (s_cpu_clock++ < clocks_before_count) {
        return;
    }
    s_cpu_clock = 0;
    PSX_ASSERT(s.timer_mode[TimerNum::HorzRetrace].fields.sync_enable == 0);
    s.timer_val[TimerNum::HorzRetrace]++;
}

void dotClockStep()
{
    // TODO idk if this timing is good
    static u32 s_cpu_clock = 0;
    const u32 cpu_clocks_per_frame = 33'868'800 / 60;
    const u32 dots_per_scanline = 853; // NTSC
    const u32 scanline_per_frame = 263; // NTSC
    const u32 cpu_clocks_per_dot = cpu_clocks_per_frame / (dots_per_scanline * scanline_per_frame);
    const u16 clk_src = s.timer_mode[TimerNum::Dotclock].fields.clock_src;
    const u32 clocks_before_count = clk_src == 0 || clk_src == 2 
                                    ? 0 : cpu_clocks_per_dot;
    if (s_cpu_clock++ < clocks_before_count) {
        return;
    }
    s_cpu_clock = 0;
    PSX_ASSERT(s.timer_mode[TimerNum::Dotclock].fields.sync_enable == 0);
    s.timer_val[TimerNum::Dotclock]++;
}

} // end private ns
} // end ns
}