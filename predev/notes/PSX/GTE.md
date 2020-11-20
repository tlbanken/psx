# The Geometry Transformation Engine (GTE)
- heart of all 3D calculations on the PSX
- can perform vector and matrix operations, perspective transformations, color equations, . . .
- it is much faster than the CPU on these operations
- mounter as the second coprocessor and as such is no physical address in the memory of the PSX
- all control is done through special instructions

## Basic Mathematics
- GTE is an engine for vector mathematics
- a point in 3D can be represented as `[X,Y,Z]`
- two kinds
    1. vectors of variable length
    2. vectors of unit length of 1.0 (normal vectors)
- the first is used to describe a location, the other describes direction
- rotation is performed by multiplying with a rotation matrix

```
sN = sin(N), cN = cos(N)

Rotation angle A about X axis:
|   1   0   0   |
|   0  cA -sA   |
|   0  sA  cA   |

Rotation angle B about Y axis:
|  cB   0  sB   |
|   0   1   0   |
| -sB   0  cB   |

Rotation angle C about Z axis:
|  cC -sC   0   |
|  sC  cC   0   |
|   0   0   1   |
```

## Brief Function Descriptions
### RTPS/RTPT
- rotate, translate and perspective transformation
- these two functions perform final 3d calcs on one or three vertices at once
- the points are first multiplied with a rotation matrix(R)
- then translated(TR)
- finally, perspective transformation is applied, which results in 2d scen coordinates
- also returns interpolation value to be used with various depth cueing instructions

### MVMVA
- Matrix & Vector multiplication and addition
- multiplies a vector with either the rotation matrix, the light matrix, or color matrix
- then adds the translation vector or background color vector

### DCPL
- Depth cue light color
- first calc a color from a light vector and provided RGB value
    - light vector: normal vector of a plane multiplied with the light matrix and zero limited
- performs depth cueing by interpolating between the far color vector and the newfound color

### DPCS/DPCT
- Depth cue single/triple
- performs depth cueing by interpolating between a color and the far color vector on one or three colors

### INTPL
- interpolation
- interpolates between a vector and the far color vector

### SQR
- Square
- calculates the square of a vector

### NCS/NCT
- normal color
- calculates a color from the normal of a point or plane and the light sources and colors
- the basic color of the plane or point the normal refers to is assumed to be white

### NCDS/NCDT
- normal color depth cue
- same as NCS/NCT but also performs depth cueing (Like DPCS/DPCT)

### CDP
- a color is calculated from a light vector (bas color is assumed to be white) and depth cueing is performed (like DPCS)

### CC
- a color is caclulated from a light vector and a base color

### NCLIP
- calculates the outer product of three 2d points
- the 3 vertices should be stored clockwise according to the visual point

### AVSZ3/AVSZ4
- adds 3 or 4 z values together and multiplies them by a fixed point value
- this value is normally chosen so that this function returns the average of the z values
    - usually further divided by 2 or 4 for easy adding to the OT

### OP
- calculates the outer product of 2 vectors

### GPF
- multiplies 2 vectors
- also returns the result as 24bit rgb value

### GPL
- multiplies a vector with a scalar and adds the result to another vector
- als returns the result as 24bit rgb value

## Instructions
- the CPU has six special load and store instructions for the GTE registers
- also has an instruction to issue commands to the coprocessor
```
LWC2 gd, imm(base)      stores value at imm(base) in GTE data register gd
SWC2 gd, imm(base)      stores GTE data register at imm(base)
MTC2 rt, gd             stores register rt in GTE data register gd
MFC2 rt, gd             stores GTE data register gd in register rt
CTC2 rt, gc             stores register rt in GTE control register gc
CFC2 rt, gc             stores GTE control register in register rt
COP2 b25                Issues a GTE command


*** KEY ***
rt          CPU register 0-31
gd          GTE data register 0-31
gc          GTE control register 0-31
imm         16 bit immediate value
base        CPU register 0-31
imm(base)   address pointed to by base + imm
b25         25 bit wide data field

NOTE: GTE load and store instructions have a delay of 2 instructions, for
any GTE commands or operations accessing that register
```

## Programming the GTE
- before use, the GTE must be turned on
- the GTE has bit 30 allocated to it in the status register of cop0
- before any GTE instruction is used, this bit must be set
- GTE instructions and functions should not be used in
    - delay slots of jumps and branches
    - event handlers or interrupts
- if an instruction that reads a GTE register or a GTE command is executed before the current GTE command is finished, the CPU will hold until the instruction has finished

## Registers
- the GTE has
    - 32 data registers
    - 32 control registers
    - 32 bits wide
- control registers are commonly called Cop2C
- data registers are called Cop2D

### Control Registers (Cop2C)
| Number | Name | Description |
| ------ | ---- | ----------- |
| 0 | R11R12 | Rotation matrix elements 1 to 1, 1 to 2 |
| 1 | R13R21 | Rotation matrix elements 1 to 3, 2 to 1 |
| 2 | R22R23 | Rotation matrix elements 2 to 2, 2 to 3 |
| 3 | R31R32 | Rotation matrix elements 3 to 1, 3 to 2 |
| 4 | R33 | Rotation matrix elements 3 to 3 |
| 5 | TRX | Translation vector X |
| 6 | TRY | Translation vector Y |
| 7 | TRZ | Translation vector Z |
| 8 | L11L12 | Light source matrix elements 1 to 1, 1 to 2 |
| 9 | L13L21 | Light source matrix elements 1 to 3, 2 to 1 |
| 10 | L22L23 | Light source matrix elements 2 to 2, 2 to 3 |
| 11 | L31L32 | Light source matrix elements 3 to 1, 3 to 2 |
| 12 | L33 | Light source matrix elements 3 to 3 |
| 13 | RBK | Background color red component |
| 14 | BBK | Background color blue component |
| 15 | GBK | Background color green component |
| 16 | LR1LR2 | Light color matrix source 1&2 red component |
| 17 | LR3LG1 | Light color matrix source 3 red, 1 green component |
| 18 | LG2LG3 | Light color matrix source 2&3 green component |
| 19 | LB1LB2 | Light color matrix source 1&2 blue component |
| 20 | LB3 | Light color matrix source 3 blue component |
| 21 | RFC | Far color red component |
| 22 | GFC | Far color green component |
| 23 | BFC | Far color blue component |
| 24 | OFX | Screen offset X |
| 25 | OFY | Screen offset Y |
| 26 | H | Projection plane distance |
| 27 | DQA | Depth queuing parameter A (coefficient) |
| 28 | DQB | Depth queuing parameter B (offset) |
| 29 | ZSF3 | Z3 average scale factor (normally 1/3) |
| 30 | ZSF4 | Z4 average scale factor (normally 1/4) |
| 31 | FLAG | Returns any calculation errors |

## Control Register Format
- the GTE uses signed, fixed point register for mathematics

### R11R12
```
|31                                                                              0|
|                   R11                  |                   R12                  |
-----------------------------------------------------------------------------------
| sign | integral part | fractional part | sign | integral part | fractional part |
-----------------------------------------------------------------------------------
   1            3              12           1            3              12
```

### R13R21
```
|31                                                                              0|
|                   R13                  |                   R21                  |
-----------------------------------------------------------------------------------
| sign | integral part | fractional part | sign | integral part | fractional part |
-----------------------------------------------------------------------------------
   1            3              12           1            3              12
```

### R22R23
```
|31                                                                              0|
|                   R22                  |                   R23                  |
-----------------------------------------------------------------------------------
| sign | integral part | fractional part | sign | integral part | fractional part |
-----------------------------------------------------------------------------------
   1            3              12           1            3              12
```

### R31R32
```
|31                                                                              0|
|                   R31                  |                   R32                  |
-----------------------------------------------------------------------------------
| sign | integral part | fractional part | sign | integral part | fractional part |
-----------------------------------------------------------------------------------
   1            3              12           1            3              12
```

### R33
```
|31                                                                              0|
|                                        |                   R32                  |
-----------------------------------------------------------------------------------
|                  0                     | sign | integral part | fractional part |
-----------------------------------------------------------------------------------
                  16                        1            3              12
```

### TRX
```
|31                                                                              0|
-----------------------------------------------------------------------------------
| sign |                           Integral part                                  |
-----------------------------------------------------------------------------------
   1                                    31
```

### TRY
```
|31                                                                              0|
-----------------------------------------------------------------------------------
| sign |                           Integral part                                  |
-----------------------------------------------------------------------------------
   1                                    31
```

### TRZ
```
|31                                                                              0|
-----------------------------------------------------------------------------------
| sign |                           Integral part                                  |
-----------------------------------------------------------------------------------
   1                                    31
```

### L11L12
```
|31                                                                              0|
|                   L11                  |                   L12                  |
-----------------------------------------------------------------------------------
| sign | integral part | fractional part | sign | integral part | fractional part |
-----------------------------------------------------------------------------------
   1            3              12           1            3              12
```

### L13L21
```
|31                                                                              0|
|                   L13                  |                   L21                  |
-----------------------------------------------------------------------------------
| sign | integral part | fractional part | sign | integral part | fractional part |
-----------------------------------------------------------------------------------
   1            3              12           1            3              12
```

### L22L23
```
|31                                                                              0|
|                   L22                  |                   L23                  |
-----------------------------------------------------------------------------------
| sign | integral part | fractional part | sign | integral part | fractional part |
-----------------------------------------------------------------------------------
   1            3              12           1            3              12
```

### L31L32
```
|31                                                                              0|
|                   L31                  |                   L32                  |
-----------------------------------------------------------------------------------
| sign | integral part | fractional part | sign | integral part | fractional part |
-----------------------------------------------------------------------------------
   1            3              12           1            3              12
```

### L33
```
|31                                                                              0|
|                                        |                   R32                  |
-----------------------------------------------------------------------------------
|                  0                     | sign | integral part | fractional part |
-----------------------------------------------------------------------------------
                  16                        1            3              12
```

### RBK
```
|31                                                                              0|
-----------------------------------------------------------------------------------
| sign |        integral part            |            fractional part             |
-----------------------------------------------------------------------------------
   1                  19                                    12
```

### GBK
```
|31                                                                              0|
-----------------------------------------------------------------------------------
| sign |        integral part            |            fractional part             |
-----------------------------------------------------------------------------------
   1                  19                                    12
```

### BBK
```
|31                                                                              0|
-----------------------------------------------------------------------------------
| sign |        integral part            |            fractional part             |
-----------------------------------------------------------------------------------
   1                  19                                    12
```

### LR1LR2
```
|31                                                                              0|
|                   LR1                  |                   LR2                  |
-----------------------------------------------------------------------------------
| sign | integral part | fractional part | sign | integral part | fractional part |
-----------------------------------------------------------------------------------
   1            3              12           1            3              12
```

### LR3LG1
```
|31                                                                              0|
|                   LR3                  |                   LG1                  |
-----------------------------------------------------------------------------------
| sign | integral part | fractional part | sign | integral part | fractional part |
-----------------------------------------------------------------------------------
   1            3              12           1            3              12
```

### LG2LG3
```
|31                                                                              0|
|                   LG2                  |                   LG3                  |
-----------------------------------------------------------------------------------
| sign | integral part | fractional part | sign | integral part | fractional part |
-----------------------------------------------------------------------------------
   1            3              12           1            3              12
```

### LB1LB2
```
|31                                                                              0|
|                   LB1                  |                   LB2                  |
-----------------------------------------------------------------------------------
| sign | integral part | fractional part | sign | integral part | fractional part |
-----------------------------------------------------------------------------------
   1            3              12           1            3              12
```

### LB3
```
|31                                                                              0|
|                                        |                   LB3                  |
-----------------------------------------------------------------------------------
|                  0                     | sign | integral part | fractional part |
-----------------------------------------------------------------------------------
                  16                        1            3              12
```

### RFC
```
|31                                                                              0|
-----------------------------------------------------------------------------------
| sign |        integral part            |            fractional part             |
-----------------------------------------------------------------------------------
   1                  27                                     4
```

### GFC
```
|31                                                                              0|
-----------------------------------------------------------------------------------
| sign |        integral part            |            fractional part             |
-----------------------------------------------------------------------------------
   1                  27                                     4
```

### BFC
```
|31                                                                              0|
-----------------------------------------------------------------------------------
| sign |        integral part            |            fractional part             |
-----------------------------------------------------------------------------------
   1                  27                                     4
```

### OFX
```
|31                                                                              0|
-----------------------------------------------------------------------------------
| sign |        integral part            |            fractional part             |
-----------------------------------------------------------------------------------
   1                  15                                    16
```

### OFY
```
|31                                                                              0|
-----------------------------------------------------------------------------------
| sign |        integral part            |            fractional part             |
-----------------------------------------------------------------------------------
   1                  15                                    16
```

### H
```
|31                                                                              0|
|                                        |                    H                   |
-----------------------------------------------------------------------------------
|                  0                     |              integral part             |
-----------------------------------------------------------------------------------
                  16                                         16           
```

### DQA
```
|31                                                                              0|
|                                        |                   DQA                  |
-----------------------------------------------------------------------------------
|                  0                     | sign | integral part | fractional part |
-----------------------------------------------------------------------------------
                  16                        1            7               8
```

### DQB
```
|31                                                                              0|
|                                        |                   DQB                  |
-----------------------------------------------------------------------------------
|                  0                     | sign | integral part | fractional part |
-----------------------------------------------------------------------------------
                  16                        1            7               8
```

### ZSF3
```
|31                                                                              0|
|                                        |                   DQB                  |
-----------------------------------------------------------------------------------
|                  0                     | sign | integral part | fractional part |
-----------------------------------------------------------------------------------
                  16                        1            3              12
```

### ZSF4
```
|31                                                                              0|
|                                        |                   DQB                  |
-----------------------------------------------------------------------------------
|                  0                     | sign | integral part | fractional part |
-----------------------------------------------------------------------------------
                  16                        1            3              12
```

### FLAGS
```
31      Logical sum of bits 30-23 and bits 18-13
30      Calculation test result #1 overflow (2^43 or more)
29      Calculation test result #2 overflow (2^43 or more)
28      Calculation test result #3 overflow (2^43 or more)
27      Calculation test result #1 underflow (less than -2^43)
26      Calculation test result #2 underflow (less than -2^43)
25      Calculation test result #3 underflow (less than -2^43)
24      Limiter A1 out of range (less than 0, or less than -2^15, or 2^15 or more)
23      Limiter A2 out of range (less than 0, or less than -2^15, or 2^15, or more)
22      Limiter A3 out of range (less than 0, or less than -2^15, or 2^15, or more)
21      Limiter B1 out of range (less than 0, or 2^8 or more)
20      Limiter B2 out of range (less than 0, or 2^8 or more)
19      Limiter B3 out of range (less than 0, or 2^8 or more)
18      Limiter C out of range (less than 0, or 2^16 or more)
17      Divide overflow generated (quotient of 2.0 or more)
16      Calculation test result #4 overflow (2^31 or more)
15      Calculation test result #4 underflow (less than -2^31)
14      Limiter D1 out of range (less than 2^10, or 2^10 or more)
13      Limiter D2 out of range (less than 2^10, or 2^10 or more)
12      Limiter E out of range (less than 0, or 2^12 or more)
```

## Data Registers
- data registers consist of the other "half" of the GTE
- in some functions format are different from that one that's given here
- the numbers in the format fields are the signed, integer, and fractional parts of the field
    - 1,3,12 means signed(1 bit), 3 bits integeral part, 12 bits fractional part

| Number | Name | r/w | 31 - 16 | 15 - 0 | Format | Description |
| ------ | ---- | --- | ------- | ------ | ------ | ----------- |
| 0 | VXY0 | r/w | VY0 | VX0 | 1,3,12 or 1,15,0 | Vector 0 X and Y |
| 1 | VZ0 | r/w | 0 | VZ0 | 1,3,12 or 1,15,0 | Vector 0 Z |
| 2 | VXY1 | r/w | VY1 | VX1 | 1,3,12 or 1,15,0 | Vector 1 X and Y |
| 3 | VZ1 | r/w | 0 | VZ1 | 1,3,12 or 1,15,0 | Vector 1 Z |
| 4 | VXY2 | r/w | VY2 | VX2 | 1,3,12 or 1,15,0 | Vector 2 X and Y |
| 5 | VZ2 | r/w | 0 | VZ2 | 1,3,12 or 1,15,0 | Vector 2 Z |
| 6 | RGB | r/w | Code, R | G,B | 8 bits for each | RGB value. Code is passed, but not used in calculation |
| 7 | OTZ | r | 0 | OTZ | 0,15,0 | Z Average value |
| 8 | IR0 | r/w | Sign | IR0 | 1,3,12 | Intermediate value 0, Format may differ |
| 9 | IR1 | r/w | Sign | IR1 | 1,3,12 | Intermediate value 1, Format may differ |
| 10 | IR2 | r/w | Sign | IR2 | 1,3,12 | Intermediate value 2, Format may differ |
| 11 | IR3 | r/w | Sign | IR3 | 1,3,12 | Intermediate value 3, Format may differ |
| 12 | SXY0 | r/w | SX0 | SY0 | 1,15,0 | Screen XY coordinate FIFO (Note 1) |
| 13 | SXY1 | r/w | SX1 | SY1 | 1,15,0 | Screen XY coordinate FIFO |
| 14 | SXY2 | r/w | SX2 | SY2 | 1,15,0 | Screen XY coordinate FIFO |
| 15 | SXYP | r/w | SXP | SYP | 1,15,0 | Screen XY coordinate FIFO |
| 16 | SZ0 | r/w | 0 | SZ0 | 0,16,0 | Screen Z FIFO (Note 1) |
| 17 | SZ1 | r/w | 0 | SZ1 | 0,16,0 | Screen Z FIFO |
| 18 | SZ2 | r/w | 0 | SZ2 | 0,16,0 | Screen Z FIFO |
| 19 | SZ3 | r/w | 0 | SZ3 | 0,16,0 | Screen Z FIFO |
| 20 | RGB0 | r/w | CF0,B0 | G0,R0 | 8 bits each | Characteristic color FIFO (Note 1) |
| 21 | RGB1 | r/w | CD1,B1 | G1,R1 | 8 bits each | Characteristic color FIFO |
| 22 | RGB2 | r/w | CD2,B2 | G0,R2 | 8 bits each | CD2 is the bit pattern of currently executed function |
| 23 | RES1 | - | - | - | - | Prohibited |
| 24 | MAC0 | r/w | MAC0 | MAC0 | 1,31,0 | Sum of products value 1 |
| 25 | MAC1 | r/w | MAC1 | MAC1 | 1,31,0 | Sum of products value 1 |
| 26 | MAC2 | r/w | MAC2 | MAC2 | 1,31,0 | Sum of products value 1 |
| 27 | MAC3 | r/w | MAC3 | MAC3 | 1,31,0 | Sum of products value 1 |
| 28 | IRGB | w | 0 | IB,IG,IR | Note 2 | Note 2 |
| 29 | ORGB | r | 0 | 0B,0G,0R | Note 3 | Note 3 |
| 30 | LZCS | w | LZCS | LZCS | 1,31,0 | Leading zero count source data (Note 4) |
| 31 | LZCR | r | LZCR | LZCR | 6,6,0 | Leading zero count result (Note 4) |

### Note 1
```
The SXYx, SZx, and RGBx are FIFO. The last calculation result is stored
in the last regiser, and previous results are stored in previous registers.
So for example when a new SXY value is obtained the following happens:
    SXY0 = SXY1
    SXY1 = SXY2
    SXY2 = SXYP
    SXYP = result
```

### Note 2
```
                    IRGB
|31     15|14       10|9        5|4     0|
------------------------------------------
|    0    |     R     |     G    |   B   |
------------------------------------------

When writing a value to IRGB the following happens:
IR1 = IR format converted to (1,11,4)
IR2 = IG format converted to (1,11,4)
IR3 = IB format converted to (1,11,4)
```

### Note 3
```
                    0RGB
|31     15|14       10|9        5|4     0|
------------------------------------------
|    0    |     R     |     G    |   B   |
------------------------------------------

When writing a value to IRGB the following happens:
IR = (IR1 >> 7) & 0x1f
IG = (IR2 >> 7) & 0x1f
IB = (IR3 >> 7) & 0x1f
```

### Note 4
- reading LZCR returns the leading 0 count of LZCS if LZCS is positive and the leading 1 count of LZCS if LZCS is negative

## GTE Commands
- describes the calculations performed by GTE functions
- IN refers to registers needed for calculation
- OUT refers to registers modified from calculation
- calculation format on left side is size in which data is stored
- calc format on right side is size in which the calculation is performed
- results which affect flags will be denoted by brackets on the right side of the equal sign
- Lm_ identifier means the value is limited to the bottom or ceiling of the check if it exceeds the boundary

### FLAGS (again)
```
bit         name - description
31          checksum
30          A1 - Result larger than 43 bits and positive
29          A2 - Result larger than 43 bits and positive
28          A3 - Result larger than 43 bits and positive
27          A1 - Result larger than 43 bits and negative
26          A2 - Result larger than 43 bits and negative
25          A3 - Result larger than 43 bits and negative
24          B1 - Value negative (lm=1) or larger than 15 bits (lm=0)
23          B2 - Value negative (lm=1) or larger than 15 bits (lm=0)
22          B3 - Value negative (lm=1) or larger than 15 bits (lm=0)
21          C1 - Value negative or larger than 8 bits
20          C2 - Value negative or larger than 8 bits
19          C3 - Value negative or larger than 8 bits
18          D  - Value negative or larger than 16 bits
17          E  - Divide overflow (quotient > 2.0)
16          F  - Result larger than 31 bits and positive
15          F  - Result larger than 31 bits and negative
14          G1 - Value larger than 10 bits
13          G2 - Value larger than 10 bits
12          H  - Value negative or larger than 12 bits
```

### Field descriptions
```
|24     20| 19 |18     17|16    15|14   13|12   11| 10 |9       0|
------------------------------------------------------------------
|         | sf |    mx   |    v   |   cv  |       | lm |         |
------------------------------------------------------------------

sf      0   vector format (1,31,0)
        1   vector format (1,19,12)
mx      0   multiply with rotation matrix
        1   multipy with light matrix
        2   multiply with color matrix
        3   unknown
v       0   V0 source vector (short)
        1   V1 source vector (short)
        2   V2 source vector (short)
        3   IR source vector (long)
cv      0   Add translation vector
        1   Add back color vector
        2   Unknown
        3   Add no vector
lm      0   No negative limit
        1   Limit negative results to 0
```


### RTPS
```
Cycles:         15
Command:        cop2 0x018_0001
Description:    Perspective transform

Fields: None
In:     V0          Vector to transform         [1,15,0]
        R           Rotation matrix             [1,3,12]
        TR          Translation vector          [1,31,0]
        H           View plane distance         [0,16,0]
        DQA         Depth que interpolation val [1,7,8]
        DQB
        OFX         Screen offset values        [1,15,16]
        OFY
Out:    SXY fifo    Screen XY coords (short)    [1,15,0]
        SZ  fifo    Screen Z coords (short)     [0,16,0]
        IR0         Interpolation val for       [1,3,12]
                    depth queing                
        IR1         Screen X (short)            [1,15,0]
        IR2         Screen Y (Short)            [1,15,0]
        IR3         Screen Z (short)            [1,15,0]
        MAC1        Screen X (long)             [1,31,0]
        MAC2        Screen Y (long)             [1,31,0]
        MAC3        Screen Z (long)             [1,31,0]

Calculation:
[1,31,0] MAC1 = A1[TRX + R11*VX0 + R12*VY0 + R13*VZ0]       [1,31,12]
[1,31,0] MAC2 = A2[TRY + R21*VX0 + R22*VY0 + R23*VZ0]       [1,31,12]
[1,31,0] MAC3 = A3[TRZ + R31*VX0 + R32*VY0 + R33*VZ0]       [1,31,12]

Notes:
Z values are limited downwards at 0.5 * H. For smaller z values you'll
have write your own routine.
```

### RTPT
```
Cycles: 23
Command: 0x028_0030
Description: Perspective transform on 3 points

Fields: None
In:     V0          Vector to transform         [1,15,0]
        V1
        V2
        R           Rotation matrix             [1,3,12]
        TR          Translation vector          [1,31,0]
        H           View plane distance         [0,16,0]
        DQA         Depth que interpolation val [1,7,8]
        DQB
        OFX         Screen offset values        [1,15,16]
        OFY
Out:    SXY fifo    Screen XY coord (short)     [1,15,0]
        SZ  fifo    Screen Z coord (short)      [0,16,0]
        IR0         Interpolation value for     [1,3,12]
                    depth queing
        IR1         Screen X (short)            [1,15,0]
        IR2         Screen Y (short)            [1,15,0]
        IR3         Screen Z (short)            [1,15,0]
        MAC1        Screen X (long)             [1,31,0]
        MAC2        Screen Y (long)             [1,31,0]
        MAC3        Screen Z (long)             [1,31,0]

Calculation: Same as RTPS, but repeats for V1 and V2
```

### MVMVA
```
Cycles: 8
Command: cop2 0x040_0012
Description: Multiply vector by matrix and vector addition

Fields: sf, mx, v, cv, lm
In:     V0/V1/V2/IR     Vector v0, v1, v2 or [IR1, IR2, IR3]
        R/LLM/LCM       Rotation, light or color matrix     [1,3,12]
        TR/BK           Translation or background color vector
Out:    [IR1, IR2, IR3] Short vector
        [MAC1, MAC2, MAC3] Long vector

Calculation:
MX = matrix specified by mx
V = vector specified by v
CV = vector specified by cv

MAC1 = A1[CV1 + MX11*V1 + MX12*V2 + MX13*V3]
MAC2 = A2[CV2 + MX21*V1 + MX22*V2 + MX23*V3]
MAC3 = A3[CV3 + MX31*V1 + MX32*V2 + MX33*V3]
IR1 = Lm_B1[MAC1]
IR2 = Lm_B2[MAC2]
IR3 = Lm_B3[MAC3]

Notes:
The cv field allows selectoin of the far color vector, but this vector
is not added correctly by the GTE.
```

### DPCL
```
Cycles: 8
Command: cop2 0x068_0029
Description: Depth Cue Color light

Fields: None
In:     RGB             Primary color   R,G,B,CODE  [0,8,0]
        IR0             Interpolation value         [1,3,12]
        [IR1,IR2,IR3]   Local color vector        [1,3,12]
        CODE            Code value from RGB    CODE [0,8,0]
        FC              Far color                   [1,27,4]
Out:    RGBn            RGB fifo        Rn,Gn,Bn,CDn[0,8,0]
        [IR1,IR2,IR3]   Color vector                [1,11,4]
        [MAC1,MAC2,MAC3]Color Vector                [1,27,4]

Calculation:
[1,27,4]    MAC1 = A1[R*IR1 + IR0*(Lm_B1[RFC - R * IR1])]   [1,27,16]
[1,27,4]    MAC2 = A2[G*IR2 + IR0*(Lm_B1[GFC - G * IR2])]   [1,27,16]
[1,27,4]    MAC3 = A3[B*IR3 + IR0*(Lm_B1[BFC - B * IR3])]   [1,27,16]
[1,11,4]    IR1 = Lm_B1[MAC1]                               [1,27,4]
[1,11,4]    IR2 = Lm_B2[MAC2]                               [1,27,4]
[1,11,4]    IR3 = Lm_B3[MAC3]                               [1,27,4]
[0,8,0]     Cd0<-Cd1<-Cd2<-CODE
[0,8,0]     R0<-R1<-R2<-Lm_C1[MAC1]                         [1,27,4]
[0,8,0]     G0<-G1<-G2<-Lm_C2[MAC2]                         [1,27,4]
[0,8,0]     B0<-B1<-B2<-Lm_C3[MAC3]                         [1,27,4]
```

### DPCS
```
Cycles: 8
Command: cop2 0x078_0010
Description: Depth Cueing

Fields: None
In:     IR0             Interpolation value         [1,3,12]
        RGB             Color           R,G,B,CODE  [0,8,0]
        FC              Far color      RFC,GFC,BFC  [1,27,4]
Out:    RGBn            RGB fifo       Rn,Gn,CDn    [0,8,0]
        [IR1,IR2,IR3]   Color vector                [1,11,4]
        [MAC1,MAC2,MAC3] Color vector               [1,27,4]

Calculations:
[1,27,4]    MAC1 = A1[(R + IR0*(Lm_B1[RFC - R]))]   [1,27,16][lm=0]
[1,27,4]    MAC2 = A2[(G + IR0*(Lm_B1[GFC - G]))]   [1,27,16][lm=0]
[1,27,4]    MAC3 = A3[(B + IR0*(Lm_B1[BFC - B]))]   [1,27,16][lm=0]
[1,11,4]    IR1 = Lm_B1[MAC1]                       [1,27,4][lm=0]
[1,11,4]    IR2 = Lm_B2[MAC2]                       [1,27,4][lm=0]
[1,11,4]    IR3 = Lm_B3[MAC3]                       [1,27,4][lm=0]
[0,8,0]     Cd0<-Cd1<-Cd2<-CODE
[0,8,0]     R0<-R1<-R2<-Lm_C1[MAC1]                 [1,27,4]
[0,8,0]     G0<-G1<-G2<-Lm_C2[MAC2]                 [1,27,4]
[0,8,0]     B0<-B1<-B2<-Lm_C3[MAC3]                 [1,27,4]
```

### DPCT
```
Cycles: 17
Command: cop2 0x0f8_002A
Description: Depth cue color RGB0,RGB1,RGB2

Fields: None
In:     IR0             Interpolation value             [1,3,12]
        RGB0,RGB1,RGB2  Colors in RGB fifo   Rn,Gn,CDn  [0,8,0]
        FC              Far color           RFC,GFC,BFC [1,27,4]
Out:    RGBn            RGB fifo             Rn,Gn,CDn  [0,8,0]
        [IR1,IR2,IR3]   Color vector                    [1,11,4]
        [MAC1,MAC2,MAC3] Color vector                   [1,27,4]

Calculations:
[1,27,4]  MAC1 = A1[R0 + IR0*(Lm_B1[RFC - R0])]         [1,27,16][lm=0]
[1,27,4]  MAC2 = A2[G0 + IR0*(Lm_B1[GFC - G0])]         [1,27,16][lm=0]
[1,27,4]  MAC3 = A3[B0 + IR0*(Lm_B1[BFC - B0])]         [1,27,16][lm=0]
[1,11,4]  IR1 = Lm_B1[MAC1]                             [1,27,4][lm=0]
[1,11,4]  IR2 = Lm_B2[MAC2]                             [1,27,4][lm=0]
[1,11,4]  IR1 = Lm_B3[MAC3]                             [1,27,4][lm=0]
[0,8,0]   Cd0<-Cd1<-Cd2<-CODE
[0,8,0]   R0<-R1<-R2<-Lm_C1[MAC1]                       [1,27,4]
[0,8,0]   G0<-G1<-G2<-Lm_C2[MAC2]                       [1,27,4]
[0,8,0]   B0<-B1<-B2<-Lm_C3[MAC3]                       [1,27,4]

Performs this calculation 3 times so all three RGB values have been
replaced by the depth cued RGB values
```

### INTPL
```
Cycles: 8
Command: cop2 0x098_0011
Description: Interpolation of vector and far color

Fields: None
In:     [IR1,IR2,IR3]   Vector                      [1,3,12]
        IR0             Interpolation value         [1,3,12]
        CODE            Code value from RGB         [0,8,0]
        FC              Far color      RFC,GFC,BFC  [1,27,4]
Out:    RGBn            RGB fifo       Rn,Gn,Bn,CDn [0,8,0]
        [IR1,IR2,IR3]   Color vector                [1,11,4]
        [MAC1,MAC2,MAC3] Color vector               [1,27,4]

Calculations:
[1,27,4]  MAC1 = A1[IR1 + IR0*(Lm_B1[RFC - IR1])]   [1,27,16]
[1,27,4]  MAC2 = A2[IR2 + IR0*(Lm_B1[GFC - IR2])]   [1,27,16]
[1,27,4]  MAC3 = A3[IR3 + IR0*(Lm_B1[BFC - IR3])]   [1,27,16]
[1,11,4]  IR1 = Lm_B1[MAC1]                         [1,27,4]
[1,11,4]  IR2 = Lm_B2[MAC2]                         [1,27,4]
[1,11,4]  IR3 = Lm_B3[MAC3]                         [1,27,4]
[0,8,0]   R0<-R1<-R2<-Lm_C1[MAC1]                   [1,27,4]
[0,8,0]   G0<-G1<-G2<-Lm_C2[MAC2]                   [1,27,4]
[0,8,0]   B0<-B1<-B2<-Lm_C3[MAC3]                   [1,27,4]
```

### SQR
```
Cycles: 5
Command: cop2 0x0A0_0428
Description: Square of vector

Fields: sf
In:     [IR1,IR2,IR3]       vector          [1,15,0][1,3,12]
Out:    [IR1,IR2,IR3]       vector^2        [1,15,0][1,3,12]
        [MAC1,MAC2,MAC3]    vector^2        [1,31,0][1,19,12]

Calculation: (left format sf=0, right format sf=1)
[1,31,0][1,19,12]  MAC1 = A1[IR1*IR2]       [1,43,0][1,31,12]
[1,31,0][1,19,12]  MAC2 = A2[IR2*IR2]       [1,43,0][1,31,12]
[1,31,0][1,19,12]  MAC3 = A3[IR3*IR3]       [1,43,0][1,31,12]
[1,15,0][1,3,12]   IR1 = Lm_B1[MAC1]        [1,31,0][1,19,12][lm=1]
[1,15,0][1,3,12]   IR2 = Lm_B2[MAC2]        [1,31,0][1,19,12][lm=1]
[1,15,0][1,3,12]   IR3 = Lm_B3[MAC3]        [1,31,0][1,19,12][lm=1]
```

### NCS
```
Cycles: 14
Command: cop2 0x0C8_041E
Description: Normal color v0

Fields: None
In:     V0              Normal Vector                   [1,3,12]
        BK              Background color    RBK,GBK,BBK [1,19,12]
        CODE            Code value from RGB     CODE    [0,8,0]
        LCM             Color matrix                    [1,3,12]
        LLM             Light matrix                    [1,3,12]
Out:    RGBn            RGB fifo           Rn,Gn,Bn,CDn [0,8,0]
        [IR1,IR2,IR3]   Color vector                    [1,11,4]
        [MAC1,MAC2,MAC3] Color vector                   [1,27,4]

Calculations:
[1,19,12]  MAC1 = A1[L11*VX0 + L12*VY0 + L13*VZ0]       [1,19,24]
[1,19,12]  MAC2 = A2[L21*VX0 + L22*VY0 + L23*VZ0]       [1,19,24]
[1,19,12]  MAC3 = A3[L31*VX0 + L32*VY0 + L33*VZ0]       [1,19,24]
[1,3,12]   IR1 = Lm_B1[MAC1]                            [1,19,12][lm=1]
[1,3,12]   IR2 = Lm_B2[MAC2]                            [1,19,12][lm=1]
[1,3,12]   IR3 = Lm_B3[MAC3]                            [1,19,12][lm=1]
[1,19,12]  MAC1 = A1[RBK + LR1*IR1 + LR2*IR2 + LR3*IR3] [1,19,24]
[1,19,12]  MAC2 = A2[GBK + LG1*IR1 + LG2*IR2 + LG3*IR3] [1,19,24]
[1,19,12]  MAC3 = A3[BBK + LB1*IR1 + LB2*IR2 + LB3*IR3] [1,19,24]
[1,3,12]   IR1 = Lm_B1[MAC1]                            [1,19,12][lm=1]
[1,3,12]   IR2 = Lm_B1[MAC2]                            [1,19,12][lm=1]
[1,3,12]   IR3 = Lm_B1[MAC3]                            [1,19,12][lm=1]
[0,8,0]   Cd0<-Cd1<-Cd2<-CODE
[0,8,0]   R0<-R1<-R2<-Lm_C1[MAC1]                       [1,27,4]
[0,8,0]   G0<-G1<-G2<-Lm_C2[MAC2]                       [1,27,4]
[0,8,0]   B0<-B1<-B2<-Lm_C3[MAC3]                       [1,27,4]
```

### NCT
```
Cycles: 30
Command: cop2 0x0D8_0420
Description: Normal color v0, v1, v2

Fields: None
In:     V0,V1,V2        Normal Vector                   [1,3,12]
        BK              Background color    RBK,GBK,BBK [1,19,12]
        CODE            Code value from RGB     CODE    [0,8,0]
        LCM             Color matrix                    [1,3,12]
        LLM             Light matrix                    [1,3,12]
Out:    RGBn            RGB fifo           Rn,Gn,Bn,CDn [0,8,0]
        [IR1,IR2,IR3]   Color vector                    [1,11,4]
        [MAC1,MAC2,MAC3] Color vector                   [1,27,4]

Calculations:
Same as NCS, but repeated for V1 and V2
```

### NCDS
```
Cycles: 19
Command: cop2 0x0E8_0413
Description: Normal color depth cuev0

Fields: None
In:     V0              Normal vector                   [1,3,12]
        BK              Background color    RBK,GBK,BBK [1,19,12]
        RGB             Primary color       R,G,B,CODE  [0,8,0]
        LLM             Light matrix                    [1,3,12]
        LCM             Color matrix                    [1,3,12]
        IR0             Interpolation value             [1,3,12]
Out:    RGBn            RGB fifo          Rn,Gn,Bn,CDn  [0,8,0]
        [IR1,IR2,IR3]   Color vector                    [1,11,4]
        [MAC1,MAC2,MAC3] Color vector                   [1,27,4]

Calculation:
[1,19,12]  MAC1 = A1[L11*VX0 + L12*VY0 + L13*VZ0]       [1,19,24]
[1,19,12]  MAC2 = A1[L21*VX0 + L22*VY0 + L23*VZ0]       [1,19,24]
[1,19,12]  MAC3 = A1[L31*VX0 + L32*VY0 + L33*VZ0]       [1,19,24]
[1,3,12]   IR1 = Lm_B1[MAC1]                            [1,19,12][lm=1]
[1,3,12]   IR2 = Lm_B2[MAC2]                            [1,19,12][lm=1]
[1,3,12]   IR3 = Lm_B3[MAC3]                            [1,19,12][lm=1]
[1,19,12]  MAC1 = A1[RBK + LR1*IR1 + LR2*IR2 + LR3*IR3] [1,19,24]
[1,19,12]  MAC2 = A1[GBK + LG1*IR1 + LG2*IR2 + LG3*IR3] [1,19,24]
[1,19,12]  MAC3 = A1[BBK + LB1*IR1 + LB2*IR2 + LB3*IR3] [1,19,24]
[1,3,12]   IR1 = Lm_B1[MAC1]                            [1,19,12][lm=1]
[1,3,12]   IR2 = Lm_B2[MAC2]                            [1,19,12][lm=1]
[1,3,12]   IR3 = Lm_B3[MAC3]                            [1,19,12][lm=1]
[1,27,4]   MAC1 = A1[R*IR1 + IR0*(Lm_B1[RFC - R*IR1])]  [1,27,16][lm=0]
[1,27,4]   MAC2 = A1[G*IR2 + IR0*(Lm_B2[GFC - G*IR2])]  [1,27,16][lm=0]
[1,27,4]   MAC3 = A1[B*IR3 + IR0*(Lm_B3[BFC - B*IR3])]  [1,27,16][lm=0]
[1,3,12]   IR1 = Lm_B1[MAC1]                            [1,27,4][lm=1]
[1,3,12]   IR2 = Lm_B2[MAC2]                            [1,27,4][lm=1]
[1,3,12]   IR3 = Lm_B3[MAC3]                            [1,27,4][lm=1]
[0,8,0]   Cd0<-Cd1<-Cd2<-CODE
[0,8,0]   R0<-R1<-R2<-Lm_C1[MAC1]                       [1,27,4]
[0,8,0]   G0<-G1<-G2<-Lm_C2[MAC2]                       [1,27,4]
[0,8,0]   B0<-B1<-B2<-Lm_C3[MAC3]                       [1,27,4]
```

### NCDT
```
Cycles: 44
Command: cop2 0x0F8_0416
Description: Normal color depth cue v0,v1,v2

Fields: None
In:     V0              Normal vector                   [1,3,12]
        V1              Normal vector                   [1,3,12]
        V2              Normal vector                   [1,3,12]
        BK              Background color    RBK,GBK,BBK [1,19,12]
        FC              Far color           RFC,GFC,BFC [1,27,4]
        RGB             Primary color       R,G,B,CODE  [0,8,0]
        LLM             Light matrix                    [1,3,12]
        LCM             Color matrix                    [1,3,12]
        IR0             Interpolation value             [1,3,12]
Out:    RGBn            RGB fifo        Rn,Gn,Bn,CDn    [0,8,0]
        [IR1,IR2,IR3]   Color vector                    [1,11,4]
        [MAC1,MAC2,MAC3] Color vector                   [1,27,4]

Calculation:
Same as NCDS but repeats for v1 and v2
```

### NCCS
```
Cycles: 17
Command: cop2 0x108_041B
Description: Normal color col. v0

Fields: None
In:     V0              Normal vector                   [1,3,12]
        BK              Background color    RBK,GBK,BBK [1,19,12]
        RGB             Primary color       R,G,B,CODE  [0,8,0]
        LLM             Light matrix                    [1,3,12]
        LCM             Color matrix                    [1,3,12]
Out:    RGBn            RGB fifo        Rn,Gn,Bn,CDn    [0,8,0]
        [IR1,IR2,IR3]   Color vector                    [1,11,4]
        [MAC1,MAC2,MAC3] Color vector                   [1,27,4]

Calculation:
[1,19,12]  MAC1 = A1[L11*VX0 + L12*VY0 + L13*VZ0]       [1,19,24]
[1,19,12]  MAC2 = A2[L21*VX0 + L22*VY0 + L23*VZ0]       [1,19,24]
[1,19,12]  MAC3 = A3[L31*VX0 + L32*VY0 + L33*VZ0]       [1,19,24]
[1,3,12]   IR1 = Lm_B1[MAC1]                            [1,19,12][lm=1]
[1,3,12]   IR2 = Lm_B2[MAC2]                            [1,19,12][lm=1]
[1,3,12]   IR3 = Lm_B3[MAC3]                            [1,19,12][lm=1]
[1,19,12]  MAC1 = A1[RBK + LR1*IR1 + LR2*IR2 + LR3*IR3] [1,19,24]
[1,19,12]  MAC2 = A2[GBK + LG1*IR1 + LG2*IR2 + LG3*IR3] [1,19,24]
[1,19,12]  MAC3 = A3[BBK + LB1*IR1 + LB2*IR2 + LB3*IR3] [1,19,24]
[1,3,12]   IR1 = Lm_B1[MAC1]                            [1,19,12][lm=1]
[1,3,12]   IR2 = Lm_B2[MAC2]                            [1,19,12][lm=1]
[1,3,12]   IR3 = Lm_B3[MAC3]                            [1,19,12][lm=1]
[1,27,4]   MAC1 = A1[R*IR1]                             [1,27,16]
[1,27,4]   MAC2 = A2[G*IR2]                             [1,27,16]
[1,27,4]   MAC3 = A3[B*IR3]                             [1,27,16]
[1,3,12]   IR1 = Lm_B1[MAC1]                            [1,27,4][lm=1]
[1,3,12]   IR2 = Lm_B2[MAC2]                            [1,27,4][lm=1]
[1,3,12]   IR3 = Lm_B3[MAC3]                            [1,27,4][lm=1]
[0,8,0]   Cd0<-Cd1<-Cd2<-CODE
[0,8,0]   R0<-R1<-R2<-Lm_C1[MAC1]                       [1,27,4]
[0,8,0]   G0<-G1<-G2<-Lm_C2[MAC2]                       [1,27,4]
[0,8,0]   B0<-B1<-B2<-Lm_C3[MAC3]                       [1,27,4]
```

### NCCT
```
Cycles: 39
Command: cop2 0x118_043F
Description: Normal color col. v0,v1,v2

Fields: None
In:     V0              Normal vector 1                 [1,3,12]
        V1              Normal vector 2                 [1,3,12]
        V2              Normal vector 3                 [1,3,12]
        BK              Background color    RBK,GBK,BBK [1,19,12]
        RGB             Primary color       R,G,B,CODE  [0,8,0]
        LLM             Light matrix                    [1,3,12]
        LCM             Color matrix                    [1,3,12]
Out:    RGBn            RGB fifo        Rn,Gn,Bn,CDn    [0,8,0]
        [IR1,IR2,IR3]   Color vector                    [1,11,4]
        [MAC1,MAC2,MAC3] Color vector                   [1,27,4]

Calculation:
Same as NCCS but repeats for v1 and v2
```

### CDP
```
Cycles: 13
Command: cop2 0x128_0414
Description: Color Depth Queue

Fields: None
In:     [IR1,IR2,IR3]   Vector                          [1,3,12]
        RGB             Primary color       R,G,B,CODE  [0,8,0]
        IR0             Interpolation value             [1,3,12]
        BK              Background color    RBK,GBK,BBK [1,19,12]
        LCM             Color matrix                    [1,3,12]
        FC              Far color           RFC,GFC,BFC [1,27,4]
Out:    RGBn            RGB fifo        Rn,Gn,Bn,CDn    [0,8,0]
        [IR1,IR2,IR3]   Color vector                    [1,11,4]
        [MAC1,MAC2,MAC3] Color vector                   [1,27,4]

Calculation:
[1,19,12]  MAC1 = A1[RBK + LR1*IR1 + LR2*IR2 + LR3*IR3] [1,19,24]
[1,19,12]  MAC1 = A1[RBK + LR1*IR1 + LR2*IR2 + LR3*IR3] [1,19,24]
[1,19,12]  MAC1 = A1[RBK + LR1*IR1 + LR2*IR2 + LR3*IR3] [1,19,24]
[1,3,12]   IR1 = Lm_B1[MAC1]                            [1,19,12][lm=1]
[1,3,12]   IR2 = Lm_B2[MAC2]                            [1,19,12][lm=1]
[1,3,12]   IR3 = Lm_B3[MAC3]                            [1,19,12][lm=1]
[1,27,4]   MAC1 = A1[R*IR1 + IR0*(Lm_B1[RFC - R*IR1])]  [1,27,16][lm=0]
[1,27,4]   MAC2 = A2[G*IR2 + IR0*(Lm_B2[GFC - G*IR2])]  [1,27,16][lm=0]
[1,27,4]   MAC3 = A3[B*IR3 + IR0*(Lm_B3[BFC - B*IR3])]  [1,27,16][lm=0]
[1,3,12]   IR1 = Lm_B1[MAC1]                            [1,27,4][lm=1]
[1,3,12]   IR2 = Lm_B2[MAC2]                            [1,27,4][lm=1]
[1,3,12]   IR3 = Lm_B3[MAC3]                            [1,27,4][lm=1]
[0,8,0]   Cd0<-Cd1<-Cd2<-CODE
[0,8,0]   R0<-R1<-R2<-Lm_C1[MAC1]                       [1,27,4]
[0,8,0]   G0<-G1<-G2<-Lm_C2[MAC2]                       [1,27,4]
[0,8,0]   B0<-B1<-B2<-Lm_C3[MAC3]                       [1,27,4]
```

### CC
```
Cycles: 11
Command: cop2 0x138_041C
Description: Color Col.

Fields: None
In:     [IR1,IR2,IR3]   Vector                          [1,3,12]
        BK              Background color    RBK,GBK,BBK [1,19,12]
        RGB             Primary color       R,G,B,CODE  [0,8,0]
        LCM             Color matrix                    [1,3,12]
Out:    RGBn            RGB fifo        Rn,Gn,Bn,CDn    [0,8,0]
        [IR1,IR2,IR3]   Color vector                    [1,11,4]
        [MAC1,MAC2,MAC3] Color vector                   [1,27,4]

Calculations:
[1,19,12]  MAC1 = A1[RBK + LR1*IR1 + LR2*IR2 + LR3*IR3] [1,19,24]
[1,19,12]  MAC2 = A2[GBK + LG1*IR1 + LG2*IR2 + LG3*IR3] [1,19,24]
[1,19,12]  MAC3 = A3[BBK + LB1*IR1 + LB2*IR2 + LB3*IR3] [1,19,24]
[1,3,12]   IR1 = Lm_B1[MAC1]                            [1,19,12][lm=1]
[1,3,12]   IR2 = Lm_B2[MAC2]                            [1,19,12][lm=1]
[1,3,12]   IR3 = Lm_B3[MAC3]                            [1,19,12][lm=1]
[1,27,4]   MAC1 = A1[R*IR1]                             [1,27,16]
[1,27,4]   MAC2 = A2[G*IR2]                             [1,27,16]
[1,27,4]   MAC3 = A3[B*IR3]                             [1,27,16]
[1,3,12]   IR1 = Lm_B1[MAC1]                            [1,19,12][lm=1]
[1,3,12]   IR2 = Lm_B2[MAC2]                            [1,19,12][lm=1]
[1,3,12]   IR3 = Lm_B3[MAC3]                            [1,19,12][lm=1]
[0,8,0]   Cd0<-Cd1<-Cd2<-CODE
[0,8,0]   R0<-R1<-R2<-Lm_C1[MAC1]                       [1,27,4]
[0,8,0]   G0<-G1<-G2<-Lm_C2[MAC2]                       [1,27,4]
[0,8,0]   B0<-B1<-B2<-Lm_C3[MAC3]                       [1,27,4]
```

### NCLIP
```
Cycles: 8
Command: cop2 0x140_0006
Description: Normal clipping

Fields: None
In:     SXY0,SXY1,SXY2      Screen Coordinates          [1,15,0]
Out:    MAC0                Outerproduct of SXY1 and    [1,31,0]
                            SXY2 with SXY0 as origin

Calculation:
[1,31,0]  MAC0 = F[SX0*SY1 + SX1*SY2 + SX2*SY0 - SX0*SY2 - SX1*SY0 - SX2*SY1]  [1,43,0]
```

### AVSZ3
```
Cycles: 5
Command: cop2 0x158_002D
Description: Average of three Z values

Fields: None
In:     SZ1,SZ2,SZ3         Z-Values                    [0,16,0]
        ZSF3                Divider                     [1,3,12]
Out:    OTZ                 Average                     [0,16,0]
        MAC0                Average                     [1,31,0]

Calculation:
[1,31,0]  MAC0 = F[ZSF3*SZ1 + ZSF3*SZ2 + ZSF3*SZ3]      [1,31,12]
[0,16,0]  OTZ = Lm_D[MAC0]                              [1,31,0]
```

### AVSZ4
```
Cycles: 6
Command: cop2 0x168_002E
Description: Average of four Z values

Fields: None
In:     SZ1,SZ2,SZ3,SZ4     Z-Values                [0,16,0]
        ZSF4                Divider                 [1,3,12]
Out:    OTZ                 Average                 [0,16,0]
        MAC0                Average                 [1,31,0]

Calculation:
[1,31,0]  MAC0 = F[ZSF4*SZ0 + ZSF4*SZ1 + ZSF4*SZ2 + ZSF4*SZ3]   [1,31,12]
[0,16,0]  OTZ = Lm_D[MAC0]                                      [1,31,0]
```

### OP
```
Cycles: 6
Command: cop2 0x170_000C
Description: Outer Product

Fields: sf
In:     [R11R12, R22R23, R33]   Vector 1
        [IR1,IR2,IR3]           Vector 2
Out:    [IR1,IR2,IR3]           Outer product
        [MAC1,MAC2,MAC3]        Outer product

Calculation: (D1=R11R12, D2=R22R23, D3=R33)
    MAC1 = A1[D2*IR3 - D3*IR2]
    MAC2 = A2[D3*IR1 - D1*IR3]
    MAC3 = A3[D1*IR2 - D2*IR1]
    IR1 = Lm_B1[MAC0]
    IR2 = Lm_B2[MAC1]
    IR3 = Lm_B3[MAC2]
```

### GPF
```
Cycles: 6
Command: cop2 0x190_003D
Description: General purpose interpolation

Fields: sf
In:     IR0             Scaling factor
        CODE            code field of RGB
        [IR1,IR2,IR3]   vector
Out:    [IR1,IR2,IR3]   vector
        [MAC1,MAC2,MAC3] vector
        RGB2            RGB fifo

Calculation:
          MAC1 = A1[IR0 * IR1]
          MAC2 = A2[IR0 * IR2]
          MAC3 = A3[IR0 * IR3]
          IR1 = Lm_B1[MAC1]
          IR2 = Lm_B2[MAC2]
          IR3 = Lm_B3[MAC3]
[0,8,0]   Cd0<-Cd1<-Cd2<-CODE
[0,8,0]   R0<-R1<-R2<-Lm_C1[MAC1]                       [1,27,4]
[0,8,0]   G0<-G1<-G2<-Lm_C2[MAC2]                       [1,27,4]
[0,8,0]   B0<-B1<-B2<-Lm_C3[MAC3]                       [1,27,4]
```

### GPL
```
Cycles: 5
Command: cop2 0x1A0_003E
Description: general purpose interpolation

Fields: sf
In:     IR0             Scaling factor
        CODE            Code field of RGB
        [IR1,IR2,IR3]   vector
        [MAC,MAC2,MAC3] vector
Out:    [IR1,IR2,IR3]   vector
        [MAC1,MAC2,MAC3] vector
        RGB2            RGB fifo

Calculation:
        MAC1 = A1[MAC1 + IR0*IR1]
        MAC2 = A2[MAC2 + IR0*IR2]
        MAC3 = A3[MAC3 + IR0*IR3]
        IR1 = Lm_B1[MAC1]
        IR2 = Lm_B2[MAC2]
        IR3 = Lm_B3[MAC3]
[0,8,0]   Cd0<-Cd1<-Cd2<-CODE
[0,8,0]   R0<-R1<-R2<-Lm_C1[MAC1]                       [1,27,4]
[0,8,0]   G0<-G1<-G2<-Lm_C2[MAC2]                       [1,27,4]
[0,8,0]   B0<-B1<-B2<-Lm_C3[MAC3]                       [1,27,4]
```

## List of Common MVMVA Instructions
| Name | Command | Description |
| ---- | ------- | ----------- |
| rtv0 | cop2 0x048_6012 | v0 * rotmatrix |
| rtv1 | cop2 0x048_E012 | v1 * rotmatrix |
| rtv2 | cop2 0x049_6012 | v2 * rotmatrix |
| rtir12 | cop2 0x049_E012 | ir * rotmatrix |
| rtir0 | cop2 0x041_E012 | ir * rotmatrix |
| rtv0tr | cop2 0x048_0012 | v0 * rotmatrix + tr vector |
| rtv1tr | cop2 0x048_8012 | v1 * rotmatrix + tr vector |
| rtv2tr | cop2 0x049_0012 | v2 * rotmatrix + tr vector |
| rtirtr | cop2 0x049_8012 | ir * rotmatrix + tr vector |
| rtv0bk | cop2 0x048_2012 | v0 * rotmatrix + bk vector |
| rtv1bk | cop2 0x048_A012 | v1 * rotmatrix + bk vector |
| rtv2bk | cop2 0x049_2012 | v2 * rotmatrix + bk vector |
| rtirbk | cop2 0x049_A012 | ir * rotmatrix + bk vector |
| ll | cop2 0x04A_6412 | v0 * light matrix. Lower limit result to 0 |
| llv0 | cop2 0x04A_6012 | v0 * light matrix |
| llv1 | cop2 0x04A_E012 | v1 * light matrix |
| llv2 | cop2 0x04B_6012 | v2 * light matrix |
| llvir | cop2 0x04B_E012 | ir * light matrix |
| llv0tr | cop2 0x04A_0012 | v0 * light matrix + tr vector |
| llv1tr | cop2 0x04A_8012 | v1 * light matrix + tr vector |
| llv2tr | cop2 0x04B_0012 | v2 * light matrix + tr vector |
| llirtr | cop2 0x04B_8012 | ir * light matrix + tr vector |
| llv0bk | cop2 0x04A_2012 | v0 * light matrix + bk vector |
| llv1bk | cop2 0x04A_A012 | v1 * light matrix + bk vector |
| llv2bk | cop2 0x04B_2012 | v2 * light matrix + bk vector |
| llirbk | cop2 0x04B_A012 | ir * light matrix + bk vector |
| lc | cop2 0x04D_A412 | v0 * color matrix, Lower limit clamped to 0 |
| lcv0 | cop2 0x04C_6012 | v0 * color matrix |
| lcv1 | cop2 0x04C_E012 | v1 * color matrix |
| lcv2 | cop2 0x04D_6012 | v2 * color matrix |
| lcvir | cop2 0x04D_E012 | ir * color matrix |
| lcv0tr | cop2 0x04C_0012 | v0 * color matrix + tr vector |
| lcv1tr | cop2 0x04C_8012 | v1 * color matrix + tr vector |
| lcv2tr | cop2 0x04D_0012 | v2 * color matrix + tr vector |
| lcirtr | cop2 0x04D_8012 | ir * color matrix + tr vector |
| lev0bk | cop2 0x04C_2012 | v0 * color matrix + bk vector |
| lev1bk | cop2 0x04C_A012 | v1 * color matrix + bk vector |
| lev2bk | cop2 0x04D_2012 | v2 * color matrix + bk vector |
| leirbk | cop2 0x04D_A012 | ir * color matrix + bk vector |

## Other instructions
| Name | Command | Description | Format |
| ---- | ------- | ----------- | ------ |
| sqr12 | cop2 0x0A8_0428 | square of ir | 1,19,12 |
| sqr0 | cop2 0x0A8_0428 | square of ir | 1,31,0 |
| op12 | cop2 0x178_000C | outer product | 1,19,12 |
| op0 | cop2 0x170_000C | outer product | 1,31,0 |
| gpf12 | cop2 0x198_003D | general purpose interpolation | 1,19,12 |
| gpf0 | cop2 0x190_003D | general purpose interpolation | 1,31,0 |
| gpl12 | cop2 0x1A8_003E | general pupose interpolation | 1,19,12 |
| gpl0 | cop2 0x1A0_003E | general purpose interpolation | 1,31,0 |




