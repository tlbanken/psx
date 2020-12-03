# TODO
- Cop0 vs Interrupts
- Renderer Purpose
- Framework for audio/video?

# Memory
## AddressSpace (Interface)
- abstracts r/w into different address spaces
### Functions
- read 8,16,32 bit val
- write 8,16,32 bit val
### State
- none
### Used by
- Bus
### Uses
- none

## Bus (Class)
- Bus for handling r/w into address spaces
### Functions
- read 8,16,32 bit val
- write 8,16,32 bit val
- add AddressSpace with priority
### State
- Priority Queue of AddressSpace objects
### Used by
- Cpu,...
### Uses
- AddressSpace Objects

## Ram (Class : AddressSpace)
- Main system RAM
### Functions
### State
- 2048 KB memory
### Used by
- Bus
### Uses
- TLB

## DMA (Class : AddressSpace)
### Functions
- manage data through channels
### State
### Used By
### Uses

## Tlb (Class)
- Translation Lookaside Buffer
### Functions
### State
### Used by
- Cop0
- Ram
### Uses

# CPU
## Cpu (Class)
- R3000 CPU
### Functions
- step
- instruction impls
### State
- primary op vector
- secondary op vector
- 32 general purpose registers
- 3 Special Registers
    - HI
    - LO
    - PC
### Used by
### Uses
- Cop0
- Gte (Cop2)
- Bus

## Cop0 (class)
- System Control Coprocessor
### Functions
### State
- 16 32-bit registers
### Used by
### Uses
- TLB

## Interrupts (class : AddressSpace)
### Functions
### State
- Status interrupt register
- Mask interrupt register
### Used by
- Bus
### Uses
- Cop0

# VIDEO
## Gpu (Class : AddressSpace)
### Functions
- interpret command
- handle I/O registers (Data, Control/Status)
### State
- 1MB frame buffer
- 64 byte command FIFO buffer
- rendering mode
- some drawing states
    - drawing offset
    - drawing clip area
    - dither flag
    - draw to display flag
    - mask enable flag
    - mask judement enable flag
- current renderer (SW vs HW)
### Used by
- Bus
### Uses
- Bus
- Renderer

## Gte (Class)
### Functions
- handle special instructions
### State
- 32 data 32-bit registers
- 32 control 32-bit registers
### Used by
### Uses

## MDEC (class : AddressSpace)
### Functions
### State
- Macroblock list
- Control Register
- Status Register
### Used by
- Bus
### Uses

## Renderer (Interface)
### Functions
### State
### Used by
### Uses

## SWRenderer (Class : Renderer)
### Functions
### State
### Used by
### Uses

## HWRenderer (Class : Renderer)
### Functions
### State
### Used by
### Uses

# Sound
## Spu (class : AddressSpace)
### Functions
### State
- 512KB sound buffer
- 24 voices
### Used by
- Bus
### Uses

# Root Counters
## RootCounters (Class : AddressSpace)
- unsure about this
- TODO: Object for each counter?
### Functions
### State
- counter value (0..2)
- counter target
### Used by
### Uses

# IO Ports
## IOPort (class : AddressSpace)
### Functions
- handle IO port reads and writes
### State
- FIFO of commands
- Contoller 1 and 2
- Memory Card 1 and  2
### Used by
- Bus
### Uses
- Controller
- MemoryCard

## Controller (interface)
### Functions
### State
- ID
### Used by
- IOPort
### Uses

## KBController (class : Controller)
### Functions
### State
### Used by
### Uses

## PadController (class : Controller)
### Functions
### State
### Used by
### Uses

## MemoryCard (class)
### Functions
### State
- ID
- 128 KB of Storage
    - 16 blocks
    - each block is 64 frames/sectors
### Used by
- IOPort
### Uses

