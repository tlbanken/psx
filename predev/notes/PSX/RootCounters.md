# Root Counters
## Overview
- Root counters are timers in the PSX
- 4 root counters
- each have three registers
    - one with current value
    - one with the counter mode
    - one with the target value

| Counter | Base Address | Synced to |
| ------- | ------------ | --------- |
| 0 | 0x1f80_1100 | pixelclock |
| 1 | 0x1f80_1110 | horizontal retrace |
| 2 | 0x1f80_1120 | 1/8 system clock |
| 3 | - | Vertical Retrace |

## Count (0x11n0) (R/W)
```
0-15    Current Counter value (incrementing)
16-31   Garbage
```
- Auto increments
- writable (can set to any value)
- reset to 0x0000 on any write to Counter Mode register and on counter overflow
    - overflow on either 0xffff or selected target value

## Mode (0x11n4) (R/W)
- In one-shot mode the IRQ is pulsed/toggled only once
```
0       Synchronization Enable (0=Free Run, 1=Synchronize via Bit1-2)
1-2     Synchronization Mode
        Synchronization for Counter 0:
            0 = Pause counter during Hblank(s)
            1 = Reset counter to 0x0000 at Hblank(s)
            2 = Reset counter to 0x0000 at Hblank(s) and pause outside of Hblank
            3 = Pause until Hblank occurs once, then switch to Free Run
        Synchronization Modes for Counter 1:
            Same as above, but using Vblank instead of Hblank
        Synchronization Modes for Counter 2:
            0 or 3 = Stop counter at current value (forever, no h/v-blank start)
            1 or 2 = Free Run (same as when Synchronization Disabled)
3       Reset counter to 0x0000 (0=After Counter=0xffff, 1=After Counter=Target)
4       IRQ when Counter=Target (0=Disabled, 1=Enable)
5       IRQ when Counter=0xffff (0=Disable, 1=Enable)
6       IRQ Once/Repeat Mode (0=One-shot, 1=Repeatedly)
7       IRQ Pulse/Toggle Mode (0=Short Bit10=0 Pulse, 1=Toggle Bit10 on/off)
8-9     Clock Souce (0-3)
            Counter 0: 0 or 2 = System Clock, 1 or 3 = Dotclock
10      Interrupt Request (0=Yes, 1=No) (Set after writing)
11      Reached Target Value (0=No, 1=Yes) (reset after reading)
12      Reached 0xffff value (0=No, 1=Yes) (Reset after reading)
13-15   Unknown (seems to always be zero)
16-31   Garbage
```


## Counter Target (0x11n8) (R/W)
- when the Target flag is set, the counter increments up to (including) the selected target value
    - resets to 0x0000
```
0-15    Counter Target Value
16-31   Garbage
```



