# SPU - Sound Processing Unit
## Introduction
- the SPU is responsible for all aural capabilities of the psx
- 24 voices
- 512KB sound buffer
- also has ADSR envelope filters for each voice
- lots of other features

## Sound Buffer
- 512KB sound buffer
- data stored compressed into blocks of 16 bytes
- each block contains the following:
    - 14 packed sample bytes
    - 2 header bytes
        - one for packing
        - one for sample end and looping info
- decoded into 28 sample bytes (= 14 16bit samples)
- first 4KB of buffer, stores the decoded data of CD audio after volume processing and the sound data of voice 1 and voice 3 after envelope processing
- the decoded data is stored as 16 bit signed values, one sample per clock (44.1 KHz)
- following first 4KB are 8 bytes reserved by that system
- memory beyond that is free to store samples, up to the reverb work area if the effect processor is used
- the size of this work area depends on which type of effect is being processed

### Memory Layout
- the SPU memory is not mapped to the CPU bus
- it can be accessed only via I/O or via DMA transfers (DMA4)

| Mem Region Start | Mem Region End | Description |
| ---------------- | -------------- | ----------- |
| `0x0_0000` | `0x0_03ff` | CD audio left |
| `0x0_0400` | `0x0_07ff` | CD audio right |
| `0x0_0800` | `0x0_0bff` | Voice 1 |
| `0x0_0c00` | `0x0_0fff` | Voice 3 |
| `0x0_1000` | `0xX_XXXX` | ADPCM Samples |
| `0xX_XXXX` | `0x7_FFFF` | Reverb work area |

> ADPCM: Adaptive Differential Pulse-Code Modulation

## Voices
- 24 hardware voices
- voices can be used for:
    - reproducing sample data
    - noise
    - frequency modulator on the next voice
- each voice has it's own programmable ADSR envelope filter
- main volume can be programmed independently for left and right output

```
lvl |
 ^  |
 |  |        *                 
    |       * * Dr      *      
 Sl |_ _ _ *_ _*_ _*_ _ _ _*_ _ _ _ _ _ _ _ _ 
    |     *     *            *                    
    |    *        *           *  Rr                 
    |   * Ar         *   *     *                   
    |  *             Sr      *  *                    
    | *                        * *                        
    |________________________________________
                                    --> t

*** KEY ***
Ar      Attack rate, which specifies the speed at which the volume increases from
        zero to it's maximum value, as soon as the note on is given. The slope can
        be set to linear or exponential
Dr      Decay rate specifies the speed at which the volume decreases to the sustain
        level. Decay is always decreasing exponentially.
Sl      Sustain level, base level from which sustain starts.
Sr      Sustain rate is the rate at which the volume of the sustained note increases
        or decreases. This can be either linear or exponential.
Rr      Release rate is teh rate at which the volume of the note decreases as soon
        as the note off is given.
lvl     Volume level
t       Time

The overall volume can also be set to sweep up or down linearly or exponentially from
it's current value. This can be done seperatly for left and right.
```

## SPU Operation
- occupies the area `0x1f80_1c00 - 0x1f80_1dff`
- all registers are 16 bits wide

## Voice Data Area
- `0x1f80_1c00 - 0x1f80_1d7f`
- 8 16 bit registers

### Volume (0x1f80_1c00 - 0x1f80_1d7f)
```
0x1f80_1xx0     Volume left
0x1f80_1xx2     Volume Right

(xx = 0xc0 + voice number)
```

#### Volume Mode
```
Volume mode

|15                            0|
---------------------------------
| 0 | S |           VV          |
---------------------------------
  1   1             14

*** KEY ***
VV      0x0000 - 0x3fff     Voice Volume
S       0                   Phase Normal
        1                   Inverted
```

#### Sweep Mode
```
Sweep mode

|15                                        0|
---------------------------------------------
| 1 | Sl | Dr | Ph |        |       VV      |
---------------------------------------------
  1   1    1    1      5            7

*** KEY ***
VV      0x0000 - 0x007f     Voice Volume
Sl      0                   Linear Slope
        1                   Exponential Slope
Dr      0                   Increase
        1                   Decrease
Ph      0                   Normal phase
        1                   Inverted phase

In sweep mode, the current volume increases to it's maximum value, or decreases to
its minimum value, according to mode. Choose phase equal to the phase of the current
volume.
```

## Pitch (0x1f80_1xx4)
```
Pitch

|15                     0|
--------------------------
|   |        Pt          |
--------------------------
  2          14

*** KEY ***
Pt      0x0000-0x3fff       Specifies pitch

Any value can be set, table shows only octaves:
0x0200  -3 octaves
0x0400  -2
0x0800  -1
0x1000  sample pitch
0x2000  +1
0x3fff  +2
```

## Start address of Sound (0x1f80_1xx6)
```
|15                                0|
-------------------------------------
|               Addr                |
-------------------------------------

*** KEY ***
Addr        Start address of sound in Sound buffer /8
```

## Attack/Decay/Sustain level (0x1f80_1xx8)
```
Attack/Decay/Sustain level

|15                                        0|
| Am |      Ar      |     Dr    |     Sl    |
  1         7             4           4

*** KEY ***
Am      0       Attack mode Linear
        1       Exponential
Ar      0-7f    Attack rate
Dr      0-f     Decay rate
Sl      0-f     Sustain level
```

## Sustain Rate, Release Rate (0x1f80_1xxa)
```
Sustain Rate, Release Rate

|15                                                    0|
---------------------------------------------------------
| Sm | Sd | 0 |         Sr          | Rm |      Rr      |
---------------------------------------------------------
  1    1    1           7             1         5

*** KEY ***
Sm      0       sustain rate mode linear
        1       exponential
Sd      0       sustain rate mode increase
        1       decrease
Sr      0-7f    sustain rate
Rm      0       Linear decease
        1       exponential decrease
Rr      0-1f    Release Rate

Note: decay mode is always Exponential decrease, and thus cannont be set
```

## Current ADSR volume (0x1f80_1xxc)
```
Current ADSR volume

|15                        0|
-----------------------------
|           ASDRvol         |
-----------------------------

*** KEY ***
ADSRvol     Returns the current envelope volume when read
```

## Repeat Address (0x1f80_1xxe)
```
Repeat Address

|15                                    0|
-----------------------------------------
|                   Ra                  |
-----------------------------------------

*** KEY ***
Ra      0x0000-0xffff       Address sample loops to at end

Note: Setting this register only has effect after the voice has started (ie. KeyON),
else the loop address gets reset
```

## SPU Global Registers
### Main Volume Left/Right
```
0x1f80_1d80     Main volume left
0x1f80_1d82     Main volume right

|15                        0|
-----------------------------
|           MVol            |
-----------------------------

*** KEY ***
MVol    0x0000-0xffff   Main volume

Sets Main volume, these work the same as the channel registers. See those for details
```

### Reverberation Depth Left/Right
```
0x1f80_1d84     Reverberation depth left
0x1f80_1d86     Reverberation depth right

|15                        0|
-----------------------------
| P |           Rvd         |
-----------------------------
  1             15

*** KEY ***
Rvd     0x0000-0x7fff       Sets the wet volume for the effect
P       0                   Normal phase
        1                   Inverted phase
```

### Common Layout Registers
```
The following registers have a common layout.

*** LAYOUT ***
First Register:
|15                                                                             0|
----------------------------------------------------------------------------------
| c15 | c14 | c13 | c12 | c11 | c10 | c9 | c7 | c6 | c5 | c4 | c3 | c2 | c1 | c0 |
----------------------------------------------------------------------------------
   1     1     1     1     1     1    1    1    1    1    1    1    1    1    1

|15                                                            0|
-----------------------------------------------------------------
|       0       | c17 | c16 | c15 | c14 | c13 | c12 | c11 | c10 |
-----------------------------------------------------------------
        8          1     1     1     1     1     1     1     1

c0-c17  0   Mode for channel cxx off
        1   Mode for channel cxx on

*** REGISTERS ***
0x1f80_1d88     Voice ON (0-15)
0x1f80_1d8a     Voice ON (16-23)
Sets the current voice to key on (ie. start ads)

0x1f80_1d8c     Voice OFF (0-15)
0x1f80_1d8e     Voice OFF (16-23)
Sets the current voice to key off (ie. release)

0x1f80_1d90     Channel FM (pitch lfo) mode (0-15)
0x1f80_1d92     Channel FM (pitch lfo) mode (16-23)
Sets the channel frequency modulation. Uses the previous channels modulator.

0x1f80_1d94     Channel Noise mode (0-15)
0x1f80_1d96     Channel Noise mode (16-23)
Sets the channel to noise.

0x1f80_1d98     Channel Reverb mode (0-15)
0x1f80_1d9a     Channel Reverb mode (16-23)
Sets reverb for the channel. As soon as the sample ends, the reverb for that channel
is turned off.

0x1f80_1d9c     Channel ON/OFF (0-15)
0x1f80_1d9e     Channel ON/OFF (16-23)
Returns whether the channel is mute or not.
```

## Reverb Work Area Start (0x1f80_1da2)
```
|15                            0|
---------------------------------
|           Revwa               |
---------------------------------

*** KEY ***
Revwa       0x0000-0xffff       Reverb work area start in sound buffer /8
```

## Sound buffer IRQ address (0x1f80_1da4)
```
|15                        0|
-----------------------------
|           IRQa            |
-----------------------------

*** KEY ***
IRQa        0x0000-0xffff       IRQ address in sound buffer /8
```

## SPU Data (0x1f80_1da8)
```
|15                                0|
-------------------------------------
|               Data                |
-------------------------------------
Data forwarding reg, for non DMA transfer
```

## SPU Control sp0 (0x1f80_1daa)
```
|15                                                         0|
--------------------------------------------------------------
| En | Mu |     Noise   | Rv | Irq | DMA | Er | Cr | Ee | Ce |
--------------------------------------------------------------
  1    1        6         1     1     2    1    1    1    1

*** KEY ***
En      0       SPU off
        1       SPU on
Mu      0       Mute SPU
        1       Unmute SPU
Noise           Noise clock frequency
Rv      0       Reverb Disabled
        1       Reverb Enabled
Irq     0       Irq disabled
        1       Irq enabled
DMA     00
        01      Non DMA write (transfer through data reg)
        10      DMA Write
        11      DMA Read
Er      0       Reverb for external off
        1       Reverb for external on
Cr      0       Reverb for CD off
        1       Reverb for CD on
Ee      0       External audio off
        1       External audio on
Ce      0       CD audio off
        1       CD audio on
```

## SPU status 1 (0x1f80_1dac)
```
SPU Status

|15                                0|
-------------------------------------
|                                   |
-------------------------------------
In SPU init routines, this register gets loaded with 0x4
```

## SPU Status 2 (0x1f80_1dae)
```
|15                                    0|
-----------------------------------------
|           | Dh | Rd |                 |
-----------------------------------------

*** KEY ***
Dh      0       Decoding in first half of buffer
        1       Decoding in second half of buffer
Rd      0       SPU ready to transfer
        1       SPU nont ready

Some of bits 9-0 are also ready/not ready states. More on that later. Functions that
wait for the SPU to be ready, wait for bits a-0 to become 0
```

## CD Volume Left/Right
```
0x1f80_1db0     CD Volume left
0x1f80_1db2     CD volume right

|15                            0|
---------------------------------
| P |           CDvol           |
---------------------------------
  1               15

*** KEY ***
CDvol       0x0000-0x7fff       Set volume of CD input
P           0                   Normal phase
            1                   Inverted phase
```

## Extern Volume Left/Right
```
0x1f80_1db4     Extern volume left
0x1f80_1db6     Extern volume right

|15                            0|
---------------------------------
| P |           Exvol           |
---------------------------------
  1               15

*** KEY ***
Exvol       0x0000-0x7fff       Set volume of external input
P           0                   Normal phase
            1                   Inverted phase
```

## Reverb Volume and Address Registers (R/W)
| Addr | Reg | Name | Type | Explanation |
| ---- | --- | ---- | ---- | ----------- |
| `0x1f80_1d84` | spu | vLOUT | volume | Reverb Output Volume Left |
| `0x1f80_1d86` | spu | vROUT | volume | Reverb output volume right |
| `0x1f80_1da2` | spu | mBASE | base | reverb work area start address in Sound RAM |
| `0x1f80_1dc0` | rev00 | dAPF1 | disp | reverb APF offset 1 |
| `0x1f80_1dc2` | rev01 | dAPF2 | disp | reverb APF offset 2 |
| `0x1f80_1dc4` | rev02 | vIIR | volume | reverb reflection volume 1 |
| `0x1f80_1dc6` | rev03 | vCOMB1 | volume | reverb Comb volume 1 |
| `0x1f80_1dc8` | rev04 | vCOMB2 | volume | reverb Comb volume 2 |
| `0x1f80_1dca` | rev05 | vCOMB3 | volume | reverb Comb volume 3 |
| `0x1f80_1dcc` | rev06 | vCOMB4 | volume | reverb Comb volume 4 |
| `0x1f80_1dce` | rev07 | vWALL | volume | reverb reflection volume 2 |
| `0x1f80_1dd0` | rev08 | vAPF1 | volume | reverb APF volume 1 |
| `0x1f80_1dd2` | rev09 | vAPF2 | volume | reverb APF volume 2 |
| `0x1f80_1dd4` | rev0A | mLSAME | src/dst | reverb same side reflection address 1 left |
| `0x1f80_1dd6` | rev0B | mRSAME | src/dst | reverb same side reflection address 1 right |
| `0x1f80_1dd8` | rev0C | mLCOMB1 | src | reverb comb address 1 left |
| `0x1f80_1dda` | rev0D | mRCOMB1 | src | reverb comb address 1 right |
| `0x1f80_1ddc` | rev0E | mLCOMB2 | src | reverb comb address 2 left |
| `0x1f80_1dde` | rev0F | mRCOMB2 | src | reverb comb address 2 right |
| `0x1f80_1de0` | rev10 | dLSAME | src | reverb same side reflection address 2 left |
| `0x1f80_1de2` | rev11 | dRSAME | src | reverb same side reflection address 2 right |
| `0x1f80_1de4` | rev12 | mLDIFF | src/dst | reverb different side reflect address 1 left |
| `0x1f80_1de6` | rev13 | mRDIFF | src/dst | reverb different side reflect address 1 right|
| `0x1f80_1de8` | rev14 | mLCOMB3 | src | reverb comb address 3 left |
| `0x1f80_1dea` | rev15 | mRCOMB3 | src | reverb comb address 3 right |
| `0x1f80_1dec` | rev16 | mLCOMB4 | src | reverb comb address 4 left |
| `0x1f80_1dee` | rev17 | mRCOMB4 | src | reverb comb address 4 right |
| `0x1f80_1df0` | rev18 | dLDIFF | src | reverb different side reflect address 2 left |
| `0x1f80_1df2` | rev19 | dRDIFF | src | reverb different side reflect address 2 right |
| `0x1f80_1df4` | rev1A | mLAPF1 | src/dst | reverb APF address 1 left |
| `0x1f80_1df6` | rev1B | mRAPF1 | src/dst | reverb APF address 1 right |
| `0x1f80_1df8` | rev1C | mLAPF2 | src/dst | reverb APF address 2 left |
| `0x1f80_1dfa` | rev1D | mRAPF2 | src/dst | reverb APF address 2 right |
| `0x1f80_1dfc` | rev1E | vLIN | volume | reverb input volume left |
| `0x1f80_1dfe` | rev1F | vRIN | volume | reverb input volume right |

- All volume registers are signed 16bit
- all src/dst/disp/base regs are addresses in SPU mem (divided by 8)
- src/dst are relative to the current buffer address
- the disp registers are relative to src registers
- the base register defines the start address of the reverb buffer (end address is fixed, at 0x7FFFE)
- writing a val to mBASE does additionally set the current buffer address to that value

### Voice 0..23 reverb mode on (EON) (R/W) (0x1f80_1d98)
```
0-23    Voice 0..23 Destination (0=To Mixer, 1=To Mixer and to Reverb)
24-31   Not used
```

### Reverb Bits in SPU Control Register (R/W)
- this reg contains a reverb master enable flag, and reverb enabled flags for External audio input and CD audio input

## SPU Reverb Formula
```
  ___Input from Mixer (Input volume multiplied with incoming data)_____________
  Lin = vLIN * LeftInput    ;from any channels that have Reverb enabled
  Rin = vRIN * RightInput   ;from any channels that have Reverb enabled

  ____Same Side Reflection (left-to-left and right-to-right)___________________
  [mLSAME] = (Lin + [dLSAME]*vWALL - [mLSAME-2])*vIIR + [mLSAME-2]  ;L-to-L
  [mRSAME] = (Rin + [dRSAME]*vWALL - [mRSAME-2])*vIIR + [mRSAME-2]  ;R-to-R

  ___Different Side Reflection (left-to-right and right-to-left)_______________
  [mLDIFF] = (Lin + [dRDIFF]*vWALL - [mLDIFF-2])*vIIR + [mLDIFF-2]  ;R-to-L
  [mRDIFF] = (Rin + [dLDIFF]*vWALL - [mRDIFF-2])*vIIR + [mRDIFF-2]  ;L-to-R

  ___Early Echo (Comb Filter, with input from buffer)__________________________
  Lout=vCOMB1*[mLCOMB1]+vCOMB2*[mLCOMB2]+vCOMB3*[mLCOMB3]+vCOMB4*[mLCOMB4]
  Rout=vCOMB1*[mRCOMB1]+vCOMB2*[mRCOMB2]+vCOMB3*[mRCOMB3]+vCOMB4*[mRCOMB4]

  ___Late Reverb APF1 (All Pass Filter 1, with input from COMB)________________
  Lout=Lout-vAPF1*[mLAPF1-dAPF1], [mLAPF1]=Lout, Lout=Lout*vAPF1+[mLAPF1-dAPF1]
  Rout=Rout-vAPF1*[mRAPF1-dAPF1], [mRAPF1]=Rout, Rout=Rout*vAPF1+[mRAPF1-dAPF1]

  ___Late Reverb APF2 (All Pass Filter 2, with input from APF1)________________
  Lout=Lout-vAPF2*[mLAPF2-dAPF2], [mLAPF2]=Lout, Lout=Lout*vAPF2+[mLAPF2-dAPF2]
  Rout=Rout-vAPF2*[mRAPF2-dAPF2], [mRAPF2]=Rout, Rout=Rout*vAPF2+[mRAPF2-dAPF2]

  ___Output to Mixer (Output volume multiplied with input from APF2)___________
  LeftOutput  = Lout*vLOUT
  RightOutput = Rout*vROUT

  ___Finally, before repeating the above steps_________________________________
  BufferAddress = MAX(mBASE, (BufferAddress+2) AND 7FFFEh)
  Wait one 22050Hz cycle, then repeat the above stuff
```

### Notes on Calculations
- the value written to mem are saturated to `-0x8000..+0x7fff`
- the multiplicatoin results are divided by `0x8000`, to fit them to 16bit range
- reverb buffer left and right reverbs addresses should be choosen so that one half of the buffer contains Left sampless, and the other half Right samples
    - (ie. the data is L,L,L,...R,R,R,...)
    - it is *NOT* interlaced (ie. the data is L,R,L,R,...)

## SPU Interrupt
### Sound RAM IRQ Address (IRQ9)
```
15-0    Address in sound buffer divided by eight
```

### Voice Interrupt
- triggers an IRQ when a voice reads ADPCM data from the IRQ address

> NOTE: Metal Gear Solid uses IRQ Address so maybe look into it in the future

