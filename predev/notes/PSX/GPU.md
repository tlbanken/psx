# Overview

[http://hitmen.c02.at/files/docs/psx/psx.pdf](http://hitmen.c02.at/files/docs/psx/psx.pdf)

- GPU is responsible for the graphical output of the PSX
- 1MB frame buffer
- 16 bits per pixel
- 1024x512 resolution
- 2Kb texture cache
- 15-bit color or 24-bit color
- PSX has no FPU, instead uses Geometry Transformation Engine (GTE) for all 3d calculations
    - performs vector and matrix operations
    - perspective transformation
    - color equations
    - mounted as second coprocessor (Cop2)

# Graphics Processing Unit (GPU)
- 1MB frame buffer
- can take "commands" from CPU, or via DMA to place objects on the frame buffer to be displayed
- 64 byte command FIFO buffer
    - hold up to 3 commands
    - connected to a DMA channel for transfer of image data and linked command lists (channel 2)
    - DMA channel for reverse clearing an Ordering Table (channel 6)

## Communication and Ordering Tables (OT)

- all data regarding drawing environment are sent as packets to GPU
- Each packet tells GPU how and where to draw one primitive, or sets one of the drawing env paramaters
- display environment set up through single word commands using control port of GPU

### Packets

- can be forwarded word-by-word through data port of GPU
- can use DMA to efficiently for large numbers of packets
    - list of packets is sent
    - each item in list contains a header which is one word address of next entry and size of packet and packet itself
    - This means packets don't need to be stored sequentially
    - GPU processes the packets it gets in the order they are offered
    - first item in the list gets drawn first
- To insert a packet into the middle of the list simply find the packet after which needs it to be processed, replace the address in that packet with the address of the new packet, and let that point to the address that was replaced.

### Ordering Table (Not done by hardware??)

- used to aid in finding a location in the list  
    TODO (if done in HW)

## The Frame Buffer

- memory which stores all graphic data which the GPU can access and manipulate, while drawing and displaying an image.
- memory is under the GPU and cannot be accessed by the CPU directly
- size of 1MB
- treated as a space of 1024 pixels wide and 512 pixels high
- each "pixel" has the size of one word (16 bit)
- not treated linearly like usual mem
    - instead accessed through coordinates, with an upper left corner of (0,0) and lower right corner of (1023, 511)

### Color Formats

- when data is displayed from frame buffer, a retangular area is read from the specified coordinate within this memory
- the size of this area can be chosen from several hardware defined types
- these hardware sizes are only valid when the X and Y stop/start registers are at their default values
- two color formats
    - 15-bit direct display
    - 24-bit direct display

```
15-bit direct Display

|15                                         0|
----------------------------------------------
| M |    Blue    |    Green    |     Red     |
----------------------------------------------
  1       5             5             5

Each value has a value of 0-31. The MSB of a pixel (M) is used to
mask the pixel.
```

```
24-bit direct display

|       Pixel 0	    |       Pixel 1	    |       Pixel 2	    |
-------------------------------------------------------------
|    G0   |    R0   |    R1   |    B0   |    B1   |    G1   |
-------------------------------------------------------------
|15      8|7       0|15      8|7       0|15      8|7       0|

Notes:  
The 24bit pixels occupy 3 bytes, so each 6 bytes contain two 24bit  
pixels [R0,G0,B0], [R1,G1,B1]

```

## Primitives

- A basic figure which the GPU can draw is called a primitive

### Polygon

- can draw 3-point and 4-point polygons
- each point of the polygon specifies a point in the frame buffer
- the polygon can also be gourad shaded

```

Here is the ordering of 4 point polygon

1          2  
*----------*  
|          |
|          |  
|          |  
*----------*  
3          4

```
- internally, the 4-point polygon is processed as two 3 point polygons
- note that the gpu will not draw the right most and bottom edge
- make sure adjoining polygons have the same coordinate if you want them to touch each other

### Polygon with texture

- same as above polygon, except a texture is applied
- each vertexx of teh polygon maps to a point on a texture page in the frame buffer
- the polygon can be gourad shaded
- Since 4-point polygons are processed as two 3-point polygons, texture mapping is also done independently for both halves

### Rectangle

- defined by the location of the top left corner and its width and height
- width and height can be either free, 8x8, or 16x16
- draw much faster than a polygon

### Sprite

- a textured rectangle, defined as a rectangle with coordinates on a texture page
- No gourad shading is possible
- nothing in common with traditional sprite
- Unlike PSX sprite, traditional sprite is NOT drawn to the bitmap, but gets sent to the screen instead of the actual graphics data at that location at display time

### Line

- straight line between 2 specified points
- can be gourad shaded
- special form is the polyline, for which an arbitrary number of points can be specified

### Dot

- draws one pixel at the specified coordinate and in the specified color
- like a special form of rectangle, with a size of 1x1

## Textures

- an image put on a polygon or sprite
- must prepare the data beforehand in the frame buffer
- this image is called a texture pattern
    - located on a texture page
    - standard size
    - located somewhere in the frame buffer
- can be stored in 3 different modes

```
15-bit direct mode

|                  I0                   |
|15                                    0|
-----------------------------------------
| S |   Blue    |   Green   |   Red     |
-----------------------------------------
  1       5           5          5


Each color has a value of 0-31. The MSB of a pixel (S) is used  
to specify if the pixel is semi transparent or not.
```

```
8-bit CLUT mode
Each pixel is defined by 8bits and the value of the pixel is
converted to a 15-bit color using the CLUT (color lookup table)
much like standard VGA pictures. So in effect you have 256 colors
which are in 15bit precision.

|15            8|7             0|
---------------------------------
|       I1      |       I0      |
---------------------------------

I0 is the index to the CLUT for the left pixel, I1 for the right.
```

```
4-bit CLUT mode  
Same as 8-bit CLUT mode except that only 16 colors can be used

|15                            0|
---------------------------------
|   I3  |   I2  |   I1  |   I0  |
---------------------------------
    4        4      4       4

I0 is first drawn to the left to I3 to the right
```

### Textures Pages

- texture pages have a unit size of 256x256 pixels, regardless of color mode
    - 64 pixels wide for 4bit CLUT
    - 128 pixels wide for 8bit CLUT
    - 256 pixels wide for 16-bit direct

### Texture Windows

- The area within a texture window is repeated throughout the texture page
- data is not stored all over the texture page, but the GPU reads the repeated patterns as if they were
- the X and Y and H and W must be multiples of 8
- more than one texture page can be set up, but each primitive can only contain texture from one page

### CLUT (Color Lookup Table)

- the CLUT is the table where the colors are stored for the image data in the CLUT modes
- the pixels of those images are used as indexes to this table
- arranged in the frame buffer as...
    - 256x1 image for the 8bit CLUT mode
    - 16x1 image for the 4bit CLUT mode
- more than one CLUT can be prepared but only one can be used for each primative

### Texture Caching

- used to speed up repeated reads of a texture
- cache size depends on the color mode being used
    - 4-bit CLUT: 64x64
    - 8-bit CLUT: 8x8
    - 15-bit direct: 32x32

### Cache Blocks

- the texture page is divided into non-overlapping cache blocks
- cache entries
    - each block is divided into 256 cache entries
    - entries are numbered sequentially and are 8 bytes wide
    - a cache entry holds 16 4-bit CLUT pixels, 8 8-bit CLUT pixels, or 4 15-bit direct pixels
- can only hold one cache entry by the same number
    - if for example, a piece of texture spans multiple cache blocks and it has data on entry 9 of block 1, but also on entry 9 of block 2, these cannot be in the cache at once

## Rendering options

There are 3 modes which affect the way the GPU renders the primitives to the frame buffer.

### Semi Transparency

- when semi transparency is set for a pixel, GPU reads pixel it wants to write to, then calcs the color it will write from the 2 pixels according to the mode selected
- there are 4 semi-transparency modes in the GPU

```
B = the pixel read from the image in the frame buffer  
F = the half transparent pixel

(1.0 * B) + (0.5 * F)  
(1.0 * B) + (1.0 * F)  
(1.0 * B) - (1.0 * F)  
(1.0 * B) + (0.25 * F)
```
### Shading

- GPU has a shading function, which will scale the color of a primitive to a specified brightness
- 2 shading models: Flat and Gourad
    - Flat: the brightness value is specified for the entire primitive
    - Gourad: a different brightness value can be given for each vertex of a primitive, and the brightness between these points is automatically interpolated

### Mask

- prevents the GPU from writing to specific pixels when drawing in the frame buffer
- when the CPU is drawing a primitive to a masked area...
    - it first reads the pixel at the coordinate it wants to write to
    - checks if it's masking bit is set
    - if so, refrains from writing to that particular pixel
- the masking bit is the MSBit of the pixel
- to set this bit, the GPU provides a mask out mode, which sets te MSB of any pixel i writes
- if both mask out and mask evaluation are on, the GPU wiill not draw to pixels with set MSB's, and will draw pixels with set MSB's to the others, these in turn becoming masked pixels

## Drawing Environment

The drawing environment specifies all global parameters the GPU needs for drawing primitives.

### Drawing offset

- locates the top left corner of the drawing area
- coordinates of primitives originate to this point
- Example: if drawing offset is (0,240) and a vertex of a polygon is (16,20) it will be drawn to the frame buffer at (0+16, 240+20)

### Drawing clip area

- Specifies the max range the GPU draws primitives to
- in effect, it specifies the top left and bottom right corner of the drawing area

### Dither enable

- when enabled, the GPU will dither areas during shading
- it will internally in 24-bit and dither the colors when converting back to 15-bit
- when it is off, the lower 3 bits of each color simply get discarded

### Draw to display enable

- this will enabled/disable any drawing to the area that is currently displayed

### Mask enable

- when turned on, any pixel drawn to the frame buffer by the GPU will have a set masking bit

### Mask judgement enable

- specifies if the mask data from the frame buffer is evaluated at the time of drawing

## Display Environment

Contains all information about the display and the area displayed.

### Display area in frame buffer

- specifies the resolution of the display
- size can be set as follows:
    - Width: 256, 320, 384, 512, or 640 pixels
    - Height: 240, or 480
- these sizes are only an indication on how many pixels will be displayed using a default start end
- these settings only specify the resolution of the display

### Display start/end

- specifies where the display area is positioned on the screen and how much data gets sent to the screen
- screen sizes of the display area are valid only if the horizontal/vertical start/end values are default
    - by changing these, you can get bigger/smaller display screens

### Interlace enable

- when enabled the GPU will display the even and odd lines of the display area alternately
- it is necessary to set this when using 480 lines as the number of scan lines on the TV screen are not sufficient to display 480 lines

### 15bit/24bit direct display

- switches between 15bit/24bit display mode

### Video mode

- selects which video mode to use, which are either PAL or NTSC

## GPU Operation

### GPU Control registers

- 2 32-bit IO ports for the GPU
    - GPU Data: `0x1f80_1810`
    - GPU Control/Status: `0x1f80_1814`
- the data register is used to exchange data with the GPU
- the control/status register gives the status of the GPU when read, and sets the control bits when written to

### Control/Status Register (0x1f80_1814)

```
                    STATUS (Read) HIGH

|31                                                                                               16|
-----------------------------------------------------------------------------------------------------
| lcf | dma | com | img | busy | ? | ? | den | isinter | isrgb24 | Video | Height | Width0 | Width1 |
-----------------------------------------------------------------------------------------------------
   1     2     1     1      1    1   1    1       1         1         1       1        2        1


*** KEY ***  
        W0   W1  
Width   00   0       256 pixels  
        01   0       320  
        10   0       512  
        11   0       640  
        00   1       384

Height  0            240 pixels  
        1            480

Video   0    NTSC  
        1    PAL  
isrgb24 0    15-bit direct mode  
        1    24-bit direct mode  
isinter 0    Interlace off  
        1    Interlace on  
den     0    Display enabled  
        1    Display disabled  
busy    0    GPU is Busy (i.e. drawing primitives)  
        1    GPU is Idle  
img     0    Not Ready to send image (packet $c0)  
        1    Ready  
com     0    Not Ready to recieve commands  
        1    Ready  
dma     00   DMA off, communication through GP0  
        01   Unknown  
        10   DMA CPU -> GPU  
        11   DMA GPU -> CPU  
lcf     0    Drawing even lines in interlace mode  
        1    Drawing uneven lines in interlace mode
```

```
                    Status (read) Low
|15                                                     0|
----------------------------------------------------------
| ? | ? | ? | me | md | dfe | dtd | tp | abr | ty |  tx  |
----------------------------------------------------------
  1   1   1   1    1     1     1     2    2     1     4


*** KEY ***
tx      0       0                   Texture page X = tx*64
        1       64
        2       128
        3       196
        4       ...
ty      0       0
        1       256
abr     00      (0.5*B) + (0.5*F)   Semi Transparent State
        01      (1.0*B) + (1.0*F)
        10      (1.0*B) - (1.0*F)
        11      (1.0*B) + (0.25*F)
tp      00      4-bit CLUT          Texture Page Color Mode
        01      8-bit CLUT
        10      15-bit
dtd     0       Dither off
        1       Dither on
dfe     0       off                 Draw to display area prohibited
        1       on                  Draw to display area allowed
md      0       off                 Do not apply mask bit to drawn pixels
        1       on                  Apply mask bit to drawn pixels
me      0       off                 Draw over pixel with mask set
        1       on                  No drawing to pixels with set mask bit

```

```
CONTROL (write)

|31                   16|15                     0|
--------------------------------------------------
|       command         |       parameter        |
--------------------------------------------------
```

### Commands/Parameters
#### Reset GPU

```
Command         0x00  
Parameter       0x00_0000  
Description     Resets the GPU. Also turns off the screen.  
                (sets status to 0x1480_2000)
```

#### Reset Command Buffer
```
Command         0x01  
Parameter       0x00_0000  
Description     Resets the command buffer.
```

#### Reset IRQ
```
Command         0x02  
Parameter       0x00_0000  
Description     Resets the IRQ
```

#### Display Enable
```
Command         0x03  
Parameter       0x00_0000 Display disable  
                0x00_0001 Display enable  
Description     Turns on/off display. Note that a turned off screen still  
                gives the flicker of NTSC on a pal screen if NTSC mode is  
                selected
```

#### DMA Setup
```
Command         0x04  
Parameter       0x00_0000 DMA disabled  
                0x00_0001 Unknown DMA Function  
                0x00_0002 DMA CPU to GPU  
                0x00_0003 DMA GPU to CPU  
Description     Sets DMA direction
```

#### Start of Display Area
```
Command         0x05  
Parameter       bit 0x00-0x09 X (0-1023)  
                bit 0x0a-0x12 Y (0-512) = Y << 10 + X  
Description     Locates the top left corner of the display area
```

#### Horizontal Display Range
```
Command         0x06  
Parameter       bit 0x00-0x0b X1 (0x1f4-0xCFA)  
                bit 0x0c-0x17 X2 = X1 + X2 << 12  
Description     Specifies the horizontal range within which the display  
                area is displayed. The display is relative to the display start  
                so X coordinate 0 will be at the value in X1. The display end is  
                not relative to the display start. The number of pixels that get  
                sent to the screen in 320 mode are (X2-X1)/8. How many actually  
                are visible depends on your TV/monitor. (normally $260-$c56)
```

#### Vertical Display Range
```
Command         0x07  
Parameter       bit 0x00-0x09 Y1  
                bit 0x0a-0x14 Y2 = Y1 + Y2 << 10  
Description     Specifies the vertical range within which the display.  
                The display is relative to the display start, so Y coordinate 0  
                will be at the value in Y1. The display end is not relative to the  
                display start. The number of pixels that get sent to the display are  
                Y2-Y1, in 240 mode. (Not sure about the default values, should be  
                something like NTSC $010-$100, PAL $023-$123)
```

#### Display Mode
```
Command         0x08  
Parameter       bit 0x00-0x01 Width 0  
                bit 0x02 Height  
                bit 0x03 Video mode: See above  
                bit 0x04 Isrgb24  
                bit 0x05 Isinter  
                bit 0x06 Width 1  
                bit 0x07 Reverse Flag  
Description     Sets the display mode
```

#### Unknown
```
Command         0x09  
Parameter       0x00_0001 ??  
Description     Used with value 0x00_0001

```

#### GPU Info -- TODO
```
Command         0x10  
Parameter       0x00_0000  
                0x00_0001  
                0x00_0002  
                0x00_0003 Draw area top left???  
                0x00_0004 Draw area bottom right???  
                0x00_0005 Draw offset???  
                0x00_0006  
                0x00_0007 ???  
???
```

#### ????
```
Command         0x20  
Parameter       ???????  
Description     Used with value 0x00_0504
```

### Command Packets, Data Register
- primitive command packets use an 8 bit command value which is present in all packets.
- they contain a 3 bit type block and a 5 bit option block of which the meaning of the bits depend on the type

#### Types
```
000     GPU command
001     Polygon primitive
010     Line primititve
011     Sprite primitive
100     Transfer command
111     Environment command
```

#### Option Blocks for Primitives

```
            Polygon

|7         5|4                           0|  
|   Type    |            Option           |
-------------------------------------------
| 0 | 0 | 1 | IIP | VTX | TME | ABE | TGE |
-------------------------------------------
  1   1   1    1     1     1     1     1

            Line

|7         5|4                           0|  
|   Type    |            Option           |
-------------------------------------------
| 0 | 1 | 0 | IIP | PLL |  0  | ABE |  0  |
-------------------------------------------
  1   1   1    1     1     1     1     1


            Sprite

|7         5|4                           0|  
|   Type    |            Option           |
-------------------------------------------
| 0 | 1 | 0 |    size   | TME | ABE |  0  |
-------------------------------------------
  1   1   1       2        1     1     1

*** KEY ***  
IIP     0    Flat Shading  
        1    Gourad Shading  
VTX     0    3 vertex polygon  
        1    4 vertex polygon  
TME     0    Texture mapping off  
        1    Texture mapping on  
ABE     0    Semi Transparency off  
        1    Semi transparency on  
TGE     0    Brightness calculation at time of texture mapping on  
        1    off. (draw texture as is)  
Size    00   Free size (Specified by W/H)  
        01   1x1  
        10   8x8  
        11   16x16  
PLL     0    Single line (2 vertices)  
        1    Polyline (n vertices)  
```

#### Color Information

- color info is forwarded as 24-bit data
- it is parsed to 15-bit by the GPU

```
Layout

|23                                0|
-------------------------------------
|   Blue    |   Green   |   Red     |
-------------------------------------
      8           8          8
```

#### Shading Information
- textured primitive shading data is forwarded by this packet
- Layout is the same as the color data (see above)
- RGB values controling the brightness of the individual colors ($00-$7f)
- a value of $80 in a color will take the former value as data

#### Texture Page Information
- the data is 16 bits wide

```
Layout


*** KEY *** (TODO -- look this over)  
TX      0-0xf       X*64 t          texture page x coordinate  
TY      0           0               texture page y coordinate  
        1           256  
ABR     0           0.5*B + 0.5*F   Semi transparency mode  
        1           1.0*B + 1.0*F  
        2           1.0*B - 1.0*F  
        3           1.0*B + 0.25*F  
TP      0           4-bit CLUT  
        1           8-bit CLUT  
        2           15-bit direct
```

#### CLUT-ID
- specifies the location of the CLUT data
- data is 16-bits

```
Layout

|15                    8|7                     0|
-------------------------------------------------
|   y-coordinate 0-511  |   x-coordinate X/16   |
-------------------------------------------------

```

## Packets
### Abbreviations
```
BGR         Color/Shading info  
xn,yn       16 bit values of X and Y in frame buffer  
un,vn       8 bit values of X and Y in texture page  
tpage       texture page information packet  
clut        CLUT ID
```

### Packet List
- packets sent to the GPU are processed as a group of data, each one word wide
- the data must be written to the GPU data register (0x1f80_1810) sequentially
- once all data has been received, the GPU starts operation

### Overview of packet commands
#### Primitive drawing packets
```
0x20    monochrome 3 point polygon  
0x24    textured 3 point polygon  
0x28    monochrome 4 point polygon  
0x2c    textured 4 point polygon  
0x30    gradated 3 point polygon  
0x34    gradated textured 3 point polygon  
0x38    gradated 4 point polygon  
0x3c    gradated textured 4 point polygon  
0x40    monochrome line  
0x48    monochrome polyline  
0x50    gradated line  
0x58    gradated line polyline  
0x60    rectangle  
0x64    sprite  
0x68    dot  
0x70    8x8 rectangle  
0x74    8x8 sprite  
0x78    16x16 rectangle  
0x7c    16x16 sprite
```

#### GPU command & Transfer packets
```
0x01    clear cache  
0x02    frame buffer rectangle draw  
0x80    move image in frame buffer  
0xa0    send image to frame buffer  
0xc0    copy image from frame buffer

```

#### Draw mode/environment setting packets
```
0xe1    draw mode setting  
0xe2    texture window setting  
0xe3    set drawing area top left  
0xe4    set drawing area bottom right  
0xe5    drawing offset  
0xe6    mask setting
```

### Packet Descriptions
#### Primitive Packets
```
0x20 Monochrome 3 point polygon

Order |31     24|23       16|15       8|7         0|
  1   |   0x20  |              BGR                 | Comand + Color
  2   |         y0          |          x0          | Vertex 0
  3   |         y1          |          x1          | Vertex 1
  4   |         y2          |          x2          | Vertex 2
```

```
0x24 Textured 3 point polygon

Order |31     24|23       16|15       8|7         0|
  1   |   0x24  |              BGR                 | Comand + Color
  2   |         y0          |          x0          | Vertex 0
  3   |        CLUT         |    v0    |     u0    | CLUT ID + Texture coordinates vertex 0
  4   |         y1          |          x1          | Vertex 1
  5   |        tpage        |    v1    |     u1    | Texture page + Texture coordinates vertex 1
  6   |         y2          |          x2          | Vertex 2
  7   |                     |    v2    |     u2    | texture coordinates vertex 1
```

```
0x28 Monochrome 4 point polygon

Order |31     24|23       16|15       8|7         0|
  1   |   0x28  |              BGR                 | Comand + Color
  2   |         y0          |          x0          | Vertex 0
  3   |         y1          |          x1          | Vertex 1
  4   |         y2          |          x2          | Vertex 2
  5   |         y3          |          x3          | Vertex 3
```

```
0x2c Textured 4 point polygon

Order |31     24|23       16|15       8|7         0|
  1   |   0x2c  |              BGR                 | Comand + Color
  2   |         y0          |          x0          | Vertex 0
  3   |        CLUT         |    v0    |     u0    | CLUT ID + Texture coordinates vertex 0
  4   |         y1          |          x1          | Vertex 1
  5   |        tpage        |    v1    |     u1    | Texture page + Texture coordinates vertex 1
  6   |         y2          |          x2          | Vertex 2
  7   |                     |    v2    |     u2    | texture coordinates vertex 1
  8   |         y3          |          x3          | Vertex 3
  9   |                     |    v3    |     u3    | texture coordinates vertex 2
```

```
0x30 Gradated 3 point polygon

Order |31     24|23       16|15       8|7         0|
  1   |   0x30  |              BGR0                | Comand + Color Vertex 0
  2   |         y0          |          x0          | Vertex 0
  3   |         |              BGR1                | Color Vertex 1
  4   |         y1          |          x1          | Vertex 1
  5   |         |              BGR2                | Color Vertex 2
  6   |         y2          |          x2          | Vertex 2
```

```
0x34 shaded textured 3 point polygon

Order |31     24|23       16|15       8|7         0|
  1   |   0x34  |              BGR0                | Comand + Color Vertex 0
  2   |         y0          |          x0          | Vertex 0
  3   |        CLUT         |    v0    |     u0    | CLUT ID + Texture coordinates vertex 0
  4   |         |              BGR1                | Color Vertex 1
  5   |         y1          |          x1          | Vertex 1
  6   |        tpage        |    v1    |     u1    | Texture page + Texture coordinates vertex 1
  7   |         |              BGR2                | Color Vertex 2
  8   |         y2          |          x2          | Vertex 2
  9   |                     |    v2    |     u2    | Texture coordinates vertex 2
```

```
0x38 gradated 4 point polygon

Order |31     24|23       16|15       8|7         0|
  1   |   0x38  |              BGR0                | Comand + Color Vertex 0
  2   |         y0          |          x0          | Vertex 0
  3   |         |              BGR1                | Color Vertex 1
  4   |         y1          |          x1          | Vertex 1
  5   |         |              BGR2                | Color Vertex 2
  6   |         y2          |          x2          | Vertex 2
  5   |         |              BGR3                | Color Vertex 3
  6   |         y3          |          x3          | Vertex 3
```

```
0x3c shaded textured 4 point polygon

Order |31     24|23       16|15       8|7         0|
  1   |   0x3c  |              BGR0                | Comand + Color Vertex 0
  2   |         y0          |          x0          | Vertex 0
  3   |        CLUT         |    v0    |     u0    | CLUT ID + Texture coordinates vertex 0
  4   |         |              BGR1                | Color Vertex 1
  5   |         y1          |          x1          | Vertex 1
  6   |        tpage        |    v1    |     u1    | Texture page + Texture coordinates vertex 1
  7   |         |              BGR2                | Color Vertex 2
  8   |         y2          |          x2          | Vertex 2
  9   |                     |    v2    |     u2    | Texture coordinates vertex 2
  10  |         |              BGR3                | Color Vertex 3
  11  |         y3          |          x3          | Vertex 3
  12  |                     |    v3    |     u3    | Texture coordinates vertex 3
```

```
0x40 monochrome line

Order |31     24|23       16|15       8|7         0|
  1   |   0x40  |              BGR                 | Comand + Color
  2   |         y0          |          x0          | Vertex 0
  3   |         y1          |          x1          | Vertex 1
```

```
0x48 single color polyline

Order |31     24|23       16|15       8|7         0|
  1   |   0x48  |              BGR                 | Comand + Color
  2   |         y0          |          x0          | Vertex 0
  3   |         y1          |          x1          | Vertex 1
  4   |         y2          |          x2          | Vertex 2
 ...  |         yn          |          xn          | Vertex n
 ...  |                 0x5555_5555                | Termination Code

Any number of points can be entered, end with termination code
```

```
0x50 gradated line

Order |31     24|23       16|15       8|7         0|
  1   |   0x50  |              BGR0                | Comand + Color Vertex 0
  2   |         y0          |          x0          | Vertex 0
  3   |         |              BGR1                | Color Vertex 1
  4   |         y1          |          x1          | Vertex 1
```

```
0x58 gradated polyline

Order |31     24|23       16|15       8|7         0|
  1   |   0x58  |              BGR0                | Comand + Color Vertex 0
  2   |         y0          |          x0          | Vertex 0
  3   |         |              BGR1                | Color Vertex 1
  4   |         y1          |          x1          | Vertex 1
  5   |         |              BGR2                | Color Vertex 2
  6   |         y2          |          x2          | Vertex 2
 ...  |         |              BGRn                | Color Vertex n
 ...  |         yn          |          xn          | Vertex n
 ...  |                 0x5555_5555                | Termination Code
```

```
0x60 Rectangle

Order |31     24|23       16|15       8|7         0|
  1   |   0x60  |              BGR                 | Comand + Color
  2   |         y           |          x           | upper left corner location
  3   |         h           |          w           | height and width
```

```
0x64 Sprite

Order |31     24|23       16|15       8|7         0|
  1   |   0x64  |              BGR                 | Comand + Color
  2   |         y           |          x           | upper left corner location
  3   |        CLUT         |    v     |     u     | CLUT ID + Texture coordinates page y,x
  4   |         h           |          w           | height and width
```

```
0x68 Dot

Order |31     24|23       16|15       8|7         0|
  1   |   0x68  |              BGR                 | Comand + Color
  2   |         y           |          x           | location
```

```
0x70 8x8 Rectangle

Order |31     24|23       16|15       8|7         0|
  1   |   0x70  |              BGR                 | Comand + Color
  2   |         y           |          x           | location
```

```
0x74 8x8 Sprite

Order |31     24|23       16|15       8|7         0|
  1   |   0x74  |              BGR                 | Comand + Color
  2   |         y           |          x           | location
  3   |        CLUT         |    v     |     u     | CLUT ID + Texture coordinates page y,x
```

```
0x78 16x16 Rectangle

Order |31     24|23       16|15       8|7         0|
  1   |   0x78  |              BGR                 | Comand + Color
  2   |         y           |          x           | location
```

```
0x7c 16x16 Sprite

Order |31     24|23       16|15       8|7         0|
  1   |   0x7c  |              BGR                 | Comand + Color
  2   |         y           |          x           | location
  3   |        CLUT         |    v     |     u     | CLUT ID + Texture coordinates page y,x
```

#### GPU Command & Transfer packets
```
0x01 Clear Cache

Order |31 24|23 16|15 8|7 0|  
1 | 0x01 | 0 | clear cache
```

```
0x02 frame buffer rectangle draw

Order |31     24|23       16|15       8|7         0|
  1   |   0x02  |              BGR                 | Comand + Color
  2   |         y           |          x           | upper left corner location
  3   |         h           |          w           | height and width

Fills the area in the frame buffer with the value in RGB. This command will draw
without regard to drawing environment settings. Coordinates are absolute frame
buffer coordinates. max width is 0x3ff, max height is 0x1ff.
```

```
0x80 Rectangle

Order |31     24|23       16|15       8|7         0|
  1   |   0x80  |              BGR                 | Comand + Color
  2   |        sy           |         sx           | Source coordinate  
  3   |        dy           |         dx           | Destination Coordinate  
  4   |         h           |          w           | height and width of transfer

Copies data within frame buffer
```

```
0x01 0xa0 send image to frame buffer

Order |31     24|23       16|15       8|7         0|
  1   |   0x01  |                                  | Reset command buffer (write to GP1 or GP0)
  2   |   0xa0  |               BGR                | Command + Color
  3   |         y           |          x           | Destination Coordinate
  4   |         h           |          w           | height and width of transfer
  5   |        pix1         |         pix0         | image data
  6.. |                                            |
  ... |        pixn         |         pixn-1       |

Transfers data from main memory to frame buffer. If the number of pixels to be
sent is odd, an extra should be sent. (32 bits per packet)
```

```
0x01 0xc0 send image to frame buffer

Order |31     24|23       16|15       8|7         0|
  1   |   0x01  |                                  | Reset command buffer (write to GP1 or GP0)
  2   |   0xc0  |                BGR               | Command + Color
  3   |         y           |          x           | Destination Coordinate
  4   |         h           |          w           | height and width of transfer
  5   |        pix1         |         pix0         | image data
  6.. |                    ...                     |
  ... |        pixn         |         pixn-1       |

Transfers data from frame buffer to main memory. Wait for bit 27 of the status  
register to be set before reading the image data. When the number of pixels is  
odd, an extra pixel is read at the end. (Because one packet is 32 bits)
```

#### Draw mode/environment setting packets
- some of these packets can also be by primitive packets
- it is the last packet of either that the GPU received that is active
- so if a primitive sets tpage info, it will overwrite the existeing data, even if it was sent by an `0xe?` packet

```
0xe1 draw mode setting

|31      24|23     11| 10  |  9  |8  7|6   5|  4 |3  0|
-------------------------------------------------------
|   0xe1   |         | dte | dtd | tp | abr | ty | tx |
-------------------------------------------------------

(See top of section for key)
```

```
0xe2 texture window setting

|31       24|23   20|19   15|14   10|9     5|4     0|
-----------------------------------------------------
|   0xe2    |       |   twx |   twy |   tww |   twh |
-----------------------------------------------------

*** KEY ***
twx     Texture window X, (twx * 8)
twy     Texture window Y, (twy * 8)
tww     Texture window width, 256 - (tww * 8)
twh     Texture window height, 256 - (twh * 8)
```

```
0xe3 set drawing area top left

|31      24|23     16|15      8|7       0|
------------------------------------------
|   0xe3   |         |    Y    |    X    |
------------------------------------------

Sets the drawing area top left corner. X & Y are absolute frame  
buffer coordinates.
```

```
0xe4 set drawing area bottom right

|31     24|23       16|15       8|7         0|
----------------------------------------------
|   0xe4  |           |    Y     |     X     |
----------------------------------------------

Sets the drawing area bottom right corner. X & Y are absolute frame
buffer coordinates.
```

```
0xe5 drawing offset

|31      24|23   20|13   11|10          0|
------------------------------------------
|   0xe5   |       | OffsY |    OffsX    |
------------------------------------------

Offset Y = y << 11  
Sets the drawing area offset within the drawing area. X & Y are offsets  
in the frame buffer.
```

```
0xe6 drawing offset

|31     24|23           2|      1|      0|
------------------------------------------
|   0xe6  |              | Mask2 | Mask1 |
------------------------------------------

*** KEY ***
Mask1   Set mask bit while drawing. 1 = on
Mask2   Do not draw to mask areas. 1 = on
        While mask1 is in, the GPU will set the MSB of all pixels it draws.
        while mask2 is on, the GPU will not te to pixels with set MSB's.
```

## DMA and the GPU

- the GPU has two DMA channels allocated to it
- DMA channel 2 is used to send linked packet lists to the GPU and to transfer image data to and from the frame buffer
- DMA channel 6 sets up an empty linked list, of which each entry points to the previous (i.e. reverse clear an OT.)

### DMA Second Memory Address Register (D2\_MADR) -- 0x1f80\_10a0

```
|31                     0|
--------------------------
|          MADR          |
--------------------------

MADR    Pointer to the virtual address the DMA will start reading from/writing to
```

### DMA Second Block Control Register (D2\_BCR) -- 0x1f80\_10a4

```
|31                            0|
---------------------------------
|       BA      |       BS      |
---------------------------------
        16              16

*** KEY ***
BA      Amount of blocks
BS      Block size (words)

Sets up the DMA blocks. Once started the dMA will send BA blocks of BS
words. Don't set a block size larger then $10 words, as the command buffer
of the GPU is 64 bytes.
```

### DMA Second Channel Control Register (D2\_CHCR) -- 0x1f80\_10a8

```
|31                                     0|
--------------------------------------------------
|  0  | TR |        0       | LI | CO |  0  | DR |
--------------------------------------------------
   7    1           13         1   1     8    1

*** KEY ***
TR      0   No DMA Transfer busy
        1   Start DMA transfer/DMA transfer busy
LR      1   Transfer linked list. (GPU only)
CO      1   Transfer continuous stream of data
DR      1   Direction from memory
        0   Direction from memory

This configures the DMA channel. The DMA starts when bit 18 is set.
DMA is finished as soon as bit 18 is cleared again. To send or receive
data to/from VRAM send the appropriate GPU packets first (0xa0/0xc0)
```

### DMA Sixth Block Control Register (D6\_BCR) -- 0x1f80\_10e4

```
|31                             0|
----------------------------------
|               BC               |
----------------------------------

BC      Number of list entries
```

### DMA Sixth Channel Control Register (D6\_CHCR) -- 0x1f80\_10e8

```
|31                                                     0|
----------------------------------------------------------
|   0   | TR |      0       | LI | CO |     0       | DR |
----------------------------------------------------------
    7     1         13        1    1        8         1

*** KEY ***
TR      0   No DMA transfer busy
        1   Start DMA transfer/DMA transfer busy
LR      1   Transfer linked list. (GPU Only)
CO      1   Transfer continuous stream of data.
DR      1   Direction from memory
        0   Direction from memory

This configures the DMA channel. The DMA starts when bit 18 is set.
DMA is finished as soon as bit 18 is cleared again. To send or receive
data to/from VRAM send the appropriate GPU packets first (0xa0/0xc0).
When this register is set to $1100_0002, the DMA channel will create an
empty linked list of D6_BCR entries ending at the address in D6_MADR.
Each entry has a size of 0, and points to the previous. The first entry
is ???. So if D6_MADR = $8010_0010, D6_BCR = $0000_0004, and the DMA
is kicked this will result in a list looking like this:
    0x8010_0000 0x00ff_ffff
    0x8010_0004 0x0010_0000
    0x8010_0008 0x0010_0004
    0x8010_000c 0x0010_0008
    0x8010_0010 0x0010_000c
```

### DMA Primary Control Register (DPCR) -- 0x1f80_10f0

```
|31                                                   0|
--------------------------------------------------------
|     | DMA6 | DMA5 | DMA4 | DMA3 | DMA2 | DMA1 | DMA0 |
--------------------------------------------------------
   4     4      4      4      4       4     4      4


Each register has 4 bit control block allocated in this register.
Bit 3   1 = DMA Enabled
    2   Unknown
    1   Unknown
    0   Unknown
Bit 3 must be set for a channel to operate
```

## Common GPU functions, step by step

### Initializing GPU
>First thing to do when using the GPU is to initialize it.
>To do that take the following steps:

1.  Reset the GPU (GP1 command $00). This turns off the display as well.
2.  Set horizontal and vertical start/end. (GP1 command $06, $07)
3.  Set display mode. (GP1 command $08)
4.  Set display offset. (GP1 command $05)
5.  Set draw mode. (GP0 command $e1)
6.  Set draw area. (GP0 command $e3, $e4)
7.  Set draw offset. (GP0 command $e5)
8.  Enable display.

### Sending a linked list
- the normal way to send large numbers of primitives is by using a linked list DMA transfer
- this list is built up of entries of which each points to the next
- the last entry in the list should have 0xffff_ffff as pointer, which is the terminator
- as soon as this value is found DMA is ended
- if the entry size is set to 0, no data will be transfered to the GPU and the next entry is processed

#### Entry Format
```
$nnYYYYYY

nn          number of words in the list entry
YYYYYY      address of the next list entry & 0x00ff_ffff
```

#### Steps to sending the list
1. Wait for the GPU to be ready to receive commands. (bit `$1c == 1`)
2. Enable DMA channel 2
3. Set GPU to DMA CPU->GPU mode. (`$0400_0002`)
4. set D2_MADR to the start of the list
5. set D2_BCR to zero
6. Set D2_CHCR to link mode, memory->GPU and DMA enable. (`$0100_0401`)

### Uploading Image data through DMA
1. Wait for the GPU to be idle and DMA to finish. Enable DMA channel 2 if necessary
2. Send the 'Send image to VRAM' primitive. (You can send this through DMA if you want. Use the linked list method described above)
3. Set DMA to CPU->GPU (`$0400_0002`)(If you didn't do so already in the previous step)
4. Set D2_MADR to the start of the list
5. Set D2_BCR with: bits 31-16 = Number o fwords to send (`H*W/2`), 15-0 = Block size of 1 word. (`$01`), if `H*W` is odd, add 1. (Pixels are 2 bytes, send an extra blank pixel in case of an odd amount)
6. Set D2_CHCR to continuous mode, memory -> GPU and DMA enable. (`$0100_0201`)

- note that H, W, X and Y are always in frame buffer pixels, even if you send image data in other formats
- you can use bigger block sizes if you need more speed
- if the number of words to be sent is not a multiple of the block size, you'll have to send the remainder separately, because the GPU only accepts an extra halfword if the number of pixles is odd
- also take care not to use block sizes bigger than `0x10` as the buffer of the GPU is only 64 bytes

### Waiting to send commands
>You can send new commands as soon as DMA has ceased and the GPU is ready.
1. wait for bit `$18` to become 0 in D2_CHCR
2. wait for bit `$1c` to become 1 in GP1



